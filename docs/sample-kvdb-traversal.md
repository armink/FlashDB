# Traverse all KV

This example demonstrates that if you traverse all KVs in KVDB, users can add their own processing actions when traversing KVs.

## Code description

In the following sample code, first initialize the iterator of KVDB, and then use the iterator API to traverse all KVs of KVDB one by one.

The traversed KV object contains some attributes of KV, including: key name, value saved addr, value length, etc. The user can read it out through `fdb_blob_read` in cooperation with `fdb_kv_to_blob`, and do some business processing of his own.

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

