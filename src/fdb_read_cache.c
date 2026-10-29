/*
 * Copyright (c) 2020, Armink, <armink.ztl@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Transient sector read cache.
 *
 * FlashDB keeps no persisted record of how much of a sector is actually used, so
 * every KV search which has to find the end of the used area - or the next KV
 * node - reads the sector chunk by chunk up to its end. For a sector which is
 * mostly erased that is thousands of reads for a few hundred bytes of data, and
 * it is the dominant cost of loading a small database.
 *
 * This file removes that cost while a database is loading. The layer which knows
 * that such a scan is about to start (the KVDB, see read_sector_info) announces the
 * sector it reads, and from then on every read of that sector is served from RAM,
 * until the sector is modified (which drops the copy) or the cache is released,
 * whichever comes first.
 *
 * The cache lives inside two bounded phases, and is given back at the end of each of
 * them, so it is never kept beyond the work which needs it:
 * - the initialization (fdb_kvdb_init, the window is returned by its __exit), and
 * - an application driven iteration (fdb_kv_iterate, the window is returned when the
 *   iteration ends).
 * A single key read or write at run time never allocates it: without such a phase the
 * storage is read directly.
 *
 * Properties:
 * - The memory is taken from the heap by the first read which goes through the cache
 *   and returned when the phase which opened it ends, so the resident size is one
 *   window (FDB_READ_CACHE_WINDOW, the whole sector by default) for the duration of
 *   that phase. If the allocation fails the cache stays inactive and every read falls
 *   back to the plain storage path, so a build with the cache enabled behaves exactly
 *   like a build without it when no memory is available.
 * - It is a pure read cache: it never holds data beyond what the storage has, and
 *   any modification of the cached sector drops the copy.
 * - It sits above the storage access layer, so it is independent of the backend
 *   (native flash, file system, ...).
 */

#include <flashdb.h>
#include <fdb_low_lvl.h>
#include <inttypes.h>
#include <string.h>

#if FDB_USING_READ_CACHE

#define FDB_LOG_TAG "[cache]"

/* a platform may enable the cache from its build system only, so both keys need a
 * fallback here and not just in the configuration template */
#ifndef FDB_READ_CACHE_WINDOW
#define FDB_READ_CACHE_WINDOW         0
#endif

/* The allocator used for the cache memory. A platform overrides the two together,
 * they have to match, e.g. both to its own heap. */
#if !defined(FDB_READ_CACHE_MALLOC) || !defined(FDB_READ_CACHE_FREE)
#include <stdlib.h>
#endif
#ifndef FDB_READ_CACHE_MALLOC
#define FDB_READ_CACHE_MALLOC(size)   malloc(size)
#endif
#ifndef FDB_READ_CACHE_FREE
#define FDB_READ_CACHE_FREE(ptr)      free(ptr)
#endif

/* the RAM copy of one sector plus its book keeping */
struct fdb_read_cache {
    uint8_t *buf;                                /**< window buffer, points right after this struct */
    uint32_t window;                             /**< window size in bytes */
    uint32_t sec;                                /**< the sector the cache is enabled for */
    uint32_t blk;                                /**< window address currently held by buf */
    bool filling;                                /**< a window copy is running, do not re-enter */
};

static struct fdb_read_cache *read_cache_of(fdb_db_t db)
{
    return (struct fdb_read_cache *)db->read_cache;
}

/* the window is the whole sector unless a smaller one is configured */
static uint32_t read_cache_window(fdb_db_t db)
{
#if (FDB_READ_CACHE_WINDOW > 0)
    if (FDB_READ_CACHE_WINDOW < db->sec_size) {
        return FDB_READ_CACHE_WINDOW;
    }
#endif
    return db->sec_size;
}

fdb_err_t _fdb_read_cache_begin(fdb_db_t db, uint32_t sec_addr)
{
    struct fdb_read_cache *cache = read_cache_of(db);

    /* the cache lives inside the initialization and inside an application driven
     * iteration only: once such a phase is given back a single key operation never
     * allocates it again (the next phase opens in fdb_kvdb_init/fdb_kv_iterate) */
    if (!db->read_cache_load) {
        return FDB_NO_ERR;
    }

    /* Remember the sector the scan is reading. The window itself is allocated by the
     * first read which needs it (see _fdb_read_cache_read), so a scan which reads
     * nothing through the cache allocates nothing. */
    db->read_cache_sec = sec_addr;

    if (cache != NULL && cache->sec != sec_addr) {
        /* the window holds data of another sector, it is not usable anymore */
        cache->sec = sec_addr;
        cache->blk = FDB_FAILED_ADDR;
    }

    return FDB_NO_ERR;
}

void _fdb_read_cache_release(fdb_db_t db)
{
    struct fdb_read_cache *cache = read_cache_of(db);

    if (cache != NULL) {
        FDB_READ_CACHE_FREE(cache);
        db->read_cache = NULL;
    }
    /* releasing a loaded database ends the loading phase: no read may allocate a
     * window again before the next fdb_kvdb_init() */
    db->read_cache_load = false;
    db->read_cache_sec = FDB_FAILED_ADDR;
}

/* Copy the window which covers addr into RAM and return the address inside it. */
static bool read_cache_window_get(fdb_db_t db, struct fdb_read_cache *cache, uint32_t addr,
                                  const uint8_t **data)
{
    uint32_t blk = FDB_ALIGN_DOWN(addr, cache->window);
    uint32_t sec_end = cache->sec + db->sec_size;
    size_t size = cache->window;

    if (blk + size > sec_end) {
        size = sec_end - blk;
    }
    if (cache->blk != blk) {
        cache->filling = true;
        if (_fdb_flash_read(db, blk, cache->buf, size) != FDB_NO_ERR) {
            cache->filling = false;
            cache->blk = FDB_FAILED_ADDR;
            return false;
        }
        cache->filling = false;
        cache->blk = blk;
    }
    *data = cache->buf + (addr - blk);
    return true;
}

bool _fdb_read_cache_read(fdb_db_t db, uint32_t addr, void *buf, size_t size)
{
    struct fdb_read_cache *cache = read_cache_of(db);
    uint32_t sec = FDB_ALIGN_DOWN(addr, db->sec_size);
    uint32_t window = read_cache_window(db);
    uint32_t end;
    const uint8_t *data;

    /* only the sector a scan announced may use the cache, every other read (a sector
     * header, a read of the writing path) stays on the plain storage path */
    if (db->read_cache_sec != sec) {
        return false;
    }
    /* a window copy is running, serve nothing until it is complete */
    if (cache != NULL && cache->filling) {
        return false;
    }
    /* only serve reads which stay inside one window, a read which cannot be served
     * must not allocate the cache either. The copy of the last window of a sector
     * ends with the sector, so the guard must not reach past it. */
    end = FDB_ALIGN_DOWN(addr, window) + window;
    if (end > sec + db->sec_size) {
        end = sec + db->sec_size;
    }
    if (addr + size > end) {
        return false;
    }
    if (cache == NULL) {
        /* a single allocation holds the book keeping and the window itself */
        cache = (struct fdb_read_cache *)FDB_READ_CACHE_MALLOC(sizeof(struct fdb_read_cache) + window);
        if (cache == NULL) {
            /* no memory for the cache, the plain storage path keeps working */
            FDB_INFO("Warning: no memory for a %" PRIu32 " bytes read cache, reading the storage directly.\n",
                    window);
            /* stop trying for this scan, one failure is enough */
            db->read_cache_sec = FDB_FAILED_ADDR;
            return false;
        }
        memset(cache, 0, sizeof(struct fdb_read_cache));
        cache->buf = (uint8_t *)(cache + 1);
        cache->window = window;
        cache->blk = FDB_FAILED_ADDR;
        cache->sec = sec;
        db->read_cache = cache;
    }
    if (!read_cache_window_get(db, cache, addr, &data)) {
        return false;
    }

    memcpy(buf, data, size);
    return true;
}

void _fdb_read_cache_invalidate(fdb_db_t db, uint32_t addr)
{
    struct fdb_read_cache *cache = read_cache_of(db);

    /* the copy is stale as soon as the sector is modified */
    if (cache != NULL && cache->sec == FDB_ALIGN_DOWN(addr, db->sec_size)) {
        cache->blk = FDB_FAILED_ADDR;
    }
}

#endif /* FDB_USING_READ_CACHE */
