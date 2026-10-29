# 配置说明

FlashDB 的使用时，可以通过 fdb_cfg.h 对其进行功能配置，该文件模板位于 `inc` 目录下，也可以去具体的 demo 工程里拷贝。下面详细介绍一下配置详情

## FDB_USING_KVDB

使能 KVDB 功能

### FDB_KV_AUTO_UPDATE

使能 KV 自动升级功能。该功能使能后， `fdb_kvdb.ver_num` 存储了当前数据库的版本，如果版本发生变化时，会自动触发升级动作，将更新新的默认 KV 集合至当前数据库中。

## FDB_USING_TSDB

使能 TSDB 功能

### FDB_TSDB_FIXED_BLOB_SIZE

将每条 TSL 的 blob 长度固定为指定字节数，默认不定义。定义后，TSL 索引中不再保存 `log_len` 与 `log_addr` 两个字段（共 8 字节），log 地址由该 TSL 在扇区中的位置直接计算得到，因此可以降低每条 TSL 的 Flash 开销（按 `FDB_WRITE_GRAN` 对齐后，常见的 1/8/32/64 bit 写粒度下每条 TSL 可节省 8 字节），适合记录定长数据（例如单个 float 温度值加时间戳）的场景。

- 定义该宏后，`fdb_tsl_append` 与 `fdb_tsl_append_with_ts` 只接受长度 **严格等于** 该值的 blob，长度不匹配时返回 `FDB_WRITE_ERR`；
- 该配置与变长 blob 的存储格式不兼容，不同的取值之间也不兼容，因此在定义、取消定义或修改该值之前，需要先清空（擦除）数据库；
- `fdb_tsl_max_blob_count()` 的容量估算同样以该值作为每条 TSL 的长度。

## FDB_USING_FAL_MODE

使能 FAL 模式，FAL 里的分区用于存储数据库。该模式下，FlashDB 直接操作 Flash，所以性能较好

## FDB_USING_FILE_POSIX_MODE

使用 POSIX 的文件模式，需要系统提供 open/read/write/close 相关文件访问接口。

## FDB_USING_FILE_LIBC_MODE

使用 C 标准库的文件模式，需要系统提供 fopen/fread/fwrite/fclose 相关文件访问接口。

> FDB_USING_FILE_LIBC_MODE 与 FDB_USING_FILE_POSIX_MODE 模式只能二选一。相比 FAL 模式，文件模式下数据库的存储位置、大小及数量没有限制。

## FDB_WRITE_GRAN

Flash 写粒度，单位为 bit。目前支持

- 1: nor flash
- 8: stm32f2/f4 片上 Flash 
- 32: stm32f1 片上 Flash 
- 64: stm32f7 片上 Flash
- 128: stm32h5 片上 Flash
- 256: stm32h7 片上 Flash

如果数据库中使用了多种 Flash 规格，例如：既有 nor flash，也有 stm32f4 片上 Flash ，此时取最大值作为配置项，即：8 bit

> 该配置对 KVDB 与 TSDB 均有效。KV 的 value 长度、TSL 的 blob 长度都会按该粒度向上对齐后写入，所以实际占用的 Flash 空间可能略大于数据本身长度。

## FDB_USING_READ_CACHE

使能 KV 加载阶段的临时扇区读缓存。该配置项带数值，默认 `0`（关闭），定义为 `1` 时开启。

FlashDB 不会把每个扇区的已使用区域长度记录到 Flash 中，所以 KV 查找时需要从扇区头部逐块读取到扇区末尾，才能定位 KV 链表及已使用区域的结束位置。对于只存了少量数据的扇区，这会带来上千次存储读取，是小容量数据库加载耗时的主要部分。开启该缓存后，扫描期间该扇区会被一次性读入 RAM，后续读取直接命中缓存；扇区在写入或擦除前会使缓存失效，因此不会读到过期数据。

缓存是临时借用的，只在下面两个阶段内有效，阶段结束后立即释放，所以运行期单次 KV 读写不会占用扇区大小的 RAM：

- `fdb_kvdb_init()` 初始化阶段，初始化结束时释放；
- `fdb_kv_iterate()` 迭代阶段，迭代结束时释放。

若申请 RAM 失败，缓存保持不生效，所有读取回退到直接访问存储的路径，行为与关闭该功能时完全一致。

> 该缓存默认使用 `malloc`/`free`，可以同时定义 `FDB_READ_CACHE_MALLOC` 与 `FDB_READ_CACHE_FREE` 两个宏，替换为自己平台的堆实现。

### FDB_READ_CACHE_WINDOW

读缓存一次性复制到 RAM 的字节数，同时也是 RAM 窗口的大小，默认 `0`。

- `0`：窗口大小为整个扇区。这是常规场景的推荐值，因为该缓存要服务的扫描本身就会遍历到扇区末尾，一次性复制整个扇区所需的访问次数最少；
- 设为大于 `0` 且小于扇区大小的值时，窗口按该大小分块复制，用于限制 RAM 占用；读取落在当前窗口之外时，会重新复制对应的窗口；
- 不小于扇区大小的取值没有实际效果。

## FDB_BIG_ENDIAN

MCU 大小端配置，默认不配置时，系统自动使用小端配置

## FDB_PRINT(...)

打印函数宏定义配置，默认不配置时，使用 printf 作为打印日志是输出函数。用户也可以自定义新的打印函数宏定义，例如：

```C
#define FDB_PRINT(...)              my_printf(__VA_ARGS__)
```

## FDB_DEBUG_ENABLE

使能调试信息输出。关闭该配置时，系统将不会输出用于调试的日志。