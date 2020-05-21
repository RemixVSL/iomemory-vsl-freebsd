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

#include <sys/kdb.h>
#include <sys/kthread.h>
#include <sys/poll.h>
#include <sys/rwlock.h>
#include <sys/sched.h>
#include <sys/selinfo.h>
#include <sys/sbuf.h>

#include <vm/vm.h>
#include <vm/pmap.h>

#include <machine/pc/bios.h>

#include <fio/port/ktypes.h>
#include <fio/port/dbgset.h>
#include "pci_dev.h"

extern int kfio_kvprint(int level, const char *format, va_list ap);

kfio_cpu_t kfio_current_cpu(void)
{
    return curcpu;
}

/**
 * level = [KFIO_PRINT_NOTE, KFIO_PRINT_WARN, KFIO_PRINT_PANIC]
 */
int kfio_kprint(int level, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    kfio_kvprint(level, format, ap);
    va_end(ap);

    return 0;
}

int kfio_print(const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    kfio_kvprint(KFIO_PRINT_NOTE, format, ap);
    va_end(ap);

    return 0;
}

int kfio_snprintf(char *buffer, fio_size_t n, const char *format, ...)
{
    va_list ap;
    int rc;

    va_start(ap, format);
    rc = kfio_vsnprintf(buffer, n, format, ap);
    va_end(ap);

    return rc;
}

int kfio_vsnprintf(char *buffer, fio_size_t n, const char *format, va_list ap)
{
    return vsnprintf(buffer, n, format, ap);
}

int kfio_strcmp(const char *s1, const char *s2)
{
    return strcmp(s1, s2);
}

fio_size_t kfio_strlen(const char *s1)
{
    return strlen(s1);
}

int kfio_strncmp(const char *s1, const char *s2, fio_size_t n)
{
    return strncmp(s1, s2, n);
}

char *kfio_strncpy(char *dst, const char *src, fio_size_t n)
{
    return strncpy(dst, src, n);
}

char *kfio_strcat(char *dst, const char *src)
{
    return strcat(dst, src);
}

char *kfio_strncat(char *dst, const char *src, int size)
{
    if (size != 0)
    {
        char *d = dst;
        const char *s = src;

        while (*d != 0) d++;
        do
        {
            if ((*d = *s++) == 0)
                break;
            d++;
        }
        while (--size != 0);
        *d = 0;
    }
    return (dst);
}

void *kfio_memset(void *dst, int c, fio_size_t n)
{
    return memset(dst, c, n);
}

int kfio_memcmp(const void *m1, const void *m2, fio_size_t n)
{
    return memcmp(m1, m2, n);
}

void *kfio_memcpy(void *dst, const void *src, fio_size_t n)
{
    return memcpy(dst, src, n);
}

void *kfio_memmove(void *dst, const void *src, fio_size_t n)
{
    bcopy(src, dst, n);
    return (dst);
}

int kfio_stoi(const char *p, int *nchar)
{
    int n, c;

    for (n=0, *nchar=0; ;(*nchar)++){
        c = *p++;
        if ((c < '0') || (c > '9'))
            break;
        n = (n * 10) + (c - '0');
    }
    return (n);
}

unsigned long kfio_strtoul(const char *nptr, char **endptr, int base)
{
    return strtoul(nptr, endptr, base);
}

/* PCI functions */

/**
 * @brief reads one byte from PCI config space at offset off
 */
int kfio_pci_read_config_byte(kfio_pci_dev_t *pdev, int off, uint8_t *val)
{
    uint32_t v;

    v = pci_read_config(pdev, off, sizeof(*val));
    *val = (uint8_t) v;
    return (0);
}

/**
 * @brief reads two bytes from PCI config space at offset off
 */
int kfio_pci_read_config_word(kfio_pci_dev_t *pdev, int off, uint16_t *val)
{
    uint32_t v;

    v = pci_read_config(pdev, off, sizeof(*val));
    *val = (uint16_t) v;
    return (0);
}

/**
 * @brief reads four bytes from PCI config space at offset off
 */
int kfio_pci_read_config_dword(kfio_pci_dev_t *pdev, int off, uint32_t *val)
{
    uint32_t v;

    v = pci_read_config(pdev, off, sizeof(*val));
    *val = v;
    return (0);
}

/**
 * @brief writes one byte to PCI config space at offset off
 */
int kfio_pci_write_config_byte(kfio_pci_dev_t *pdev, int off, uint8_t val)
{
    pci_write_config(pdev, off, val, sizeof(val));
    return (0);
}

/**
 * @brief writes two bytes to PCI config space at offset off
 */
int kfio_pci_write_config_word(kfio_pci_dev_t *pdev, int off, uint16_t val)
{
    pci_write_config(pdev, off, val, sizeof(val));
    return (0);
}

/**
 * @brief writes four bytes to PCI config space at offset off
 */
int kfio_pci_write_config_dword(kfio_pci_dev_t *pdev, int off, uint32_t val)
{
    pci_write_config(pdev, off, val, sizeof(val));
    return (0);
}

const char *kfio_pci_name(kfio_pci_dev_t *pdev)
{
    struct kfio_freebsd_pci_dev *pd = device_get_softc(pdev);

    return pd->pci_name;
}

uint16_t kfio_pci_get_vendor(kfio_pci_dev_t *pdev)
{
    return pci_get_vendor(pdev);
}

uint32_t kfio_pci_get_devnum(kfio_pci_dev_t *pdev)
{
    return pci_get_device(pdev);
}

/**
 * @brief returns PCI bus
 */
uint8_t kfio_pci_get_bus(kfio_pci_dev_t *pdev)
{
    return pci_get_bus(pdev);
}

/**
 * @brief returns device num
 */
uint8_t kfio_pci_get_devicenum(kfio_pci_dev_t *pdev)
{
    return pci_get_slot(pdev);
}

/*
 * PCI interrupt routing table.
 *
 * $PIR in the BIOS segment contains a PIR_table
 * int 1a:b106 returns PIR_table in buffer at es:(e)di
 * int 1a:b18e returns PIR_table in buffer at es:(e)di
 * int 1a:b406 returns es:di pointing to the BIOS PIR_table
 */
struct pir_header
{
    int8_t  ph_signature[4];
    u_int16_t   ph_version;
    u_int16_t   ph_length;
    u_int8_t    ph_router_bus;
    u_int8_t    ph_router_dev_fn;
    u_int16_t   ph_pci_irqs;
    u_int16_t   ph_router_vendor;
    u_int16_t   ph_router_device;
    u_int32_t   ph_miniport;
    u_int8_t    ph_res[11];
    u_int8_t    ph_checksum;
} __packed;

struct pir_intpin
{
    u_int8_t    link;
    u_int16_t   irqs;
} __packed;

struct pir_entry
{
    u_int8_t        pe_bus;
    u_int8_t        pe_res1:3;
    u_int8_t        pe_device:5;
    struct pir_intpin   pe_intpin[4];
    u_int8_t    pe_slot;
    u_int8_t    pe_res3;
} __packed;

struct pir_table
{
    struct pir_header   pt_header;
    struct pir_entry    pt_entry[0];
} __packed;

#define REQUIRED_PIR_VERSION    0x100

/**
 * @brief returns device slot
 * This function tries to locate $PIR table in BIOS and looks for matching
 * PCI link entry for this device. The slot number is this table is what
 * we are looking for. 0 means on-board device, non-zero was supposed
 * to be meaninful PCI slot number as supplied by motherboard vendor.
 */
uint8_t kfio_pci_get_slot(kfio_pci_dev_t *pdev)
{
    struct pir_header *header;
    struct pir_table  *pt;
    uint32_t sigaddr;
    int      i, count;
    uint8_t  slot_num;

    /* Look for $PIR and then _PIR. */
    sigaddr = bios_sigsearch(0, "$PIR", 4, 16, 0);
    if (sigaddr == 0)
        sigaddr = bios_sigsearch(0, "_PIR", 4, 16, 0);
    if (sigaddr == 0)
    {
        return 0;
    }

    slot_num = 0;

    /*
     * Map the header.  We first map a fixed size to get the actual
     * length and then map it a second time with the actual length so
     * we can verify the checksum and walk the list.
     */
    header = pmap_mapbios(sigaddr, sizeof(struct pir_header));

    /* Verify table length. */
    if (header->ph_length <= sizeof(struct pir_header) ||
        header->ph_version != REQUIRED_PIR_VERSION)
    {
        pmap_unmapbios((vm_offset_t)header, sizeof(struct pir_header));
        return 0;
    }

    pt = pmap_mapbios(sigaddr, header->ph_length);

    /* No need for header at this point */
    pmap_unmapbios((vm_offset_t)header, sizeof(struct pir_header));

    /* Ok, we've got a valid table. */
    count = (pt->pt_header.ph_length - sizeof(struct pir_header)) /
             sizeof(struct pir_entry);
    for (i = 0; i < count; i++)
    {
        struct pir_entry *pe = &pt->pt_entry[i];

        if (pe->pe_bus == pci_get_bus(pdev) &&
            pe->pe_device == pci_get_slot(pdev))
        {
            slot_num = pe->pe_slot;
            break;
        }
    }

    pmap_unmapbios((vm_offset_t)pt, pt->pt_header.ph_length);
    return slot_num;
}

kfio_pci_bus_t *kfio_bus_from_pci_dev(kfio_pci_dev_t *pdev)
{
    /* fct -> pci */
    return device_get_parent(pdev);
}

kfio_pci_bus_t *kfio_pci_bus_parent(kfio_pci_bus_t *bus)
{
    /* pci -> pcib -> pci */
    return device_get_parent(kfio_pci_bus_self(bus));
}

int kfio_pci_bus_istop(kfio_pci_bus_t *bus)
{
    return kfio_pci_bus_number(bus) == 0;
}

kfio_pci_dev_t *kfio_pci_bus_self(kfio_pci_bus_t *bus)
{
    /* pci -> pcib */
    return device_get_parent(bus);
}

uint8_t kfio_pci_bus_number(kfio_pci_bus_t *bus)
{
    return pcib_get_bus(bus);
}

/**
 * @brief returns PCI function
 */
uint8_t kfio_pci_get_function(kfio_pci_dev_t *pdev)
{
    return pci_get_function(pdev);
}

/**
 * @brief returns PCI subsystem vendor id
 */
uint16_t kfio_pci_get_subsystem_vendor(kfio_pci_dev_t *pdev)
{
    return pci_get_subvendor(pdev);
}

/**
 * @brief returns PCI subsystem device
 */
uint16_t kfio_pci_get_subsystem_device(kfio_pci_dev_t *pdev)
{
    return pci_get_subdevice(pdev);
}

uint64_t kfio_pci_resource_start(kfio_pci_dev_t *pdev, uint16_t bar)
{
    struct kfio_freebsd_pci_dev *pd = device_get_softc(pdev);

    if (pd->bar_resource != NULL)
    {
        if (rman_get_rid(pd->bar_resource) == PCIR_BAR(bar))
            return rman_get_start(pd->bar_resource);
    }

    return 0;
}

uint32_t kfio_pci_resource_len(kfio_pci_dev_t *pdev, uint16_t bar)
{
    struct kfio_freebsd_pci_dev *pd = device_get_softc(pdev);

    if (pd->bar_resource != NULL)
    {
        if (rman_get_rid(pd->bar_resource) == PCIR_BAR(bar))
            return rman_get_size(pd->bar_resource);
    }

    return 0;
}

/**
 * (enable == 0) => use MSI interrupts else use legacy interrupts
 */
void kfio_iodrive_intx (kfio_pci_dev_t *pci_dev, int enable)
{
    uint16_t c, n;

    kfio_pci_read_config_word(pci_dev, PCIR_COMMAND, &c);

    if (enable)
        n = c & ~PCIM_CMD_INTX_DISABLE;
    else
        n = c | PCIM_CMD_INTX_DISABLE;

    if (n != c)
        kfio_pci_write_config_word(pci_dev, PCIR_COMMAND, n);
}

#if IODRIVE_DISABLE_PCIE_RELAXED_ORDERING

#ifndef PCI_EXP_DEVCTL_RELAX_EN
#  define PCI_EXP_DEVCTL_RELAX_EN 0x0010
#endif

void kfio_pci_disable_relaxed_ordering(kfio_pci_dev_t *pdev)
{
    uint16_t cap;
    int      pos;

    dbgprint(DBGS_GENERAL, "iodrive_pci_probe: disabling relaxed ordering...\n");

    pos = pci_find_capability(pdev, PCI_CAP_ID_EXP);
    if (pci_find_extcap(pd->dev, PCIY_EXPRESS, &pos) == 0)
    {
        kfio_pci_read_config_word(pdev, pos + PCIR_EXPRESS_DEVICE_CTL, &cap);

        if (cap & PCI_EXP_DEVCTL_RELAX_EN)
        {
            cap &= ~PCI_EXP_DEVCTL_RELAX_EN;

            kfio_pci_write_config_word(pdev, pos + PCIR_EXPRESS_DEVICE_CTL,
                                       cap);
        }
    }
}
#endif

/*
 * Spinlock wrappers
 */
C_ASSERT(sizeof(fusion_spinlock_t) >= sizeof(struct mtx));

void fusion_init_spin(fusion_spinlock_t *lock, const char *name)
{
    struct mtx *m = (struct mtx *)lock;

    mtx_init(m, name, NULL, MTX_DEF | MTX_DUPOK);
}

void fusion_destroy_spin(fusion_spinlock_t *lock)
{
    struct mtx *m = (struct mtx *)lock;

    mtx_destroy(m);
}

/*
 * returns non-zero if locked by this process, else 0
 */
int fusion_spin_is_locked(fusion_spinlock_t *lock)
{
    struct mtx *m = (struct mtx *)lock;

    return mtx_owned(m);
}

void _fusion_spin_lock(fusion_spinlock_t *lock, const char *file, int line)
{
    struct mtx *m = (struct mtx *)lock;

    _mtx_lock_flags(m, 0, file, line);
}

/**
 * returns non-zero when lock is acquired and 0 if lock is already held.
 */
int _fusion_spin_trylock(fusion_spinlock_t *lock, const char *file, int line)
{
    struct mtx *m = (struct mtx *)lock;

    return mtx_trylock_flags_(m, 0, file, line) ? 1 : 0;
}

void _fusion_spin_unlock(fusion_spinlock_t *lock, const char *file, int line)
{
    struct mtx *m = (struct mtx *)lock;

    _mtx_unlock_flags(m, 0, file, line);
}

/*
 * Mutex wrappers
 */
C_ASSERT(sizeof(fusion_mutex_t) >= sizeof(struct sx));

void fusion_mutex_init(fusion_mutex_t *lock, const char *name)
{
    struct sx *s = (struct sx *)lock;

    sx_init_flags(s, name, SX_DUPOK);
}

void fusion_mutex_destroy(fusion_mutex_t *lock)
{
    struct sx *s = (struct sx *)lock;

    sx_destroy(s);
}

int _fusion_mutex_trylock(fusion_mutex_t *lock, const char *file, int line)
{
    struct sx *s = (struct sx *)lock;

    return sx_try_xlock_(s, file, line) ? 1 : 0;
}

void _fusion_mutex_lock(fusion_mutex_t *lock, const char *file, int line)
{
    struct sx *s = (struct sx *)lock;

    _sx_xlock(s, 0, file, line);
}

void _fusion_mutex_unlock(fusion_mutex_t *lock, const char *file, int line)
{
    struct sx *s = (struct sx *)lock;

    _sx_xunlock(s, file, line);
}

/*
 * RW spinlock wrappers
 */
C_ASSERT(sizeof(fusion_rw_spinlock_t) >= sizeof(struct rwlock));

void fusion_init_rw_spin(fusion_rw_spinlock_t *lock, const char *name)
{
    struct rwlock *rw = (struct rwlock *)lock;

    rw_init_flags(rw, name, RW_DUPOK);
}

void fusion_destroy_rw_spin(fusion_rw_spinlock_t *lock)
{
    struct rwlock *rw = (struct rwlock *)lock;

    rw_destroy(rw);
}

void _fusion_spin_read_lock(fusion_rw_spinlock_t *lock, const char *file, int line)
{
    struct rwlock *rw = (struct rwlock *)lock;

    _rw_rlock(rw, file, line);
}

void _fusion_spin_write_lock(fusion_rw_spinlock_t *lock, const char *file, int line)
{
    struct rwlock *rw = (struct rwlock *)lock;

    _rw_wlock(rw, file, line);
}

void _fusion_spin_read_unlock(fusion_rw_spinlock_t *lock, const char *file, int line)
{
    struct rwlock *rw = (struct rwlock *)lock;

    _rw_runlock(rw, file, line);
}

void _fusion_spin_write_unlock(fusion_rw_spinlock_t *lock, const char *file, int line)
{
    struct rwlock *rw = (struct rwlock *)lock;

    _rw_wunlock(rw, file, line);
}

/*
 * RW semaphores
 */
C_ASSERT(sizeof(fusion_rwsem_t) >= sizeof(struct sx));

void fusion_rwsem_init(fusion_rwsem_t *lock, const char *name)
{
    struct sx *s = (struct sx *)lock;

    sx_init_flags(s, name, SX_DUPOK);
}

void fusion_rwsem_delete(fusion_rwsem_t *lock)
{
    struct sx *s = (struct sx *)lock;

    sx_destroy(s);
}

void _fusion_rwsem_down_read(fusion_rwsem_t *lock, const char *file, int line)
{
    struct sx *s = (struct sx *)lock;

    _sx_slock(s, 0, file, line);
}

void _fusion_rwsem_up_read(fusion_rwsem_t *lock, const char *file, int line)
{
    struct sx *s = (struct sx *)lock;

    _sx_sunlock(s, file, line);
}

void _fusion_rwsem_down_write(fusion_rwsem_t *lock, const char *file, int line)
{
    struct sx *s = (struct sx *)lock;

    _sx_xlock(s, 0, file, line);
}

void _fusion_rwsem_up_write(fusion_rwsem_t *lock, const char *file, int line)
{
    struct sx *s = (struct sx *)lock;

    _sx_xunlock(s, file, line);
}

#if __FreeBSD_version >= 800002
#  define kthread_create    kproc_create
#  define kthread_exit      kproc_exit
#endif

struct fusion_thread_args
{
    fusion_kthread_func_t func;
    void                 *args;
};

static void
fusion_kthread_wrapper(void *data)
{
    struct fusion_thread_args params;
    int exitcode;

    /* Copy data on stack and get rid of malloced data. */
    memcpy(&params, data, sizeof(params));
    free(data, M_TEMP);

#if __FreeBSD_version < 700044
    mtx_lock_spin(&sched_lock);
    sched_prio(curthread, PRIBIO);
    mtx_unlock_spin(&sched_lock);
#else
    thread_lock(curthread);
    sched_prio(curthread, PRIBIO);
    thread_unlock(curthread);
#endif

    /*
     * Run passed in function and terminate with kthread_exit
     * to make FreeBSD kernel happy.
     */
    exitcode = params.func(params.args);
    kthread_exit(exitcode);
}

void fusion_create_kthread(fusion_kthread_func_t func, void *data,
                           void *fusion_nand_device, const char *fmt, ...)
{
    struct fusion_thread_args *params;
    struct sbuf *s;
    va_list ap;

    s = sbuf_new(NULL, NULL, 0, SBUF_AUTOEXTEND);

    va_start(ap, fmt);
    sbuf_vprintf(s, fmt, ap);
    va_end(ap);

    sbuf_finish(s);

    params = malloc(sizeof *params, M_TEMP, M_WAITOK);
    params->func = func;
    params->args = data;

    kthread_create(fusion_kthread_wrapper, params, NULL, 0, 0, "%s", sbuf_data(s));

    sbuf_delete(s);
}

/*
 * Userspace <-> Kernelspace routines
 * XXX: all the kfio_[put\get]_user_*() routines are superfluous
 * and ought be replaced with kfio_copy_[to|from]_user()
 */
int kfio_copy_from_user(void *to, const void *from, unsigned len)
{
    return copyin(from, to, len);
}

int kfio_copy_to_user(void *to, const void *from, unsigned len)
{
    return copyout(from, to, len);
}

/* copies values from kernel to user space
 */
int kfio_put_user_8(int x, uint8_t *arg)
{
    uint8_t t = (uint8_t) x;
    return copyout(&t, arg, 1);
}

int kfio_put_user_16(int x, uint16_t *arg)
{
    uint16_t t = (uint16_t) x;
    return copyout(&t, arg, 2);
}

int kfio_put_user_32(int x, uint32_t *arg)
{
    uint32_t t = (uint32_t) x;
    return copyout(&t, arg, 4);
}

int kfio_put_user_64(int x, uint64_t *arg)
{
    uint64_t t = (uint64_t) x;
    return copyout(&t, arg, 8);
}

void kfio_dump_stack()
{
    kdb_backtrace();
}

void *kfio_pci_get_drvdata(kfio_pci_dev_t *pdev)
{
    struct kfio_freebsd_pci_dev *pd = device_get_softc(pdev);

    return pd->prv;
}

void kfio_pci_set_drvdata(kfio_pci_dev_t *pdev, void *data)
{
    struct kfio_freebsd_pci_dev *pd = device_get_softc(pdev);

    pd->prv = data;
}

/*
 * Platform specific allocations and structure size checks
 */

/**
 * @brief checks size of abstracted fusion_* structures against the size of the
 * OS-specific structure they wrap.
 */
int kfio_checkstructs(void)
{
    int rc = 0;
    char badstr[64];


    if (sizeof(fusion_spinlock_t) < sizeof(struct mtx))
    {
        snprintf(badstr, sizeof(badstr), "spinlock (%d)",
                 (int)sizeof(struct mtx));
        rc = -1;
    }

    if (sizeof(fusion_mutex_t) < sizeof(struct sx))
    {
        snprintf(badstr, sizeof(badstr), "mutex (%d)",
                 (int)sizeof(struct mtx));
        rc = -1;
    }

    if (sizeof(fusion_rw_spinlock_t) < sizeof(struct rwlock))
    {
        snprintf(badstr, sizeof(badstr), "rwlock (%d)",
                 (int)sizeof(struct sx));
        rc = -1;
    }

    if (sizeof(fusion_rwsem_t) < sizeof(struct sx))
    {
        snprintf(badstr, sizeof(badstr), "rw_semaphore (%d)",
                 (int)sizeof(struct sx));
        rc = -1;
    }

    if (sizeof(kfio_poll_struct) < sizeof(struct selinfo))
    {
        snprintf(badstr, sizeof(badstr), "kfio_poll_struct (%d)",
                 (int)sizeof(struct selinfo));
        rc = -1;
    }

    if (sizeof(fusion_condvar_t) < sizeof(struct cv))
    {
        snprintf(badstr, sizeof(badstr), "fusion_condvar_t (%d)",
                 (int)sizeof(struct cv));
        rc = -1;
    }

    if (rc)
    {
        errprint("\nThe kernel source and the shipped object %s sizes are inconsistent.\n"
                 "Your kernel is currently not supported; please contact customer support\n"
                 "for assistance.\n", badstr);
    }

    return (rc);
}
