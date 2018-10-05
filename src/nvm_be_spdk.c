/*
 * be_spdk - Backend using SPDK
 *
 * Copyright (C) Simon A. F. Lund <slund@cnexlabs.com>
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
	.name = "NVM_BE_SPDK",

	.open = nvm_be_nosys_open,
	.close = nvm_be_nosys_close,

	.idfy = nvm_be_nosys_idfy,
	.rprt = nvm_be_nosys_rprt,
	.gfeat = nvm_be_nosys_gfeat,
	.sfeat = nvm_be_nosys_sfeat,
	.sbbt = nvm_be_nosys_sbbt,
	.gbbt = nvm_be_nosys_gbbt,

	.scalar_erase = nvm_be_nosys_scalar_erase,
	.scalar_write = nvm_be_nosys_scalar_write,
	.scalar_read = nvm_be_nosys_scalar_read,

	.vector_erase = nvm_be_nosys_vector_erase,
	.vector_write = nvm_be_nosys_vector_write,
	.vector_read = nvm_be_nosys_vector_read,
	.vector_copy = nvm_be_nosys_vector_copy,

	.async_init = nvm_be_nosys_async_init,
	.async_term = nvm_be_nosys_async_term,
	.async_poke = nvm_be_nosys_async_poke,
	.async_wait = nvm_be_nosys_async_wait,
};
#else
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <spdk/nvme_ocssd.h>
#include <liblightnvm.h>
#include <nvm_be.h>
#include <nvm_async.h>
#include <nvm_be_spdk.h>
#include <nvm_dev.h>

/**
 * Attaches only to the device matching the traddr
 */
static bool probe_cb(void *cb_ctx, const struct spdk_nvme_transport_id *trid,
		     struct spdk_nvme_ctrlr_opts *opts)
{
	struct nvm_be_spdk_state *state = cb_ctx;

	if (spdk_nvme_transport_id_compare(&state->trid, trid)) {
		NVM_DEBUG("trid->traddr: %s != state->trid.traddr: %s",
			  trid->traddr, state->trid.traddr);
		return false;
	}

	/* Disable CMB sqs / cqs for now due to shared PMR / CMB */
	opts->use_cmb_sqs = false;

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
	struct nvm_be_spdk_state *state = cb_ctx;
	int num_ns = spdk_nvme_ctrlr_get_num_ns(ctrlr);

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

void nvm_be_spdk_close(struct nvm_dev *dev)
{
	struct nvm_be_spdk_state *state = NULL;

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

struct nvm_dev *nvm_be_spdk_open(const char *dev_path, int NVM_UNUSED(flags))
{
	int err;
	struct nvm_dev *dev = NULL;
	struct nvm_be_spdk_state *state = NULL;

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

	// Setup NVMe IO qpair for SYNC commands
	state->qpair = spdk_nvme_ctrlr_alloc_io_qpair(state->ctrlr, NULL, 0);
	if (!state->qpair) {
		NVM_DEBUG("FAILED: allocating qpair");
		nvm_be_spdk_close(dev);
		return NULL;
	}

	// Setup IO qpair lock for SYNC commands
	omp_init_lock(&state->qpair_lock);

	const struct spdk_nvme_ns_data *nsdata = spdk_nvme_ns_get_data(state->ns);
	if (nsdata == NULL) {
		NVM_DEBUG("FAILED: spdk_nvme_ns_get_data");
		nvm_be_spdk_close(dev);
		return NULL;
	}

	dev->ns = *(struct nvm_nvme_ns *) nsdata;

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

	NVM_DEBUG("Let's go!");

	return dev;
}

/**
 * Context for asynchronous command submission and completion
 *
 * The context is not thread-safe and the intent is that the user must
 * initialize the opaque `struct nvm_async_ctx` via nvm_async_init() pr. thread,
 * which is then delegated to the backend, in this case NVM_BE_SPDK, which then
 * initialized a struct containing what it needs for a submission / completion
 * path, in the case of NVM_BE_SPDK, then a qpair is needed and thus allocated
 * and de-allocated by:
 *
 * The NVM_BE_SPDK specific context is a SPDK qpair and it is carried inside:
 *
 * nvm_async_ctx->be_ctx
 *
 */
struct nvm_async_ctx *nvm_be_spdk_async_init(struct nvm_dev *dev,
					     uint32_t depth,
					     uint16_t NVM_UNUSED(flags))
{
	struct nvm_be_spdk_state *state = dev->be_state;
	struct spdk_nvme_io_qpair_opts qpair_opts = { 0 };
	struct nvm_async_ctx *ctx = NULL;

	spdk_nvme_ctrlr_get_default_io_qpair_opts(state->ctrlr, &qpair_opts,
						  sizeof(qpair_opts));

	if (depth) {
		NVM_DEBUG("depth: %d given, overwriting defaults", depth);
		NVM_DEBUG("default_qpair_opts: { iqr: %u, iqs: %u, prio: %d }",
			  qpair_opts.io_queue_requests,
			  qpair_opts.io_queue_size,
			  qpair_opts.qprio);

		qpair_opts.io_queue_size = depth;
		qpair_opts.io_queue_requests = depth * 2;
	}

	NVM_DEBUG("qpair_opts: { iqr: %u, iqs: %u, prio: %d }",
		  qpair_opts.io_queue_requests,
		  qpair_opts.io_queue_size,
		  qpair_opts.qprio);

	ctx = calloc(1, sizeof(*ctx));
	if (!ctx) {
		NVM_DEBUG("FAILED: calloc, ctx: %p, errno: %s",
			  (void*)ctx, strerror(errno));
		// Propagate errno
		return NULL;
	}

	ctx->depth = qpair_opts.io_queue_size;

	ctx->be_ctx = spdk_nvme_ctrlr_alloc_io_qpair(state->ctrlr,
						     &qpair_opts,
						     sizeof(qpair_opts));
	if (!ctx->be_ctx) {
		NVM_DEBUG("FAILED: alloc. qpair errno: %s", strerror(errno));
		free(ctx);
		// Propagate errno
		return NULL;
	}

	return ctx;
}

int nvm_be_spdk_async_term(struct nvm_dev *NVM_UNUSED(dev),
			   struct nvm_async_ctx *ctx)
{
	if (!ctx) {
		NVM_DEBUG("FAILED: ctx: %p", (void*)ctx);
		errno = EINVAL;
		return -1;
	}

	if (!ctx->be_ctx) {
		NVM_DEBUG("FAILED: be_ctx: %p", (void*)ctx->be_ctx);
		errno = EINVAL;
		return -1;
	}

	{
		struct spdk_nvme_qpair *qpair = ctx->be_ctx;
		int err = spdk_nvme_ctrlr_free_io_qpair(qpair);
		if (err) {
			NVM_DEBUG("FAILED: free qpair: %p, errno: %s",
				  (void*)qpair, strerror(errno));
			// Propagate errno
			return -1;
		}

		free(ctx);
	}

	return 0;
}

int nvm_be_spdk_async_poke(struct nvm_dev *NVM_UNUSED(dev),
			   struct nvm_async_ctx *ctx, uint32_t max)
{
	struct spdk_nvme_qpair *qpair = ctx->be_ctx;
	int32_t res;

	res = spdk_nvme_qpair_process_completions(qpair, max);
	if (res < 0) {
		NVM_DEBUG("FAILED: processing completions: res: %d", res);
		return -1;
	}

	return res;
}

int nvm_be_spdk_async_wait(struct nvm_dev *dev, struct nvm_async_ctx *ctx)
{
	int acc = 0;

	while(ctx->outstanding) {
		int res;

		res = nvm_be_spdk_async_poke(dev, ctx, 0);
		if (res < 0) {
			NVM_DEBUG("FAILED: nvm_be_spdk_async_poke");
			return -1;
		}

		acc += res;
	}

	return acc;
}

struct cpl_ctx {
	struct spdk_nvme_cpl cpl;
	bool completed;
};

static void cmd_sync_admin_cb(void *cb_arg, const struct spdk_nvme_cpl *cpl)
{
	struct cpl_ctx *ctx = cb_arg;

	if (spdk_nvme_cpl_is_error(cpl)) {
		NVM_DEBUG("FAILED: spdk_nvme_cpl_is_error");
	}

	memcpy(&ctx->cpl, cpl, sizeof(*cpl));
	ctx->completed = 1;
}

static inline int cmd_sync_admin(struct nvm_dev *dev, struct nvm_nvme_cmd *cmd,
			      void *buf, size_t buf_len, struct nvm_ret *ret)
{
	struct nvm_be_spdk_state *state = dev->be_state;
	struct spdk_nvme_cmd *nvme_cmd = (struct spdk_nvme_cmd *)cmd;
	struct cpl_ctx ctx = { 0 };
	void *buf_dma = NULL;

	if (buf && buf_len) {
		size_t alloc_len = NVM_BE_SPDK_ALIGN * ((buf_len + NVM_BE_SPDK_ALIGN - 1) / NVM_BE_SPDK_ALIGN);

		buf_dma = spdk_dma_malloc(alloc_len, NVM_BE_SPDK_ALIGN, NULL);
		if (!buf_dma) {
			NVM_DEBUG("FAILED: spdk_dma_malloc");
			return -1;
		}
	}

	if (spdk_nvme_ctrlr_cmd_admin_raw(state->ctrlr, nvme_cmd, buf_dma,
					  buf_len, cmd_sync_admin_cb, &ctx)) {
		NVM_DEBUG("FAILED: spdk_nvme_ctrlr_cmd_admin_raw");
		spdk_dma_free(buf_dma);
		return -1;
	}

	while (!ctx.completed) {
		spdk_nvme_ctrlr_process_admin_completions(state->ctrlr);
	}

	if (buf && buf_len) {
		memcpy(buf, buf_dma, buf_len);
		spdk_dma_free(buf_dma);
	}

	if (ret) {
		ret->result.cdw0 = ctx.cpl.cdw0;
		ret->status = ctx.cpl.status.sc
				| (ctx.cpl.status.sct << 8)
				| (ctx.cpl.status.m   << 13)
				| (ctx.cpl.status.dnr << 14);
	}

	return 0;
}

static int cmd_sync_admin_glp(struct nvm_dev *dev, void *buf, uint32_t ndw,
			     uint64_t lpo, int opt, struct nvm_ret *ret)
{
	struct nvm_nvme_cmd cmd = { 0 };
	struct nvm_be_spdk_state *state = dev->be_state;

	if (lpo & 0x3) {
		errno = EINVAL;
		return -1;
	}

	cmd.opcode = NVM_AOPC_RPRT;
	cmd.nsid = state->nsid;
	cmd.rprt.lid = 0xCA;
	cmd.rprt.lsp = opt;
	cmd.rprt.numdu = ndw >> 16;
	cmd.rprt.numdl = ndw & 0xffff;
	cmd.lpou = lpo >> 32;
	cmd.lpol = lpo & 0xfffffff;

	if (cmd_sync_admin(dev, &cmd, buf, (ndw + 1) * 4, ret)) {
		NVM_DEBUG("FAILED: cmd_sync_admin");
		return -1;
	}

	return 0;
}

struct nvm_spec_idfy *nvm_be_spdk_idfy(struct nvm_dev *dev, struct nvm_ret *ret)
{
	struct nvm_be_spdk_state *state = dev->be_state;
	struct nvm_spec_idfy *idfy = NULL;
	const size_t idfy_len = sizeof(*idfy);

	struct nvm_nvme_cmd cmd = { 0 };

	cmd.opcode = NVM_AOPC_IDFY;
	cmd.nsid = state->nsid;

	idfy = nvm_buf_alloca(NVM_BE_SPDK_ALIGN, idfy_len);
	if (!idfy) {
		errno = ENOMEM;
		return NULL;
	}
	memset(idfy, 0, sizeof(*idfy));

	if (cmd_sync_admin(dev, &cmd, idfy, idfy_len, ret)) {
		NVM_DEBUG("FAILED: cmd_sync_admin");
		return NULL;
	}

	return idfy;
}

int nvm_be_spdk_gfeat(struct nvm_dev *dev, uint8_t id,
		       union nvm_spec_feat *feat,
		       struct nvm_ret *ret)
{
	struct nvm_be_spdk_state *state = dev->be_state;

	struct nvm_nvme_cmd cmd = { 0 };
	struct nvm_ret _ret;

	cmd.opcode = NVM_AOPC_GFEAT;
	cmd.nsid = state->nsid;
	cmd.gfeat.fid = id;

	ret = ret ? ret : &_ret;

	if (cmd_sync_admin(dev, &cmd, NULL, 0, ret)) {
		NVM_DEBUG("FAILED: cmd_sync_admin");
		return -1;
	}

	*((uint32_t *) feat) = ret->result.cdw0;

	return 0;
}

int nvm_be_spdk_sfeat(struct nvm_dev *dev, uint8_t id,
		       const union nvm_spec_feat *feat,
		       struct nvm_ret *ret)
{
	struct nvm_be_spdk_state *state = dev->be_state;

	struct nvm_nvme_cmd cmd = { 0 };

	cmd.opcode = NVM_AOPC_SFEAT;
	cmd.nsid = state->nsid;
	cmd.sfeat.fid = id;
	cmd.sfeat.feat = *feat;

	if (cmd_sync_admin(dev, &cmd, NULL, 0, ret)) {
		NVM_DEBUG("FAILED: cmd_sync_admin");
		return -1;
	}

	return 0;
}

struct nvm_spec_rprt *nvm_be_spdk_rprt(struct nvm_dev *dev,
					      struct nvm_addr *addr,
					      int NVM_UNUSED(opt),
					      struct nvm_ret *ret)
{
	const size_t DESCR_NBYTES = sizeof(struct nvm_spec_rprt_descr);
	const struct nvm_geo *geo = nvm_dev_get_geo(dev);
	size_t lpo_off = addr ? nvm_addr_gen2lpo(dev, *addr) : 0;
	struct nvm_spec_rprt *rprt = NULL;
	size_t rprt_len, ndescr;

	if (NVM_SPEC_VERID_20 != dev->verid) {
		errno = EINVAL;
		return NULL;
	}

	ndescr = addr ? geo->l.nchunk : geo->l.nchunk * geo->l.npunit * geo->l.npugrp;
	rprt_len = ndescr * DESCR_NBYTES + sizeof(rprt->ndescr);

	rprt = nvm_buf_alloca(NVM_BE_SPDK_ALIGN, rprt_len);
	if (!rprt) {
		errno = ENOMEM;
		return NULL;
	}
	memset(rprt, 0, rprt_len);
	rprt->ndescr = ndescr;

	for (size_t i = 0; i < ndescr; i += 0x1000) {
		const size_t count = NVM_MIN(0x1000, ndescr - i);

		if (cmd_sync_admin_glp(dev, &rprt->descr[i],
				      (count * DESCR_NBYTES) / 4 - 1,
				      lpo_off + i * DESCR_NBYTES, 0x0, ret)) {
			free(rprt);
			return NULL;
		}
	}

	return rprt;
}

struct nvm_spec_bbt *nvm_be_spdk_gbbt(struct nvm_dev *dev, struct nvm_addr addr,
				      struct nvm_ret *ret)
{
	const struct nvm_geo *geo = nvm_dev_get_geo(dev);
	struct nvm_be_spdk_state *state = dev->be_state;

	const uint32_t nblks = dev->geo.g.nblocks * dev->geo.g.nplanes;
	struct nvm_spec_bbt *bbt = NULL;
	const size_t bbt_len = sizeof(*bbt) + sizeof(*(bbt->blk)) * nblks;

	struct nvm_nvme_cmd cmd = { 0 };

	bbt = nvm_buf_alloc(geo, bbt_len);
	if (!bbt) {
		errno = ENOMEM;
		return NULL;
	}
	memset(bbt, 0, sizeof(*bbt));

	cmd.opcode = NVM_AOPC_GBBT;
	cmd.nsid = state->nsid;
	cmd.addrs = nvm_addr_gen2dev(dev, addr);

	if (cmd_sync_admin(dev, &cmd, bbt, bbt_len, ret)) {
		NVM_DEBUG("FAILED: cmd_sync_admin");
		nvm_buf_free(bbt);
		return NULL;
	}

	return bbt;
}

int nvm_be_spdk_sbbt(struct nvm_dev *dev, struct nvm_addr *addrs,
			    int naddrs, uint16_t flags, struct nvm_ret *ret)
{
	const size_t addrs_len = naddrs * sizeof(uint64_t);
	uint64_t addrs_phys = 0;
	uint64_t *addrs_dma = NULL;

	struct nvm_nvme_cmd cmd = { 0 };

	cmd.opcode = NVM_AOPC_SBBT; // Construct command
	cmd.s12.control = flags;
	cmd.s12.naddrs = naddrs - 1;

	if (cmd.s12.naddrs) {
		addrs_dma = spdk_dma_malloc(addrs_len, NVM_BE_SPDK_ALIGN,
					    &addrs_phys);
		if (!addrs_dma) {
			NVM_DEBUG("FAILED: spdk_dma_malloc(addrs)");
			return -1;
		}

		for (int i = 0; i < naddrs; ++i)
			addrs_dma[i] = nvm_addr_gen2dev(dev, addrs[i]);

		cmd.addrs = addrs_phys;
	} else {
		cmd.addrs = nvm_addr_gen2dev(dev, addrs[0]);
	}

	if (cmd_sync_admin(dev, &cmd, NULL, 0, ret)) {
		NVM_DEBUG("FAILED: cmd_sync_admin");
		spdk_dma_free(addrs_dma);
		return -1;
	}

	spdk_dma_free(addrs_dma);

	return 0;
}

//
// TODO: Fix this cmd-wrapping by moving the DMA allocations up into
// 'nvm_buf_alloc' or similar function and thereby achieving zero-copy from
// application to device and much cleaner code...
//
struct cmd_wrap {
	struct nvm_ret *ret;

	int opcode;
	struct nvm_nvme_cmd cmd;
	struct spdk_nvme_cmd *nvme_cmd;

	void *data;
	void *data_dma;
	size_t data_len;

	struct spdk_nvme_dsm_range *dsmr;

	void *meta;
	void *meta_dma;
	size_t meta_len;

	int naddrs;
	uint64_t addr_dfmt;	// Address on device-format for SCALAR CMDS

	uint64_t *addrs_dma;	// DMA-allocated addresses
	size_t addrs_len;	// # nbytes of DMA-allocated addresses

	uint64_t *dst_dma;

	int completed;		// When used in SYNC callbacks
};

static inline void wrap_term(struct cmd_wrap *wrap)
{
	/* TODO: hoist up into 'nvm_buf_alloc' */
	spdk_dma_free(wrap->data_dma);
	spdk_dma_free(wrap->meta_dma);

	spdk_dma_free(wrap->addrs_dma);
	spdk_dma_free(wrap->dst_dma);
	free(wrap);
}

static inline void wrap_cpl(struct cmd_wrap *wrap,
			    const struct spdk_nvme_cpl *cpl)
{
	if (wrap->ret) {
		wrap->ret->result.cdw0 = cpl->cdw0;
		wrap->ret->status = cpl->status.sc
					| (cpl->status.sct << 8)
					| (cpl->status.m   << 13)
					| (cpl->status.dnr << 14);
	}

	switch(wrap->cmd.opcode) {
	case NVM_DOPC_SCALAR_READ:
	case NVM_DOPC_VECTOR_READ:
	case NVM_DOPC_VECTOR_ERASE:
		if (wrap->data) {
			memcpy(wrap->data, wrap->data_dma, wrap->data_len);
		}
		if (wrap->meta) {
			memcpy(wrap->meta, wrap->meta_dma, wrap->meta_len);
		}
	default:
		break;
	}

	wrap->completed = spdk_nvme_cpl_is_error(cpl) ? -1 : 1;
}

static void cmd_sync_cb(void *cb_arg, const struct spdk_nvme_cpl *cpl)
{
	wrap_cpl(cb_arg, cpl);
}

static void cmd_async_cb(void *cb_arg, const struct spdk_nvme_cpl *cpl)
{
	struct cmd_wrap *wrap = cb_arg;

	wrap->ret->async.ctx->outstanding -= 1;

	wrap_cpl(wrap, cpl);
	wrap->ret->async.cb(wrap->ret, wrap->ret->async.cb_arg);
	wrap_term(wrap);
}

/**
 * Setup submission entry and allocate DMA memory for the given opcode
 */
static inline struct cmd_wrap *wrap_setup(struct nvm_dev *dev, int opcode,
					  void *data, void *meta,
					  struct nvm_addr addrs[],
					  struct nvm_addr dst[],
					  int naddrs,
					  int flags,
					  struct nvm_ret *ret)
{
	struct nvm_be_spdk_state *state = dev->be_state;
	const struct nvm_geo *geo = &dev->geo;
	struct nvm_nvme_cmd *cmd = NULL;
	struct cmd_wrap *wrap;

	wrap = calloc(1, sizeof(*wrap));
	if (!wrap) {
		NVM_DEBUG("FAILED: allocating wrap");
		// Propagate errno from calloc
		return NULL;
	}

	wrap->nvme_cmd = (struct spdk_nvme_cmd *)&wrap->cmd;
	cmd = &wrap->cmd;

	cmd->opcode = opcode;
	cmd->nsid = state->nsid;

	// Forward all arguments to 'struct cmd_wrap'
	wrap->completed = 0;
	wrap->ret = ret;
	wrap->opcode = opcode;
	wrap->data = data;
	wrap->meta = meta;
	wrap->naddrs = naddrs;
	wrap->addrs_len = naddrs * sizeof(uint64_t);

	if (NVM_DOPC_SCALAR_ERASE == opcode) {
		wrap->data_len = sizeof(*wrap->dsmr) * naddrs;
		wrap->data_dma = spdk_dma_malloc(wrap->data_len,
						 NVM_BE_SPDK_ALIGN, NULL);
		if (!wrap->data_dma) {
			NVM_DEBUG("FAILED: nvm_buf_alloc of DSM range");
			goto failed;
		}

		wrap->dsmr = wrap->data_dma;
		for(int idx = 0; idx < naddrs; ++idx) {
			const uint64_t slba = nvm_addr_gen2dev(dev, addrs[idx]);

			wrap->dsmr[idx].attributes.raw = 0;
			wrap->dsmr[idx].length = geo->l.nsectr;
			wrap->dsmr[idx].starting_lba = slba;
		}

		cmd->dsm.nr = naddrs - 1;
		cmd->dsm.ad = 1;

		return wrap;
	}

	switch (dev->verid) {
	case NVM_SPEC_VERID_12:
		cmd->s12.naddrs = naddrs - 1;
		cmd->s12.control = flags;

		wrap->data_len = data ? geo->g.sector_nbytes * naddrs : 0;
		wrap->meta_len = geo->g.meta_nbytes * naddrs;
		break;

	case NVM_SPEC_VERID_20:
		cmd->ewrc.naddrs = naddrs - 1;
		wrap->data_len = data ? geo->l.nbytes * naddrs : 0;
		wrap->meta_len = meta ? geo->l.nbytes_oob * naddrs : 0;
		break;

	default:
		NVM_DEBUG("FAILED: dev->verid: %d", dev->verid);
		errno = EINVAL;
		goto failed;
	}
	if (opcode == NVM_DOPC_VECTOR_ERASE) {
		wrap->meta_len = meta ? naddrs * sizeof(struct nvm_spec_rprt_descr) : 0;
	}

	// Do DMA allocation on behalf of the user
	// TODO: hoist up into nvm_buf_alloc
	if (data) {		// Allocate and populate data
		wrap->data_dma = spdk_dma_malloc(wrap->data_len,
						 NVM_BE_SPDK_ALIGN, NULL);
		if (!wrap->data_dma) {
			NVM_DEBUG("FAILED: spdk_dma_malloc(data_dma)");
			goto failed;
		}

		switch (opcode) {
		case NVM_DOPC_VECTOR_WRITE:
		case NVM_DOPC_SCALAR_WRITE:
			memcpy(wrap->data_dma, wrap->data, wrap->data_len);
		default:
			break;
		}
	}

	// Do DMA allocation on behalf of the user
	// TODO: hoist up into nvm_buf_alloc
	if (meta) {		// Allocate and populate meta
		uint64_t meta_phys = 0;

		wrap->meta_dma = spdk_dma_malloc(wrap->meta_len,
						 NVM_BE_SPDK_ALIGN,
						 &meta_phys);
		if (!wrap->meta_dma) {
			NVM_DEBUG("FAILED: spdk_dma_malloc(meta_dma)");
			goto failed;
		}

		switch (opcode) {
		case NVM_DOPC_VECTOR_WRITE:
		case NVM_DOPC_SCALAR_WRITE:
			memcpy(wrap->meta_dma, wrap->meta, wrap->meta_len);
		default:
			break;
		}

		wrap->nvme_cmd->mptr = meta_phys;
	}

	switch (opcode) {
		break;

	case NVM_DOPC_SCALAR_WRITE:
	case NVM_DOPC_SCALAR_READ:
		cmd->addrs = nvm_addr_gen2dev(dev, addrs[0]);
		/* FALLTHRU */

	case NVM_DOPC_SCALAR_ERASE:
		return wrap;		// For SCALAR we are done preparing

	default:			// For VECTOR we continue
		break;
	}

	// DMA allocation and address translation for VECTOR commands
	if (naddrs > 1) {
		uint64_t addrs_phys = 0;

		wrap->addrs_dma = spdk_dma_malloc(wrap->addrs_len,
						  NVM_BE_SPDK_ALIGN,
						  &addrs_phys);
		if (!wrap->addrs_dma) {
			NVM_DEBUG("FAILED: spdk_dma_malloc(addrs)");
			goto failed;
		}

		for (int i = 0; i < naddrs; ++i) {
			wrap->addrs_dma[i] = nvm_addr_gen2dev(dev, addrs[i]);
		}

		cmd->addrs = addrs_phys;
	} else {
		cmd->addrs = nvm_addr_gen2dev(dev, addrs[0]);
	}

	if (dst) {		// Addrs. for COPY(DST)
		if (naddrs > 1) {
			uint64_t dst_phys = 0;

			wrap->dst_dma = spdk_dma_malloc(wrap->addrs_len,
							NVM_BE_SPDK_ALIGN,
							&dst_phys);
			if (!wrap->dst_dma) {
				NVM_DEBUG("FAILED: spdk_dma_malloc(dst)");
				goto failed;
			}

			for (int i = 0; i < naddrs; ++i) {
				wrap->dst_dma[i] = nvm_addr_gen2dev(dev, dst[i]);
			}
			cmd->addrs_dst = dst_phys;
		} else {
			cmd->addrs_dst = nvm_addr_gen2dev(dev, dst[0]);
		}
	}

	return wrap;

failed:
	wrap_term(wrap);
	// Propagate errno
	return NULL;
}

static inline int cmd_async_ewrc(struct nvm_dev *dev, struct nvm_addr addrs[],
				 struct nvm_addr dst[], int naddrs,
				 void *data, void *meta, uint16_t flags,
				 int opcode, struct nvm_ret *ret)
{
	struct nvm_be_spdk_state *state = dev->be_state;
	struct spdk_nvme_qpair *qpair = ret->async.ctx->be_ctx;
	struct cmd_wrap *wrap = NULL;
	int err = 0;

	// Early exit when queue is full
	if ((ret->async.ctx->outstanding + 1) > ret->async.ctx->depth) {
		errno = EAGAIN;
		return -1;
	}

	// Setup CMD arguments
	wrap = wrap_setup(dev, opcode, data, meta, addrs, dst, naddrs, flags,
			  ret);
	if (!wrap) {
		NVM_DEBUG("FAILED: allocating cmd_wrap");
		// Propagate errno from calloc
		return -1;
	}

	// Submit command
	ret->async.ctx->outstanding += 1;

	err = spdk_nvme_ctrlr_cmd_io_raw_with_md(state->ctrlr,
						 qpair,
						 wrap->nvme_cmd,
						 wrap->data_dma,
						 wrap->data_len,
						 wrap->meta_dma,
						 cmd_async_cb,
						 wrap);
	if (err) {
		ret->async.ctx->outstanding -= 1;
		NVM_DEBUG("FAILED: submission failed");
		goto failed;
	}

	return 0;

failed:
	wrap_term(wrap);
	// Propagate errno
	return -1;
}

static inline int cmd_sync_ewrc(struct nvm_dev *dev,
				       struct nvm_addr addrs[],
				       struct nvm_addr dst[], int naddrs,
				       void *data, void *meta, uint16_t flags,
				       int opcode, struct nvm_ret *ret)
{
	struct nvm_be_spdk_state *state = dev->be_state;
	struct spdk_nvme_qpair *qpair = state->qpair;
	omp_lock_t *qpair_lock = &state->qpair_lock;

	struct cmd_wrap *wrap = NULL;
	int res = 0;
	int err;

	wrap = wrap_setup(dev, opcode, data, meta, addrs, dst, naddrs, flags,
			  ret);
	if (!wrap) {
		NVM_DEBUG("FAILED: allocating cmd_wrap");
		// Propagate errno from calloc
		return -1;
	}

	// Submit command
	omp_set_lock(qpair_lock);
	err = spdk_nvme_ctrlr_cmd_io_raw_with_md(state->ctrlr,
						 qpair,
						 wrap->nvme_cmd,
						 wrap->data_dma,
						 wrap->data_len,
						 wrap->meta_dma,
						 cmd_sync_cb,
						 wrap);
	omp_unset_lock(qpair_lock);

	if (err) {
		NVM_DEBUG("FAILED: cmd_sync_ewrc, err: %d", err);
		res = -1;
		goto out;
	}

	// Wait for completion
	while (!wrap->completed) {
		omp_set_lock(qpair_lock);
		spdk_nvme_qpair_process_completions(qpair, 0);
		omp_unset_lock(qpair_lock);
	}

	if (wrap->completed < 0) {
		res = -1;
		errno = EIO;
	}

out:
	wrap_term(wrap);

	return res;
}

int nvm_be_spdk_scalar_erase(struct nvm_dev *dev, struct nvm_addr addrs[],
			     int naddrs, uint16_t flags,
			     struct nvm_ret *ret)
{
	switch(flags & NVM_CMD_MASK_IOMD) {
	case NVM_CMD_ASYNC:
		if ((!ret) || (!ret->async.ctx) || (!ret->async.ctx->be_ctx)) {
			NVM_DEBUG("FAILED: ret: %p", (void*)ret);
			errno = EINVAL;
			return -1;
		}
		return cmd_async_ewrc(dev, addrs, NULL, naddrs, NULL, NULL,
				      flags, NVM_DOPC_SCALAR_ERASE, ret);

	case NVM_CMD_SYNC:
	default:
		return cmd_sync_ewrc(dev, addrs, NULL, naddrs, NULL, NULL,
				     flags, NVM_DOPC_SCALAR_ERASE, ret);
	}
}

int nvm_be_spdk_scalar_write(struct nvm_dev *dev, struct nvm_addr addr,
			     int naddrs, const void *data, const void *meta,
			     uint16_t flags, struct nvm_ret *ret)
{
	void *cdata = (char *)data;
	void *cmeta = (char *)meta;

	switch(flags & NVM_CMD_MASK_IOMD) {
	case NVM_CMD_ASYNC:
		if ((!ret) || (!ret->async.ctx) || (!ret->async.ctx->be_ctx)) {
			NVM_DEBUG("FAILED: ret: %p", (void*)ret);
			errno = EINVAL;
			return -1;
		}
		return cmd_async_ewrc(dev, &addr, NULL, naddrs,
				      cdata, cmeta,
				      flags, NVM_DOPC_SCALAR_WRITE, ret);

	case NVM_CMD_SYNC:
	default:
		return cmd_sync_ewrc(dev, &addr, NULL, naddrs,
				     cdata, cmeta,
				     flags, NVM_DOPC_SCALAR_WRITE, ret);
	}
}

int nvm_be_spdk_scalar_read(struct nvm_dev *dev, struct nvm_addr addr,
			    int naddrs, void *data, void *meta, uint16_t flags,
			    struct nvm_ret *ret)
{
	switch(flags & NVM_CMD_MASK_IOMD) {
	case NVM_CMD_ASYNC:
		if ((!ret) || (!ret->async.ctx) || (!ret->async.ctx->be_ctx)) {
			NVM_DEBUG("FAILED: ret: %p", (void*)ret);
			errno = EINVAL;
			return -1;
		}
		return cmd_async_ewrc(dev, &addr, NULL, naddrs,
				      data, meta,
				      flags, NVM_DOPC_SCALAR_READ, ret);

	case NVM_CMD_SYNC:
	default:
		return cmd_sync_ewrc(dev, &addr, NULL, naddrs,
				     data, meta,
				     flags, NVM_DOPC_SCALAR_READ, ret);
	}
}

int nvm_be_spdk_vector_erase(struct nvm_dev *dev, struct nvm_addr addrs[],
			     int naddrs, void *meta, uint16_t flags,
			     struct nvm_ret *ret)
{
	switch(flags & NVM_CMD_MASK_IOMD) {
	case NVM_CMD_ASYNC:
		if ((!ret) || (!ret->async.ctx) || (!ret->async.ctx->be_ctx)) {
			NVM_DEBUG("FAILED: ret: %p", (void*)ret);
			errno = EINVAL;
			return -1;
		}
		return cmd_async_ewrc(dev, addrs, NULL, naddrs,
				      NULL, meta,
				      flags, NVM_DOPC_VECTOR_ERASE, ret);

	case NVM_CMD_SYNC:
	default:
		return cmd_sync_ewrc(dev, addrs, NULL, naddrs,
				     NULL, meta,
				     flags, NVM_DOPC_VECTOR_ERASE, ret);
	}
}

int nvm_be_spdk_vector_write(struct nvm_dev *dev,
				    struct nvm_addr addrs[], int naddrs,
				    const void *data, const void *meta,
				    uint16_t flags, struct nvm_ret *ret)
{
	char *cdata = (char *)data;
	char *cmeta = (char *)meta;

	switch(flags & NVM_CMD_MASK_IOMD) {
	case NVM_CMD_ASYNC:
		if ((!ret) || (!ret->async.ctx) || (!ret->async.ctx->be_ctx)) {
			NVM_DEBUG("FAILED: ret: %p", (void*)ret);
			errno = EINVAL;
			return -1;
		}
		return cmd_async_ewrc(dev, addrs, NULL, naddrs,
				      cdata, cmeta,
				      flags, NVM_DOPC_VECTOR_WRITE, ret);

	case NVM_CMD_SYNC:
	default:
		return cmd_sync_ewrc(dev, addrs, NULL, naddrs,
				     cdata, cmeta,
				     flags, NVM_DOPC_VECTOR_WRITE, ret);
	}
}

int nvm_be_spdk_vector_read(struct nvm_dev *dev, struct nvm_addr addrs[],
			    int naddrs, void *data, void *meta, uint16_t flags,
			    struct nvm_ret *ret)
{
	switch(flags & NVM_CMD_MASK_IOMD) {
	case NVM_CMD_ASYNC:
		if ((!ret) || (!ret->async.ctx) || (!ret->async.ctx->be_ctx)) {
			NVM_DEBUG("FAILED: ret: %p", (void*)ret);
			errno = EINVAL;
			return -1;
		}
		return cmd_async_ewrc(dev, addrs, NULL, naddrs,
				      data, meta,
				      flags, NVM_DOPC_VECTOR_READ, ret);

	case NVM_CMD_SYNC:
	default:
		return cmd_sync_ewrc(dev, addrs, NULL, naddrs,
				     data, meta,
				     flags, NVM_DOPC_VECTOR_READ, ret);
	}
}

int nvm_be_spdk_vector_copy(struct nvm_dev *dev, struct nvm_addr src[],
			    struct nvm_addr dst[], int naddrs, uint16_t flags,
			    struct nvm_ret *ret)
{
	switch(flags & NVM_CMD_MASK_IOMD) {
	case NVM_CMD_ASYNC:
		if ((!ret) || (!ret->async.ctx) || (!ret->async.ctx->be_ctx)) {
			NVM_DEBUG("FAILED: ret: %p", (void*)ret);
			errno = EINVAL;
			return -1;
		}
		return cmd_async_ewrc(dev, src, dst, naddrs,
				      NULL, NULL,
				      flags, NVM_DOPC_VECTOR_COPY, ret);

	case NVM_CMD_SYNC:
	default:
		return cmd_sync_ewrc(dev, src, dst, naddrs,
				     NULL, NULL,
				     flags, NVM_DOPC_VECTOR_COPY, ret);
	}
}

struct nvm_be nvm_be_spdk = {
	.id = NVM_BE_SPDK,
	.name = "NVM_BE_SPDK",

	.open = nvm_be_spdk_open,
	.close = nvm_be_spdk_close,

	.async_init = nvm_be_spdk_async_init,
	.async_term = nvm_be_spdk_async_term,
	.async_poke = nvm_be_spdk_async_poke,
	.async_wait = nvm_be_spdk_async_wait,

	.idfy = nvm_be_spdk_idfy,
	.rprt = nvm_be_spdk_rprt,
	.gfeat = nvm_be_spdk_gfeat,
	.sfeat = nvm_be_spdk_sfeat,
	.sbbt = nvm_be_spdk_sbbt,
	.gbbt = nvm_be_spdk_gbbt,

	.scalar_erase = nvm_be_spdk_scalar_erase,
	.scalar_write = nvm_be_spdk_scalar_write,
	.scalar_read = nvm_be_spdk_scalar_read,

	.vector_erase = nvm_be_spdk_vector_erase,
	.vector_write = nvm_be_spdk_vector_write,
	.vector_read = nvm_be_spdk_vector_read,
	.vector_copy = nvm_be_spdk_vector_copy,
};
#endif
