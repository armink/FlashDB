# blob type KV example

This example mainly demonstrates the related functions of blob KV. Blob KV is a more commonly used type, and its value is a binary type without type restrictions. Functionally, blob KV is also compatible with string KV. In the use of API, blob KV has a set of independent APIs, which can quickly realize the storage of various types of KV to KVDB, such as basic types, arrays and structures.

## Code description

The sample code is located in `samples/kvdb_type_blob.c`, and a KV named `"temp"` is used to store the temperature value, respectively demonstrating the whole process of blob KV from `create->read->modify->delete`. The general content is as follows:

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

## Running log

It can be seen from the log:

- First create a KV named `"temp"` and give the initial value 36℃
- Read the current value of `"temp"` KV and find that it is the same as the initial value
- Modify the value of `"temp"` KV to 38℃
- Finally delete `"temp"` KV

```
[FlashDB][sample][kvdb][blob] ==================== kvdb_type_blob_sample ====================
[FlashDB][sample][kvdb][blob] create the 'temp' blob KV, value is: 36
[FlashDB][sample][kvdb][blob] get the 'temp' value is: 36
[FlashDB][sample][kvdb][blob] set 'temp' value to 38
[FlashDB][sample][kvdb][blob] delete the 'temp' finish
[FlashDB][sample][kvdb][blob] ===========================================================
```

