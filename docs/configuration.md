# Configuration

When using FlashDB, you can configure its functions through `fdb_cfg.h`. The file template is located in the `inc` directory, or you can copy it in the specific demo project. Let's introduce the configuration details in detail below

## FDB_USING_KVDB

Enable KVDB feature

### FDB_KV_AUTO_UPDATE

Enable KV automatic upgrade function. After this function is enabled, `fdb_kvdb.ver_num` stores the version of the current database. If the version changes, it will automatically trigger an upgrade action and update the new default KV collection to the current database.

## FDB_USING_TSDB

Enable TSDB feature

## FDB_WRITE_GRAN

Flash write granularity, the unit is bit. Currently supports

- 1: nor flash
- 8: stm32f2/f4 on-chip Flash
- 32: stm32f1 on-chip Flash

If multiple Flash specifications are used in the database, for example: both nor flash and stm32f4 on-chip Flash, the maximum value is used as the configuration item, namely: 8 bit

## FDB_BIG_ENDIAN

MCU small-endian configuration, when the default is not configured, the system automatically uses the small-endian configuration

## FDB_PRINT(...)

The print function macro defines the configuration. When it is not configured by default, using `printf` as the print log is the output function. Users can also customize new print function macro definitions, for example:

```C
#define FDB_PRINT(...) my_printf(__VA_ARGS__)
```

## FDB_DEBUG_ENABLE

Enable debugging information output. When this configuration is closed, the system will not output logs for debugging.