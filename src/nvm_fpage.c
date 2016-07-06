/*
 * fpage - Flash page functions
 *
 * Copyright (C) 2015 Javier González <javier@cnexlabs.com>
 * Copyright (C) 2015 Matias Bjørling <matias@cnexlabs.com>
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
#include <liblightnvm.h>
#include <nvm.h>

struct nvm_fpage* nvm_fpage_new(void)
{
	struct nvm_fpage *fpage = malloc(sizeof(*fpage));
	if (fpage)
		memset(fpage, 0, sizeof(*fpage));

	return fpage;
}

void nvm_fpage_free(struct nvm_fpage **fpage)
{
	if (!fpage || !*fpage)
		return;

	free(fpage);
	fpage = NULL;
}

void nvm_fpage_pr(struct nvm_fpage *fpage)
{
	printf("fpage: sec_size(%d), page_size(%d), pln_pg_size(%d)"
		", max_sec_io(%d)\n",
		fpage->sec_size, fpage->page_size, fpage->pln_pg_size,
		fpage->max_sec_io);
}

uint32_t nvm_fpage_get_sec_size(struct nvm_fpage *fpage)
{
	return fpage->sec_size;
}

uint32_t nvm_fpage_get_page_size(struct nvm_fpage *fpage)
{
	return fpage->page_size;
}

uint32_t nvm_fpage_get_pln_pg_size(struct nvm_fpage *fpage)
{
	return fpage->pln_pg_size;
}

uint32_t nvm_fpage_get_max_sec_io(struct nvm_fpage *fpage)
{
	return fpage->max_sec_io;
}
