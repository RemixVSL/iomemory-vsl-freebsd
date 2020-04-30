
//-----------------------------------------------------------------------------
// Copyright (c) 2006-2014, Fusion-io, Inc.(acquired by SanDisk Corp. 2014)
// Copyright (c) 2014-2015 SanDisk Corp. and/or all its affiliates. All rights reserved.
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

/*
 * This is a list of Kernel Feature config flags.  They should only be
 * necessary when e vendor kernel deviates from the features of a standard,
 * kernel.org kernel.  If a feature is necessary for a particular kernel
 * then a secondary file should be created with the name
 * kffio-<VERSION>.<PATCHLEVEL>.<SUBLEVEL><EXTRAVERSION>.h.  This is an
 * example for a particular RHEL 5.1 kernel:
 *
 *        kffio-2.6.18-53.1.14.el5debug
 *
 * The feature flags should then be #define'd in that file as described in
 * the usage comment.  Any flag that is not #define'd will be treated as
 * working equivalently to a stock kernel.org kernel of the equivalent
 * VERSION, PATCHLEVEL and SUBLEVEL.
 */

#ifndef _FIO_PORT_FREEBSD_KFIO_CONFIG_H_
#define _FIO_PORT_FREEBSD_KFIO_CONFIG_H_


#define KFIOC_WORKDATA_PASSED_IN_WORKSTRUCT 0
#define KFIOC_HAS_IRQF_SHARED 0

#define KFIOC_HAS_PCI_ERROR_HANDLERS 0


#define KFIOC_HAS_GLOBAL_REGS_POINTER 0

#define KFIOC_HAS_SYSRQ_KEY_OP_ENABLE_MASK 0

#define KFIOC_PCI_REGISTER_DRIVER_RETURNS_0_ON_SUCCESS 0
#define KFIOC_HAS_DISK_STATS_READ_WRITE_ARRAYS 0

#define KFIOC_HAS_DEBUGFS 0

#define KFIOC_HAS_LINUX_SCATTERLIST_H 0

#define KFIOC_KMEM_CACHE_CREATE_REMOVED_DTOR 0

#define KFIOC_BIO_ENDIO_REMOVED_BYTES_DONE 0

#define KFIOC_INVALIDATE_BDEV_REMOVED_DESTROY_DIRTY_BUFFERS 0
#define KFIOC_HAS_CONFIG_OUT_OF_LINE_PFN_TO_PAGE 0
#define KFIOC_NEEDS_VIRT_TO_PHYS 0

#define KFIOC_NEEDS_SYSRQ_SWAP_KEY_OPS 0
#define KFIOC_HAS_BLK_UNPLUG 0

#define KFIOC_HAS_KMEM_CACHE 0
#define KFIOC_NEEDS_GFP_T 0

#define KFIOC_HAS_BLK_BARRIER 0


#define KFIOC_HAS_MUTEX_SUBSYSTEM 0

#define KFIOC_HAS_INTPTR_T 0

#define KFIOC_STRUCT_FILE_HAS_PATH 0

#define KFIOC_UNREGISTER_BLKDEV_RETURNS_VOID 0

#define KFIOC_PARTITION_STATS 0

#define KFIOC_HAS_LONG_BITS 0

#define KFIOC_HAS_NEW_BLOCK_METHODS 0

#define KFIOC_DISCARD 0

#define KFIOC_ 0

#endif /* _FIO_PORT_FREEBSD_KFIO_CONFIG_H_ */

