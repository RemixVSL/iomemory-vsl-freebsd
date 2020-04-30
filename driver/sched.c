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
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/taskqueue.h>


struct _freebsd_work_struct
{
    fusion_work_func_t func;
    struct task task;
};

/*******************************************************************************
 * FreeBSD tasks will call the following function which will call the
 *   fusion_work_func_t passed in as a member of the OS-specific work struct.
 */
static void fusion_task_function(void *context, int pending)
{
    struct _freebsd_work_struct *pw = (struct _freebsd_work_struct *) context;

    pw->func((struct fusion_work_struct *)pw);
}

void noinline fusion_setup_dpc(fusion_dpc_t *wq, fusion_work_func_t func)
{
    struct _freebsd_work_struct *pw = (struct _freebsd_work_struct *) wq;

    pw->func = func;
    TASK_INIT(&pw->task, 0, fusion_task_function, pw);
}

void noinline fusion_setup_delayed_dpc(fusion_delayed_dpc_t *wq, fusion_work_func_t func)
{
    struct _freebsd_work_struct *pw = (struct _freebsd_work_struct *) wq;

    pw->func = func;
    TASK_INIT(&pw->task, 0, fusion_task_function, pw);
}

/**
 */
void noinline fusion_schedule_dpc(fusion_dpc_t *work)
{
    fusion_schedule_work(work);
}

/**
 * @brief schedule some work
 */
void noinline fusion_schedule_work(fusion_dpc_t *work )
{
    struct _freebsd_work_struct *pw = (struct _freebsd_work_struct *) work;

    taskqueue_enqueue(taskqueue_thread, &pw->task);
}

/**
 * @brief schedule delayed work
 * @param wq
 * @param sleep_time - delay time in micro seconds.
 */
void fusion_schedule_delayed_work(fusion_delayed_dpc_t *wq, uint64_t sleep_time )
{
    timeout((timeout_t *)fusion_schedule_work, (void *) wq, fusion_usectohz(sleep_time) );
}
