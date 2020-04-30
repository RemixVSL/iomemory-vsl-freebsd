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
#include <fio/port/dbgset.h>
#include "pci_dev.h"

#include <sys/malloc.h>
#include <sys/bus.h>
#include <machine/bus.h>
#include <machine/resource.h>
#include <machine/bus_dma.h>
#include <machine/param.h>
#include <vm/vm.h>
#include <vm/vm_extern.h>
#include <vm/pmap.h>


static MALLOC_DEFINE(M_FUSION_IO, FIO_DRIVER_NAME, "Fusion-io driver buffers");

struct _fusion_freebsd_dma {
    bus_dma_tag_t  tag;
    bus_dmamap_t   map;
} __attribute((aligned(8)));

/**
 * Called from bus_dmamap_load(..)
 */
static void kfio_map_dma_callback(void *arg, bus_dma_segment_t *segs,
                                  int nseg, int error)
{
    struct fusion_dma_t *dma_handle = arg;

    if (error)
       return;

    KASSERT(nseg == 1, ("Expected single segment, got %d", nseg));
    dma_handle->phys_addr = segs->ds_addr;
}

/**
 *  @brief allocates locked down memory suitable for DMA transfers.
 *  @param pdev - pointer to device handle
 *          (struct kfio_freebsd_pci_dev *)
 *  @param size - size of allocation
 *  @param dma_handle - member .phys_addr of struct will be set to the
 *  physical addr on successful return.  The remainder of the structure is opaque.
 *  @return virtual address of allocated memory or NULL on failure.
 *
 *  The current io-drive has 64 bit restrictions on alignment and buffer length,
 *  and no restrictions on address ranges with DMA.
 */
void noinline *kfio_dma_alloc_coherent(kfio_pci_dev_t *pdev, unsigned int size,
    struct fusion_dma_t *dma_handle)
{
    struct _fusion_freebsd_dma *pfd = (struct _fusion_freebsd_dma *)&dma_handle->_private;
    struct kfio_freebsd_pci_dev *pd = device_get_softc(pdev);
    int rc;
    void *vaddr;

    FUSION_ALLOCATION_TRIPWIRE_TEST();

    rc = bus_dma_tag_create(pd->parent_dma_tag, 8, 0, BUS_SPACE_MAXADDR,
        BUS_SPACE_MAXADDR, NULL, NULL, size, 1, size, 0, NULL, NULL, &pfd->tag);

    if (rc)
    {
        kfio_print("%s:%i Error in creating DMA tag\n", __FUNCTION__, __LINE__);
        return NULL;
    }

    rc = bus_dmamem_alloc(pfd->tag, &vaddr, BUS_DMA_WAITOK | BUS_DMA_COHERENT, &pfd->map);

    if (rc)
    {
        kfio_print("%s:%i Error in allocating dma memory (wanted %u bytes)\n",
            __FUNCTION__, __LINE__, size);
        goto free_handle;
    }

    rc = bus_dmamap_load(pfd->tag, pfd->map, vaddr, size,
            kfio_map_dma_callback, dma_handle, BUS_DMA_NOCACHE | BUS_DMA_NOWAIT);

    if (ENOMEM == rc)
    {
        kfio_print("%s:%i Insufficient memory for loading DMA map(wanted %u bytes)\n",
            __FUNCTION__, __LINE__, size);
        goto free_mem;
    }

    if (EINVAL == rc)
    {
        kfio_print("%s:%i Invalid value for loading DMA map(wanted %u bytes)\n",
            __FUNCTION__, __LINE__, size);
        goto free_mem;
    }

    if (EINPROGRESS == rc)
    {
        kfio_print("%s:%i Waiting to loading DMA map(wanted %u bytes)\n",
            __FUNCTION__, __LINE__, size);
// XXX:we should wait for the calback to return
        goto free_mem;
    }

    if (rc)
    {
        kfio_print("%s:%i Unknown error %d in loading DMA map\n",
            __FUNCTION__, __LINE__, rc);
        goto free_mem;
    }

    return vaddr;
free_mem:
    bus_dmamem_free(pfd->tag, vaddr, pfd->map);

free_handle:
    bus_dma_tag_destroy(pfd->tag);
    return NULL;
}

/**
 * @brief frees memory allocated by kfio_dma_alloc_coherent().
 *
 */
void noinline kfio_dma_free_coherent(kfio_pci_dev_t *pdev, unsigned int size,
    void *vaddr, struct fusion_dma_t *dma_handle)
{
    struct _fusion_freebsd_dma *pfd =
            (struct _fusion_freebsd_dma *) &dma_handle->_private;

    bus_dmamap_unload(pfd->tag, pfd->map);
    bus_dmamem_free(pfd->tag, vaddr, pfd->map);
    bus_dma_tag_destroy(pfd->tag);
    pfd->tag = NULL;
    pfd->map = NULL;

    dma_handle->phys_addr = 0;
}

static void *__kfio_malloc(fio_size_t size)
{
    /* align to cache line (64 bytes on x64)
     * N.B. the returned memory is wired */
    FUSION_ALLOCATION_TRIPWIRE_TEST();
    return malloc(MAX(64, size), M_FUSION_IO, M_WAITOK);
}

/**
 * @brief allocates kernel wired memory, hopefully aligned on cache line
 *        boundary. Unfortunately, FreeBSD kernel does not expose underlying
 *        hardware cache line size, so we assume 64 bit for the only architecture
 *        we run on today: amd64.
 * No i/o is allowed to fulfill request, but the allocation may block.
 *
 */
void *noinline kfio_malloc(fio_size_t size)
{
    return __kfio_malloc(size);
}

void *noinline kfio_malloc_node(fio_size_t size, kfio_numa_node_t node)
{
    return __kfio_malloc(size);
}

/**
 */
static void *__kfio_malloc_atomic(fio_size_t size)
{
    /* Align to cache line (64 bytes on x64).
     * N.B. the returned memory is wired */
    FUSION_ALLOCATION_TRIPWIRE_TEST();
    return malloc(MAX(64, size), M_FUSION_IO, M_WAITOK);
}

void* noinline kfio_malloc_atomic(fio_size_t size)
{
    return __kfio_malloc_atomic(size);
}

void* noinline kfio_malloc_atomic_node(fio_size_t size, kfio_numa_node_t node)
{
    return __kfio_malloc_atomic(size);
}

/**
 */
void noinline kfio_free(void *ptr, fio_size_t size)
{
    free(ptr, M_FUSION_IO);
}

/**
 * @brief allocates virtual memory mapped into kernel space that is
 * not assumed to be contiguous.  Linux does assume the memory is wired.
 */
void *noinline kfio_vmalloc(fio_size_t size)
{
    FUSION_ALLOCATION_TRIPWIRE_TEST();
    return malloc(size, M_FUSION_IO, M_WAITOK);
}

/**
 */
void noinline kfio_vfree(void *ptr, fio_size_t sz)
{
    free(ptr, M_FUSION_IO);
}

/*---------------------------------------------------------------------------*/

/**
 * @brief called after successful call to kfio_alloc_0_page()
 */
void noinline kfio_free_page(fusion_page_t pg)
{
    free(pg, M_FUSION_IO);
}

/** @brief returns fusion_page_t for a virtual address
 */
fusion_page_t noinline kfio_page_from_virt(void *vaddr)
{
    return (fusion_page_t)vaddr;
}

/** @brief returns a virtual address given a page
 *  called from iodrive_dma_noop_allocate()
 */
void * noinline kfio_page_address(fusion_page_t pg)
{
    return (void *) pg;
}

/**
 * The flags passed in are @e not compatible with the linux flags.
 */
fusion_page_t noinline kfio_alloc_0_page(kfio_maa_t flags)
{
    int kmflag = 0;

    if (!(flags & (KFIO_MAA_NOIO | KFIO_MAA_NOWAIT)) )
        kmflag = M_WAITOK;
    if (flags & KFIO_MAA_NOIO)
        kmflag = M_NOWAIT;
    if (flags & KFIO_MAA_NOWAIT)
        kmflag = M_NOWAIT;

    return malloc(PAGE_SIZE, M_FUSION_IO, kmflag | M_ZERO);
}

/*---------------------------------------------------------------------------*/

int kfio_dma_sync(struct fusion_dma_t *dma_hdl, uint64_t offset, size_t length, unsigned type)
{
    struct _fusion_freebsd_dma *pfd =
                      (struct _fusion_freebsd_dma *) &dma_hdl->_private;

    bus_dmamap_sync(pfd->tag, pfd->map,
        type == KFIO_DMA_SYNC_FOR_DRIVER ? BUS_DMASYNC_POSTREAD : BUS_DMASYNC_PREWRITE);

    return 0;
}

/// @brief Pin the user pages in memory.
/// Note:  This needs to be called from within a process
///        context (i.e. ioctl or other syscall) for the user address passed in
/// @param pages     array that receives pointers to the pages pinned. Should be at least nr_pages long
/// @param nr_pages  number of pages from start to pin
/// @param start     starting userspace address
/// @param write     whether pages will be written to by the caller
///
/// @return          number of pages pinned
int kfio_get_user_pages(fusion_user_page_t *pages, int nr_pages, fio_uintptr_t start, int write)
{
    int i, error;

    start = trunc_page(start);

    /* Lock whole  affected memory range  in single call. */
    error = vslock((void *)start, nr_pages * FUSION_PAGE_SIZE);
    if (error != 0)
    {
        return -error;
    }

    /*
     * Split into individual page-sized chunks to conform
     * to the API idiosyncrasies.
     */
    for (i = 0, error = 0; i < nr_pages; i++)
    {
        pages[i] = (void *)start;
        start += FUSION_PAGE_SIZE;
    }

    return nr_pages;
}

/// @brief Release the mapped pages from the page cache
/// Note:  This needs to be called from within the same process
///        context (i.e. ioctl or other syscall) kfio_get_user_pages
///        is called.
/// @param pages     the array of pages to be released
/// @param nr_pages  number of pages to be released
///
/// @return          none
void kfio_put_user_pages(fusion_user_page_t *pages, int nr_pages)
{
    fio_uintptr_t start;
    int i, j;

    /*
     * Scan page list, coalescing contiguous regions.
     * This undoes splitting done in kfio_get_user_pages.
     */
    for (i = 0; i < nr_pages; i = j)
    {
        start = (fio_uintptr_t)pages[i];

        for (j = i + 1; j < nr_pages; j++)
        {
            start += FUSION_PAGE_SIZE;

            if (start != (fio_uintptr_t)pages[j])
            {
                break;
            }
        }
        vsunlock(pages[i], (j - i) * FUSION_PAGE_SIZE);
    }
}
