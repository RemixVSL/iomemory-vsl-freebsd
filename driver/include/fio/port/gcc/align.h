//-----------------------------------------------------------------------------
// Copyright (c) 2011-2014, Fusion-io, Inc.(acquired by SanDisk Corp. 2014)
// Copyright (c) 2014 SanDisk Corp. and/or all its affiliates. All rights reserved.
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
//----------------------------------------------------------------------------------------------------------------------------------------------------------

#ifndef __FIO_PORT_GCC_ALIGN_H__
#define __FIO_PORT_GCC_ALIGN_H__

#define FUSION_STRUCT_ALIGN(alignment_byte)    __attribute__((aligned(alignment_byte)))

/*
 * Since some incorrect uses of FUSION_STRUCT_ALIGN don't produce a compile
 * error nor do they align, all common code uses of FUSION_STRUCT_ALIGN
 * should verify proper alignment by calling FUSION_STRUCT_ALIGN_VERIFY.
 */
#define FUSION_STRUCT_ALIGN_VERIFY(align_bytes,structure) \
   C_ASSERT(__alignof__(structure) == (align_bytes))

#define DECLARE_RESERVE(x) char foo[x] __attribute__ ((aligned(8)))

#define FUSION_DMA_ALIGN(x)     x __attribute__((aligned(32)))

#endif
