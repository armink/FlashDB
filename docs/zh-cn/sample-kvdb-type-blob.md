# blob 类型 KV 示例

该示例主要演示了blob KV 的相关功能，blob KV 是一个比较常用类型，其 value 是一个没有类型限制的二进制类型。在功能上，blob KV 也兼容字符串 KV 。在 API 的使用上， blob KV 拥有一套独立的 API ，可以很快速的实现各种类型 KV 到 KVDB 中的存储，比如：基本类型，数组以及结构体等。

## 代码说明

示例代码位于 `samples/kvdb_type_blob.c` ，使用一个名为 `"temp"` 的 KV 来存储温度值，分别演示了 blob KV 从 `创建->读取->修改->删除` 的全过程。大致内容如下：

```C
void kvdb_type_blob_sample(fdb_kvdb_t kvdb)
{
    struct fdb_blob blob;

    FDB_INFO("==================== kvdb_type_blob_sample ====================\n");

    { /* CREATE new Key-Value */
        int temp_data = 36;

        /* It will create new KV node when "temp" KV not in database.
         * fdb_blob_make: It's a blob make function, and it will return the blob when make finish.
         */
        fdb_kv_set_blob(kvdb, "temp", fdb_blob_make(&blob, &temp_data, sizeof(temp_data)));
        FDB_INFO("create the 'temp' blob KV, value is: %d\n", temp_data);
    }

    { /* GET the KV value */
        int temp_data = 0;

        /* get the "temp" KV value */
        fdb_kv_get_blob(kvdb, "temp", fdb_blob_make(&blob, &temp_data, sizeof(temp_data)));
        /* the blob.saved.len is more than 0 when get the value successful */
        if (blob.saved.len > 0) {
            FDB_INFO("get the 'temp' value is: %d\n", temp_data);
        }
    }

    { /* CHANGE the KV value */
        int temp_data = 38;

        /* change the "temp" KV's value to 38 */
        fdb_kv_set_blob(kvdb, "temp", fdb_blob_make(&blob, &temp_data, sizeof(temp_data)));
        FDB_INFO("set 'temp' value to %d\n", temp_data);
    }

    { /* DELETE the KV by name */
        fdb_kv_del(kvdb, "temp");
        FDB_INFO("delete the 'temp' finish\n");
    }

    FDB_INFO("===========================================================\n");
}
```

## 运行日志

通过日志可以看出：

- 首先创建了一个 KV 名为 `"temp"` ，并给予初值 36℃
- 读取 `"temp"` KV 当前的值，发现与初值相同
- 修改 `"temp"` KV 的值为  38℃
- 最后删除 `"temp"` KV

```
[FlashDB][sample][kvdb][blob] ==================== kvdb_type_blob_sample ====================
[FlashDB][sample][kvdb][blob] create the 'temp' blob KV, value is: 36
[FlashDB][sample][kvdb][blob] get the 'temp' value is: 36
[FlashDB][sample][kvdb][blob] set 'temp' value to 38
[FlashDB][sample][kvdb][blob] delete the 'temp' finish
[FlashDB][sample][kvdb][blob] ===========================================================
```

