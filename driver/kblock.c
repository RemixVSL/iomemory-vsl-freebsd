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

#include <sys/param.h>
#include <sys/bio.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <geom/geom_disk.h>

#include "port-internal.h"
#include "pci_dev.h"

#include <fio/port/dbgset.h>
#include <fio/port/kfio.h>
#include <fio/port/bitops.h>
#include <fio/port/freebsd/kblock.h>

int iodrive_barrier_sync = 1;

/*******************************************************************************
 */
static int  freebsd_disk_open(struct disk *dev);
static int  freebsd_disk_close(struct disk *dev);
static int  freebsd_disk_ioctl(struct disk *dev, u_long cmd, void *data,
                               int fflag, struct thread *td);
static void freebsd_disk_strategy(struct bio *bp);

/******************************************************************************
 * called from fio_create_blockdev()
 */
int
kfio_block_create_device(kfio_pci_dev_t *pcidev, struct fio_device *dev,
                         const char *name, uint64_t capacity, uint32_t sector_size,
                         struct kfio_disk **diskp)
{
    struct kfio_disk *disk;

    struct disk *dp;
    struct kfio_freebsd_pci_dev *pdev = device_get_softc(pcidev);

    disk = kfio_vmalloc(sizeof(*disk));
    if (disk == NULL)
    {
        *diskp = NULL;
        return -ENOMEM;
    }

    kfio_memset(disk, 0, sizeof(*disk));

    fusion_cv_lock_init(&disk->bio_lock, "fio_bio_lk");
    fusion_condvar_init(&disk->bio_cv,   "fio_bio_cv");

    dp = disk_alloc();

    dp->d_flags    = DISKFLAG_CANDELETE;
    dp->d_open     = freebsd_disk_open;
    dp->d_close    = freebsd_disk_close;
    dp->d_ioctl    = freebsd_disk_ioctl;
    dp->d_strategy = freebsd_disk_strategy;

    dp->d_name = name;
    dp->d_unit = pdev->unit;
    dp->d_maxsize = MAXPHYS;

    dp->d_sectorsize = sector_size;
    dp->d_mediasize  = capacity;
    dp->d_fwsectors  = 63;
    dp->d_fwheads    = 255;

    pdev->p_fio_device = dev;

    dp->d_drv1 = disk;

    disk->dp        = dp;
    disk->pci_dev   = pdev;
    disk->fio_dev   = dev;
    disk->bio_queue = &pdev->bioq;

    disk->dev_state = DEAD;

    *diskp = disk;

    return (0);
}

int
kfio_block_expose_device(struct kfio_disk *disk)
{
    disk->dev_state = RUNNING;
    disk_create(disk->dp, DISK_VERSION);
    return (0);
}

/******************************************************************************
 * called from fio_teardown_blockdev()
 */
void
kfio_block_destroy_device(struct kfio_disk *disk)
{
    if (disk == NULL)
    {
        return;
    }

    disk->pci_dev->p_fio_device = NULL;

    /*
     * Tell strategy routine not to accept any more IO from OS./
     */
    fusion_cv_lock(&disk->bio_lock);
    disk->dev_state = DEAD;
    fusion_cv_unlock(&disk->bio_lock);

    /*
     * Return all incomplete requests with an error.
     */
    bioq_flush(disk->bio_queue, NULL, ENXIO);

    /*
     * Kill the user visible device.
     */
    disk_destroy(disk->dp);

    /*
     * Destroy the private disk structure.
     */
    fusion_condvar_destroy(&disk->bio_cv);
    fusion_cv_lock_destroy(&disk->bio_lock);

    kfio_vfree(disk, sizeof(*disk));
}

void
kfio_block_get_device_name(struct kfio_disk *disk, char *name, size_t size)
{
    kfio_snprintf(name, size, "%s%d", disk->dp->d_name, disk->dp->d_unit);
}

/*******************************************************************************
 */
static int
freebsd_disk_open(struct disk *dp)
{
    struct kfio_disk *disk;
    int ret;

    disk = dp->d_drv1;
    if (NULL == disk || NULL == disk->fio_dev)
        return ENXIO;

    ret = fio_open(disk->fio_dev);
    return (ret < 0) ? -ret: ret;
}

/*******************************************************************************
 */
static int
freebsd_disk_close(struct disk *dp)
{
    struct kfio_disk *disk;
    int ret;

    disk = dp->d_drv1;
    if (NULL == disk || NULL == disk->fio_dev)
        return ENXIO;

    ret = fio_release(disk->fio_dev);
    return (ret < 0) ? -ret: ret;
}

/**
 * @brief ioctl function passed to block device operations structure
 */
static int
freebsd_disk_ioctl(struct disk *dp, u_long cmd, void *data,
                   int fflag, struct thread *td)
{
    struct kfio_disk *disk;
    int ret;

    disk = dp->d_drv1;
    if (NULL == disk || NULL == disk->fio_dev)
        return ENXIO;

    ret = fio_ioctl(disk->fio_dev, (unsigned) cmd, (fio_uintptr_t)((void **)data)[0]);
    return (ret < 0) ? -ret: 0;
}

static void
freebsd_disk_strategy(struct bio *bio)
{
    struct kfio_disk *disk;

    disk = bio->bio_disk->d_drv1;

    if (NULL == disk)
    {
        biofinish(bio, NULL, ENXIO);
    }
    else if (bio->bio_bcount == 0)
    {
        bio->bio_resid = 0;
        biodone(bio);
    }
    else
    {
        fusion_cv_lock(&disk->bio_lock);
        if (disk->dev_state == DEAD)
        {
            biofinish(bio, NULL, ENXIO);
        }
        else
        {
            bio->bio_driver1 = bio->bio_data;
            bioq_disksort(disk->bio_queue, bio);
            fusion_condvar_broadcast(&disk->bio_cv);
        }
        fusion_cv_unlock(&disk->bio_lock);
    }
}

static void
freebsd_bio_completor(kfio_bio_t *fbio, uint64_t bytes_done, int error)
{
    struct bio *bp = (struct bio *)fbio->fbio_parameter;

    /*
     * Unmap DMA data if one was present.
     */
    if (fbio->fbio_cmd == KBIO_CMD_READ || fbio->fbio_cmd == KBIO_CMD_WRITE)
    {
        kfio_sgl_dma_unmap(fbio->fbio_sgl);
    }

    /*
     * Destroy double buffer if user buffer was unaligned and we had to
     * allocate one in kfio_block_dequeue_bio.
     */
    if (bp->bio_driver1 != bp->bio_data)
    {
        if (bp->bio_cmd == BIO_READ)
            kfio_memcpy(bp->bio_driver1, bp->bio_data, bytes_done);
        kfio_vfree(bp->bio_data, (bp->bio_bcount + 7) & ~8);
        bp->bio_data = bp->bio_driver1;
    }

    bp->bio_resid = bp->bio_bcount - bytes_done;
    if (error)
        biofinish(bp, NULL, error < 0 ? - error : error);
    else
        biodone(bp);
}

kfio_bio_t *
kfio_block_dequeue_bio(struct kfio_disk *disk)
{
    struct fio_device *dev;
    struct disk       *dp;
    struct bio        *bp;
    kfio_bio_t        *fbio;
    int                error;

    dp = disk->dp;

    /*
     * Try to get next bio request to proceed.
     */
    bp = bioq_takefirst(disk->bio_queue);
    if (bp == NULL)
    {
        return NULL;
    }

    fusion_cv_unlock(&disk->bio_lock);

    /*
     * If user buffer is not aligned to something that our hardware
     * can handle, allocate temporary buffer and copy data there.
     */
    bp->bio_driver1 = bp->bio_data;

    if (((uintptr_t)bp->bio_data & 7) != 0)
    {
        static int unaligned_warned;

        if (!unaligned_warned)
        {
            errprint("%s%d: unaligned buffer, expect reduced performance\n",
                     dp->d_name, dp->d_unit);
            unaligned_warned = 1;
        }
        /*
         * This will alocate properly aligned buffer because of
         * how malloc works in FreeBSD kernel.
         */
        bp->bio_data = kfio_vmalloc((bp->bio_bcount + 7) & ~8);

        if (bp->bio_cmd == BIO_WRITE)
            kfio_memcpy(bp->bio_data, bp->bio_driver1, bp->bio_bcount);
    }

    error = 0;

    dev   = disk->fio_dev;
    fbio  = kfio_bio_alloc(dev);

    if (fbio == NULL)
    {
        if (kfio_bio_should_fail_requests(dev))
            error = EIO;
        else
            error = ENOMEM;
        goto error_exit;
    }

    fbio->fbio_offset = bp->bio_offset;
    fbio->fbio_size   = bp->bio_bcount;

    fbio->fbio_completor = freebsd_bio_completor;
    fbio->fbio_parameter = (fio_uintptr_t)bp;

    if (bp->bio_cmd == BIO_DELETE)
    {
        fbio->fbio_cmd = KBIO_CMD_DISCARD;

        return fbio;
    }
    else
    /* This is a read or write request. */
    {
        if (bp->bio_cmd == BIO_WRITE)
        {
            fbio->fbio_cmd = KBIO_CMD_WRITE;
        }
        else if (bp->bio_cmd == BIO_READ)
        {
            fbio->fbio_cmd = KBIO_CMD_READ;
        }
        else
        {
            error = ENOTSUP;
            goto error_exit;
        }

        error = kfio_sgl_map_bytes(fbio->fbio_sgl, bp->bio_data, bp->bio_bcount);

        if (error == 0)
        {
            error = kfio_sgl_dma_map(fbio->fbio_sgl, fbio->fbio_dmap,
                fbio->fbio_cmd == KBIO_CMD_READ ? IODRIVE_DMA_DIR_READ : IODRIVE_DMA_DIR_WRITE);

            if (error > 0)
            {
                kassert(fbio->fbio_size == kfio_sgl_size(fbio->fbio_sgl));

                return fbio;
            }
        }

    }

    /*
     * We failed for one reason or another. Just let the caller try again if error
     * is retryable. Only ENOMEM is at this moment.
     */
error_exit:

    if (bp->bio_driver1 != bp->bio_data)
    {
        kfio_vfree(bp->bio_data, (bp->bio_bcount + 7) & ~8);
        bp->bio_data = bp->bio_driver1;
    }

    if (fbio != NULL)
    {
        kfio_bio_free(fbio);
    }

    if (error != ENOMEM)
    {
        bp->bio_resid = bp->bio_bcount;
        biofinish(bp, NULL, error < 0 ? - error : error);
        fusion_cv_lock(&disk->bio_lock);
    }
    else
    {
        fusion_cv_lock(&disk->bio_lock);
        bioq_insert_head(disk->bio_queue, bp);
    }
    return NULL;
}
