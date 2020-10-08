# Demo project details

## Function code description

The demo project mainly demonstrates the process of FlashDB from initialization to running the example.

### main.c

In the demo project, the `main function` in `main.c` is the entry function. This function is divided into two sections, which initialize a KVDB and TSDB objects respectively, and then execute the corresponding example functions. The general content is as follows:

```C
#ifdef FDB_USING_KVDB
    { /* KVDB Sample */
        struct fdb_default_kv default_kv;

        default_kv.kvs = default_kv_table;
        default_kv.num = sizeof(default_kv_table) / sizeof(default_kv_table[0]);
        /* set the lock and unlock function if you want */
        fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_LOCK, lock);
        fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_UNLOCK, unlock);
        /* Key-Value database initialization
         *
         *       &kvdb: database object
         *       "env": database name
         * "fdb_kvdb1": The flash partition name base on FAL. Please make sure it's in FAL partition table.
         *              Please change to YOUR partition name.
         * &default_kv: The default KV nodes. It will auto add to KVDB when first initialize successfully.
         *        NULL: The user data if you need, now is empty.
         */
        result = fdb_kvdb_init(&kvdb, "env", "fdb_kvdb1", &default_kv, NULL);

        if (result != FDB_NO_ERR) {
            return -1;
        }

        /* run basic KV samples */
        kvdb_basic_sample(&kvdb);
        /* run string KV samples */
        kvdb_type_string_sample(&kvdb);
        /* run blob KV samples */
        kvdb_type_blob_sample(&kvdb);
    }
#endif /* FDB_USING_KVDB */

#ifdef FDB_USING_TSDB
    { /* TSDB Sample */
        /* set the lock and unlock function if you want */
        fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_SET_LOCK, lock);
        fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_SET_UNLOCK, unlock);
        /* Time series database initialization
         *
         *       &tsdb: database object
         *       "log": database name
         * "fdb_tsdb1": The flash partition name base on FAL. Please make sure it's in FAL partition table.
         *              Please change to YOUR partition name.
         *    get_time: The get current timestamp function.
         *         128: maximum length of each log
         *        NULL: The user data if you need, now is empty.
         */
        result = fdb_tsdb_init(&tsdb, "log", "fdb_tsdb1", get_time, 128, NULL);
        /* read last saved time for simulated timestamp */
        fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_GET_LAST_TIME, &counts);

        if (result != FDB_NO_ERR) {
            return -1;
        }

        /* run TSDB sample */
        tsdb_sample(&tsdb);
    }
#endif /* FDB_USING_TSDB */
```

#### Set lock and unlock

Before initializing KVDB and TSDB, it is usually necessary to set the `lock callback` and `unlock callback` through the `control` function:

- KVDB：
  - fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_LOCK, lock);
  - fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_UNLOCK, unlock);
- TSDB：
  - fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_SET_LOCK, lock);
  - fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_SET_UNLOCK, unlock);

For bare metal platforms, the lock and unlock callbacks are usually set to close interrupt and open interrupt functions. The RTOS platform generally uses mutex mutex lock or binary semaphore take and release actions as locking and unlocking methods.

####  timestamp simulation

For TSDB, the timestamp in the normal project should be obtained through RTC or network clock, but here to enhance the versatility of the demonstration project, use `fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_GET_LAST_TIME, &counts);` to get the last used timestamp of TSDB, Deposit in `counts`. Every time you use `get_time` to get the current time, it will add one to `counts` to simulate the action of moving forward in time and avoid repetition.

Therefore, the time stamp simulated by this method does not have the meaning of real-time time, just to make the time stamp inserted in each record not repeated.

#### Example

This article mainly explains initialization related code functions. For more detailed explanations of example functions, please read the corresponding detailed explanations of the examples.

| sample function                | description       | detailed                                      |
| ------------------------------ | ------------------ | ----------------------------------------- |
| kvdb_basic_sample(&kvdb) | KVDB basic example | [click to view](zh-cn/sample-kvdb-basic) |
| kvdb_type_string_sample(&kvdb) | KV example of string type | [click to view](zh-cn/sample-kvdb-type-string) |
| kvdb_type_blob_sample(&kvdb) | Blob type KV sample | [click to view](zh-cn/sample-kvdb-type-blob) |
| tsdb_sample(&tsdb) | TSDB basic example | [click to view](zh-cn/sample-tsdb-basic) |

## First run log

The log will be explained in sections below.

### FAL initialization

Flash device information and partition table information will be printed when FAL is initialized.

```
[D/FAL] (fal_flash_init:65) Flash device |             stm32_onchip | addr: 0x08000000 | len: 0x00040000 | blk_size: 0x00000800 |initialized finish.
[I/FAL] ==================== FAL partition table ====================
[I/FAL] | name      | flash_dev    |   offset   |    length  |
[I/FAL] -------------------------------------------------------------
[I/FAL] | fdb_tsdb1 | stm32_onchip | 0x0001a000 | 0x00002000 |
[I/FAL] | fdb_kvdb1 | stm32_onchip | 0x0001c000 | 0x00004000 |
[I/FAL] =============================================================
[I/FAL] Flash Abstraction Layer (V0.5.0) initialize success.
```

### KVDB initialization

Each time KVDB is initialized, it will check whether the sector header information is correct (some attribute information is stored in the sector header). If it is not correct, the sector will be formatted automatically.

When the Flash is used for the first time, it usually needs to be formatted, so the log at the first initialization will contain formatted information. After the format is successful, there is no need to format it again for each subsequent initialization.

```
[FlashDB][kv][env] (src/fdb_kvdb.c:1599) KVDB in partition fdb_kvdb1, size is 16384 bytes.
[FlashDB][kv][env] Sector header info is incorrect. Auto format this sector (0x00000000).
[FlashDB][kv][env] Sector header info is incorrect. Auto format this sector (0x00000800).
[FlashDB][kv][env] Sector header info is incorrect. Auto format this sector (0x00001000).
[FlashDB][kv][env] Sector header info is incorrect. Auto format this sector (0x00001800).
[FlashDB][kv][env] Sector header info is incorrect. Auto format this sector (0x00002000).
[FlashDB][kv][env] Sector header info is incorrect. Auto format this sector (0x00002800).
[FlashDB][kv][env] Sector header info is incorrect. Auto format this sector (0x00003000).
[FlashDB][kv][env] Sector header info is incorrect. Auto format this sector (0x00003800).
[FlashDB][kv][env] All sector header is incorrect. Set it to default.
[FlashDB] FlashDB V1.0.0 beta is initialize success.
[FlashDB] You can get the latest version on https://github.com/armink/FlashDB .
```

### TSDB initialization

Similar to KVDB, when TSDB is initialized for the first time, it will automatically perform formatting.

```
[FlashDB][tsl][log] Sector (0x00000000) header info is incorrect.
[FlashDB][tsl][log] All sector format finished.
[FlashDB][tsl][log] (src/fdb_tsdb.c:759) TSDB (log) oldest sectors is 0x00000000, current using sector is 0x00000000.
```

### Run the example

For detailed log explanation, please read [Sample Document](sample-kvdb-basic)
```
[FlashDB][sample][kvdb][basic] ==================== kvdb_basic_sample ====================
[FlashDB][sample][kvdb][basic] get the 'boot_count' value is 0
[FlashDB][sample][kvdb][basic] set the 'boot_count' value to 1
[FlashDB][sample][kvdb][basic] ===========================================================
[FlashDB][sample][kvdb][string] ==================== kvdb_type_string_sample ====================
[FlashDB][sample][kvdb][string] create the 'temp' string KV, value is: 36C
[FlashDB][sample][kvdb][string] get the 'temp' value is: 36C
[FlashDB][sample][kvdb][string] set 'temp' value to 38C
[FlashDB][sample][kvdb][string] delete the 'temp' finish
[FlashDB][sample][kvdb][string] ===========================================================
[FlashDB][sample][kvdb][blob] ==================== kvdb_type_blob_sample ====================
[FlashDB][sample][kvdb][blob] create the 'temp' blob KV, value is: 36
[FlashDB][sample][kvdb][blob] get the 'temp' value is: 36
[FlashDB][sample][kvdb][blob] set 'temp' value to 38
[FlashDB][sample][kvdb][blob] delete the 'temp' finish
[FlashDB][sample][kvdb][blob] ===========================================================
Omit TSDB initialization log...
[FlashDB][sample][tsdb] ==================== tsdb_sample ====================
[FlashDB][sample][tsdb] append the new status.temp (36) and status.humi (85)
[FlashDB][sample][tsdb] append the new status.temp (38) and status.humi (90)
[FlashDB][sample][tsdb] [query_cb] queried a TSL: time: 1, temp: 36, humi: 85
[FlashDB][sample][tsdb] [query_cb] queried a TSL: time: 2, temp: 38, humi: 90
[FlashDB][sample][tsdb] [query_by_time_cb] queried a TSL: time: 1, temp: 36, humi: 85
[FlashDB][sample][tsdb] [query_by_time_cb] queried a TSL: time: 2, temp: 38, humi: 90
[FlashDB][sample][tsdb] query count is: 2
[FlashDB][sample][tsdb] set the TSL (time 1) status from 2 to 3
[FlashDB][sample][tsdb] set the TSL (time 2) status from 2 to 3
[FlashDB][sample][tsdb] ===========================================================
```

## Secondary run log

Here we mainly look at the initialization logs of KVDB and TSDB. Compared with the first initialization, it can be found that the initialization logs at the second startup will be relatively small, and the formatting process is mainly missing.

```
[D/FAL] (fal_flash_init:65) Flash device |             stm32_onchip | addr: 0x08000000 | len: 0x00040000 | blk_size: 0x00000800 |initialized finish.
[I/FAL] ==================== FAL partition table ====================
[I/FAL] | name      | flash_dev    |   offset   |    length  |
[I/FAL] -------------------------------------------------------------
[I/FAL] | fdb_tsdb1 | stm32_onchip | 0x0001a000 | 0x00002000 |
[I/FAL] | fdb_kvdb1 | stm32_onchip | 0x0001c000 | 0x00004000 |
[I/FAL] =============================================================
[I/FAL] Flash Abstraction Layer (V0.5.0) initialize success.
[FlashDB][kv][env] (D:/Program/STM32/FlashDB/src/fdb_kvdb.c:1599) KVDB in partition fdb_kvdb1, size is 16384 bytes.
[FlashDB] FlashDB V1.0.0 beta is initialize success.
[FlashDB] You can get the latest version on https://github.com/armink/FlashDB .
Omit the sample run log...
[FlashDB][tsl][log] (D:/Program/STM32/FlashDB/src/fdb_tsdb.c:759) TSDB (log) oldest sectors is 0x00000000, current using sector is 0x00000000.
Omit the sample run log...
```

