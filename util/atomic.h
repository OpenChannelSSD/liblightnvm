/*
 * atomic - atomic operations
 *
 * Copyright (C) 2015 Javier González <javier@cnexlabs.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
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
#ifndef __NVM_ATOMIC_H
#define __NVM_ATOMIC_H

typedef struct atomic_cnt {
	int cnt;
	pthread_spinlock_t lock;
} atomic_cnt;

static inline void atomic_init(struct atomic_cnt *cnt)
{
	pthread_spin_init(&cnt->lock, PTHREAD_PROCESS_SHARED);
}

static inline void atomic_set(struct atomic_cnt *cnt, int value)
{
	pthread_spin_lock(&cnt->lock);
	cnt->cnt = value;
	pthread_spin_unlock(&cnt->lock);
}

static inline void atomic_assign_inc(struct atomic_cnt *cnt, int *dst)
{
	pthread_spin_lock(&cnt->lock);
	cnt->cnt++;
	*dst = cnt->cnt;
	pthread_spin_unlock(&cnt->lock);
}

static inline void atomic_inc(struct atomic_cnt *cnt)
{
	pthread_spin_lock(&cnt->lock);
	cnt->cnt++;
	pthread_spin_unlock(&cnt->lock);
}

static inline int atomic_dec_and_test(struct atomic_cnt *cnt)
{
	int ret;

	pthread_spin_lock(&cnt->lock);
	cnt->cnt--;
	ret = (cnt->cnt == 0) ? 1 : 0;
	pthread_spin_unlock(&cnt->lock);

	return ret;
}

#endif /* __NVM_ATOMIC_H */
