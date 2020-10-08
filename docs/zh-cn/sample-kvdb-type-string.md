# 字符串类型 KV 示例

该示例主要演示了字符串 KV 的相关功能，字符串 KV 作为一个特殊的 KV 类型，其 Key 与 Value 均为字符串，常被用于参数存储、命令存储等可读性要求较高的场景。

## 代码说明

示例代码位于 `samples/kvdb_type_string.c` ，使用一个名为 `"temp"` 的 KV 来存储温度值，分别演示了字符串 KV 从 `创建->读取->修改->删除` 的全过程。大致内容如下：

```C
void kvdb_type_string_sample(fdb_kvdb_t kvdb)
{
    FDB_INFO("==================== kvdb_type_string_sample ====================\n");
    { /* CREATE new Key-Value */
        char temp_data[10] = "36C";

        /* It will create new KV node when "temp" KV not in database. */
        fdb_kv_set(kvdb, "temp", temp_data);
        FDB_INFO("create the 'temp' string KV, value is: %s\n", temp_data);
    }

    { /* GET the KV value */
        char *return_value, temp_data[10] = { 0 };

        /* Get the "temp" KV value.
         * NOTE: The return value saved in fdb_kv_get's buffer. Please copy away as soon as possible.
         */
        return_value = fdb_kv_get(kvdb, "temp");
        /* the return value is NULL when get the value failed */
        if (return_value != NULL) {
            strncpy(temp_data, return_value, sizeof(temp_data));
            FDB_INFO("get the 'temp' value is: %s\n", temp_data);
        }
    }

    { /* CHANGE the KV value */
        char temp_data[10] = "38C";

        /* change the "temp" KV's value to "38.1" */
        fdb_kv_set(kvdb, "temp", temp_data);
        FDB_INFO("set 'temp' value to %s\n", temp_data);
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
[FlashDB][sample][kvdb][string] ==================== kvdb_type_string_sample ====================
[FlashDB][sample][kvdb][string] create the 'temp' string KV, value is: 36C
[FlashDB][sample][kvdb][string] get the 'temp' value is: 36C
[FlashDB][sample][kvdb][string] set 'temp' value to 38C
[FlashDB][sample][kvdb][string] delete the 'temp' finish
[FlashDB][sample][kvdb][string] ===========================================================
```

