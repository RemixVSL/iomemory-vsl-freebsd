//-----------------------------------------------------------------------------
// Copyright (c) 2006-2014, Fusion-io, Inc.(acquired by SanDisk Corp. 2014)
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
//-----------------------------------------------------------------------------
/** @file include/fio/port/freebsd/kblock.h
 *     NO OS-SPECIFIC REFERENCES ARE TO BE IN THIS FILE
 *
 */
#ifndef __FIO_PORT_FREEBSD_KBLOCK_H__
#define __FIO_PORT_FREEBSD_KBLOCK_H__

struct kfio_disk
{
    struct fio_device           *fio_dev;
    struct kfio_freebsd_pci_dev *pci_dev;
    struct disk                 *dp;
    enum { RUNNING, DEAD }       dev_state;
    fusion_condvar_t             bio_cv;
    fusion_cv_lock_t             bio_lock;
    struct bio_queue_head       *bio_queue;
};

/*
 * Functions implemented by the FreeBSD-specific kblock implementation
 * in porting layer.
 */
int  kfio_block_create_device(kfio_pci_dev_t *pcidev, struct fio_device *dev,
                              const char *name, uint64_t capacity, uint32_t sector_size,
                              struct kfio_disk **diskp);
int  kfio_block_expose_device(struct kfio_disk *disk);
void kfio_block_destroy_device(struct kfio_disk *disk);
void kfio_block_get_device_name(struct kfio_disk *disk, char *name, size_t size);
kfio_bio_t *kfio_block_dequeue_bio(struct kfio_disk *disk);

#endif // __FIO_PORT_FREEBSD_KBLOCK_H__
