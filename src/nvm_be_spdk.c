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
#include <liblightnvm.h>
#include <nvm_be.h>

#ifndef NVM_BE_SPDK_ENABLED

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
#include <nvm_async.h>
#include <nvm_dev.h>
#include <nvm_cmd.h>
#include <nvm_sgl.h>
#include <nvm_be.h>
#include <nvm_be_spdk.h>

#ifndef NVM_BE_SPDK_HAS_SGL
int spdk_nvme_ctrlr_cmd_iov_raw_with_md(
			struct spdk_nvme_ctrlr *NVM_UNUSED(ctrlr),
			struct spdk_nvme_qpair *NVM_UNUSED(qpair),
			struct spdk_nvme_cmd *NVM_UNUSED(cmd),
			spdk_nvme_cmd_cb NVM_UNUSED(cb_fn),
			void *NVM_UNUSED(cb_arg),
			spdk_nvme_req_reset_sgl_cb NVM_UNUSED(reset_sgl_fn),
			spdk_nvme_req_next_sge_cb NVM_UNUSED(next_sge_fn),
			void *NVM_UNUSED(metadata))
{
	errno = ENOSYS;
	return -1;
}
#endif

static void sgl_reset_cb(void *cb_arg, uint32_t NVM_UNUSED(offset))
{
	struct nvm_sgl *sgl = ((struct nvm_cmd_wrap *) cb_arg)->data;
	nvm_sgl_reset(sgl);
}

static int sgl_next_sge_cb(void *cb_arg, void **address, uint32_t *length)
{
	struct nvm_sgl *sgl = ((struct nvm_cmd_wrap *) cb_arg)->data;
	return nvm_sgl_next_sge(sgl, address, length);
}

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
	if (!dev) {
		// Propagate `errno` from malloc
		return NULL;
	}
	memset(dev, 0, sizeof(*dev));

	state = malloc(sizeof(*state));
	if (!state) {
		nvm_be_spdk_close(dev);
		// Propagate `errno` from malloc
		return NULL;
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
		NVM_DEBUG("FAILED: ..._id_parse(%s), err: %d", dev_path, err);

		errno = -err;
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

	dev->nsid = state->nsid;
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
		qpair_opts.io_queue_size = depth;
		qpair_opts.io_queue_requests = depth * 2;
	}

	ctx = calloc(1, sizeof(*ctx));
	if (!ctx) {
		NVM_DEBUG("FAILED: calloc, ctx: %p, errno: %s",
			  (void*)ctx, strerror(errno));
		// Propagate errno
		return NULL;
	}

	ctx->depth = qpair_opts.io_queue_size;

	ctx->be_ctx = spdk_nvme_ctrlr_alloc_io_qpair(state->ctrlr, &qpair_opts,
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

	if (spdk_nvme_ctrlr_cmd_admin_raw(state->ctrlr, nvme_cmd, buf, buf_len,
					  cmd_sync_admin_cb, &ctx)) {
		NVM_DEBUG("FAILED: spdk_nvme_ctrlr_cmd_admin_raw");
		return -1;
	}

	while (!ctx.completed) {
		spdk_nvme_ctrlr_process_admin_completions(state->ctrlr);
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

	idfy = nvm_buf_alloc(dev, idfy_len, NULL);
	if (!idfy) {
		errno = ENOMEM;
		return NULL;
	}
	memset(idfy, 0, sizeof(*idfy));

	if (cmd_sync_admin(dev, &cmd, idfy, idfy_len, ret)) {
		NVM_DEBUG("FAILED: cmd_sync_admin");
		nvm_buf_free(dev, idfy);
		return NULL;
	}

	return idfy;
}

int nvm_be_spdk_gfeat(struct nvm_dev *dev, uint8_t id,
		       union nvm_nvme_feat *feat,
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

	feat->a = ret->result.cdw0;

	return 0;
}

int nvm_be_spdk_sfeat(struct nvm_dev *dev, uint8_t id,
		       const union nvm_nvme_feat *feat,
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

	rprt = nvm_buf_alloc(dev, rprt_len, NULL);
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
			nvm_buf_free(dev, rprt);
			return NULL;
		}
	}

	return rprt;
}

struct nvm_spec_bbt *nvm_be_spdk_gbbt(struct nvm_dev *dev, struct nvm_addr addr,
				      struct nvm_ret *ret)
{
	struct nvm_be_spdk_state *state = dev->be_state;

	const uint32_t nblks = dev->geo.g.nblocks * dev->geo.g.nplanes;
	struct nvm_spec_bbt *bbt = NULL;
	const size_t bbt_len = sizeof(*bbt) + sizeof(*(bbt->blk)) * nblks;

	struct nvm_nvme_cmd cmd = { 0 };

	bbt = nvm_buf_alloc(dev, bbt_len, NULL);
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
		nvm_buf_free(dev, bbt);
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

static void cmd_sync_cb(void *cb_arg, const struct spdk_nvme_cpl *cpl)
{
	nvm_cmd_wrap_cpl(cb_arg, (const struct nvm_nvme_cpl*)cpl);
}

static void cmd_async_cb(void *cb_arg, const struct spdk_nvme_cpl *cpl)
{
	struct nvm_cmd_wrap *wrap = cb_arg;

	wrap->ret->async.ctx->outstanding -= 1;

	nvm_cmd_wrap_cpl(wrap, (const struct nvm_nvme_cpl*)cpl);
	wrap->ret->async.cb(wrap->ret, wrap->ret->async.cb_arg);
	nvm_cmd_wrap_term(wrap);
}

static inline int cmd_async_ewrc(struct nvm_dev *dev, struct nvm_addr addrs[],
				 struct nvm_addr dst[], int naddrs,
				 void *data, void *meta, uint16_t flags,
				 int opcode, struct nvm_ret *ret)
{
	struct nvm_be_spdk_state *state = dev->be_state;
	struct spdk_nvme_qpair *qpair = ret->async.ctx->be_ctx;
	struct nvm_cmd_wrap *wrap = NULL;
	int err = 0;

	// Early exit when queue is full
	if ((ret->async.ctx->outstanding + 1) > ret->async.ctx->depth) {
		errno = EAGAIN;
		return -1;
	}

	wrap = nvm_cmd_wrap_setup(dev, opcode, data, meta, addrs, dst, naddrs,
				  flags, ret);
	if (!wrap) {
		NVM_DEBUG("FAILED: allocating nvm_cmd_wrap");
		// Propagate errno from calloc
		return -1;
	}

	// Submit command
	ret->async.ctx->outstanding += 1;

	if (flags & NVM_CMD_SGL) {
		err = spdk_nvme_ctrlr_cmd_iov_raw_with_md(state->ctrlr,
						 qpair,
						 (struct spdk_nvme_cmd *)&wrap->cmd,
						 cmd_async_cb,
						 wrap,
						 sgl_reset_cb,
						 sgl_next_sge_cb,
						 wrap->meta);
	} else {
		err = spdk_nvme_ctrlr_cmd_io_raw_with_md(state->ctrlr,
						 qpair,
						 (struct spdk_nvme_cmd *)&wrap->cmd,
						 wrap->data,
						 wrap->data_len,
						 wrap->meta,
						 cmd_async_cb,
						 wrap);
	}

	if (err) {
		ret->async.ctx->outstanding -= 1;
		NVM_DEBUG("FAILED: submission failed");
		goto failed;
	}

	return 0;

failed:
	nvm_cmd_wrap_term(wrap);
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

	struct nvm_cmd_wrap *wrap = NULL;
	int res = 0;
	int err;

	wrap = nvm_cmd_wrap_setup(dev, opcode, data, meta, addrs, dst, naddrs,
				  flags, ret);
	if (!wrap) {
		NVM_DEBUG("FAILED: allocating nvm_cmd_wrap");
		// Propagate errno from calloc
		return -1;
	}

	// Submit command
	omp_set_lock(qpair_lock);
	if (flags & NVM_CMD_SGL) {
		err = spdk_nvme_ctrlr_cmd_iov_raw_with_md(state->ctrlr,
						 qpair,
						 (struct spdk_nvme_cmd *)&wrap->cmd,
						 cmd_sync_cb,
						 wrap,
						 sgl_reset_cb,
						 sgl_next_sge_cb,
						 wrap->meta);
	} else {
		err = spdk_nvme_ctrlr_cmd_io_raw_with_md(state->ctrlr,
						 qpair,
						 (struct spdk_nvme_cmd *)&wrap->cmd,
						 wrap->data,
						 wrap->data_len,
						 wrap->meta,
						 cmd_sync_cb,
						 wrap);
	}
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
	nvm_cmd_wrap_term(wrap);

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
