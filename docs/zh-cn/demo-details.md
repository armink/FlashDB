# 演示工程详细说明

## 功能代码说明

演示工程主要演示了 FlashDB 从初始化，到示例运行的过程。

### main.c

演示工程中，`main.c` 中的 `main 函数` 为入口函数，该函数分为两段，分别初始化了一个 KVDB 和 TSDB 对象，然后执行与之对应的示例函数，大致内容如下：

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

#### 设置加锁与解锁

初始化 KVDB 及 TSDB 前通常需要通过 `control` 函数设置 `加锁回调` 与 `解锁回调` ：

- KVDB：
  - fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_LOCK, lock);
  - fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_UNLOCK, unlock);
- TSDB：
  - fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_SET_LOCK, lock);
  - fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_SET_UNLOCK, unlock);

对于裸机平台，加锁与解锁回调通常设置为关中断与开中断函数。而 RTOS 平台一般使用 mutex 互斥锁或 二值信号量 的 take 及 release 动作作为加锁与解锁的方式。

#### 模拟时间戳

对于 TSDB，正常项目中的时间戳应当通过 RTC 或者网络时钟方式来获取，但是这里为了增强演示工程的通用性，使用 `fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_GET_LAST_TIME, &counts);` 获取 TSDB 上次使用的时间戳，存入 `counts` 。每次使用 `get_time` 获取当前时间时，会对 `counts` 进行加一处理，模拟时间往前走的动作，避免重复。

所以，使用这种方法模拟的时间戳没有实时时间的含义，只是为了让每条记录插入的时间戳不重复。

#### 示例

本文主要讲解初始化相关代码功能，更多关于示例函数的详细讲解，请阅读对应的示例详解。

| 示例函数                       | 说明               | 详解                                      |
| ------------------------------ | ------------------ | ----------------------------------------- |
| kvdb_basic_sample(&kvdb)       | KVDB 基础示例      | [点击查看](zh-cn/sample-kvdb-basic)       |
| kvdb_type_string_sample(&kvdb) | 字符串类型 KV 示例 | [点击查看](zh-cn/sample-kvdb-type-string) |
| kvdb_type_blob_sample(&kvdb)   | blob 类型 KV 示例  | [点击查看](zh-cn/sample-kvdb-type-blob)   |
| tsdb_sample(&tsdb)             | TSDB 基础示例      | [点击查看](zh-cn/sample-tsdb-basic)       |

## 首次运行日志

下面将对日志进行分段讲解。

### FAL 初始化

FAL 初始化时会打印 Flash 设备信息及分区表信息。

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

### KVDB 初始化

KVDB 每次初始化时会检查扇区头部信息是否正确（扇区头部存储了一些属性信息），如果不正确，将会自动格式化该扇区。

Flash 首次使用时，通常需要格式化处理，所以首次初始化时的日志会含有格式化的信息。格式化成功后，后续每次初始化无需再次格式化。

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

### TSDB 初始化

与 KVDB 类似，TSDB 首次初始化时，也会自动执行格式化动作。

```
[FlashDB][tsl][log] Sector (0x00000000) header info is incorrect.
[FlashDB][tsl][log] All sector format finished.
[FlashDB][tsl][log] (src/fdb_tsdb.c:759) TSDB (log) oldest sectors is 0x00000000, current using sector is 0x00000000.
```

### 运行示例

日志详解，请阅读 [示例文档](zh-cn/sample-kvdb-basic)

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
省略 TSDB 初始化日志……
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

## 二次运行日志

这里主要看下 KVDB 与 TSDB 初始化的日志，对比首次初始化可以发现，第二次启动时的初始化日志会相对少很多，主要少了格式化的过程。

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
省略示例运行日志……
[FlashDB][tsl][log] (D:/Program/STM32/FlashDB/src/fdb_tsdb.c:759) TSDB (log) oldest sectors is 0x00000000, current using sector is 0x00000000.
省略示例运行日志……
```

