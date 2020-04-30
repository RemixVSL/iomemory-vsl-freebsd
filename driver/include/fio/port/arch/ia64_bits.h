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

#ifndef __FUSION_ARCH_IA64_BITS__
#define __FUSION_ARCH_IA64_BITS__

typedef uint32_t fusion_bits_t;

#if defined(__hpc__)
// Support using HP Ansi C compiler

#include <machine/sys/inline.h>

/**
 * @brief: returns 1 iff bit # v is set in *p else returns 0.
 */
static inline int fio_test_bit(int v, const volatile fusion_bits_t *p)
{
    return ((1U << (v & 31)) & *p) != 0;
}

/**
 * @brief atomic set of bit # v of *p.
 */
static inline void fio_set_bit(int v, volatile fusion_bits_t *p)
{
    uint64_t mask = (1U << (v & 31));
    uint64_t old, ret;

    do
    {
        old = _Asm_zxt(_XSZ_4, *p);
        _Asm_mov_to_ar(_AREG_CCV, old);

        ret = _Asm_cmpxchg(_SZ_W, _SEM_REL, p, old|mask, _LDHINT_NONE);
    } while (ret != old);
}

/**
 * @brief atomic clear of bit # v of *p.
 */
static inline void fio_clear_bit(int v, volatile fusion_bits_t *p)
{
    uint64_t mask = ~(1U << (v & 31));
    uint64_t old, ret;

    do
    {
        old = _Asm_zxt(_XSZ_4, *p);
        _Asm_mov_to_ar(_AREG_CCV, old);

        ret = _Asm_cmpxchg(_SZ_W, _SEM_REL, p, old&mask, _LDHINT_NONE);
    } while (ret != old);
}

/**
 * @brief atomic set of bit # v of *p, returns old value.
 */
static inline unsigned char fio_test_and_set_bit(int v, volatile fusion_bits_t *p)
{
    uint64_t mask = (1U << (v & 31));
    uint64_t old, ret;

    do
    {
        old = _Asm_zxt(_XSZ_4, *p);
        _Asm_mov_to_ar(_AREG_CCV, old);

        ret = _Asm_cmpxchg(_SZ_W, _SEM_REL, p, old|mask, _LDHINT_NONE);
    } while (ret != old);

    return (unsigned char)((old >> v) & 1);
}

/**
 * @brief atomic clear of bit # v of *p, returns old value.
 */
static inline unsigned char fio_test_and_clear_bit(int v, volatile fusion_bits_t *p)
{
    uint64_t mask = ~(1U << (v & 31));
    uint64_t old, ret;

    do
    {
        old = _Asm_zxt(_XSZ_4, *p);
        _Asm_mov_to_ar(_AREG_CCV, old);

        ret = _Asm_cmpxchg(_SZ_W, _SEM_REL, p, old&mask, _LDHINT_NONE);
    } while (ret != old);

    return (unsigned char)((old >> v) & 1);
}

#else
// Use the sync builtins if our gcc version ( >= 4.0 ) supports it.  If not
// the following functions will be used instead.
#if __GNUC__ >= 4 && __GNUC_MINOR__ >= 2

/**
 * @brief: returns 1 iff bit # v is set in *p else returns 0.
 */
static inline int fio_test_bit(int v, const volatile fusion_bits_t *p)
{
    return ((1U << (v & 31)) & *p) != 0;
}

/**
 * @brief atomic set of bit # v of *p.
 */
static inline void fio_set_bit(int v, volatile fusion_bits_t *p)
{
    __sync_or_and_fetch(p, (1U << (v & 31)));
}

/**
 * @brief atomic clear of bit # v of *p.
 */
static inline void fio_clear_bit(int v, volatile fusion_bits_t *p)
{
    __sync_and_and_fetch(p, ~(1U << (v & 31)));
}

/**
 * @brief atomic set of bit # v of *p, returns old value.
 */
static inline unsigned char fio_test_and_set_bit(int v, volatile fusion_bits_t *p)
{
    unsigned mask = (1U << (v & 31));
    unsigned old, ret;

    do
    {
        old = *p;

        ret = __sync_val_compare_and_swap(p, old, old|mask);
    } while (ret != old);

    return (unsigned char)((old >> v) & 1);
}

/**
 * @brief atomic clear of bit # v of *p, returns old value.
 */
static inline unsigned char fio_test_and_clear_bit(int v, volatile fusion_bits_t *p)
{
    unsigned mask = ~(1U << (v & 31));
    unsigned old, ret;

    do
    {
        old = *p;

        ret = __sync_val_compare_and_swap(p, old, old&mask);
    } while (ret != old);

    return (unsigned char)((old >> v) & 1);
}

#else // __GNUC__ >= 4 && __GNUC_MINOR__ >= 2

#error Unsupported GCC version for Itanium

#endif // __GNUC__ >= 4 && __GNUC_MINOR__ >= 2

#endif // __hpc__



#endif // __FUSION_ARCH_IA64_BITS__

