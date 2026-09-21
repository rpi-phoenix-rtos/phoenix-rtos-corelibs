/*
 * Phoenix-RTOS
 *
 * Cache library
 *
 * Copyright 2022 Phoenix Systems
 * Author: Malgorzata Wrobel
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _CACHE_H
#define _CACHE_H

#include <stdio.h>
#include <stdint.h>


/* Cache write policies */
#define LIBCACHE_WRITE_BACK    0
#define LIBCACHE_WRITE_THROUGH 1


typedef struct cachectx_s cachectx_t;


typedef struct cache_devCtx_s cache_devCtx_t; /* Device driver context should be defined by flash driver */


typedef ssize_t (*cache_readCb_t)(uint64_t offset, void *buffer, size_t count, cache_devCtx_t *ctx);


typedef ssize_t (*cache_writeCb_t)(uint64_t offset, const void *buffer, size_t count, cache_devCtx_t *ctx);


/* Cached source memory interface */
typedef struct {
	cache_readCb_t readCb;
	cache_writeCb_t writeCb;
	cache_devCtx_t *ctx; /* Device driver context */
} cache_ops_t;


cachectx_t *cache_init(size_t srcMemSize, size_t lineSize, size_t linesCnt, const cache_ops_t *ops);


/* Sets the write-back granularity: a flush writes only the modified part of a
 * line, rounded out to `gran` bytes. `gran` must be a power of two and no
 * larger than the line size, and the write callback must accept an offset and
 * length that are multiples of it. Optional -- a cache that never calls this
 * writes whole lines, as it always did. Returns 0, or -EINVAL. */
int cache_setFlushGranularity(cachectx_t *cache, size_t gran);

int cache_deinit(cachectx_t *cache);


ssize_t cache_read(cachectx_t *cache, uint64_t addr, void *buffer, size_t count);


ssize_t cache_write(cachectx_t *cache, uint64_t addr, const void *buffer, size_t count, int policy);


int cache_flush(cachectx_t *cache, const uint64_t begAddr, const uint64_t endAddr);


int cache_invalidate(cachectx_t *cache, const uint64_t begAddr, const uint64_t endAddr);


int cache_clean(cachectx_t *cache, const uint64_t begAddr, const uint64_t endAddr);


#endif
