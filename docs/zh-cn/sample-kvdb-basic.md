# KVDB 基础示例

该示例主要演示了 KVDB 的基础功能，包括 KV 的获取及设置修改功能。

## 代码说明

示例代码位于 `samples/kvdb_basic_sample.c` ，在 `main.c` 有定义默认的 KV 集合表，在里面有 `boot_count` KV。通过该 KV 来记录当前系统的启动次数。每次掉电再启动时，该 KV 会自动加一，并保存至 KVDB 中。大致内容如下：

```C
void kvdb_basic_sample(fdb_kvdb_t kvdb)
{
    struct fdb_blob blob;
    int boot_count = 0;

    FDB_INFO("==================== kvdb_basic_sample ====================\n");

    { /* GET the KV value */
        /* get the "boot_count" KV value */
        fdb_kv_get_blob(kvdb, "boot_count", fdb_blob_make(&blob, &boot_count, sizeof(boot_count)));
        /* the blob.saved.len is more than 0 when get the value successful */
        if (blob.saved.len > 0) {
            FDB_INFO("get the 'boot_count' value is %d\n", boot_count);
        } else {
            FDB_INFO("get the 'boot_count' failed\n");
        }
    }

    { /* CHANGE the KV value */
        /* increase the boot count */
        boot_count ++;
        /* change the "boot_count" KV's value */
        fdb_kv_set_blob(kvdb, "boot_count", fdb_blob_make(&blob, &boot_count, sizeof(boot_count)));
        FDB_INFO("set the 'boot_count' value to %d\n", boot_count);
    }

    FDB_INFO("===========================================================\n");
}
```

## 首次运行日志

当前 `boot_count` 为 0 ，对其加一后，存入数据库。

```
[FlashDB][sample][kvdb][basic] ==================== kvdb_basic_sample ====================
[FlashDB][sample][kvdb][basic] get the 'boot_count' value is 0
[FlashDB][sample][kvdb][basic] set the 'boot_count' value to 1
[FlashDB][sample][kvdb][basic] ===========================================================
```

## 二次运行日志

当前 `boot_count` 为 1 ，说明上次的保存生效了，再对其加一后保存，供下次访问使用。

```
[FlashDB][sample][kvdb][basic] ==================== kvdb_basic_sample ====================
[FlashDB][sample][kvdb][basic] get the 'boot_count' value is 1
[FlashDB][sample][kvdb][basic] set the 'boot_count' value to 2
[FlashDB][sample][kvdb][basic] ===========================================================
```