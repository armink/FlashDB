# KVDB basic example

This example mainly demonstrates the basic functions of KVDB, including KV acquisition and setting modification functions.

## Code description

The sample code is located in `samples/kvdb_basic_sample.c`, the default KV table is defined in `main.c`, and there is `boot_count` KV in it. The KV is used to record the current system startup times. Each time the power is turned off and restarted, the KV will be automatically incremented by one and saved to KVDB. The general content is as follows:

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

## First run log

The current `boot_count` is 0, after adding one, it is stored in the database.

```
[FlashDB][sample][kvdb][basic] ==================== kvdb_basic_sample ====================
[FlashDB][sample][kvdb][basic] get the 'boot_count' value is 0
[FlashDB][sample][kvdb][basic] set the 'boot_count' value to 1
[FlashDB][sample][kvdb][basic] ===========================================================
```

## Secondary run log

The current `boot_count` is 1, indicating that the last save is effective, add one to it and save it for the next visit.

```
[FlashDB][sample][kvdb][basic] ==================== kvdb_basic_sample ====================
[FlashDB][sample][kvdb][basic] get the 'boot_count' value is 1
[FlashDB][sample][kvdb][basic] set the 'boot_count' value to 2
[FlashDB][sample][kvdb][basic] ===========================================================
```