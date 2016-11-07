/*
 * nvm_debug - liblightnvm debugging macros (internal)
 *
 * Copyright (C) 2015 Javier González <javier@cnexlabs.com>
 * Copyright (C) 2016 Simon A. F. Lund <slund@cnexlabs.com>
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

#ifndef __NVM_DEBUG_H
#define __NVM_DEBUG_H

#include <stdio.h>

#define NVM_ASSERT(c, x, ...) if(!(c)){printf("%s:%s - %d %s" x "\n", \
		__FILE__, __FUNCTION__, __LINE__, strerror(errno), \
		##__VA_ARGS__); fflush(stdout); exit(EXIT_FAILURE); }

#define NVM_ERROR(x, ...) printf("%s:%s - %d %s" x "\n", __FILE__, \
		__FUNCTION__, __LINE__, strerror(errno), ##__VA_ARGS__); \
		fflush(stdout);

#define NVM_FATAL(x, ...) printf("%s:%s - %d %s" x "\n", __FILE__, \
		__FUNCTION__, __LINE__, strerror(errno), ##__VA_ARGS__); \
		fflush(stdout);exit(EXIT_FAILURE)

#ifdef NVM_DEBUG_ENABLED
	#define NVM_DEBUG(x, ...) printf("%s:%s-%d: " x "\n", __FILE__, \
		__FUNCTION__, __LINE__, ##__VA_ARGS__);fflush(stdout);
#else
	#define NVM_DEBUG(x, ...)
#endif
#endif /* __NVM_DEBUG_H */
