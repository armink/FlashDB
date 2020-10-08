# String type KV example

This example mainly demonstrates the related functions of the string KV. As a special KV type, the Key and Value of the string KV are both strings, which are often used in scenarios with high readability requirements such as parameter storage and command storage.

## Code description

The sample code is located in `samples/kvdb_type_string.c`, and a KV named `"temp"` is used to store the temperature value, which respectively demonstrates the whole process of the string KV from `create->read->modify->delete` . The general content is as follows:

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

## Run log

It can be seen from the log:

- First create a KV named `"temp"` and give the initial value 36℃
- Read the current value of `"temp"` KV and find that it is the same as the initial value
- Modify the value of `"temp"` KV to 38℃
- Finally delete `"temp"` KV

```
[FlashDB][sample][kvdb][string] ==================== kvdb_type_string_sample ====================
[FlashDB][sample][kvdb][string] create the 'temp' string KV, value is: 36C
[FlashDB][sample][kvdb][string] get the 'temp' value is: 36C
[FlashDB][sample][kvdb][string] set 'temp' value to 38C
[FlashDB][sample][kvdb][string] delete the 'temp' finish
[FlashDB][sample][kvdb][string] ===========================================================
```

