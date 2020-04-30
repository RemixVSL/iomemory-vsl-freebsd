//-----------------------------------------------------------------------------
// Copyright (c) 2006-2014, Fusion-io, Inc.(acquired by SanDisk Corp. 2014)
// Copyright (c) 2014-2015, SanDisk Corp. and/or all its affiliates. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name of the SanDisk Corp. nor the names of its contributors
//   may be used to endorse or promote products derived from this software
//   without specific prior written permission.
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
// OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------
#if !defined (__FreeBSD__)
#error This file supports FreeBSD only
#endif

#include "port-internal.h"
#include <sys/condvar.h>

#include <fio/port/kcondvar.h>
#include <fio/port/dbgset.h>

/**
*/
void noinline fusion_schedule_yield(void)
{
#if  __FreeBSD_version >= 900032
    kern_yield(PRI_UNCHANGED);
#else
    sched_yield(curthread, NULL);
#endif
}

/**
*/
void noinline fusion_condvar_init(fusion_condvar_t *cv, const char *name)
{
    cv_init((struct cv *) cv, name);
}

/**
 */
void noinline fusion_condvar_destroy(fusion_condvar_t *cv)
{
    cv_destroy((struct cv *) cv);
}

/**
 */
void noinline fusion_condvar_signal(fusion_condvar_t *cv)
{
    cv_signal((struct cv *) cv);
}

/**
 */
void noinline fusion_condvar_broadcast(fusion_condvar_t *cv)
{
    cv_broadcast((struct cv *) cv);
}

/**
 */
void noinline fusion_condvar_wait(fusion_condvar_t *cv,
                                  fusion_cv_lock_t *lock)
{
    cv_wait((struct cv *) cv, (struct mtx *) lock);
}

/**
 * timeout is in usecs
 */
int noinline fusion_condvar_timedwait(fusion_condvar_t *cv,
                                     fusion_cv_lock_t *lock,
                                     int64_t timeout_us)
{
    int ret;

    ret = cv_timedwait((struct cv *) cv, (struct mtx *) lock,
                       fusion_usectohz(timeout_us));

    if (ret == EWOULDBLOCK)
    {
        return FUSION_WAIT_TIMEDOUT;
    }
    else if (ret)
    {
        return FUSION_WAIT_INTERRUPTED;
    }
    return FUSION_WAIT_TRIGGERED;
}

/**
 */
int noinline fusion_condvar_timedwait_noload(fusion_condvar_t *cv,
                                             fusion_cv_lock_t *lock,
                                             int64_t timeout)
{
    return fusion_condvar_timedwait(cv, lock, timeout);
}

/**
 */
uint64_t noinline fusion_condvar_timedwait_noload_elapsed(fusion_condvar_t *cv,
                                                          fusion_cv_lock_t *lock,
                                                          int64_t timeout)
{
    uint64_t now = fusion_getmicrotime();

    fusion_condvar_timedwait(cv, lock, timeout);
    return fusion_getmicrotime() - now;
}

void noinline fusion_cond_resched(void)
{
}
