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

#include <fio/port/fio-port.h>
/// @cond GENERATED_CODE
#include <fio/port/port_config.h>
/// @endcond
#if (FUSION_INTERNAL==1)
#include <fio/internal/config.h>
#endif
#include <fio/port/dbgset.h>
#include <fio/port/ifio.h>
#include <fio/port/pci.h>

#include "port-internal.h"
#include "pci_dev.h"
#include <sys/module.h>
#include <sys/bio.h>

/* Create storage for the debug flags */
/// @cond GENERATED_CODE
#include <fio/port/port_config_macros_clear.h>
/// @endcond
#define FIO_PORT_CONFIG_SET_DBGFLAG
#define FIO_PORT_CONFIG_MACRO(dbgf) [_DF_ ## dbgf] = { .name = #dbgf, .mode = S_IRUGO | S_IWUSR, .value = dbgf },

#if (FUSION_INTERNAL==1)
/// @cond GENERATED_CODE
#include <fio/internal/config_macros_clear.h>
/// @endcond
#define FIO_CONFIG_SET_DBGFLAG
#define FIO_CONFIG_MACRO(dbgf) [_DF_ ## dbgf] = { .name = #dbgf, .mode = S_IRUGO | S_IWUSR, .value = dbgf },
#endif

static struct dbgflag _dbgflags[] = {
/// @cond GENERATED_CODE
#include <fio/port/port_config_macros.h>
#if (FUSION_INTERNAL==1)
#include <fio/internal/config_macros.h>
#endif
/// @endcond
    [_DF_END] = { .name = NULL, .mode = 0, .value =0 }
};

struct dbgset _dbgset = {
    .flag_dir_name = "debug",
    .flags         = _dbgflags
};

void
__kassert_fail (const char *expr, const char *file, int line,
                const char *func, int good, int bad)
{
    errprint("iodrive: assertion failed %s:%d:%s: %s\n",
             file, line, func, expr);

    if (make_assert_nonfatal)
    {
        errprint("ASSERT STATISTICS: %d bad, %d good\n",
                 bad, good);
        kfio_dump_stack();
    }
    else
    {
        errprint("%s:%d:(in %s(). Previously %d good calls.) - %s\n", file, line, func, good, expr);
        kfail();
    }
}

extern int  init_fio_dev(void);
extern void cleanup_fio_dev(void);

extern int  init_fio_iodrive(void);
extern void cleanup_fio_iodrive(void);

extern int  init_fio_obj(void);
extern void cleanup_fio_obj(void);

extern int  init_fio_blk(void);
extern void cleanup_fio_blk(void);

extern int fio_detach(struct fio_device *, uint32_t);
extern int fio_shutdown_device(struct fio_device *);

extern void fio_auto_attach_wait(void);

int  kfio_kvprint(int level, const char *format, va_list ap);

static int freebsd_probe(device_t dev);
static int freebsd_attach(device_t dev);
static int freebsd_detach(device_t dev);
static int freebsd_shutdown(device_t dev);

/*
 * In FreeBSD < 7 module init routine does not prevent
 * this module to be added to the valid driver list. Work
 * around this by not allowing device probe to recognize
 * any devices.
 */
static int fusion_init_failed;

/*****************************************************************************/
/******************************************************************************
 * @brief driver entry point.
 */
static int
freebsd_probe(device_t dev)
{
    if (pci_get_vendor(dev) != PCI_VENDOR_ID_FUSION)
        return (ENXIO);

    if ((pci_get_device(dev) != IODRIVE3_OLD_PCI_DEVICE) &&
        (pci_get_device(dev) != IODRIVE4_PCI_DEVICE) &&
        (pci_get_device(dev) != IODRIVE3_PCI_DEVICE) &&
        (pci_get_device(dev) != IODRIVE3_BOOT_PCI_DEVICE) &&
        (pci_get_device(dev) != IOXTREME_PCI_DEVICE) &&
        (pci_get_device(dev) != IOXTREMEPRO_PCI_DEVICE) &&
        (pci_get_device(dev) != IOXTREME2_PCI_DEVICE) )
    {
        return (ENXIO);
    }

    if (fusion_init_failed)
    {
        return (ENXIO);
    }

    /*
     * Allow devices to be disabled through hints.
     */
    if (resource_disabled("fct", device_get_unit(dev)) != 0)
    {
        return (ENXIO);
    }

    device_set_desc(dev, "Fusion-io iodimm3");

    return 0;
}

/******************************************************************************
 *  called after probe returns success
 */
static int
freebsd_attach(device_t dev)
{
    struct kfio_freebsd_pci_dev *pdev;

    pdev = device_get_softc(dev);
    ASSERT(pdev != NULL);

    pdev->dev  = dev;
    pdev->unit = device_get_unit(dev);

    bioq_init(&pdev->bioq);

    snprintf(pdev->pci_name, sizeof(pdev->pci_name), "%04x:%02x:%02x.%x",
#if __FreeBSD_version >= 700053
             pci_get_domain(dev),
#else
             0,
#endif
             pci_get_bus(dev),
             pci_get_slot(dev),
             pci_get_function(dev));

    if (iodrive_pci_probe(pdev))
    {
        errprint("cannot initialize ioDrive.\n");
        return (ENXIO);
    }

    /* create_node done in create_control_device in freebsd/cdev.c */
    return (0);
}

/******************************************************************************
 */
static int
freebsd_detach(device_t dev)
{
    struct kfio_freebsd_pci_dev *pdev;

    pdev = device_get_softc(dev);

    if (NULL != pdev->p_fio_device)
    {
        int rc = fio_detach(pdev->p_fio_device, 0);
        if (rc)
        {
            errprint("Failed to detach block device on %s error %d.\n",
                     device_get_nameunit(dev), rc);
            return (ENXIO);
        }
        pdev->p_fio_device = NULL;
    }
    iodrive_pci_remove(pdev);
    return (0);
}

/******************************************************************************
 */
static int
freebsd_shutdown(device_t dev)
{
    struct kfio_freebsd_pci_dev *pdev;

    pdev = device_get_softc(dev);

    if (NULL != pdev->p_fio_device)
    {
        fio_shutdown_device(pdev->p_fio_device);
        pdev->p_fio_device = NULL;
    }

    /*
     * Normally we would not need to do that, but that is the only
     * way to tell the framework to to the cleanup on the device
     * at the moment.
     */
    iodrive_pci_remove(pdev);

    return (0);
}

static struct mtx kprint_mtx;
MTX_SYSINIT(fio_kprint, &kprint_mtx, "fioprnt", MTX_DEF);
static char kprint_buffer[512];

int
kfio_kvprint(int level, const char *fmt, va_list ap)
{
    int len;

    mtx_lock(&kprint_mtx);
    if (level > LOG_WARNING)
    {
        len = vsnprintf(kprint_buffer, sizeof (kprint_buffer), fmt, ap);
        log(level, "%s", kprint_buffer);
    }
    else
    {
        len = vprintf(fmt, ap);
    }
    mtx_unlock(&kprint_mtx);
    return len;
}

static int
fio_do_init(void)
{
    int rc;

    rc = init_fio_iodrive();
    if (rc != 0)
        goto bail;

    rc = init_fio_obj();
    if (rc != 0)
        goto bail;

    rc = init_fio_blk();
    if (rc != 0)
        goto bail;

    return 0;
bail:
    errprint("Failed to load " FIO_DRIVER_NAME " error %d: %s\n",
             rc, ifio_strerror(rc));

    if (rc < 0)
        rc = -rc;

    return rc;
}

static int
init_fio_driver(void)
{
    int rc;

    /* If the LEB map isn't loaded then don't bother trying to auto attach */
    if (!iodrive_load_eb_map)
        auto_attach = 0;

    rc = kfio_checkstructs();
    if (rc != 0)
    {
        errprint("Failed structure size check.\n");
        return ENXIO;
    }

    rc = init_fio_dev();
    if (rc != 0)
    {
        errprint("Device initialization failed: error %d.\n", rc);
        return ENXIO;
    }

    rc = fio_do_init();
    if (rc != 0)
        return rc;

    if ((rc = dbgs_create_flags_dir(&_dbgset)))
        errprint("Failed to create debug flags %d: %s\n",
                 rc, ifio_strerror(rc));
    return rc;
}

static void
cleanup_fio_driver(void)
{
    dbgs_delete_flags_dir(&_dbgset);
    cleanup_fio_blk();
    cleanup_fio_obj();
    cleanup_fio_iodrive();
    cleanup_fio_dev();
}

/******************************************************************************
 * Hook into last stage of driver load to wait for background scans to
 * finish.
 */
static void
fio_freebsd_postload(void *dummy __unused)
{
    fio_auto_attach_wait();
}
SYSINIT(fio_module, SI_SUB_INT_CONFIG_HOOKS, SI_ORDER_ANY,
        fio_freebsd_postload, NULL);

/******************************************************************************
 * Driver module glue
 */
static int
freebsd_modevent(struct module *mod __unused, int event, void *arg __unused)
{
    int result;

    result = 0;

    switch (event) {
    case MOD_LOAD:
        result = init_fio_driver();
        if (result != 0)
            fusion_init_failed = 1;
        break;

    case MOD_UNLOAD:
        if (fusion_init_failed == 0)
            cleanup_fio_driver();
        break;

    default:
        result = EOPNOTSUPP;
        break;
    }

    return (result);
}

static device_method_t fusion_methods[] = {
    DEVMETHOD(device_probe,    freebsd_probe),
    DEVMETHOD(device_attach,   freebsd_attach),
    DEVMETHOD(device_detach,   freebsd_detach),
    DEVMETHOD(device_shutdown, freebsd_shutdown),
    {0, 0}
};

static driver_t fusion_driver = {
    "fct",
    fusion_methods,
    sizeof (struct kfio_freebsd_pci_dev)
};

static devclass_t fusion_devclass;
DRIVER_MODULE(iomemory_vsl, pci, fusion_driver, fusion_devclass,
              freebsd_modevent, 0);
MODULE_DEPEND(iomemory_vsl, pci, 1, 1, 1);
