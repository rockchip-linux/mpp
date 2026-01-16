/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_ring"

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <linux/memfd.h>

#include "mpp_env.h"
#include "mpp_debug.h"
#include "mpp_common.h"
#include "mpp_mem_pool.h"
#include "mpp_singleton.h"

#include "mpp_ring.h"

#define RING_DBG_FLOW                   (0x00000001)
#define RING_DBG_INIT                   (0x00000002)

#define ring_dbg(flag, fmt, ...)        mpp_dbg(mpp_ring_debug, flag, fmt, ## __VA_ARGS__)
#define ring_dbg_f(flag, fmt, ...)      mpp_dbg_f(mpp_ring_debug, flag, fmt, ## __VA_ARGS__)

#define ring_dbg_flow(fmt, ...)         ring_dbg(RING_DBG_FLOW, fmt, ## __VA_ARGS__)
#define ring_dbg_init(fmt, ...)         ring_dbg(RING_DBG_INIT, fmt, ## __VA_ARGS__)

typedef struct MppRingImpl_t {
    void *ptr;
    rk_s32 fd;
    rk_s32 size;
    rk_s32 size_align;
} MppRingImpl;

static const char *module_name = MODULE_TAG;
static MppMemPool pool_ring = NULL;
static rk_s32 page_sz = 0;
static rk_u32 mpp_ring_debug = 0;

#ifdef __BIONIC__

/* Android ndk before API 29 */
#include <sys/syscall.h>

#ifndef MFD_CLOEXEC
#define MFD_CLOEXEC        0x0001U
#endif

#ifndef __NR_memfd_create
#if defined(__aarch64__) && !defined(__ILP32__)
#  define __NR_memfd_create  279          /* 64-bit ARM64 */
#elif defined(__arm__) || defined(__aarch64__) && defined(__ILP32__)
#  define __NR_memfd_create  356          /* 32-bit ARM / compat */
#else
#error "please define __NR_memfd_create for your arch"
#endif
#endif

static inline int memfd_create_impl(const char *name, unsigned int flags)
{
    return syscall(__NR_memfd_create, name, flags);
}

#define memfd_create  memfd_create_impl

#endif

static void *mmap_twice(RK_S32 fd, RK_S32 n)
{
    char *whole;
    char *base;
    char *base2;

    whole = mmap(NULL, 2 * n, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (whole == MAP_FAILED) {
        mpp_loge("whole mmap size %d failed %s\n", 2 * n, strerror(errno));
        return NULL;
    }

    base = mmap(whole, n, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0);
    if (base == MAP_FAILED) {
        mpp_loge("first segment mmap size %d failed %s\n", n, strerror(errno));
        munmap(whole, 2 * n);
        return NULL;
    }

    base2 = mmap(whole + n, n, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0);
    if (base2 == MAP_FAILED) {
        munmap(whole, 2 * n);
        mpp_loge("second segment mmap size %d failed %s\n", n, strerror(errno));
        return NULL;
    }

    ring_dbg_flow("mmap twice fd %d size %d whole %p base %p:%p\n",
                  fd, n, whole, base, base2);

    return whole;
}

static void munmap_twice(void *ptr, rk_s32 n)
{
    if (ptr) {
        ring_dbg_flow("mummap twice size %d ptr %p:%p\n", n, ptr, (char *)ptr + n);
        munmap(ptr, 2 * n);
    }
}

static void mpp_ring_srv_init()
{
    if (pool_ring)
        return;

    mpp_env_get_u32("mpp_ring_debug", &mpp_ring_debug, 0);

    page_sz = sysconf(_SC_PAGESIZE);
    pool_ring = mpp_mem_pool_init_f(MODULE_TAG, sizeof(MppRingImpl));

    ring_dbg_init("page size %d\n", page_sz);
}

static void mpp_ring_srv_deinit()
{
    if (pool_ring) {
        mpp_mem_pool_deinit_f(pool_ring);
        pool_ring = NULL;
    }
}

MPP_SINGLETON(MPP_SGLN_RING, mpp_ring, mpp_ring_srv_init, mpp_ring_srv_deinit)

rk_s32 mpp_ring_get(MppRing *ring, rk_s32 size, const char *name)
{
    MppRingImpl *impl = NULL;
    void *ptr = NULL;
    rk_s32 fd = -1;
    rk_s32 size_align;

    mpp_env_get_u32("mpp_ring_debug", &mpp_ring_debug, 0);

    if (NULL == ring || size <= 0) {
        mpp_loge_f("invalid input ring %p size %d\n", ring, size);
        return rk_nok;
    }

    *ring = NULL;

    if (NULL == name)
        name = module_name;

    size_align = MPP_ALIGN(size, page_sz);

    do {
        fd = memfd_create(name, MFD_CLOEXEC);
        if (fd < 0) {
            mpp_loge_f("failed to create memfd ret %d %s\n", fd, strerror(errno));
            break;
        }

        if (ftruncate(fd, size_align) < 0) {
            mpp_loge_f("failed to truncate memfd ret %d %s\n", fd, strerror(errno));
            break;
        }

        ptr = mmap_twice(fd, size_align);
        if (ptr == NULL)
            break;

        impl = (MppRingImpl*)mpp_mem_pool_get_f(pool_ring);
        if (NULL == impl) {
            mpp_loge_f("failed to get ring impl from pool\n");
            break;
        }

        impl->ptr = ptr;
        impl->fd = fd;
        impl->size = size;
        impl->size_align = size_align;
        *ring = impl;

        ring_dbg_flow("mpp_ring_get size %d -> aligned %d fd %d ptr %p\n",
                      size, size_align, fd, ptr);

        return rk_ok;
    } while (0);

    munmap_twice(ptr, size_align);

    if (fd >= 0)
        close(fd);

    return rk_nok;
}

rk_s32 mpp_ring_put(MppRing ring)
{
    MppRingImpl *impl = (MppRingImpl *)ring;

    if (NULL == impl)
        return rk_nok;

    ring_dbg_flow("mpp_ring_put aligned %d fd %d ptr %p\n",
                  impl->size_align, impl->fd, impl->ptr);

    munmap_twice(impl->ptr, impl->size_align);

    if (impl->fd >= 0)
        close(impl->fd);

    mpp_mem_pool_put_f(pool_ring, impl);

    return rk_ok;
}

void *mpp_ring_get_ptr(MppRing ring)
{
    MppRingImpl *impl = (MppRingImpl *)ring;

    return (impl != NULL) ? impl->ptr : NULL;
}

void *mpp_ring_resize(MppRing ring, rk_s32 new_size)
{
    MppRingImpl *impl = (MppRingImpl *)ring;
    rk_s32 new_aligned;
    void *new_ptr;

    if (!impl || new_size <= 0)
        return NULL;

    new_aligned = MPP_ALIGN(new_size, page_sz);

    if (new_aligned == impl->size)
        return impl->ptr;

    munmap_twice(impl->ptr, impl->size);

    if (ftruncate(impl->fd, new_aligned) < 0) {
        mpp_loge_f("resize ftruncate failed %s\n", strerror(errno));

        /* rollback original buffer */
        impl->ptr = mmap_twice(impl->fd, impl->size);
        return impl->ptr;
    }

    new_ptr = mmap_twice(impl->fd, new_aligned);
    if (new_ptr == MAP_FAILED) {
        mpp_loge("resize mmap_twice failed %s\n", strerror(errno));

        /* rollback original buffer */
        if (ftruncate(impl->fd, impl->size_align) < 0) {
            mpp_loge("resize rollback ftruncate failed %s\n", strerror(errno));
        } else {
            new_ptr = mmap_twice(impl->fd, impl->size);
            new_aligned = impl->size;
        }
    }

    ring_dbg_flow("mpp_ring_reisze %p:%d -> %p:%d\n",
                  impl->ptr, impl->size, new_ptr, new_aligned);

    impl->ptr = new_ptr;
    impl->size = new_size;
    impl->size_align = new_aligned;

    return new_ptr;
}
