# Configuration

When using FlashDB, you can configure its functions through `fdb_cfg.h`. The file template is located in the `inc` directory, or you can copy it in the specific demo project. Let's introduce the configuration details in detail below

## FDB_USING_KVDB

Enable KVDB feature

### FDB_KV_AUTO_UPDATE

Enable KV automatic upgrade function. After this function is enabled, `fdb_kvdb.ver_num` stores the version of the current database. If the version changes, it will automatically trigger an upgrade action and update the new default KV collection to the current database.

## FDB_USING_TSDB

Enable TSDB feature

### FDB_TSDB_FIXED_BLOB_SIZE

Fixes the blob length of every TSL to the given number of bytes. It is undefined by default. When it is defined, the TSL index no longer stores the `log_len` and `log_addr` fields (8 bytes in total): the log address is calculated from the position of the TSL inside the sector, so the Flash overhead of every TSL is reduced (8 bytes per TSL with the usual 1/8/32/64 bit write granularity, after the `FDB_WRITE_GRAN` alignment). It fits fixed-size records, for example a single float temperature value plus a timestamp.

- With this macro defined, `fdb_tsl_append` and `fdb_tsl_append_with_ts` only accept a blob whose length is **exactly** this value; any other length returns `FDB_WRITE_ERR`.
- The layout on Flash is incompatible with the variable-size blob layout and with any other value of this macro, so the database MUST be cleared (erased) when this macro is defined, undefined or changed.
- `fdb_tsl_max_blob_count()` also uses this value as the length of a TSL when it estimates the capacity.

## FDB_USING_FAL_MODE

Enable FAL mode, partition in FAL is used to store the database. In this mode, FlashDB directly operates Flash, so performance is better.

## FDB_USING_FILE_POSIX_MODE

Using POSIX file mode, you need to provide an open/read/write/close related file access interface.

## FDB_USING_FILE_LIBC_MODE

Using the file mode of the C standard library, you need to provide a fopen/fread/fwrte/fclose related file access interface.

> FDB_USING_FILE_LIBC_MODE and FDB_USING_FILE_POSIX_MODE mode can ONLY be one. Compared to FAL mode, the storage location, size and quantity of the database in the file mode are not limited.

## FDB_WRITE_GRAN

Flash write granularity, the unit is bit. Currently supports

- 1: nor flash
- 8: stm32f2/f4 on-chip Flash
- 32: stm32f1 on-chip Flash
- 64: stm32f7 on-chip Flash
- 128: stm32h5 on-chip Flash
- 256: stm32h7 on-chip Flash

If multiple Flash specifications are used in the database, for example: both nor flash and stm32f4 on-chip Flash, the maximum value is used as the configuration item, namely: 8 bit

> This option applies to both KVDB and TSDB. KV value lengths and TSL blob lengths are aligned up to this granularity when they are written, so the Flash space actually used can be slightly larger than the data itself.

## FDB_USING_READ_CACHE

Enable the transient sector read cache used while KV data is loading. The option takes a value: `0` (the default) disables it, `1` enables it.

FlashDB does not persist the length of the used area of a sector, so a KV search reads the sector chunk by chunk up to its end to locate the KV chain and the end of the used area. For a sector which holds only a little data that means thousands of storage reads, and it is what dominates the loading time of a small database. With the cache enabled, the sector is copied into RAM once while the scan runs and every later read is served from RAM; a write or erase of the sector drops the copy, so stale data is never returned.

The cache is borrowed, not owned: it is valid inside the two phases below and is released as soon as each of them ends, so a single KV read or write at run time never holds a sector sized buffer in RAM:

- the `fdb_kvdb_init()` phase, released when the initialization ends;
- the `fdb_kv_iterate()` phase, released when the iteration ends.

If the RAM allocation fails, the cache stays inactive and every read falls back to reading the storage directly, so the behaviour is exactly the same as with the feature disabled.

> The cache uses `malloc`/`free` by default. Define both `FDB_READ_CACHE_MALLOC` and `FDB_READ_CACHE_FREE` to use the heap of your platform instead.

### FDB_READ_CACHE_WINDOW

The number of bytes the read cache copies into RAM at once, which is also the size of the RAM window. The default is `0`.

- `0`: the window is the whole sector. This is the value to use in a normal build, because the scan the cache exists for walks to the end of the sector anyway, so copying the sector in one piece is the fewest accesses it can take.
- A value greater than `0` and smaller than the sector size copies the sector in windows of that size, to cap the RAM usage; a read outside the window currently held copies the matching window again.
- A value which is not smaller than the sector size has no effect.

## FDB_BIG_ENDIAN

MCU small-endian configuration, when the default is not configured, the system automatically uses the small-endian configuration

## FDB_PRINT(...)

The print function macro defines the configuration. When it is not configured by default, using `printf` as the print log is the output function. Users can also customize new print function macro definitions, for example:

```C
#define FDB_PRINT(...) my_printf(__VA_ARGS__)
```

## FDB_DEBUG_ENABLE

Enable debugging information output. When this configuration is closed, the system will not output logs for debugging.