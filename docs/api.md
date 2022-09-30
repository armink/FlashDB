# API description

## Blob

### Construct blob object

The process of constructing a blob object, its internal is the process of initializing and assigning values ​​to the blob structure, writing the incoming parameters into the blob structure, and returning it

`fdb_blob_t fdb_blob_make(fdb_blob_t blob, const void *value_buf, size_t buf_len)`

| Parameters | Description |
| --------- | ---------------------- |
| blob | blob initial object |
| value_buf | Buffer for storing data |
| buf_len | The size of the buffer |
| Return | The blob object after creation |

### Read blob data

Through the API of KVDB and TSDB, the blob object can be returned, and the storage address of the blob data is stored in the returned blob object. This API can read the blob data stored in the database and store it in `blob->buf`.

`size_t fdb_blob_read(fdb_db_t db, fdb_blob_t blob)`

| Parameters | Description |
| ---- | -------------------------- |
| db | Database Objects |
| blob | blob object |
| Return | Length of blob data actually read |

## KVDB

### Initialize KVDB

`fdb_err_t fdb_kvdb_init(fdb_kvdb_t db, const char *name, const char *path, struct fdb_default_kv *default_kv, void *user_data)`

| Parameters | Description |
| ---------- | -------------------------------------- ------------------ |
| db | Database Objects |
| name | Database name |
| path | FAL mode: the partition name in the partition table, file mode: the path where the database is saved |
| default_kv | The default KV collection, when the first initialization, the default KV will be written to the database |
| user_data | User-defined data, NULL if not available |
| Return | Error Code |

### Control KVDB

Through the command control word, the user can perform some control operations on the database

`void fdb_kvdb_control(fdb_kvdb_t db, int cmd, void *arg)`

| Parameters | Description |
| ---- | ---------- |
| db | Database Objects |
| cmd | Command control word |
| arg | Controlled parameters |
| Return | Error Code |

The supported command control words are as follows:

```C
#define FDB_KVDB_CTRL_SET_SEC_SIZE     0x00             /**< set sector size control command, this change MUST before database initialization */
#define FDB_KVDB_CTRL_GET_SEC_SIZE     0x01             /**< get sector size control command */
#define FDB_KVDB_CTRL_SET_LOCK         0x02             /**< set lock function control command */
#define FDB_KVDB_CTRL_SET_UNLOCK       0x03             /**< set unlock function control command */
#define FDB_KVDB_CTRL_SET_FILE_MODE    0x09             /**< set file mode control command, this change MUST before database initialization */
#define FDB_KVDB_CTRL_SET_MAX_SIZE     0x0A             /**< set database max size in file mode control command, this change MUST before database initialization */
#define FDB_KVDB_CTRL_SET_NOT_FORMAT   0x0B             /**< set database NOT format mode control command, this change MUST before database initialization */
```

#### Sector size and block size

The internal storage structure of FlashDB is composed of N sectors, and each formatting takes sector as the smallest unit. A sector is usually N times the size of the Flash block. For example, the block size of Nor Flash is generally 4096.

By default, KVDB will use 1 times the block size as the sector size, that is, 4096. At this time, the KVDB cannot store a KV longer than 4096. If you want to save, for example, a KV with a length of 10K, you can use the control function to set the sector size to 12K or larger.

### Deinitialize KVDB

`fdb_err_t fdb_kvdb_deinit(fdb_kvdb_t db)`

| Parameters | Description      |
| ---------- | ---------------- |
| db         | Database Objects |

### Set KV

This method can be used to increase and modify KV.

- **Add**: When there is no KV with this name in KVDB, the new operation will be performed;

- **Modify**: The KV name in the input parameter exists in the current KVDB, then the KV value is modified to the value in the input parameter;

  > In the internal implementation of KVDB, modifying KV will delete the old KV first, and then add a new KV, so the remaining capacity of the database will become smaller after the modification

Get the corresponding value by KV's name. Support two interfaces

#### Set blob type KV

`fdb_err_t fdb_kv_set_blob(fdb_kvdb_t db, const char *key, fdb_blob_t blob)`

| Parameters | Description |
| ---- | ---------------------------- |
| db | Database Objects |
| key | KV name |
| blob | blob object, as the value of KV |
| Return | Error Code |

Example:

```C
struct fdb_blob blob;
int temp_data = 36;
/* Construct a blob object by fdb_blob_make as the value of "temp" KV */
fdb_kv_set_blob(kvdb, "temp", fdb_blob_make(&blob, &temp_data, sizeof(temp_data)));
```

#### Set string type KV

`fdb_err_t fdb_kv_set(fdb_kvdb_t db, const char *key, const char *value)`

| Parameters | Description |
| ----- | ----------- |
| db | Database Objects |
| key | KV name |
| value | KV value |
| Return | Error Code |

### Get KV

#### Get blob type KV

`size_t fdb_kv_get_blob(fdb_kvdb_t db, const char *key, fdb_blob_t blob)`

| Parameters | Description |
| ---- | ------------------------------------- |
| db | Database Objects |
| key | KV name |
| blob | Return the blob value of KV through the blob object |
| Return | Error Code |

Example:

```C
struct fdb_blob blob;
int temp_data = 0;
/* Construct a blob object to store the "temp" KV value data returned by get */
fdb_kv_get_blob(kvdb, "temp", fdb_blob_make(&blob, &temp_data, sizeof(temp_data)));
/* If necessary, you can check whether blob.saved.len is greater than 0 to ensure that the get succeeds */
if (blob.saved.len > 0) {
    FDB_INFO("get the 'temp' value is: %d\n", temp_data);
}
```

#### Get KV object

Unlike the `fdb_kv_get_blob` API, this API does not execute the reading of value data during the get process. The returned KV object stores the read KV attributes. This API is suitable for scenarios where the length of the value is uncertain, or the length of the value is too long, and it needs to be read in segments.

`fdb_kv_t fdb_kv_get_obj(fdb_kvdb_t db, const char *key, fdb_kv_t kv)`

| Parameters | Description |
| ---- | -------------------------------------------- ---------------- |
| db | Database Objects |
| key | KV name |
| kv | Through the KV object, return the attributes of the KV, and then use `fdb_kv_to_blob` to convert to a blob object, and then read the data |
| Return | Error Code |

#### Get string type KV

**Note**:

- This function is not allowed to be used continuously, and `strdup` should be used when using it to ensure that the memory space of the string returned each time is independent;
- This function does not support reentrancy. The returned value is located in the internal buffer of the function. For security reasons, please lock it.

`char *fdb_kv_get(fdb_kvdb_t db, const char *key)`

| Parameters | Description |
| ---- | ----------------------------------- |
| db | Database Objects |
| key | KV name |
| Return | !=NULL: KV value; NULL: Get failed |

### Delete KV

> In the internal implementation of KVDB, deleting KV will not be completely removed from KVDB, but marked for deletion, so the remaining capacity of the database will not change after deletion.

`fdb_err_t fdb_kv_del(fdb_kvdb_t db, const char *key)`

| Parameters | Description |
| ---- | ---------- |
| db | Database Objects |
| key | KV name |
| Return | Error Code |

### Reset KVDB

Reset the KV in KVDB to the **first initial** default value

`fdb_err_t fdb_kv_set_default(fdb_kvdb_t db)`

| Parameters | Description |
| ---- | ---------- |
| db | Database Objects |
| Return | Error Code |

### Print KV information in KVDB

`void fdb_kv_print(fdb_kvdb_t db)`

| Parameters | Description |
| ---- | ---------- |
| db | Database Objects |
| Return | Error Code |

### Convert KV objects to blob objects

`fdb_blob_t fdb_kv_to_blob(fdb_kv_t kv, fdb_blob_t blob)`

| Parameters | Description |
| ---- | ------------------ |
| kv | KV object to be converted |
| blob | blob object before conversion |
| Return | Converted blob object |

### Initialize KV iterator
`fdb_kv_iterator_t fdb_kv_iterator_init(fdb_kv_iterator_t itr)`

| Parameters | Description |
| ---- | -------------------- |
| itr | Iterator object to be initialized |
| Return | Iterator object after initialization |

### Iteration KV

Using this iterator API, all KVs in the entire KVDB can be traversed.

> **Note**: Please initialize the iterator before use

`bool fdb_kv_iterate(fdb_kvdb_t db, fdb_kv_iterator_t itr)`

[Click to view sample](sample-kvdb-traversal.md)

## TSDB

### Initialize TSDB

`fdb_err_t fdb_tsdb_init(fdb_tsdb_t db, const char *name, const char *path, fdb_get_time get_time, size_t max_len, void *user_data)`

| Parameters | Description |
| --------- | ------------------------------- |
| db | Database Objects |
| name | Database name |
| path | FAL mode: the partition name in the partition table, file mode: the path where the database is saved |
| get_time | Function to get the current timestamp |
| max_len | Maximum length of each TSL |
| user_data | User-defined data, NULL if not available |
| Return | Error Code |

### Control TSDB

Through the command control word, the user can perform some control operations on the database

`void fdb_tsdb_control(fdb_tsdb_t db, int cmd, void *arg)`

| Parameters | Description |
| ---- | ---------- |
| db | Database Objects |
| cmd | Command control word |
| arg | Controlled parameters |
| Return | Error Code |

The supported command control words are as follows:

```C
#define FDB_TSDB_CTRL_SET_SEC_SIZE     0x00             /**< set sector size control command, this change MUST before database initialization */
#define FDB_TSDB_CTRL_GET_SEC_SIZE     0x01             /**< get sector size control command */
#define FDB_TSDB_CTRL_SET_LOCK         0x02             /**< set lock function control command */
#define FDB_TSDB_CTRL_SET_UNLOCK       0x03             /**< set unlock function control command */
#define FDB_TSDB_CTRL_SET_ROLLOVER     0x04             /**< set rollover control command, this change MUST after database initialization */
#define FDB_TSDB_CTRL_GET_ROLLOVER     0x05             /**< get rollover control command */
#define FDB_TSDB_CTRL_GET_LAST_TIME    0x06             /**< get last save time control command */
#define FDB_TSDB_CTRL_SET_FILE_MODE    0x09             /**< set file mode control command, this change MUST before database initialization */
#define FDB_TSDB_CTRL_SET_MAX_SIZE     0x0A             /**< set database max size in file mode control command, this change MUST before database initialization */
#define FDB_TSDB_CTRL_SET_NOT_FORMAT   0x0B             /**< set database NOT formatable mode control command, this change MUST before database initialization */
```

### Deinitialize TSDB

`fdb_err_t fdb_tsdb_deinit(fdb_tsdb_t db)`

| Parameters | Description      |
| ---------- | ---------------- |
| db         | Database Objects |

### Append TSL

For TSDB, the process of adding TSL is the process of appending a new TSL to the end of TSDB

`fdb_err_t fdb_tsl_append(fdb_tsdb_t db, fdb_blob_t blob)`

| Parameters | Description |
| ---- | --------------------------- |
| db | Database Objects |
| blob | blob object, as TSL data |
| Return | Error Code |

### Iterative TSL

Traverse the entire TSDB and execute iterative callbacks

`void fdb_tsl_iter(fdb_tsdb_t db, fdb_tsl_cb cb, void *cb_arg)`

| Parameters | Description |
| ------ | --------------------------------------- |
| db | Database Objects |
| cb | Callback function, which will be executed every time the TSL is traversed |
| cb_arg | Parameters of the callback function |
| Return | Error Code |

### Reverse iterative TSL

Reverse traverse the entire TSDB and execute iterative callbacks

`void fdb_tsl_iter_reverse(fdb_tsdb_t db, fdb_tsl_cb cb, void *arg)`

| Parameters | Description                                                  |
| ---------- | ------------------------------------------------------------ |
| db         | Database Objects                                             |
| cb         | Callback function, which will be executed every time the TSL is traversed |
| cb_arg     | Parameters of the callback function                          |
| Return     | Error Code                                                   |

### Iterate TSL by time period

According to the time range, traverse the entire TSDB and execute iterative callbacks

`void fdb_tsl_iter_by_time(fdb_tsdb_t db, fdb_time_t from, fdb_time_t to, fdb_tsl_cb cb, void *cb_arg)`

| Parameters | Description |
| ------ | --------------------------------------- |
| db | Database Objects |
| from | Start timestamp. It will be a reverse iterator when ending timestamp less than starting timestamp |
| to | End timestamp |
| cb | Callback function, which will be executed every time the TSL is traversed |
| cb_arg | Parameters of the callback function |
| Return | Error Code |

### Query the number of TSL

According to the incoming time period, query the number of TSLs that meet the state
`size_t fdb_tsl_query_count(fdb_tsdb_t db, fdb_time_t from, fdb_time_t to, fdb_tsl_status_t status)`

| Parameters | Description |
| ------ | -------------- |
| db | Database Objects |
| from | Start timestamp |
| to | End timestamp |
| status | TSL status conditions |
| Return | Quantity |

### Set TSL status

For TSL status, please refer to `enum fdb_tsl_status`. TSL status MUST be set in order. [click to view sample](sample-tsdb-basic.md)

`fdb_err_t fdb_tsl_set_status(fdb_tsdb_t db, fdb_tsl_t tsl, fdb_tsl_status_t status)`

| Parameters | Description |
| ------ | ------------ |
| db | Database Objects |
| tsl | TSL Object |
| status | TSL's new status |
| Return | Error Code |

### Clear TSDB

`void fdb_tsl_clean(fdb_tsdb_t db)`

| Parameters | Description |
| ---- | ---------- |
| db | Database Objects |
| Return | Error Code |

### Convert TSL objects to blob objects

`fdb_blob_t fdb_tsl_to_blob(fdb_tsl_t tsl, fdb_blob_t blob)`

| Parameters | Description |
| ---- | ------------------ |
| tsl | TSL object to be converted |
| blob | blob object before conversion |
| Return | Converted blob object |