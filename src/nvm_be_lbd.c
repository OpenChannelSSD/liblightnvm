/*
 * be_lbd - IOCTL be using Linux Block Device (LBD) for read, write and erase
 *
 * Copyright (C) 2015-2017 Javier Gonzáles <javier@cnexlabs.com>
 * Copyright (C) 2015-2017 Matias Bjørling <matias@cnexlabs.com>
 * Copyright (C) 2015-2017 Simon A. F. Lund <slund@cnexlabs.com>
 * Copyright (C) 2018 Klaus B. A. Jensen <klaus.jensen@cnexlabs.com>
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

#ifndef NVM_BE_LBD_ENABLED
struct nvm_be nvm_be_lbd = {
	.id = NVM_BE_LBD,
	.name = "NVM_BE_LBD",

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
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/fs.h>
#include <time.h>
#include <sys/ioctl.h>
#include <nvm_be_ioctl.h>
#include <nvm_dev.h>
#include <nvm_utils.h>
#include <nvm_debug.h>
#include <nvm_async.h>

#ifdef HAVE_LIBAIO
#include <libaio.h>
#define NVM_BE_LBD_ASYNC_DEFAULT_IODEPTH 256

typedef void (*io_prep_fn)(struct iocb *iocb, int fd, void *buf, size_t count, long long offset);

struct nvm_be_lbd_async_state {
	io_context_t aio_ctx;
	struct io_event *aio_events;
	struct iocb **iocbs;
	uint32_t depth;
};

int nvm_be_lbd_async_rw(struct nvm_dev *dev, int naddrs, void *data,
		        const off_t offset, struct nvm_ret *ret,
			io_prep_fn prep_fn)
{
	struct nvm_async_ctx *ctx = ret->async.ctx;
	struct nvm_be_lbd_async_state *state = ctx->be_ctx;

	if (ctx->outstanding == state->depth) {
		errno = EAGAIN;
		return -1;
	}

	struct iocb *iocb = state->iocbs[ctx->outstanding++];

	prep_fn(iocb, dev->fd, data, dev->geo.l.nbytes * naddrs, offset);
	iocb->data = ret;

	int r = io_submit(state->aio_ctx, 1, &iocb);
	if (r < 0) {
		errno = -r;
		return -1;
	}

	return 0;
}

int nvm_be_lbd_async_getevents(struct nvm_async_ctx *ctx, unsigned int min,
			       unsigned int max, struct timespec *timeout)
{
	struct nvm_be_lbd_async_state *state = ctx->be_ctx;

	int r, nevents = 0;
	while (ctx->outstanding) {
		if (0 == (r = io_getevents(state->aio_ctx, min, max, state->aio_events, timeout))) {
			break;
		}

		if (r < 0) return -r;

		nevents += r;

		for (int i = 0; i < r; i++) {
			struct nvm_ret *ret = state->aio_events[i].data;

			ret->async.cb(ret, ret->async.cb_arg);

			state->iocbs[--(ctx->outstanding)] = state->aio_events[i].obj;
		}
	}

	return nevents;
}

int nvm_be_lbd_async_poke(struct nvm_dev *NVM_UNUSED(dev), struct nvm_async_ctx *ctx, uint32_t max)
{
	struct nvm_be_lbd_async_state *state = ctx->be_ctx;

	struct timespec timeout = { 0, 0 };
	if (!max) {
		max = state->depth;
	}

	return nvm_be_lbd_async_getevents(ctx, 0, max, &timeout);
}


int nvm_be_lbd_async_wait(struct nvm_dev *NVM_UNUSED(dev), struct nvm_async_ctx *ctx)
{
	struct nvm_be_lbd_async_state *state = ctx->be_ctx;

	return nvm_be_lbd_async_getevents(ctx, ctx->outstanding, state->depth, NULL);
}

struct nvm_async_ctx *nvm_be_lbd_async_init(struct nvm_dev *NVM_UNUSED(dev), uint32_t depth, uint16_t NVM_UNUSED(flags))
{
	struct nvm_async_ctx *ctx = calloc(1, sizeof(*ctx));

	struct nvm_be_lbd_async_state *state = calloc(1, sizeof(*state));

	if (!depth) {
		depth = NVM_BE_LBD_ASYNC_DEFAULT_IODEPTH;
	}

	state->depth = depth;
	state->aio_events = calloc(depth, sizeof(struct io_event));
	state->iocbs = calloc(depth, sizeof(struct iocb *));

	for (unsigned int i = 0; i < depth; i++) {
		state->iocbs[i] = calloc(1, sizeof(struct iocb));
	}

	ctx->be_ctx = state;

	int err;
	if (0 != (err = io_queue_init(depth, &state->aio_ctx))) {
		errno = -err;
		return NULL;
	}

	return ctx;
}

int nvm_be_lbd_async_term(struct nvm_dev *NVM_UNUSED(dev), struct nvm_async_ctx *ctx)
{
	struct nvm_be_lbd_async_state *state = ctx->be_ctx;

	for (unsigned int i = 0; i < state->depth; i++) {
		free(state->iocbs[i]);
	}

	free(state->aio_events);
	free(state->iocbs);

	int err;
	if (0 != (err = io_queue_release(state->aio_ctx))) {
		errno = -err;
		return -1;
	}

	free(state);
	free(ctx);

	return 0;
}
#endif

static int nvm_be_lbd_scalar_erase(struct nvm_dev *dev, struct nvm_addr addrs[],
				   int naddrs, uint16_t flags,
				   struct nvm_ret *NVM_UNUSED(ret))
{
#ifdef HAVE_LIBAIO
	if (flags & NVM_CMD_ASYNC) {
		NVM_DEBUG("FAILED: flag NVM_CMD_ASYNC not supported for lbd scalar erase");
		errno = EINVAL;
		return -1;
	}
#else
	// silence gcc
	(void)flags;
#endif

	for (int i = 0; i < naddrs; i++) {
		uint64_t range[2];
		int err;

		range[0] = nvm_addr_gen2off(dev, addrs[i]);
		range[1] = dev->geo.l.nsectr << dev->ssw;

		err = ioctl(dev->fd, BLKDISCARD, &range);
		if (err) {
			NVM_DEBUG("FAILED: BLKDISCARD, err: %d, %s", err,
				  strerror(errno));
			// Propagate errno
			return -1;
		}
	}

	return 0;
}

int nvm_be_lbd_scalar_read(struct nvm_dev *dev, struct nvm_addr addr,
			   int naddrs, void *data, void *meta, uint16_t flags,
			   struct nvm_ret *ret)
{
	const off_t offset = nvm_addr_gen2off(dev, addr);
	ssize_t res;

	if (meta) {
		NVM_DEBUG("FAILED: LBD read with meta is not supported");
		errno = ENOSYS;
		return -1;
	}

#ifdef HAVE_LIBAIO
	if (flags & NVM_CMD_ASYNC) {
		return nvm_be_lbd_async_rw(dev, naddrs, data, offset,
					   ret, io_prep_pread);
	}
#else
	// silence gcc
	(void)flags;
	(void)ret;
#endif

	res = pread(dev->fd, data, dev->geo.l.nbytes * naddrs, offset);
	if (res < 0) {
		NVM_DEBUG("FAILED: res: %zd, errno: %s", res, strerror(errno));
		// Propagate errno
		return -1;
	}

	return 0;
}

int nvm_be_lbd_scalar_write(struct nvm_dev *dev, struct nvm_addr addr,
			    int naddrs, const void *data, const void *meta,
			    uint16_t flags, struct nvm_ret *ret)
{
	const off_t offset = nvm_addr_gen2off(dev, addr);
	ssize_t res;

	if (meta) {
		NVM_DEBUG("FAILED: LBD write with meta is not supported");
		errno = ENOSYS;
		return -1;
	}

#ifdef HAVE_LIBAIO
	if (flags & NVM_CMD_ASYNC) {
		return nvm_be_lbd_async_rw(dev, naddrs, (void*) data, offset,
					   ret, io_prep_pwrite);
	}
#else
	// silence gcc
	(void)flags;
	(void)ret;
#endif

	res = pwrite(dev->fd, data, dev->geo.l.nbytes * naddrs, offset);
	if (res < 0) {
		NVM_DEBUG("FAILED: res: %zd, errno: %s", res, strerror(errno));
		// Propagate errno
		return -1;
	}

	return 0;
}

struct nvm_dev *nvm_be_lbd_open(const char *dev_path, int NVM_UNUSED(flags))
{
	struct nvm_dev *dev;

	dev = nvm_be_ioctl_open(dev_path, NVM_BE_IOCTL_WRITABLE);
	if (!dev) {
		NVM_DEBUG("FAILED: opening via IOCTL_WRITABLE");
		// Propagate errno
		return NULL;
	}

	return dev;
}

struct nvm_be nvm_be_lbd = {
	.id = NVM_BE_LBD,
	.name = "NVM_BE_LBD",

	.open = nvm_be_lbd_open,
	.close = nvm_be_ioctl_close,

	.idfy = nvm_be_ioctl_idfy,
	.rprt = nvm_be_ioctl_rprt,
	.gfeat = nvm_be_ioctl_gfeat,
	.sfeat = nvm_be_ioctl_sfeat,
	.sbbt = nvm_be_ioctl_sbbt,
	.gbbt = nvm_be_ioctl_gbbt,

	.scalar_erase = nvm_be_lbd_scalar_erase,
	.scalar_write = nvm_be_lbd_scalar_write,
	.scalar_read = nvm_be_lbd_scalar_read,

	.vector_erase = nvm_be_ioctl_vector_erase,
	.vector_write = nvm_be_ioctl_vector_write,
	.vector_read = nvm_be_ioctl_vector_read,
	.vector_copy = nvm_be_ioctl_vector_copy,

#ifdef HAVE_LIBAIO
	.async_init = nvm_be_lbd_async_init,
	.async_term = nvm_be_lbd_async_term,
	.async_poke = nvm_be_lbd_async_poke,
	.async_wait = nvm_be_lbd_async_wait,
#else
	.async_init = nvm_be_nosys_async_init,
	.async_term = nvm_be_nosys_async_term,
	.async_poke = nvm_be_nosys_async_poke,
	.async_wait = nvm_be_nosys_async_wait,
#endif
};
#endif
