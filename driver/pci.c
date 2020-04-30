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

#include <fio/port/fio-port.h>
#include <fio/port/dbgset.h>
#include <fio/port/port_config.h>
#include "pci_dev.h"

extern int kfio_ignore_pci_device(kfio_pci_dev_t * pd);

/**
 * called when interrupts are functional.
 */
static void
iodrive_pci_startup(void *arg)
{
    struct kfio_freebsd_pci_dev *pd = arg;
    int rc;

    /* disconnect ourselves from the intrhook chain */
    config_intrhook_disestablish(&pd->fio_ich);

    rc = iodrive_pci_attach(pd->dev, pd->unit);
    if (rc  < 0)
    {
        device_printf(pd->dev, "failed device attach\n");
        /*
         * At this point we have attached to the pci device,
         * but have absolutely no means for userland to access
         * it and manipulate it in any way. Leave it as
         * is until user unloads the driver, there's nothing
         * else we can really do.
         */
    }
}

/**
 * called from character device attach() function
 *
 */
int iodrive_pci_probe(struct kfio_freebsd_pci_dev *pd)
{
    int rc = 0;
    uint32_t cmd;

    if (kfio_ignore_pci_device(pd->dev) != 0)
    {
        infprint("ignoring device %s\n", kfio_pci_name(pd->dev));
        return -EINVAL;
    }

    /* set busmaster */
    rc = pci_enable_busmaster(pd->dev);
    if (0 != rc)
    {
        device_printf(pd->dev, "failed to set PCI bus mastering\n");
        return -rc;
    }

    cmd = pci_read_config(pd->dev, PCIR_COMMAND, 2);
    if (cmd & PCIM_CMD_INTX_DISABLE)
    {
        pci_write_config(pd->dev, PCIR_COMMAND,
            cmd & (~PCIM_CMD_INTX_DISABLE), 2);
    }

    pd->bar_rid = PCIR_BAR(5);

    pd->bar_resource = bus_alloc_resource_any(pd->dev,
                SYS_RES_MEMORY, &pd->bar_rid, RF_ACTIVE);

    /* can not map BAR5 ? map BAR0 */
    if (NULL == pd->bar_resource)
    {
        pd->bar_rid = PCIR_BAR(0);

        pd->bar_resource = bus_alloc_resource_any(pd->dev,
                SYS_RES_MEMORY, &pd->bar_rid, RF_ACTIVE);

        if (NULL == pd->bar_resource)
        {
            device_printf(pd->dev, "Cannot map PCIe register space 0\n");
            return -ENOMEM;
        }
    }

    /** Get parent DMA tag **/
    rc = bus_dma_tag_create(bus_get_dma_tag(pd->dev), 1, 0x100000000,
         BUS_SPACE_MAXADDR, BUS_SPACE_MAXADDR, NULL, NULL, BUS_SPACE_MAXSIZE,
         BUS_SPACE_UNRESTRICTED, BUS_SPACE_MAXSIZE,
         0, NULL, NULL, &pd->parent_dma_tag);

    if (rc)
    {
        device_printf(pd->dev, "failed to get parent DMA tag\n");
        bus_release_resource(pd->dev, SYS_RES_MEMORY, pd->bar_rid, pd->bar_resource);
        pd->bar_resource = NULL;
        return -rc;
    }

    pd->fio_ich.ich_func = iodrive_pci_startup;
    pd->fio_ich.ich_arg  = pd;

    if (config_intrhook_establish(&pd->fio_ich) != 0) {
        device_printf(pd->dev, "can't establish configuration hook\n");
        rc = -ENXIO;
    }

    if (rc < 0) /* cleanup and bail */
    {
        bus_dma_tag_destroy(pd->parent_dma_tag);
        pd->parent_dma_tag = NULL;

        bus_release_resource(pd->dev, SYS_RES_MEMORY, pd->bar_rid, pd->bar_resource);
        pd->bar_resource = NULL;
    }

    return (rc);
}

int kfio_pci_map_csr(kfio_pci_dev_t *pdev, const char *device_label, struct kfio_pci_csr_handle *csr)
{
    struct kfio_freebsd_pci_dev *pd = device_get_softc(pdev);

    if (pd->bar_resource != NULL)
    {
        csr->csr_virt = rman_get_virtual(pd->bar_resource);
        csr->csr_hdl  = pd->bar_resource;
        return 0;
    }
    return -ENOMEM;
}

void kfio_pci_unmap_csr(kfio_pci_dev_t *pdev __unused, struct kfio_pci_csr_handle *csr __unused)
{
}

/*
 * called from character device detach() function
 */
void iodrive_pci_remove(struct kfio_freebsd_pci_dev *pd)
{
    if (pd->prv != NULL)
        iodrive_pci_detach(pd->dev);

    if (pd->bar_resource != NULL)
    {
        bus_release_resource(pd->dev, SYS_RES_MEMORY, pd->bar_rid, pd->bar_resource);
        pd->bar_resource = NULL;
    }

    if (pd->parent_dma_tag != NULL)
    {
        bus_dma_tag_destroy(pd->parent_dma_tag);
        pd->parent_dma_tag = NULL;
    }
}

int kfio_pci_enable_msi(kfio_pci_dev_t *pdev)
{
    int rc, count;

    count = pci_msi_count(pdev);
    if (1 != count)
    {
        errprint("pci_msi_count() returned %d (expected 1), "
                 "reverting to legacy interrupts.\n", count);
        return -ENXIO;
    }

    rc = pci_alloc_msi(pdev, &count);
    if (0 != rc)
    {
        errprint("pci_alloc_msi() returned %d (expected 0), "\
                 "reverting to legacy interrupts.\n", rc);
        return -ENXIO;
    }
    else
    {
        if (count != 1)
        {
            errprint("pci_alloc_msi() returned count %d (expected 1), "\
                     "reverting to legacy interrupts.\n", count);
            pci_release_msi(pdev);
            return -ENXIO;
        }
    }

    return 0; /* 0 => success */
}

void kfio_pci_disable_msi(kfio_pci_dev_t *pdev)
{
    pci_release_msi(pdev);
}

/**
 * The second argument is only useful to Linux, pass in NULL for the rest
 */
void kfio_free_irq(kfio_pci_dev_t *pdev, void *dev)
{
    struct kfio_freebsd_pci_dev *pd = device_get_softc(pdev);

    if (pd->intr_cookie != NULL)
    {
        bus_teardown_intr(pd->dev, pd->intr_resource, pd->intr_cookie);
        bus_release_resource(pd->dev, SYS_RES_IRQ, pd->intr_rid, pd->intr_resource);

        pd->intr_cookie = NULL;
        pd->intr_resource = NULL;
    }
}

/**
 * @brief This is the ISR registered with the operating system.
 * the first argument is struct iodrive_dev *
 */
static void kfio_handle_irq_wrapper(void *p_iod)
{
    (void) iodrive_intr_fast(0, p_iod);
}

/**
 * arg 3 is iodrive_dev *
 */
int kfio_request_irq(kfio_pci_dev_t *pdev, const char *devname,
                     void *iodrive_dev, int msi_enabled)
{
    struct kfio_freebsd_pci_dev *pd = device_get_softc(pdev);
    int rc;

    if (msi_enabled)
    {
        pd->intr_rid = 1;
        infprint("Allocating MSI interrupts\n");
    }
    else
    {
        pd->intr_rid = 0;
        infprint("Allocating legacy interrupt\n");
    }

    pd->intr_resource = bus_alloc_resource_any(pd->dev, SYS_RES_IRQ,
                                               &pd->intr_rid, RF_SHAREABLE | RF_ACTIVE);

    if (NULL == pd->intr_resource)
    {
        errprint("%s Cannot allocate interrupt\n", __FUNCTION__);
        return -1;
    }

#if __FreeBSD_version >= 700031
    rc = bus_setup_intr(pd->dev, pd->intr_resource, INTR_TYPE_BIO|INTR_MPSAFE,
                        NULL, kfio_handle_irq_wrapper, iodrive_dev, &pd->intr_cookie);
#else
    rc = bus_setup_intr(pd->dev, pd->intr_resource, INTR_TYPE_BIO|INTR_MPSAFE,
                        kfio_handle_irq_wrapper, iodrive_dev, &pd->intr_cookie);
#endif

    if (rc)
    {
        bus_release_resource(pd->dev, SYS_RES_IRQ, pd->intr_rid, pd->intr_resource);
        errprint("Failure to setup interrupt handler: %d\n", rc);
        return -1;
    }

    return 0;
}

int kfio_get_irq_number(kfio_pci_dev_t *pdev, uint32_t *irq)
{
    struct kfio_freebsd_pci_dev *pd = device_get_softc(pdev);

    *irq = (uint32_t)rman_get_rid(pd->intr_resource);
    return 0;
}

