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

#ifndef __FUSION_FREEBSD_KTYPES_H__
#define __FUSION_FREEBSD_KTYPES_H__

#ifndef __FIO_PORT_KTYPES_H__
#error Do not include this file directly - instead include <fio/port/ktypes.h>
#endif

#include <fio/port/freebsd/kassert.h>

#if !defined(__KERNEL__)
#  include <fio/port/stdint.h>
#  include <fio/port/bitops.h>
#  include <fio/port/ufio.h> //kassert()
#  include <stdio.h> // fprintf
#else
#  include <sys/param.h>
#  include <sys/systm.h> //KASSERT
#  include <sys/kdb.h> //KASSERT
#endif
#include <sys/endian.h>

#include <fio/port/freebsd/commontypes.h>

#define DECLARE_RESERVE(x) char foo[x] __attribute__ ((aligned(8)))

typedef int kfio_numa_node_t;

/*
 * Earlier FreeBSD version require more space reservation for lock
 * primitives.
 */
#if __FreeBSD_version <= 700000
#  define FUSION_FREEBSD_SX_RESERVE     128
#  define FUSION_FREEBSD_SEMA_RESERVE    96
#  define FUSION_FREEBSD_MTX_RESERVE     72
#  define FUSION_FREEBSD_RWLOCK_RESERVE  72
#  define FUSION_FREEBSD_CV_RESERVE      16
#else
#  define FUSION_FREEBSD_SX_RESERVE      48
#  define FUSION_FREEBSD_SEMA_RESERVE    72
#  define FUSION_FREEBSD_MTX_RESERVE     48
#  define FUSION_FREEBSD_RWLOCK_RESERVE  48
#  define FUSION_FREEBSD_CV_RESERVE      16
#endif

/**
 * brief @struct __fusion_poll_struct
 * OS-independent structure for poll support:
 * Linux wait_queue_head_t
 * Solaris struct pollhead
 * FreeBSD struct selinfo
 *
 */
struct FUSION_STRUCT_ALIGN(8) __fusion_poll_struct {
  DECLARE_RESERVE(80);
};


#define KERNEL_SECTOR_SIZE   512 /* ARGH */

typedef struct device kfio_pci_dev_t;
typedef struct device kfio_pci_bus_t;

typedef int  kfio_irqreturn_t;

#define FIO_IRQ_NONE        (0)
#define FIO_IRQ_HANDLED     (1)
#define FIO_IRQ_RETVAL(x)   ((x) != 0)

#define FIO_WORD_SIZE       8
#define FIO_BITS_PER_LONG   64

#define LODWORD(quad)            ( (uint32_t)(quad) )
#define HIDWORD(quad)            ( (uint32_t)(((quad) >> 32) & 0x00000000ffffffffULL) )

#define ASSERT(x)                ((void)0)

#include <fio/port/list.h>

/**  the following comes through the porting layer */
#define FUSION_PAGE_SHIFT           12
#define FUSION_PAGE_SIZE            (1ULL<<FUSION_PAGE_SHIFT)
#define FUSION_PAGE_MASK            (~(FUSION_PAGE_SIZE-1))
#define FUSION_PAGE_ALIGN_UP(addr)  ((char*)((((fio_uintptr_t)(addr))+FUSION_PAGE_SIZE-1)&FUSION_PAGE_MASK))
#define FUSION_ROUND_TO_PAGES(size) ((fio_uintptr_t)FUSION_PAGE_ALIGN_UP(size))

#if !defined(S_IRWXU)
#define S_IRWXU 00700
#define S_IRUSR 00400
#define S_IWUSR 00200
#define S_IXUSR 00100

#define S_IRWXG 00070
#define S_IRGRP 00040
#define S_IWGRP 00020
#define S_IXGRP 00010

#define S_IRWXO 00007
#define S_IROTH 00004
#define S_IWOTH 00002
#define S_IXOTH 00001
#endif

#define S_IRUGO         (S_IRUSR|S_IRGRP|S_IROTH)
#define S_IWUGO         (S_IWUSR|S_IWGRP|S_IWOTH)
#define S_IXUGO         (S_IXUSR|S_IXGRP|S_IXOTH)

#define READ 0
#define WRITE 1

#ifdef _KERNEL
#  define KFIO_BIO_READ    1
#  define KFIO_BIO_WRITE   2
#  define KFIO_BIO_DISCARD 3
#endif

// Valid values for use_workqueue.
#define USE_QUEUE_NONE   0 ///< directly submit requests.
#define USE_QUEUE_WQ     1 ///< use kernel work queue.
#define USE_QUEUE_SINGLE 2 ///< use single kernel thread.

/*
 * In the FUSION_DEBUG case, use the filename and line numbers for debugging
 * operations.  Otherwise, use default values to avoid the unneeded bloat.
 */
#if FUSION_DEBUG > 0
#define FIO_LOCK_FILE   __FILE__
#define FIO_LOCK_LINE   __LINE__
#else
#define FIO_LOCK_FILE   NULL
#define FIO_LOCK_LINE   0
#endif

struct __fusion_spinlock
{
  /**
   * FreeBSD: struct mtx
   */
  DECLARE_RESERVE(FUSION_FREEBSD_MTX_RESERVE);
} __attribute__((aligned(8)));

typedef struct __fusion_spinlock  fusion_spinlock_t;

extern void fusion_init_spin(fusion_spinlock_t *, const char *name);
extern void fusion_destroy_spin(fusion_spinlock_t *);
extern int  fusion_spin_is_locked(fusion_spinlock_t *);

#define fusion_spin_unlock(s)            _fusion_spin_unlock((s), FIO_LOCK_FILE, FIO_LOCK_LINE)
#define fusion_spin_unlock_irqrestore(s) _fusion_spin_unlock((s), FIO_LOCK_FILE, FIO_LOCK_LINE)
#define fusion_spin_lock(s)              _fusion_spin_lock((s), FIO_LOCK_FILE, FIO_LOCK_LINE)
#define fusion_spin_lock_irqdisabled(s)  _fusion_spin_lock((s), FIO_LOCK_FILE, FIO_LOCK_LINE)
#define fusion_spin_lock_irqsave(s)      _fusion_spin_lock((s), FIO_LOCK_FILE, FIO_LOCK_LINE)
#define fusion_spin_lock_irqsave_nested(s, subclass) \
                                         _fusion_spin_lock((s), FIO_LOCK_FILE, FIO_LOCK_LINE)
#define fusion_spin_trylock(s)           _fusion_spin_trylock((s), FIO_LOCK_FILE, FIO_LOCK_LINE)
#define fusion_spin_trylock_irqsave(s)   _fusion_spin_trylock((s), FIO_LOCK_FILE, FIO_LOCK_LINE)

extern void _fusion_spin_unlock(fusion_spinlock_t *, const char *, int);
extern void _fusion_spin_lock(fusion_spinlock_t *, const char *, int);
extern int  _fusion_spin_trylock(fusion_spinlock_t *, const char *, int);

struct __fusion_mutex
{
  /**
   * FreeBSD: struct sx
   */
  DECLARE_RESERVE(FUSION_FREEBSD_SX_RESERVE);
} __attribute__((aligned(8)));

typedef struct __fusion_mutex  fusion_mutex_t;

extern void fusion_mutex_init(fusion_mutex_t *lock, const char *);
extern void fusion_mutex_destroy(fusion_mutex_t *lock);

#define fusion_mutex_trylock(m) _fusion_mutex_trylock((m), FIO_LOCK_FILE, FIO_LOCK_LINE)
#define fusion_mutex_lock(m)    _fusion_mutex_lock((m), FIO_LOCK_FILE, FIO_LOCK_LINE)
#define fusion_mutex_unlock(m)  _fusion_mutex_unlock((m), FIO_LOCK_FILE, FIO_LOCK_LINE)

extern int  _fusion_mutex_trylock(fusion_mutex_t *, const char *, int);
extern void _fusion_mutex_lock(fusion_mutex_t *, const char *, int);
extern void _fusion_mutex_unlock(fusion_mutex_t *, const char *, int);

#if defined(__KERNEL__)

struct __fusion_rwsem
{
  /**
   * FreeBSD: struct sx
   */
  DECLARE_RESERVE(FUSION_FREEBSD_SX_RESERVE);
} __attribute__((aligned(8)));

typedef struct __fusion_rwsem fusion_rwsem_t;

extern void fusion_rwsem_init(fusion_rwsem_t *, const char *name);
extern void fusion_rwsem_delete(fusion_rwsem_t *);

extern void _fusion_rwsem_down_read(fusion_rwsem_t *, const char *, int);
extern void _fusion_rwsem_up_read(fusion_rwsem_t *, const char *, int);
extern void _fusion_rwsem_down_write(fusion_rwsem_t *, const char *, int);
extern void _fusion_rwsem_up_write(fusion_rwsem_t *, const char *, int);

#define fusion_rwsem_down_read(rw)  _fusion_rwsem_down_read((rw), FIO_LOCK_FILE, FIO_LOCK_LINE)
#define fusion_rwsem_up_read(rw)    _fusion_rwsem_up_read((rw), FIO_LOCK_FILE, FIO_LOCK_LINE)
#define fusion_rwsem_down_write(rw) _fusion_rwsem_down_write((rw), FIO_LOCK_FILE, FIO_LOCK_LINE)
#define fusion_rwsem_up_write(rw)   _fusion_rwsem_up_write((rw), FIO_LOCK_FILE, FIO_LOCK_LINE)

#endif

struct __fusion_rwspin
{
  /**
   * FreeBSD: struct rwlock
   */
  DECLARE_RESERVE(FUSION_FREEBSD_RWLOCK_RESERVE);
} __attribute__((aligned(8)));

typedef struct __fusion_rwspin fusion_rw_spinlock_t;

extern void fusion_init_rw_spin(fusion_rw_spinlock_t *, const char *name);
extern void fusion_destroy_rw_spin(fusion_rw_spinlock_t *);

extern void _fusion_spin_read_lock(fusion_rw_spinlock_t *, const char *, int);
extern void _fusion_spin_write_lock(fusion_rw_spinlock_t *, const char *, int);
extern void _fusion_spin_read_unlock(fusion_rw_spinlock_t *, const char *, int);
extern void _fusion_spin_write_unlock(fusion_rw_spinlock_t *, const char *, int);

#define fusion_spin_read_lock(s)    _fusion_spin_read_lock((s), FIO_LOCK_FILE, FIO_LOCK_LINE)
#define fusion_spin_write_lock(s)   _fusion_spin_write_lock((s), FIO_LOCK_FILE, FIO_LOCK_LINE)
#define fusion_spin_read_unlock(s)  _fusion_spin_read_unlock((s), FIO_LOCK_FILE, FIO_LOCK_LINE)
#define fusion_spin_write_unlock(s) _fusion_spin_write_unlock((s), FIO_LOCK_FILE, FIO_LOCK_LINE)


typedef void *fusion_kthread_t;

#define MAX_KTHREAD_NAME_LENGTH  256 // Actually the name buffer will auto-extend
typedef int (*fusion_kthread_func_t)(void *);

//nand_device can be NULL in the following fxn call
void fusion_create_kthread(fusion_kthread_func_t func, void *data, void *fusion_nand_device, const char *fmt, ...);

/**
 * used to guarantee ordering of stores especially when writing to
 * mapped CSR memory on the card.
 */
#define kfio_barrier()          __asm__ volatile("mfence":::"memory")

#define fusion_divmod(quotient, remainder, dividend, divisor)  do { \
   quotient  = dividend / divisor;  \
   remainder = dividend % divisor; \
} while(0)

// Returns true if we're running on what is considered a server OS
#define fusion_is_server_os()     0

#define kfio_rdtsc()  rdtsc()

/* FreeBSD port implements kinfo backend using sysctl. */
#define KFIO_INFO_USE_OS_BACKEND 1
#define KFIO_SUPPORTS_DUAL_PIPE 1

#endif // __FUSION_FREEBSD_KTYPES_H__
