/*
 * async - Implementation of ASYNC public API and internal helper functions
 *
 * Copyright (C) Simon A. F. Lund <slund@cnexlabs.com>
 * Copyright (C) Klaus Birkelund Abildgaard Jensen <klaus.jensen@cnexlabs.com>

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
#include <stdio.h>
#include <errno.h>
#include <liblightnvm.h>
#include <nvm_be.h>
#include <nvm_dev.h>
#include <nvm_async.h>

struct nvm_async_ctx *nvm_async_init(struct nvm_dev *dev, uint32_t depth,
				     uint16_t flags)
{
	return dev->be->async_init(dev, depth, flags);
}

int nvm_async_term(struct nvm_dev *dev, struct nvm_async_ctx *ctx)
{
	return dev->be->async_term(dev, ctx);
}

int nvm_async_wait(struct nvm_dev *dev, struct nvm_async_ctx *ctx)
{
	return dev->be->async_wait(dev, ctx);
}

int nvm_async_poke(struct nvm_dev *dev, struct nvm_async_ctx *ctx, uint32_t max)
{
	return dev->be->async_poke(dev, ctx, max);
}

uint32_t nvm_async_get_depth(struct nvm_async_ctx *ctx) {
	return ctx->depth;
}
