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

#define KSCATTER_IMPL

#include "port-internal.h"
#include "pci_dev.h"
#include <sys/bio.h>
#include <sys/bus_dma.h>
#include <sys/bus.h>
#include <fio/port/dbgset.h>

struct freebsd_sgl
{
    uint32_t           uio_max;
    uint32_t           uio_num;
    uint32_t           uio_size;
    struct thread     *uio_td;
    enum uio_seg       uio_segflg;
    bus_dma_segment_t *seg_ptr;
    int                seg_num;
    kfio_pci_dev_t    *pci_dev;
    int                pci_dir;
    bus_dma_tag_t      dma_tag;
    bus_dmamap_t       dma_map;
    struct iovec       uio_vec[1];
};

struct kfio_dma_map
{
    uint32_t              map_offset;
    uint32_t              map_length;
    uint32_t              seg_count;
    uint32_t              seg_skip;
    uint32_t              seg_trunc;
    bus_dma_segment_t    *seg_first;
    bus_dma_segment_t    *seg_last;
};


int
kfio_sgl_alloc_nvec(kfio_pci_dev_t *pcidev, kfio_numa_node_t node, kfio_sg_list_t **sgl, int nvecs)
{
    struct kfio_freebsd_pci_dev *pdev = device_get_softc(pcidev);
    struct freebsd_sgl *fsg;
    int rc;

    fsg = kfio_vmalloc(sizeof(*fsg) + (nvecs - 1) * sizeof(struct iovec));
    if (NULL == fsg)
    {
        return -ENOMEM;
    }

    fsg->uio_max  = nvecs;
    fsg->uio_num  = 0;
    fsg->uio_size = 0;

    fsg->seg_num = 0;
    fsg->seg_ptr = NULL;

    fsg->pci_dev = pcidev;
    fsg->pci_dir = 0;

    fsg->dma_tag = NULL;
    fsg->dma_map = NULL;

    rc = bus_dma_tag_create(pdev->parent_dma_tag, 1, 0, BUS_SPACE_MAXADDR,
                            BUS_SPACE_MAXADDR, NULL, NULL,
                            BUS_SPACE_MAXSIZE_32BIT, nvecs,
                            BUS_SPACE_MAXSIZE_32BIT, BUS_DMA_ALLOCNOW,
                            NULL, NULL, &fsg->dma_tag);
    if (rc)
       goto bail;

    rc = bus_dmamap_create(fsg->dma_tag, 0, &fsg->dma_map);
    if (rc)
       goto bail;

    *sgl = fsg;
    return 0;

bail:
    kfio_sgl_destroy(fsg);
    *sgl = NULL;
    return -rc;
}

void
kfio_sgl_destroy(kfio_sg_list_t *sgl)
{
    struct freebsd_sgl *fsg = sgl;

    if (fsg->dma_tag != NULL)
    {
        if (fsg->seg_ptr != NULL)
           kfio_sgl_dma_unmap(fsg);

        bus_dmamap_destroy(fsg->dma_tag, fsg->dma_map);
        bus_dma_tag_destroy(fsg->dma_tag);
    }
    kfio_vfree(fsg, sizeof(*fsg) + (fsg->uio_max - 1) * sizeof(struct iovec));
}

void
kfio_sgl_reset(kfio_sg_list_t *sgl)
{
    struct freebsd_sgl *fsg = sgl;

    if (fsg->seg_ptr != NULL)
       kfio_sgl_dma_unmap(fsg);

    fsg->uio_num  = 0;
    fsg->uio_size = 0;
}

uint32_t
kfio_sgl_size(kfio_sg_list_t *sgl)
{
    struct freebsd_sgl *fsg = sgl;

    return fsg->uio_size;
}

int
kfio_sgl_map_bytes(kfio_sg_list_t *sgl, const void *buffer, uint32_t size)
{
    struct freebsd_sgl *fsg = sgl;

    if (fsg->uio_num >= fsg->uio_max)
    {
        dbgprint(DBGS_GENERAL, "%s: too few sg entries (cnt: %u nvec: %d size: %d)\n",
               __func__, fsg->uio_num, fsg->uio_max, size);

        return -ENOMEM;
    }

    if (fsg->uio_num != 0 && fsg->uio_segflg != UIO_SYSSPACE)
    {
        dbgprint(DBGS_GENERAL, "%s: cannot mix user and kernel segments within a sg map\n",
               __func__);

        return -ENOMEM;
    }

    // Casting is to get rid of const
    fsg->uio_vec[fsg->uio_num].iov_base = (void *)(uintptr_t)buffer;
    fsg->uio_vec[fsg->uio_num].iov_len  = size;

    fsg->uio_size += size;
    fsg->uio_num++;
    fsg->uio_segflg = UIO_SYSSPACE;
    fsg->uio_td = NULL;

    return 0;
}

int
kfio_sgl_map_page(kfio_sg_list_t *sgl, fusion_page_t page,
                  uint32_t offset, uint32_t size)
{
    caddr_t buffer = (char *)page + offset;

    return kfio_sgl_map_bytes(sgl, buffer, size);
}

/**
 * callback for loading dma map. If error we have failed.
 */
static void
_fusion_dmamap_callback(void *arg, bus_dma_segment_t *segs, int nsegs,
                        bus_size_t size __unused, int error)
{
    struct freebsd_sgl *fsg = arg;

    if (0 == error)
    {
        fsg->seg_ptr = segs;
        fsg->seg_num = nsegs;
    }
    else
    {
        fsg->seg_ptr = NULL;
        fsg->seg_num = 0;
    }
}

int
kfio_sgl_dma_map(kfio_sg_list_t *sgl, kfio_dma_map_t *dmap, int dir)
{
    struct freebsd_sgl *fsg = sgl;
    struct uio uio;
    int rc;

    uio.uio_segflg = fsg->uio_segflg;
    uio.uio_td     = fsg->uio_td;
    uio.uio_offset = 0;
    uio.uio_rw     = (dir == IODRIVE_DMA_DIR_WRITE) ? UIO_WRITE : UIO_READ;

    uio.uio_resid  = fsg->uio_size;
    uio.uio_iovcnt = fsg->uio_num;
    uio.uio_iov    = fsg->uio_vec;

    rc = bus_dmamap_load_uio(fsg->dma_tag, fsg->dma_map, &uio,
                             _fusion_dmamap_callback, fsg, BUS_DMA_NOCACHE);
    if (rc != 0)
        return -rc;

    /*
     * Fill in a map covering the whole scatter-gather list if caller is
     * interested in the information.
     */
    if (dmap != NULL)
    {
        dmap->map_offset = 0;
        dmap->map_length = fsg->uio_size;
        dmap->seg_count  = fsg->seg_num;
        dmap->seg_skip   = 0;
        dmap->seg_trunc  = 0;
        dmap->seg_first  = &fsg->seg_ptr[0];
        dmap->seg_last   = &fsg->seg_ptr[fsg->seg_num - 1];
    }
    return fsg->seg_num;
}

int
kfio_sgl_dma_unmap(kfio_sg_list_t *sgl)
{
    struct freebsd_sgl *fsg = sgl;

    if (fsg->seg_ptr != NULL)
    {
        bus_dmamap_unload(fsg->dma_tag, fsg->dma_map);
        fsg->seg_ptr = NULL;
        fsg->seg_num = 0;
    }

    return 0;
}

void
kfio_sgl_dump(kfio_sg_list_t *sgl, kfio_dma_map_t *dmap, const char *prefix, unsigned dump_contents)
{
    struct freebsd_sgl *fsg = sgl;
    kfio_sgl_iter_t cur;
    kfio_sgl_phys_t seg;
    uint32_t spill, used;
    int i, j;

    infprint("%s sglist %p num %u max %u size %u\n",
             prefix, fsg, fsg->uio_num, fsg->uio_max, fsg->uio_size);

    if (dmap != NULL)
    {
        infprint("%s map %p offset %u size %u count %u\n",
                 prefix, dmap, dmap->map_offset, dmap->map_length, dmap->seg_count);
    }

    spill = 0;
    cur = kfio_sgl_phys_first(sgl, &seg);
    for (i = 0, j = 0; i < fsg->uio_num; i++)
    {
        const uint8_t *bp = fsg->uio_vec[i].iov_base;
        uint32_t  len = fsg->uio_vec[i].iov_len;

        infprint("%s iovec %d: vaddr: %p size: %u\n",
                 prefix, i, bp, len);

        for ( /*none */; len > 0 && cur != NULL; j++)
        {
            if (spill > 0)
            {
                if (spill > len)
                    used = len;
                else
                    used = spill;

                infprint("%s   mvec %d paddr: 0x%" PRIx64 " psize: %u (%s%u bytes)\n",
                         prefix, j, seg.addr, seg.len,
                         (used == spill) ? "last " : "", used);
                spill -= used;
            }
            else if (seg.len > len)
            {
                infprint("%s   mvec %d paddr: 0x%" PRIx64 " psize: %u (first %u bytes)\n",
                         prefix, j, seg.addr, seg.len, len);
                spill = seg.len - len;
                used  = len;
            }
            else
            {
                infprint("%s   mvec %d paddr: 0x%" PRIx64 " psize: %u\n",
                         prefix, j, seg.addr, seg.len);
                used = seg.len;
            }

            if (dmap != NULL)
            {
                if (cur == (kfio_sgl_iter_t)dmap->seg_first)
                {
                    infprint("%s        map first skip %u\n",
                             prefix, dmap->seg_skip);
                }

                if (cur == (kfio_sgl_iter_t)dmap->seg_last)
                {
                    infprint("%s        map last trunc %u\n",
                             prefix, dmap->seg_trunc);
                }
            }

            len -= used;
            cur  = kfio_sgl_phys_next(sgl, cur, &seg);
        }

        if (dump_contents)
        {
            unsigned j;

            for (j = 0; j < len; j++)
            {
                if (j % 16 == 0)
                {
                    kfio_print("\n%s%04d:", prefix, j);
                }
                kfio_print(" %02x", bp[j]);
            }
            kfio_print("\n");
        }
    }
}

/* Physical segment enumeration interface. */
kfio_sgl_iter_t
kfio_sgl_phys_first(kfio_sg_list_t *sgl, kfio_sgl_phys_t *segp)
{
    struct freebsd_sgl *fsg = sgl;

    if (fsg->seg_ptr != NULL)
    {
        if (segp != NULL)
        {
            segp->addr = fsg->seg_ptr[0].ds_addr;
            segp->len  = fsg->seg_ptr[0].ds_len;
        }
        return fsg->seg_ptr;
    }
    return NULL;
}

kfio_sgl_iter_t
kfio_sgl_phys_next(kfio_sg_list_t *sgl, kfio_sgl_iter_t iter, kfio_sgl_phys_t *segp)
{
    struct freebsd_sgl *fsg = sgl;
    bus_dma_segment_t  *seg = iter;

    if (fsg->seg_ptr != NULL)
    {
        if (seg >= fsg->seg_ptr && (seg - fsg->seg_ptr + 1) < fsg->seg_num)
        {
            seg++;

            if (segp != NULL)
            {
                segp->addr = seg->ds_addr;
                segp->len  = seg->ds_len;
            }
            return seg;
        }
    }
    return NULL;
}

kfio_dma_map_t *kfio_dma_map_alloc(int may_sleep, kfio_numa_node_t node)
{
    kfio_dma_map_t *dmap;

    if (may_sleep)
    {
        dmap = kfio_malloc(sizeof(*dmap));
    }
    else
    {
        dmap = kfio_malloc_atomic(sizeof(*dmap));
    }
    if (dmap != NULL)
    {
        kfio_memset(dmap, 0, sizeof(*dmap));
    }
    return dmap;
}

void kfio_dma_map_free(kfio_dma_map_t *dmap)
{
    if (dmap != NULL)
    {
        kfio_free(dmap, sizeof(*dmap));
    }
}

int kfio_sgl_dma_slice(kfio_sg_list_t *sgl, kfio_dma_map_t *dmap, uint32_t offset, uint32_t length)
{
    struct freebsd_sgl *fsg = sgl;
    uint32_t            i;

    dmap->map_offset = offset;

    for (i = 0; i < fsg->seg_num; i++)
    {
        bus_dma_segment_t *seg = &fsg->seg_ptr[i];

        if (offset < seg->ds_len)
        {
            dmap->seg_first = seg;
            dmap->seg_last  = seg;

            dmap->map_length = seg->ds_len - offset;
            dmap->seg_skip   = offset;
            dmap->seg_count  = 1;
            break;
        }

        offset -= seg->ds_len;
    }

    kassert(i < fsg->seg_num);
    kassert(dmap->seg_first != NULL);

    for (i = i + 1; i <= fsg->seg_num; i++)
    {
        bus_dma_segment_t *seg = &fsg->seg_ptr[i];

        if (dmap->map_length >= length)
        {
            dmap->seg_trunc   = dmap->map_length - length;
            dmap->map_length -= dmap->seg_trunc;
            break;
        }
        kassert(i < fsg->seg_num);
        dmap->map_length += seg->ds_len;
        dmap->seg_last    = seg;
        dmap->seg_count++;
    }

    return 0;
}

int kfio_dma_map_nvecs(kfio_dma_map_t *dmap)
{
    return dmap->seg_count;
}

uint32_t kfio_dma_map_size(kfio_dma_map_t *dmap)
{
    return dmap->map_length;
}

/* Physical segment enumeration interface. */
kfio_sgl_iter_t kfio_dma_map_first(kfio_dma_map_t *dmap, kfio_sgl_phys_t *segp)
{
    bus_dma_segment_t *seg = dmap->seg_first;

    if (seg != NULL)
    {
        if (segp != NULL)
        {
            segp->addr = seg->ds_addr + dmap->seg_skip;
            segp->len  = seg->ds_len - dmap->seg_skip;

            if (seg == dmap->seg_last)
                segp->len -= dmap->seg_trunc;
        }
        return seg;
    }
    return NULL;
}

kfio_sgl_iter_t kfio_dma_map_next(kfio_dma_map_t *dmap, kfio_sgl_iter_t iter, kfio_sgl_phys_t *segp)
{
    bus_dma_segment_t *seg = iter;

    if (seg >= dmap->seg_first && seg < dmap->seg_last)
    {
        seg++;

        if (segp != NULL)
        {
            segp->addr = seg->ds_addr;
            segp->len  = seg->ds_len;

            if (seg == dmap->seg_last)
                segp->len -= dmap->seg_trunc;
        }
        return seg;
    }
    return NULL;
}

/// @brief Map one or more pages into SGL
///
/// @param sgl         the scatter-gather list.
/// @param pages       An array of pages from kfio_get_user_pages
/// @param size        size of the array, in pages
/// @param offset      offset of the starting address from the page boundary
///
/// @return none
int kfio_sgl_map_user_pages(kfio_sg_list_t *sgl, fusion_user_page_t *pages,
                            uint32_t size, uint32_t offset)
{
    struct freebsd_sgl *fsg = sgl;
    void **p = (void **)pages;     // fusion_user_page_t is the userspace address

    if (fsg->uio_num != 0 && fsg->uio_segflg != UIO_USERSPACE)
    {
        dbgprint(DBGS_GENERAL, "%s: cannot mix user and kernel segments within a sg map\n",
               __func__);

        return -ENOMEM;
    }

    if (fsg->uio_num != 0 && fsg->uio_td != curthread)
    {
        // XXX should be from different processes, given how uio_td is used today, but
        // XXX that may change in the future.
        dbgprint(DBGS_GENERAL, "%s: cannot mix user segments from different threads within a sg map\n",
               __func__);

        return -ENOMEM;
    }

    // Have to do each one individually, since iovecs from atomic writes mix
    // and match addresses, so we don't know if things are contiguous or
    // not.
    fsg->uio_segflg = UIO_USERSPACE;
    fsg->uio_td = curthread;

    while (size > 0)
    {
        uint32_t mapped_bytes;

        if (fsg->uio_num >= fsg->uio_max)
        {
            dbgprint(DBGS_GENERAL, "%s: too few sg entries (cnt: %u nvec: %d size: %d)\n",
                     __func__, fsg->uio_num, fsg->uio_max, size);

            return -ENOMEM;
        }

        mapped_bytes = MIN(size, FUSION_PAGE_SIZE - offset);

        fsg->uio_vec[fsg->uio_num].iov_base = (char *)*p++ + offset;
        fsg->uio_vec[fsg->uio_num].iov_len  = mapped_bytes;
        fsg->uio_size += mapped_bytes;
        fsg->uio_num++;

        size -= mapped_bytes;
        offset = 0;
    }

    return 0;
}
