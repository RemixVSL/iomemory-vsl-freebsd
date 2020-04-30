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

#include <fio/port/dbgset.h>
#if (FUSION_INTERNAL == 1)
#include <fio/internal/config.h>
#endif

// effectively stub out the file if no KTR.
// (should be a better way to do this)
#if (defined(FUSION_INCLUDE_KTR) && FUSION_INCLUDE_KTR)

#if KTR_USE_SYSTEM
// Because FreeBSD already has the KTR facility that this was inspired by,
// we just hook into it if we are in FreeBSD and set KTR_USE_SYSTEM.
#include <fio/port/ktypes.h>
#include <fio/port/fio-port.h>
#include <fio/common/ktr.h>

#define KTR  // get ktr definitions from FreeBSD
#include <sys/ktr.h>
#undef KTR

static fusion_atomic_t eventnum = {0};
// Note that we do not check the event number against teh number of format strings or KTR_EVENT_LAST!
// It's assumed the programmer isn't THAT silly

void ktr_log_freebsd(const char *file, int line, eventtype_t event, int64_t p1, int64_t p2, int64_t p3, int64_t p4, int64_t p5)
{
    ktr_tracepoint(KTR_GEN, file, line, ktr_format_strings[event],
        (u_long) fusion_atomic_incr(&eventnum),
        (u_long)(p1), (u_long)(p2), (u_long)(p3), (u_long)(p4), (u_long)(p5));
}

#endif

#endif /* FUSION_INCLUDE_KTR */

