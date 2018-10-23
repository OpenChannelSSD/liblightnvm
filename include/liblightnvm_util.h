/*
 * liblightnvm utils - Misc. utilities used internally, examples, tests, etc.
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
#ifndef __LIBLIGHTNVM_UTIL_H
#define __LIBLIGHTNVM_UTIL_H
#include <string.h>
#include <stdio.h>

#define NVM_UNIVERSAL_SECT_SH 9

/**
 * Macro to suppress warnings on unused arguments, thanks to stackoverflow.
 */
#ifdef __GNUC__
#  define NVM_UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
#else
#  define NVM_UNUSED(x) UNUSED_ ## x
#endif

#define NVM_I64_FMT	"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c"\
			"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c"\
			"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c"\
			"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c"

#define NVM_I64_TO_STR(val) \
	(val & (1ULL << 63) ? '1' : '0'), \
	(val & (1ULL << 62) ? '1' : '0'), \
	(val & (1ULL << 61) ? '1' : '0'), \
	(val & (1ULL << 60) ? '1' : '0'), \
	(val & (1ULL << 59) ? '1' : '0'), \
	(val & (1ULL << 58) ? '1' : '0'), \
	(val & (1ULL << 57) ? '1' : '0'), \
	(val & (1ULL << 56) ? '1' : '0'), \
	(val & (1ULL << 55) ? '1' : '0'), \
	(val & (1ULL << 54) ? '1' : '0'), \
	(val & (1ULL << 53) ? '1' : '0'), \
	(val & (1ULL << 52) ? '1' : '0'), \
	(val & (1ULL << 51) ? '1' : '0'), \
	(val & (1ULL << 50) ? '1' : '0'), \
	(val & (1ULL << 49) ? '1' : '0'), \
	(val & (1ULL << 48) ? '1' : '0'), \
	(val & (1ULL << 47) ? '1' : '0'), \
	(val & (1ULL << 46) ? '1' : '0'), \
	(val & (1ULL << 45) ? '1' : '0'), \
	(val & (1ULL << 44) ? '1' : '0'), \
	(val & (1ULL << 43) ? '1' : '0'), \
	(val & (1ULL << 42) ? '1' : '0'), \
	(val & (1ULL << 41) ? '1' : '0'), \
	(val & (1ULL << 40) ? '1' : '0'), \
	(val & (1ULL << 39) ? '1' : '0'), \
	(val & (1ULL << 38) ? '1' : '0'), \
	(val & (1ULL << 37) ? '1' : '0'), \
	(val & (1ULL << 36) ? '1' : '0'), \
	(val & (1ULL << 35) ? '1' : '0'), \
	(val & (1ULL << 34) ? '1' : '0'), \
	(val & (1ULL << 33) ? '1' : '0'), \
	(val & (1ULL << 32) ? '1' : '0'), \
	(val & (1ULL << 31) ? '1' : '0'), \
	(val & (1ULL << 30) ? '1' : '0'), \
	(val & (1ULL << 29) ? '1' : '0'), \
	(val & (1ULL << 28) ? '1' : '0'), \
	(val & (1ULL << 27) ? '1' : '0'), \
	(val & (1ULL << 26) ? '1' : '0'), \
	(val & (1ULL << 25) ? '1' : '0'), \
	(val & (1ULL << 24) ? '1' : '0'), \
	(val & (1ULL << 23) ? '1' : '0'), \
	(val & (1ULL << 22) ? '1' : '0'), \
	(val & (1ULL << 21) ? '1' : '0'), \
	(val & (1ULL << 20) ? '1' : '0'), \
	(val & (1ULL << 19) ? '1' : '0'), \
	(val & (1ULL << 18) ? '1' : '0'), \
	(val & (1ULL << 17) ? '1' : '0'), \
	(val & (1ULL << 16) ? '1' : '0'), \
	(val & (1ULL << 15) ? '1' : '0'), \
	(val & (1ULL << 14) ? '1' : '0'), \
	(val & (1ULL << 13) ? '1' : '0'), \
	(val & (1ULL << 12) ? '1' : '0'), \
	(val & (1ULL << 11) ? '1' : '0'), \
	(val & (1ULL << 10) ? '1' : '0'), \
	(val & (1ULL << 9) ? '1' : '0'), \
	(val & (1ULL << 8) ? '1' : '0'), \
	(val & (1ULL << 7) ? '1' : '0'), \
	(val & (1ULL << 6) ? '1' : '0'), \
	(val & (1ULL << 5) ? '1' : '0'), \
	(val & (1ULL << 4) ? '1' : '0'), \
	(val & (1ULL << 3) ? '1' : '0'), \
	(val & (1ULL << 2) ? '1' : '0'), \
	(val & (1ULL << 1) ? '1' : '0'), \
	(val & (1ULL << 0) ? '1' : '0')

#define NVM_I32_FMT	"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c"\
			"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c"

#define NVM_I32_TO_STR(val) \
	(val & (1ULL << 31) ? '1' : '0'), \
	(val & (1ULL << 30) ? '1' : '0'), \
	(val & (1ULL << 29) ? '1' : '0'), \
	(val & (1ULL << 28) ? '1' : '0'), \
	(val & (1ULL << 27) ? '1' : '0'), \
	(val & (1ULL << 26) ? '1' : '0'), \
	(val & (1ULL << 25) ? '1' : '0'), \
	(val & (1ULL << 24) ? '1' : '0'), \
	(val & (1ULL << 23) ? '1' : '0'), \
	(val & (1ULL << 22) ? '1' : '0'), \
	(val & (1ULL << 21) ? '1' : '0'), \
	(val & (1ULL << 20) ? '1' : '0'), \
	(val & (1ULL << 19) ? '1' : '0'), \
	(val & (1ULL << 18) ? '1' : '0'), \
	(val & (1ULL << 17) ? '1' : '0'), \
	(val & (1ULL << 16) ? '1' : '0'), \
	(val & (1ULL << 15) ? '1' : '0'), \
	(val & (1ULL << 14) ? '1' : '0'), \
	(val & (1ULL << 13) ? '1' : '0'), \
	(val & (1ULL << 12) ? '1' : '0'), \
	(val & (1ULL << 11) ? '1' : '0'), \
	(val & (1ULL << 10) ? '1' : '0'), \
	(val & (1ULL << 9) ? '1' : '0'), \
	(val & (1ULL << 8) ? '1' : '0'), \
	(val & (1ULL << 7) ? '1' : '0'), \
	(val & (1ULL << 6) ? '1' : '0'), \
	(val & (1ULL << 5) ? '1' : '0'), \
	(val & (1ULL << 4) ? '1' : '0'), \
	(val & (1ULL << 3) ? '1' : '0'), \
	(val & (1ULL << 2) ? '1' : '0'), \
	(val & (1ULL << 1) ? '1' : '0'), \
	(val & (1ULL << 0) ? '1' : '0')

#define NVM_I16_FMT	"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c"

#define NVM_I16_TO_STR(val) \
	(val & (1ULL << 15) ? '1' : '0'), \
	(val & (1ULL << 14) ? '1' : '0'), \
	(val & (1ULL << 13) ? '1' : '0'), \
	(val & (1ULL << 12) ? '1' : '0'), \
	(val & (1ULL << 11) ? '1' : '0'), \
	(val & (1ULL << 10) ? '1' : '0'), \
	(val & (1ULL << 9) ? '1' : '0'), \
	(val & (1ULL << 8) ? '1' : '0'), \
	(val & (1ULL << 7) ? '1' : '0'), \
	(val & (1ULL << 6) ? '1' : '0'), \
	(val & (1ULL << 5) ? '1' : '0'), \
	(val & (1ULL << 4) ? '1' : '0'), \
	(val & (1ULL << 3) ? '1' : '0'), \
	(val & (1ULL << 2) ? '1' : '0'), \
	(val & (1ULL << 1) ? '1' : '0'), \
	(val & (1ULL << 0) ? '1' : '0')

#define NVM_I8_FMT	"%c%c%c%c%c%c%c%c"

#define NVM_I8_TO_STR(val) \
	(val & (1ULL << 7) ? '1' : '0'), \
	(val & (1ULL << 6) ? '1' : '0'), \
	(val & (1ULL << 5) ? '1' : '0'), \
	(val & (1ULL << 4) ? '1' : '0'), \
	(val & (1ULL << 3) ? '1' : '0'), \
	(val & (1ULL << 2) ? '1' : '0'), \
	(val & (1ULL << 1) ? '1' : '0'), \
	(val & (1ULL << 0) ? '1' : '0')

static inline int NVM_MIN(int x, int y) {
	return x < y ? x : y;
}

static inline int NVM_MAX(int x, int y) {
	return x > y ? x : y;
}

#ifdef NVM_DEBUG_ENABLED

#define NVM_DEBUG_FCALL(x) x

#define __FILENAME__ strrchr("/" __FILE__, '/') + 1

#define NVM_DEBUG(...) fprintf(stderr, "# %s:%s-%d: " FIRST(__VA_ARGS__) "\n" , \
	__FILENAME__, __func__, __LINE__ REST(__VA_ARGS__)); fflush(stderr);

#define FIRST(...) FIRST_HELPER(__VA_ARGS__, throwaway)
#define FIRST_HELPER(first, ...) first

#define REST(...) REST_HELPER(NUM(__VA_ARGS__), __VA_ARGS__)
#define REST_HELPER(qty, ...) REST_HELPER2(qty, __VA_ARGS__)
#define REST_HELPER2(qty, ...) REST_HELPER_##qty(__VA_ARGS__)
#define REST_HELPER_ONE(first)
#define REST_HELPER_TWOORMORE(first, ...) , __VA_ARGS__
#define NUM(...) \
	SELECT_10TH(__VA_ARGS__, TWOORMORE, TWOORMORE, TWOORMORE, TWOORMORE,\
		TWOORMORE, TWOORMORE, TWOORMORE, TWOORMORE, ONE, throwaway)
#define SELECT_10TH(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, ...) a10

#else
	#define NVM_DEBUG(...)
	#define NVM_DEBUG_FCALL(x)
#endif

#endif /* __LIBLIGHTNVM_UTIL_H */
