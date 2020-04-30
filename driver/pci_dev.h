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

#ifndef __KFIO_PORT_FREEBSD_PCI_DEV_H__
#define __KFIO_PORT_FREEBSD_PCI_DEV_H__

#if !defined (__FreeBSD__)
#error This file supports FreeBSD only
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <sys/rman.h>
#include <sys/bus.h>
#include <sys/bio.h>
#include <machine/bus.h>
#include <machine/resource.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#define KFIO_PCI_NAME_LEN   13

/* kfio_pci_dev_t * will point to this in the FreeBSD port */
struct kfio_freebsd_pci_dev
{
    device_t            dev;            /* pure black energy */
    struct cdev         *cdev;          /* char device */
    int                 unit;           /* equivalent to Solaris 'instance' */
    void                *intr_cookie;   /* ptr to cookie for bus_setup_intr() */
    struct resource     *intr_resource; /* interrupt resource */
    int                 intr_rid;       /* intr resource id */
    struct resource     *bar_resource;  /* BAR resource */
    int                 bar_rid;        /* memory resource id */
    struct intr_config_hook fio_ich;

    bus_dma_tag_t       parent_dma_tag; /* Unrestriced parent DMA tag. */
    struct bio_queue_head bioq;

    void                *p_fio_device;   /* pointer to the fio_device owner */
    void                *prv;            /* private data */
    char                pci_name[KFIO_PCI_NAME_LEN + 1];
};

extern int  iodrive_pci_probe(struct kfio_freebsd_pci_dev *pd);
extern void iodrive_pci_remove(struct kfio_freebsd_pci_dev *pd);

#endif // __KFIO_PORT_FREEBSD_PCI_DEV_H__
