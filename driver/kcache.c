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
#include <sys/queue.h>
#include <vm/uma.h>

#include <fio/port/dbgset.h>

/**
 * fusion_create_cache(c,t) :
 *        __kfio_create_cache(c, \#t, sizeof(t), __alignof__(t))
 */
int noinline __kfio_create_cache(fusion_mem_cache_t *pcache, char *name,
                                 uint32_t size, uint32_t align)
{
    dbgprint(DBGS_GENERAL, "Creating cache %s size: %d align: %d\n",
             name, size, align);

    strncpy(pcache->name, name, sizeof(pcache->name) - 1);
    pcache->name[sizeof(pcache->name)-1] = '\x0';

    pcache->p = uma_zcreate(pcache->name, size, NULL, NULL, NULL, NULL,
                            UMA_ALIGN_PTR, UMA_ZONE_ZINIT);

    if (NULL == pcache->p)
    {
        return (-ENOMEM);
    }
    return (0);
}

/**
 * @brief allocate a chunk from the given cache, possibly sleeping
 * but never doing file IO.
 *
 * This function checks if it is in interrupt context and if so will never sleep.
 * In non-interrupt context it may sleep.
 */
void *kfio_cache_alloc(fusion_mem_cache_t *cache, int can_wait)
{
    FUSION_ALLOCATION_TRIPWIRE_TEST();
    return uma_zalloc((uma_zone_t) (cache->p), M_ZERO | M_NOWAIT/*M_WAITOK*/ );
}

void *kfio_cache_alloc_node(fusion_mem_cache_t *cache, int can_wait,
                            kfio_numa_node_t node)
{
    return kfio_cache_alloc(cache, can_wait);
}

/**
 *
 */
void kfio_cache_free(fusion_mem_cache_t *cache, void *p)
{
    uma_zfree((uma_zone_t) (cache->p), p);
}
/**
 *
 */
void kfio_cache_destroy(fusion_mem_cache_t *cache)
{
    uma_zdestroy((uma_zone_t) (cache->p));
    cache->p = NULL;
}

