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
#include <sys/callout.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/timespec.h>

struct _freebsd_timer_list
{
    struct callout  cl;
    void      *ftn;
    void           *data;
};

/**
 * @brief delays in kernel context
 */
void noinline kfio_udelay(unsigned int usecs)
{
    DELAY(usecs);
}

/**
 * @brief delays in kernel context
 */
void noinline kfio_msleep(unsigned int millisecs)
{
    int timo = (int) ((((uint64_t) millisecs) * ((uint64_t)hz)) / 1000ULL);
#if __FreeBSD_version >= 700000
    pause("kfioSL",  timo);
#else
    static int pause_wchan;

    tsleep(&pause_wchan, 0, "kfioSL", timo);
#endif
}

void noinline kfio_msleep_noload(unsigned int millisecs)
{
    kfio_msleep(millisecs);
}

/**
 * @brief returns number of clock ticks since last system reboot
 */
uint64_t noinline fusion_get_lbolt(void)
{
    struct timeval tv;
    getmicrouptime(&tv);
    return (uint64_t) tvtohz(&tv);
}

/**
 * @brief returns number of seconds since last system reboot
 */
uint32_t noinline kfio_get_seconds(void)
{
    struct timeval tv;
    getmicrouptime(&tv);
    return (uint32_t) (tv.tv_sec);
}


/**
 * @brief returns current time in nanoseconds
 */
uint64_t noinline fusion_getnanotime(void)
{
    struct timespec ts;

    getnanotime(&ts);
    return (uint64_t) ((uint64_t)ts.tv_sec * 1000000000ULL) + ((uint64_t)ts.tv_nsec);
}

/**
 * @brief returns current time in microseconds
 */
uint64_t noinline fusion_getmicrotime(void)
{
    struct timeval tv;

    getmicrouptime(&tv);
    return (uint64_t) ((uint64_t)tv.tv_sec * 1000000ULL) + ((uint64_t)tv.tv_usec);
}

/**
 * @brief return current UTC wall clock time in seconds since the Unix
 *  epoch (Jan 1 1970).
 */
uint64_t noinline fusion_getwallclocktime(void)
{
    struct timeval tv;

    getmicrotime(&tv);
    return (uint64_t)tv.tv_sec;
}

/**
 * @brief returns # ticks per second
 */
int noinline fusion_HZ(void)
{
    return hz;
}
/*******************************************************************************
*******************************************************************************/
uint64_t noinline fusion_usectohz(uint64_t usec)
{
    return ((usec * (uint64_t)hz) / 1000000LL);
}

/*******************************************************************************
*******************************************************************************/
uint64_t noinline fusion_hztousec(uint64_t hertz)
{
    return (uint64_t)tick * hertz;
}

/**
 * support for delayed one shots
 */
void noinline fusion_init_timer(struct fusion_timer_list* timer)
{
    struct _freebsd_timer_list *ftimer;

    ftimer = (struct _freebsd_timer_list *)timer;

    ftimer->ftn  = NULL;
    ftimer->data = 0;

    callout_init(&ftimer->cl, 1);
}

/**
 *
 */
void noinline fusion_set_timer_function(struct fusion_timer_list* timer,
                                        void (*f) (fio_uintptr_t))
{
    struct _freebsd_timer_list *ftimer;

    ftimer = (struct _freebsd_timer_list *)timer;
    ftimer->ftn = (void *)f;
}

/**
 *
 */
void noinline fusion_set_timer_data(struct fusion_timer_list* timer,
                                    fio_uintptr_t d)
{
    struct _freebsd_timer_list *ftimer;

    ftimer = (struct _freebsd_timer_list *)timer;
    ftimer->data = (void *)d;
}

/**
 * @note t in ticks
 */
void noinline fusion_set_relative_timer(struct fusion_timer_list* timer, uint64_t t)
{
    struct _freebsd_timer_list *ftimer;

    ftimer = (struct _freebsd_timer_list *)timer;

    callout_reset(&ftimer->cl, t, ftimer->ftn, ftimer->data);
}
/**
 *
 */
void noinline fusion_del_timer(struct fusion_timer_list* timer)
{
    struct _freebsd_timer_list *ftimer;

    ftimer = (struct _freebsd_timer_list *)timer;

    callout_drain(&ftimer->cl);
}
