# 遍历所有 KV 

本示例演示了如果遍历 KVDB 中的所有 KV ，用户可以在遍历 KV 时增加自己的处理动作。

## 代码说明

下面的示例代码中，首先初始化了 KVDB 的迭代器，然后使用迭代器 API ，将 KVDB 的所有 KV 逐一遍历出来。

遍历出来的 KV 对象含有 KV 的一些属性，包括：key name, value saved addr, value length 等，用户通过 `fdb_blob_read` 配合 `fdb_kv_to_blob` 读取出来，做一些自己的业务处理。

```C
void kvdb_tarversal_sample(fdb_kvdb_t kvdb)
{
    struct fdb_kv_iterator iterator;
    fdb_kv_t cur_kv;
    struct fdb_blob blob;
    size_t data_size;
    uint8_t *data_buf;

    fdb_kv_iterator_init(&iterator);

    while (fdb_kv_iterate(kvdb, &iterator)) {
        cur_kv = &(iterator.curr_kv);
        data_size = (size_t) cur_kv->value_len;
        data_buf = (uint8_t *) malloc(data_size);
        if (data_buf == NULL) {
            FDB_INFO("Error: malloc failed.\n");
            break;
        }
        fdb_blob_read((fdb_db_t) kvdb, fdb_kv_to_blob(cur_kv, fdb_blob_make(&blob, data_buf, data_size)));
        /*
         * balabala do what ever you like with blob...
         */
        free(data_buf);
    }
}
```

