/*
 * test_rules.h - utilities for rules tests
 *
 * Copyright (C) Klaus B. A. Jensen <klaus.jensen@cnexlabs.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
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

#ifndef __TEST_RULES_H
#define __TEST_RULES_H

#include "test_util.h"

#define MAKE_RULES_TEST_1(type, name, arg)                                    \
	static void test_ ## type ## _ ## name (void)                         \
	{                                                                     \
		SPEC_20_ONLY                                                  \
		_test_ ## name (nvm_test_ ## type ## _ ## arg);               \
	}


#define MAKE_RULES_TEST_2(type, name, arg1, arg2)                             \
	static void test_ ## type ## _ ## name (void)                         \
	{                                                                     \
		SPEC_20_ONLY                                                  \
		_test_ ## name (                                              \
			nvm_test_ ## type ## _ ## arg1,                       \
			nvm_test_ ## type ## _ ## arg2);                      \
	}

#define MAKE_RULES_TEST_3(type, name, arg1, arg2, arg3)                       \
	static void test_ ## type ## _ ## name (void)                         \
	{                                                                     \
		SPEC_20_ONLY                                                  \
		_test_ ## name (                                              \
			nvm_test_ ## type ## _ ## arg1,                       \
			nvm_test_ ## type ## _ ## arg2,                       \
			nvm_test_ ## type ## _ ## arg3);                      \
	}

#define MAKE_RULES_TESTS_1(name, arg)                                         \
	MAKE_RULES_TEST_1(scalar, name, arg)                                  \
	MAKE_RULES_TEST_1(vector, name, arg)

#define MAKE_RULES_TESTS_2(name, arg1, arg2)                                  \
	MAKE_RULES_TEST_2(scalar, name, arg1, arg2)                           \
	MAKE_RULES_TEST_2(vector, name, arg1, arg2)

#define MAKE_RULES_TESTS_3(name, arg1, arg2, arg3)                            \
	MAKE_RULES_TEST_3(scalar, name, arg1, arg2, arg3)                     \
	MAKE_RULES_TEST_3(vector, name, arg1, arg2, arg3)


#endif /* __TEST_RULES_H */
