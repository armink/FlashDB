# API 说明

## Blob

### 构造 blob 对象

构造 blob 对象的过程，其内部是对 blob 结构体初始化赋值的过程，将传入的参数写入 blob 结构体中，并进行返回 

`fdb_blob_t fdb_blob_make(fdb_blob_t blob, const void *value_buf, size_t buf_len)`

| 参数      | 描述                   |
| --------- | ---------------------- |
| blob      | blob 初始对象          |
| value_buf | 存放数据的缓冲区       |
| buf_len   | 缓冲区的大小           |
| 返回      | 创建完成后的 blob 对象 |

### 读取 blob 数据

通过 KVDB 和 TSDB 的 API 可以返回 blob 对象，此时返回的 blob 对象里存放了 blob 数据的存储地址。该 API 可以将数据库里存放的 blob 数据读取出来，并存放至 `blob->buf` 。

`size_t fdb_blob_read(fdb_db_t db, fdb_blob_t blob)`

| 参数 | 描述                       |
| ---- | -------------------------- |
| db   | 数据库对象                 |
| blob | blob 对象                  |
| 返回 | 实际读取到的 blob 数据长度 |

## KVDB

### 初始化 KVDB

`fdb_err_t fdb_kvdb_init(fdb_kvdb_t db, const char *name, const char *part_name, struct fdb_default_kv *default_kv, void *user_data)`

| 参数       | 描述                                                     |
| ---------- | -------------------------------------------------------- |
| db         | 数据库对象                                               |
| name       | 数据库名称                                               |
| part_name  | 使用 FAL 分区表中的哪一个分区                            |
| default_kv | 默认 KV 集合，第一次初始化时，将会把默认 KV 写入数据库中 |
| user_data  | 用户自定义数据，没有时传入 NULL                          |
| 返回       | 错误码                                                   |

### 控制 KVDB

通过命令控制字，用户可以对数据库进行一些控制操作

`void fdb_kvdb_control(fdb_kvdb_t db, int cmd, void *arg)`

| 参数 | 描述       |
| ---- | ---------- |
| db   | 数据库对象 |
| cmd  | 命令控制字 |
| arg  | 控制的参数 |
| 返回 | 错误码     |

支持的命令控制字如下：

```C
#define FDB_KVDB_CTRL_SET_SEC_SIZE     0x0             /**< 设置扇区大小 */
#define FDB_KVDB_CTRL_GET_SEC_SIZE     0x1             /**< 获取扇区大小 */
#define FDB_KVDB_CTRL_SET_LOCK         0x2             /**< 设置加锁函数 */
#define FDB_KVDB_CTRL_SET_UNLOCK       0x3             /**< 设置解锁函数 */
```

#### 扇区大小与块大小

FlashDB 内部存储结构由 N 个扇区组成，每次格式化时是以扇区作为最小单位。而一个扇区通常是 Flash 块大小的 N 倍，比如： Nor Flash 的块大小一般为 4096。

默认 KVDB 会使用 1倍 的块大小作为扇区大小，即：4096。此时，该 KVDB 无法存入超过 4096 长度的 KV 。如果想要存入比如：10K 长度的 KV ，可以通过 control 函数，设置扇区大小为 12K，或者更大大小即可。

### 设置 KV

使用此方法可以实现对 KV 的增加和修改功能。

- **增加** ：当 KVDB 中不存在该名称的 KV 时，则会执行新增操作；
- **修改** ：入参中的 KV 名称在当前 KVDB 中存在，则把该 KV 值修改为入参中的值；

通过 KV 的名字来获取其对应的值。支持两种接口

#### 设置 blob 类型KV

`fdb_err_t fdb_kv_set_blob(fdb_kvdb_t db, const char *key, fdb_blob_t blob)`

| 参数 | 描述                         |
| ---- | ---------------------------- |
| db   | 数据库对象                   |
| key  | KV 的名称                    |
| blob | blob  对象，做为 KV 的 value |
| 返回 | 错误码                       |

示例：

```C
struct fdb_blob blob;
int temp_data = 36;
/* 通过 fdb_blob_make 构造 blob 对象，作为 "temp" KV 的 value */
fdb_kv_set_blob(kvdb, "temp", fdb_blob_make(&blob, &temp_data, sizeof(temp_data)));
```

#### 设置 string 类型KV

`fdb_err_t fdb_kv_set(fdb_kvdb_t db, const char *key, const char *value)`

| 参数  | 描述        |
| ----- | ----------- |
| db    | 数据库对象  |
| key   | KV 的名称   |
| value | KV 的 value |
| 返回  | 错误码      |

### 获取 KV

#### 获取 blob 类型 KV

`size_t fdb_kv_get_blob(fdb_kvdb_t db, const char *key, fdb_blob_t blob)`

| 参数 | 描述                                  |
| ---- | ------------------------------------- |
| db   | 数据库对象                            |
| key  | KV 的名称                             |
| blob | 通过 blob 对象，返回 KV 的 blob value |
| 返回 | 错误码                                |

示例：

```C
struct fdb_blob blob;
int temp_data = 0;
/* 构造 blob 对象，用于存储 get 回来的 "temp" KV 的 value 数据 */
fdb_kv_get_blob(kvdb, "temp", fdb_blob_make(&blob, &temp_data, sizeof(temp_data)));
/* 如果有需要，可以检查 blob.saved.len 是否大于 0 ，确保 get 成功 */
if (blob.saved.len > 0) {
    FDB_INFO("get the 'temp' value is: %d\n", temp_data);
}
```

#### 获取 KV 对象

与 `fdb_kv_get_blob` API 不同，该 API 在 get 过程中并不会执行 value 数据的读取动作。返回的 KV 对象中存放了读取回来的 KV 属性，该 API 适用于 value 长度不确定，或 value 长度过长，需要分段读取的场景。

`fdb_kv_t fdb_kv_get_obj(fdb_kvdb_t db, const char *key, fdb_kv_t kv)`

| 参数 | 描述                                                         |
| ---- | ------------------------------------------------------------ |
| db   | 数据库对象                                                   |
| key  | KV 的名称                                                    |
| kv   | 通过 KV 对象，返回 KV 的属性，可以再用 `fdb_kv_to_blob` 转换为 blob 对象，再进行数据读取 |
| 返回 | 错误码                                                       |

#### 获取字符串类型 KV

**注意** ：

- 该函数不允许连续使用，使用时需使用 strdup 包裹，确保每次返回回来的字符串内存空间独立；
- 该函数不支持可重入，返回的值位于函数内部缓冲区，出于安全考虑，请加锁保护。

`char *fdb_kv_get(fdb_kvdb_t db, const char *key)`

| 参数 | 描述                                |
| ---- | ----------------------------------- |
| db   | 数据库对象                          |
| key  | KV 的名称                           |
| 返回 | !=NULL: KV 的 value；NULL: 获取失败 |

### 删除 KV

`fdb_err_t fdb_kv_del(fdb_kvdb_t db, const char *key)`

| 参数 | 描述       |
| ---- | ---------- |
| db   | 数据库对象 |
| key  | KV 的名称  |
| 返回 | 错误码     |

### 重置 KVDB

将 KVDB 中的 KV 重置为初始时的默认值

`fdb_err_t fdb_kv_set_default(fdb_kvdb_t db)`

| 参数 | 描述       |
| ---- | ---------- |
| db   | 数据库对象 |
| 返回 | 错误码     |

### 打印 KVDB 中的 KV 信息

`void fdb_kv_print(fdb_kvdb_t db)`

| 参数 | 描述       |
| ---- | ---------- |
| db   | 数据库对象 |
| 返回 | 错误码     |

### KV 对象转换为 blob 对象

`fdb_blob_t fdb_kv_to_blob(fdb_kv_t kv, fdb_blob_t blob)`

| 参数 | 描述               |
| ---- | ------------------ |
| kv   | 待转换的 KV 对象   |
| blob | 转换前的 blob 对象 |
| 返回 | 转换后的 blob 对象 |

### 初始化 KV 迭代器
`fdb_kv_iterator_t fdb_kv_iterator_init(fdb_kv_iterator_t itr)`

| 参数 | 描述                 |
| ---- | -------------------- |
| itr  | 待初始化的迭代器对象 |
| 返回 | 初始化后的迭代器对象 |

### 迭代 KV

使用该迭代器 API，可以遍历整个 KVDB 中的所有 KV。

> **注意**：使用前请先初始化迭代器

`bool fdb_kv_iterate(fdb_kvdb_t db, fdb_kv_iterator_t itr)`

[点击查看示例](zh-cn/sample-kvdb-traversal.md)

## TSDB

### 初始化 TSDB

`fdb_err_t fdb_tsdb_init(fdb_tsdb_t db, const char *name, const char *part_name, fdb_get_time get_time, size_t max_len, void *user_data)`

| 参数      | 描述                            |
| --------- | ------------------------------- |
| db        | 数据库对象                      |
| name      | 数据库名称                      |
| part_name | 使用 FAL 分区表中的哪一个分区   |
| get_time  | 获取当前时间戳的函数            |
| max_len   | 每条 TSL 的最大长度             |
| user_data | 用户自定义数据，没有时传入 NULL |
| 返回      | 错误码                          |

### 控制 TSDB

通过命令控制字，用户可以对数据库进行一些控制操作

`void fdb_tsdb_control(fdb_tsdb_t db, int cmd, void *arg)`

| 参数 | 描述       |
| ---- | ---------- |
| db   | 数据库对象 |
| cmd  | 命令控制字 |
| arg  | 控制的参数 |
| 返回 | 错误码     |

支持的命令控制字如下：

```C
#define FDB_TSDB_CTRL_SET_SEC_SIZE     0x0             /**< 设置扇区大小 */
#define FDB_TSDB_CTRL_GET_SEC_SIZE     0x1             /**< 获取扇区大小 */
#define FDB_TSDB_CTRL_SET_LOCK         0x2             /**< 设置加锁函数 */
#define FDB_TSDB_CTRL_SET_UNLOCK       0x3             /**< 设置解锁函数 */
#define FDB_TSDB_CTRL_SET_ROLLOVER     0x4             /**< 设置是否滚动写入，默认滚动。设置非滚动时，如果数据库写满，无法再追加写入 */
#define FDB_TSDB_CTRL_GET_ROLLOVER     0x5             /**< 获取是否滚动写入 */
#define FDB_TSDB_CTRL_GET_LAST_TIME    0x6             /**< 获取上次追加 TSL 时的时间戳 */
```

### 追加 TSL

对于 TSDB ，新增 TSL 的过程，就是往 TSDB 末尾追加新 TSL 的过程

`fdb_err_t fdb_tsl_append(fdb_tsdb_t db, fdb_blob_t blob)`

| 参数 | 描述                        |
| ---- | --------------------------- |
| db   | 数据库对象                  |
| blob | blob  对象，做为 TSL 的数据 |
| 返回 | 错误码                      |

### 迭代 TSL

遍历整个 TSDB 并执行迭代回调

`void fdb_tsl_iter(fdb_tsdb_t db, fdb_tsl_cb cb, void *cb_arg)`

| 参数   | 描述                                    |
| ------ | --------------------------------------- |
| db     | 数据库对象                              |
| cb     | 回调函数，每次遍历到 TSL 时会执行该回调 |
| cb_arg | 回调函数的参数                          |
| 返回   | 错误码                                  |

### 按时间段迭代 TSL

按时间段范围，遍历整个 TSDB 并执行迭代回调

`void fdb_tsl_iter_by_time(fdb_tsdb_t db, fdb_time_t from, fdb_time_t to, fdb_tsl_cb cb, void *cb_arg)`

| 参数   | 描述                                    |
| ------ | --------------------------------------- |
| db     | 数据库对象                              |
| from   | 开始时间戳                              |
| to     | 结束时间戳                              |
| cb     | 回调函数，每次遍历到 TSL 时会执行该回调 |
| cb_arg | 回调函数的参数                          |
| 返回   | 错误码                                  |

### 查询 TSL 的数量

按照传入的时间段，查询符合状态的 TSL 数量
`size_t fdb_tsl_query_count(fdb_tsdb_t db, fdb_time_t from, fdb_time_t to, fdb_tsl_status_t status)`

| 参数   | 描述           |
| ------ | -------------- |
| db     | 数据库对象     |
| from   | 开始时间戳     |
| to     | 结束时间戳     |
| status | TSL 的状态条件 |
| 返回   | 数量           |

### 设置 TSL 状态

`fdb_err_t fdb_tsl_set_status(fdb_tsdb_t db, fdb_tsl_t tsl, fdb_tsl_status_t status)`

| 参数   | 描述         |
| ------ | ------------ |
| db     | 数据库对象   |
| tsl    | TSL 对象     |
| status | TSL 的新状态 |
| 返回   | 错误码       |

### 清空 TSDB

`void fdb_tsl_clean(fdb_tsdb_t db)`

| 参数 | 描述       |
| ---- | ---------- |
| db   | 数据库对象 |
| 返回 | 错误码     |

### TSL 对象转换为 blob 对象

`fdb_blob_t fdb_tsl_to_blob(fdb_tsl_t tsl, fdb_blob_t blob)`

| 参数 | 描述               |
| ---- | ------------------ |
| tsl  | 待转换的 TSL 对象  |
| blob | 转换前的 blob 对象 |
| 返回 | 转换后的 blob 对象 |