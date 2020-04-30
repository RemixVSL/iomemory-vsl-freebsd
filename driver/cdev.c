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

#include <sys/poll.h>
#include <sys/selinfo.h>

#include "pci_dev.h"

int fusion_create_control_device(struct fusion_nand_device *nand_dev);
int fusion_destroy_control_device(struct fusion_nand_device *nand_dev);

/******************************************************************************
 * OS-specific interfaces for char device functions that call into the
 * core fucntions.
 */
int freebsd_char_open(struct cdev *dev, int oflags, int devtype, struct thread *td);
int freebsd_char_close(struct cdev *dev, int fflag, int devtype, struct thread *td);
int freebsd_char_ioctl(struct cdev *dev, u_long cmd, caddr_t data, int fflag,
                       struct thread *td);
int freebsd_char_poll(struct cdev *dev, int events, struct thread *td);

static struct cdevsw fusion_ctl_ops = {
    .d_version   = D_VERSION,
    .d_open      = freebsd_char_open,
    .d_close     = freebsd_char_close,
    .d_ioctl     = freebsd_char_ioctl,
    .d_poll      = freebsd_char_poll,
    .d_name      = "fct"
};

static inline struct kfio_freebsd_pci_dev *
fusion_nand_get_softc(void *nand_dev)
{
    kfio_pci_dev_t *pci_dev = fusion_nand_get_pci_dev(nand_dev);
    kassert (pci_dev != NULL);

    return device_get_softc(pci_dev);
}

/******************************************************************************
 * @brief OS-specific init for char device.
 * Node gets created for char device.
 * called from fio-dev/device.c fusion_register_device()
 */
int
fusion_create_control_device(struct fusion_nand_device *nand_dev)
{
    struct kfio_freebsd_pci_dev *pdev;

    pdev = fusion_nand_get_softc(nand_dev);

    pdev->cdev = make_dev(&fusion_ctl_ops, pdev->unit,
        UID_ROOT, GID_OPERATOR, 0660, "%s", fusion_nand_get_dev_name(nand_dev));

    if (NULL == pdev->cdev)
    {
        errprint("%s Unable to initialize misc device '%s'\n",
                 fusion_nand_get_bus_name(nand_dev),
                 fusion_nand_get_dev_name(nand_dev));
        return -1;
    }
    pdev->cdev->si_drv1 = nand_dev;
    return 0;
}

/******************************************************************************
 * called from int fusion_unregister_device()
 */
int
fusion_destroy_control_device(struct fusion_nand_device *nand_dev)
{
    struct kfio_freebsd_pci_dev *pdev;

    pdev = fusion_nand_get_softc(nand_dev);

    if (pdev->cdev != NULL)
    {
        destroy_dev(pdev->cdev);
        pdev->cdev = NULL;
    }
    return 0;
}

/******************************************************************************
 */
int
freebsd_char_open(struct cdev *dev, int oflags, int devtype, struct thread *td)
{
    struct fusion_nand_device *nand_dev;

    nand_dev = dev->si_drv1;
    if (NULL == nand_dev)
    {
        errprint("Call to open() char dev with bad device, name %s\n", devtoname(dev));
        return ENXIO;
    }
    return 0;
}

/******************************************************************************
 */
int
freebsd_char_close(struct cdev *dev, int fflag, int devtype, struct thread *td)
{
    return 0;
}

/******************************************************************************
 */
int
freebsd_char_ioctl(struct cdev *dev, u_long cmd, caddr_t data,
                   int fflag, struct thread *td)
{
    struct fusion_nand_device *nand_dev;
    int result;

    nand_dev = dev->si_drv1;

    if (NULL == nand_dev)
    {
        errprint("Call to ioctl() char dev with bad device, name %s\n",
                 devtoname(dev));
        return ENXIO;
    }

    result = fusion_control_ioctl(nand_dev, (unsigned)cmd, (fio_uintptr_t)((void **)data)[0]);

    return (result < 0) ? -result : 0;
}

/**
 * Return events that are set.  If nand_dev->active_alert has nothing yet
 * set, this waits using the nand_dev->poll_queue, so the user-land application
 * that has called poll(), epoll(), or select() will get notified when something
 * interesting happens.  After return an ioctl needs to be called to get the
 * actual event data.
 */
int
freebsd_char_poll(struct cdev *dev, int events, struct thread *td)
{
    struct fusion_nand_device *nand_dev;
    struct selinfo *sel;

    nand_dev = dev->si_drv1;
    if (NULL == nand_dev)
    {
        errprint("Call to poll() char dev with bad device, name %s\n",
                 devtoname(dev));
        return ENXIO;
    }

    if (0 == (events & (POLLIN | POLLPRI)))
        return 0;

    /* XXXKAN: Race between this check and selrecord. */
    if (is_nand_device_event_set(nand_dev))
        return ((POLLIN | POLLPRI) & events);

    sel = (struct selinfo *)nand_device_get_poll_struct(nand_dev);
    selrecord(td, sel);

    return 0;
}

/**
 * Initialize the kfio_poll_struct
 * Linux - wait_queue_head_t
 * Solaris - struct pollhead
 * FreeBSD - struct selinfo
 */
void
kfio_poll_init(kfio_poll_struct *p)
{
    memset(p, 0, sizeof(struct selinfo));
}

/**
 * Called when events of interest have occurred
 */
void
kfio_poll_wake(kfio_poll_struct *p)
{
    struct selinfo *sel = (struct selinfo *)p;

    selwakeup(sel);
}
