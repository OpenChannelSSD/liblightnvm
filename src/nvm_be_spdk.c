/*
 * be_spdk - Backend using SPDK
 *
 * Copyright (C) 2015-2017 Simon A. F. Lund <slund@cnexlabs.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef NVM_BE_SPDK_ENABLED
#include <liblightnvm.h>
#include <nvm_be.h>

struct nvm_be nvm_be_spdk = {
	.id = NVM_BE_SPDK,

	.open = nvm_be_nosys_open,
	.close = nvm_be_nosys_close,

	.idfy = nvm_be_nosys_idfy,
	.rprt = nvm_be_nosys_rprt,
	.sbbt = nvm_be_nosys_sbbt,
	.gbbt = nvm_be_nosys_gbbt,

	.erase = nvm_be_nosys_erase,
	.write = nvm_be_nosys_write,
	.read = nvm_be_nosys_read,
	.copy = nvm_be_nosys_copy,
};
#else
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <spdk/stdinc.h>
#include <spdk/nvme.h>
#include <spdk/env.h>
#include <liblightnvm.h>
#include <omp.h>
#include <nvm_be.h>
#include <nvm_dev.h>
#include <nvm_utils.h>
#include <nvm_debug.h>

#define NVM_BE_SPDK_QPAIR_MAX 64
#define NVM_BE_SPDK_ALIGN 0x1000

#define NVM_BE_SPDK_QDEPTH_MAX 128

struct state {
	struct spdk_nvme_transport_id trid;
	struct spdk_env_opts opts;
	struct spdk_nvme_ctrlr *ctrlr;
	struct spdk_nvme_qpair *qpair;
	struct spdk_nvme_ns *ns;
	uint16_t nsid;

	int attached;
	int vam_outstanding;

	omp_lock_t qpair_lock;
};

static void vam_cpl(void *cb_arg, const struct spdk_nvme_cpl *cpl)
{
	struct state *state = cb_arg;

	if (spdk_nvme_cpl_is_error(cpl)) {
		NVM_DEBUG("FAILED: spdk_nvme_cpl_is_error");
	}

	--(state->vam_outstanding);
}

static inline int vam_execute(struct nvm_dev *dev, struct nvm_nvme_cmd *cmd,
			      void *buf, size_t buf_len,
			      struct nvm_ret *NVM_UNUSED(ret))
{
	struct state *state = dev->be_state;
	struct spdk_nvme_cmd *nvme_cmd = (struct spdk_nvme_cmd *)cmd;
	void *buf_dma = NULL;
	uint64_t buf_phys = 0;

	if (buf && buf_len) {
		size_t alloc_len = NVM_BE_SPDK_ALIGN * ((buf_len + NVM_BE_SPDK_ALIGN - 1) / NVM_BE_SPDK_ALIGN);

		buf_dma = spdk_dma_zmalloc(alloc_len, NVM_BE_SPDK_ALIGN, &buf_phys);
		if (!buf_dma) {
			NVM_DEBUG("FAILED: spdk_dma_malloc");
			return -1;
		}
	}

	++(state->vam_outstanding);
	if (spdk_nvme_ctrlr_cmd_admin_raw(state->ctrlr, nvme_cmd, buf_dma,
					  buf_len, vam_cpl, state)) {
		--(state->vam_outstanding);

		NVM_DEBUG("FAILED: spdk_nvme_ctrlr_cmd_admin_raw");
		spdk_dma_free(buf_dma);

		return -1;
	}
	
	while (state->vam_outstanding)
		spdk_nvme_ctrlr_process_admin_completions(state->ctrlr);

	if (buf && buf_len) {
		memcpy(buf, buf_dma, buf_len);
		spdk_dma_free(buf_dma);
	}

	return 0;
}

static struct nvm_spec_idfy *nvm_be_spdk_idfy(struct nvm_dev *dev,
					      struct nvm_ret *ret)
{
	struct state *state = dev->be_state;
	struct nvm_spec_idfy *idfy = NULL;
	const size_t idfy_len = sizeof(*idfy);

	struct nvm_nvme_cmd cmd = { 0 };

	cmd.opcode = NVM_OPC_IDFY;
	cmd.nsid = state->nsid;

	idfy = nvm_buf_alloca(NVM_BE_SPDK_ALIGN, idfy_len);
	if (!idfy) {
		errno = ENOMEM;
		return NULL;
	}
	memset(idfy, 0, sizeof(*idfy));

	if (vam_execute(dev, &cmd, idfy, idfy_len, ret)) {
		NVM_DEBUG("FAILED: vam_execute");
		return NULL;
	}

	return idfy;
}

static struct nvm_spec_rprt *nvm_be_spdk_rprt(struct nvm_dev *dev,
					      struct nvm_addr addr,
					      uint16_t naddrs,
					      int opts,
					      struct nvm_ret *ret)
{
	struct state *state = dev->be_state;
	struct nvm_spec_rprt *rprt = NULL;
	const size_t rprt_len = sizeof(*rprt) + sizeof(struct nvm_spec_rprt_descr) * naddrs;

	struct nvm_nvme_cmd cmd = { 0 };

	rprt = nvm_buf_alloca(NVM_BE_SPDK_ALIGN, rprt_len);
	if (!rprt) {
		errno = ENOMEM;
		return NULL;
	}
	memset(rprt, 0, rprt_len);

	cmd.opcode = NVM_OPC_STATE;
	cmd.nsid = state->nsid;
	cmd.addrs = nvm_addr_gen2dev(dev, addr);
	cmd.rprt.naddrs = naddrs - 1;
	cmd.rprt.ro = opts;

	if (vam_execute(dev, &cmd, rprt, rprt_len, ret)) {
		NVM_DEBUG("FAILED: vam_execute");
		nvm_buf_free(rprt);
		return NULL;
	}

	return rprt;
}

static struct nvm_spec_bbt *nvm_be_spdk_gbbt(struct nvm_dev *dev,
					     struct nvm_addr addr,
					     struct nvm_ret *ret)
{
	const struct nvm_geo *geo = nvm_dev_get_geo(dev);
	struct state *state = dev->be_state;

	const uint32_t nblks = dev->geo.nblocks * dev->geo.nplanes;
	struct nvm_spec_bbt *bbt = NULL;
	const size_t bbt_len = sizeof(*bbt) + sizeof(*(bbt->blk)) * nblks;

	struct nvm_nvme_cmd cmd = { 0 };

	bbt = nvm_buf_alloc(geo, bbt_len);
	if (!bbt) {
		errno = ENOMEM;
		return NULL;
	}
	memset(bbt, 0, sizeof(*bbt));

	cmd.opcode = NVM_OPC_STATE;
	cmd.nsid = state->nsid;
	cmd.addrs = nvm_addr_gen2dev(dev, addr);

	if (vam_execute(dev, &cmd, bbt, bbt_len, ret)) {
		NVM_DEBUG("FAILED: vam_execute");
		nvm_buf_free(bbt);
		return NULL;
	}

	return bbt;
}

static int nvm_be_spdk_sbbt(struct nvm_dev *dev, struct nvm_addr *addrs,
			    int naddrs, uint16_t flags, struct nvm_ret *ret)
{
	const size_t addrs_len = naddrs * sizeof(uint64_t);
	uint64_t addrs_phys = 0;
	uint64_t *addrs_dma = NULL;

	struct nvm_nvme_cmd cmd = { 0 };

	cmd.opcode = NVM_S12_OPC_SET_BBT; // Construct command
	cmd.s12.control = flags;
	cmd.s12.naddrs = naddrs - 1;

	if (cmd.s12.naddrs) {
		addrs_dma = spdk_dma_malloc(addrs_len, NVM_BE_SPDK_ALIGN,
					    &addrs_phys);
		if (!addrs_dma) {
			NVM_DEBUG("FAILED: spdk_dma_zmalloc(addrs)");
			return -1;
		}
		
		for (int i = 0; i < naddrs; ++i)
			addrs_dma[i] = nvm_addr_gen2dev(dev, addrs[i]);

		cmd.addrs = addrs_phys;
	} else {
		cmd.addrs = nvm_addr_gen2dev(dev, addrs[0]);
	}

	if (vam_execute(dev, &cmd, NULL, 0, ret)) {
		NVM_DEBUG("FAILED: vam_execute");
		spdk_dma_free(addrs_dma);
		return -1;
	}

	spdk_dma_free(addrs_dma);

	return 0;
}

static void vio_cb(void *cb_arg, const struct spdk_nvme_cpl *cpl)
{
	int *completed = cb_arg;

	if (spdk_nvme_cpl_is_error(cpl)) {
		NVM_DEBUG("FAILED: spdk_nvme_cpl_is_error");
		*completed = -1;
		return;
	}

	*completed = 1;
}

static inline int vio_execute(struct nvm_dev *dev, struct nvm_addr addrs[],
			      struct nvm_addr dst[], int naddrs, void *data,
			      void *meta, uint16_t flags, int opcode,
			      struct nvm_ret *NVM_UNUSED(ret))
{
	struct state *state = dev->be_state;
	struct spdk_nvme_ctrlr *ctrlr = state->ctrlr;
	struct spdk_nvme_qpair *qpair = state->qpair;
	omp_lock_t *qpair_lock = &state->qpair_lock;
	const struct nvm_geo *geo = &dev->geo;

	struct nvm_nvme_cmd cmd = { 0 };
	struct spdk_nvme_cmd *nvme_cmd = (struct spdk_nvme_cmd*)&cmd;
	int completed = 0;

	const size_t addrs_len = naddrs * sizeof(uint64_t);
	uint64_t addrs_phys = 0;
	uint64_t *addrs_dma = NULL;

	uint64_t dst_phys = 0;
	uint64_t *dst_dma = NULL;

	size_t data_len = 0;;
	void *data_dma = NULL;

	size_t meta_len = 0;
	void *meta_dma = NULL;

	int res = 0;

	cmd.opcode = opcode;
	cmd.nsid = state->nsid;
	switch (dev->verid) {
	case NVM_SPEC_VERID_12:
	case NVM_SPEC_VERID_13:
		cmd.s12.naddrs = naddrs - 1;
		cmd.s12.control = flags;
		break;

	case NVM_SPEC_VERID_20:
		cmd.ewrc.naddrs = naddrs - 1;
		break;
	}

	if (naddrs > 1) {	// Addresses for ERASE/WRITE/READ and COPY(SRC)
		addrs_dma = spdk_dma_malloc(addrs_len, NVM_BE_SPDK_ALIGN,
					    &addrs_phys);
		if (!addrs_dma) {
			NVM_DEBUG("FAILED: spdk_dma_zmalloc(addrs)");
			res = -1;
			goto out;
		}
		
		for (int i = 0; i < naddrs; ++i)
			addrs_dma[i] = nvm_addr_gen2dev(dev, addrs[i]);

		cmd.addrs = addrs_phys;
	} else {
		cmd.addrs = nvm_addr_gen2dev(dev, addrs[0]);
	}

	if (dst) {		// Addresses for COPY(DST)
		if (naddrs > 1) {
			dst_dma = spdk_dma_malloc(addrs_len, NVM_BE_SPDK_ALIGN,
						  &dst_phys);
			if (!dst_dma) {
				NVM_DEBUG("FAILED: spdk_dma_zmalloc(dst)");
				res = -1;
				goto out;
			}
			
			for (int i = 0; i < naddrs; ++i)
				dst_dma[i] = nvm_addr_gen2dev(dev, dst[i]);
			cmd.addrs_dst = dst_phys;
		} else {
			cmd.addrs_dst = nvm_addr_gen2dev(dev, dst[0]);
		}
	}

	if (data) {		// Allocate and populate data
		data_len = geo->sector_nbytes * naddrs;
		data_dma = spdk_dma_malloc(data_len, NVM_BE_SPDK_ALIGN, NULL);
		if (!data_dma) {
			NVM_DEBUG("FAILED: spdk_dma_zmalloc(data_dma)");
			res = -1;
			goto out;
		}

		if (opcode == NVM_OPC_WRITE)
			memcpy(data_dma, data, data_len);
	}

	if (meta) {		// Allocate and populate meta
		meta_len = geo->meta_nbytes * naddrs;
		meta_dma = spdk_dma_malloc(meta_len, NVM_BE_SPDK_ALIGN, NULL);
		if (!meta_dma) {
			NVM_DEBUG("FAILED: spdk_dma_zmalloc(meta_dma)");
			res = -1;
			goto out;
		}

		if (opcode == NVM_OPC_WRITE)
			memcpy(meta_dma, meta, meta_len);
	}

	// Submit command
	omp_set_lock(qpair_lock);
	if (spdk_nvme_ctrlr_cmd_io_raw_with_md(ctrlr, qpair, nvme_cmd,
					       data_dma, data_len,
					       meta_dma, vio_cb,
					       &completed)) {
		NVM_DEBUG("FAILED: spdk_nvme_ctrlr_cmd_io_raw_with_md");
		
		omp_unset_lock(qpair_lock);
		res = -1;
		goto out;
	}
	omp_unset_lock(qpair_lock);

	// Wait for completion
	while (!completed) {
		omp_set_lock(qpair_lock);
		spdk_nvme_qpair_process_completions(qpair, 0);
		omp_unset_lock(qpair_lock);
	}

	if (completed < 0) {
		res = completed;
		goto out;
	}

	if ((opcode == NVM_OPC_READ) && data)
			memcpy(data, data_dma, data_len);

	if ((opcode == NVM_OPC_READ) && meta)
			memcpy(meta, meta_dma, meta_len);

out:
	spdk_dma_free(addrs_dma);
	spdk_dma_free(dst_dma);
	spdk_dma_free(data_dma);
	spdk_dma_free(meta_dma);

	return res;
}

static int nvm_be_spdk_erase(struct nvm_dev *dev, struct nvm_addr addrs[],
			     int naddrs, uint16_t flags, struct nvm_ret *ret)
{
	return vio_execute(dev, addrs, NULL, naddrs, NULL, NULL, flags,
			   NVM_OPC_ERASE, ret);
}

static int nvm_be_spdk_write(struct nvm_dev *dev, struct nvm_addr addrs[],
			     int naddrs, void *data, void *meta, uint16_t flags,
			     struct nvm_ret *ret)
{
	return vio_execute(dev, addrs, NULL, naddrs, data, meta, flags,
			   NVM_OPC_WRITE, ret);
}

static int nvm_be_spdk_read(struct nvm_dev *dev, struct nvm_addr addrs[],
			    int naddrs, void *data, void *meta, uint16_t flags,
			    struct nvm_ret *ret)
{
	return vio_execute(dev, addrs, NULL, naddrs, data, meta, flags,
			   NVM_OPC_READ, ret);
}

static int nvm_be_spdk_copy(struct nvm_dev *dev, struct nvm_addr src[],
			    struct nvm_addr dst[], int naddrs, uint16_t flags,
			    struct nvm_ret *ret)
{
	return vio_execute(dev, src, dst, naddrs, NULL, NULL, flags,
			   NVM_OPC_COPY, ret);
}

/**
 * Attaches only to the device matching the traddr
 */
static bool probe_cb(void *cb_ctx, const struct spdk_nvme_transport_id *trid,
		     struct spdk_nvme_ctrlr_opts *NVM_UNUSED(opts))
{
	struct state *state = cb_ctx;

	if (spdk_nvme_transport_id_compare(&state->trid, trid)) {
		NVM_DEBUG("trid->traddr: %s != state->trid.traddr: %s",
			  trid->traddr, state->trid.traddr);
		return false;
	}

	return !state->attached;
}

/**
 * Sets up the state{ns, nsid, ctrlr, attached} given via the cb_ctx
 * using the first available name-space.
 */
static void attach_cb(void *cb_ctx,
		      const struct spdk_nvme_transport_id *NVM_UNUSED(trid),
		      struct spdk_nvme_ctrlr *ctrlr,
		      const struct spdk_nvme_ctrlr_opts *NVM_UNUSED(opts))
{
	struct state *state = cb_ctx;
	int num_ns = spdk_nvme_ctrlr_get_num_ns(ctrlr);

	// NOTE: namespace IDs start at 1, not 0.
	for (int nsid = 1; nsid <= num_ns; nsid++) {
		struct spdk_nvme_ns *ns = NULL;
		
		ns = spdk_nvme_ctrlr_get_ns(ctrlr, nsid);
		if (ns == NULL) {
			NVM_DEBUG("skipping invalid nsid: %d", nsid);
			continue;
		}
		if (!spdk_nvme_ns_is_active(ns)) {
			NVM_DEBUG("skipping inactive nsid: %d", nsid);
			continue;
		}
		
		state->ns = ns;
		state->nsid = nsid;
		state->ctrlr = ctrlr;
		state->attached = 1;

		break;
	}
}

static void nvm_be_spdk_close(struct nvm_dev *dev)
{
	struct state *state = NULL;

	if (!(dev && dev->be_state))
		return;

	state = dev->be_state;

	if (state->qpair) {
		spdk_nvme_ctrlr_free_io_qpair(state->qpair);
		omp_destroy_lock(&state->qpair_lock);
	}

	if (state->ctrlr)
		spdk_nvme_detach(state->ctrlr);

	free(state);
}

static struct nvm_dev *nvm_be_spdk_open(const char *dev_path,
					int NVM_UNUSED(flags))
{
	int err;
	struct nvm_dev *dev = NULL;
	struct state *state = NULL;

	dev = malloc(sizeof(*dev));
	if (!dev)
		return NULL;	// Propagate `errno` from malloc
	memset(dev, 0, sizeof(*dev));

	state = malloc(sizeof(*state));
	if (!state) {
		nvm_be_spdk_close(dev);
		return NULL;	// Propagate `errno` from malloc
	}
	memset(state, 0, sizeof(*state));

	dev->be_state = state;

	/*
	 * SPDK relies on an abstraction around the local environment named env
	 * that handles memory allocation and PCI device operations.  This
	 * library must be initialized first.
	 */
	spdk_env_opts_init(&(state->opts));
	
	state->opts.name = "liblightnvm";
	state->opts.shm_id = 0;
	state->opts.master_core = 0;

	spdk_env_init(&(state->opts));
	
	/*
	 * Parse the dev_path into transport_id so we can use it to compare to
	 * the probed controller
	 */
	state->trid.trtype = SPDK_NVME_TRANSPORT_PCIE;

	err = spdk_nvme_transport_id_parse(&state->trid, dev_path);
	if (err) {
		errno = -err;

		NVM_DEBUG("FAILED parsing dev_path: %s, err: %d", dev_path, err);
		nvm_be_spdk_close(dev);
		return NULL;
	}

	/*
	 * Start the SPDK NVMe enumeration process. probe_cb will be called for
	 * each NVMe controller found, giving our application a choice on
	 * whether to attach to each controller. attach_cb will then be called
	 * for each controller after the SPDK NVMe driver has completed
	 * initializing the controller we chose to attach.
	 */
	err = spdk_nvme_probe(&state->trid, state, probe_cb, attach_cb, NULL);
	if (err) {
		NVM_DEBUG("FAILED: spdk_nvme_probe(...) -- retrying...");

		err = spdk_nvme_probe(&state->trid, state, probe_cb, attach_cb,
				      NULL);
		if (err) {
			NVM_DEBUG("FAILED: spdk_nvme_probe(...)");
			nvm_be_spdk_close(dev);
			return NULL;
		}
	}

	if (!state->attached) {
		NVM_DEBUG("FAILED: attaching NVMe controller");
		nvm_be_spdk_close(dev);
		return NULL;
	}

	// Setup NVMe IO qpair
	state->qpair = spdk_nvme_ctrlr_alloc_io_qpair(state->ctrlr, NULL, 0);
	if (!state->qpair) {
		NVM_DEBUG("FAILED: allocating qpair");
		nvm_be_spdk_close(dev);
		return NULL;
	}

	// Setup IO qpair lock
	omp_init_lock(&state->qpair_lock);

	err = nvm_be_populate(dev, &nvm_be_spdk);
	if (err) {
		NVM_DEBUG("FAILED: nvm_be_populate, err(%d)", err);
		nvm_be_spdk_close(dev);
		return NULL;
	}

	err = nvm_be_populate_derived(dev);
	if (err) {
		NVM_DEBUG("FAILED: nvm_be_populate_derived");
		nvm_be_spdk_close(dev);
		return NULL;
	}

	NVM_DEBUG("Good to go!?");

	return dev;
}

struct nvm_be nvm_be_spdk = {
	.id = NVM_BE_SPDK,

	.open = nvm_be_spdk_open,
	.close = nvm_be_spdk_close,

	.idfy = nvm_be_spdk_idfy,
	.rprt = nvm_be_spdk_rprt,
	.sbbt = nvm_be_spdk_sbbt,
	.gbbt = nvm_be_spdk_gbbt,

	.erase = nvm_be_spdk_erase,
	.write = nvm_be_spdk_write,
	.read = nvm_be_spdk_read,
	.copy = nvm_be_spdk_copy,

};
#endif

