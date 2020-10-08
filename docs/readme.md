# FlashDB：An ultra-lightweight embedded database

[![Build Status](https://travis-ci.com/armink/FlashDB.svg?branch=master)](https://travis-ci.com/armink/FlashDB) [![license](https://img.shields.io/github/license/armink/FlashDB)](https://raw.githubusercontent.com/armink/FlashDB/master/LICENSE) 

## Introduction

[FlashDB](https://github.com/armink/FlashDB) is an ultra-lightweight embedded database that focuses on providing data storage solutions for embedded products. Different from traditional database based on file system, [FlashDB](https://github.com/armink/FlashDB) combines the features of Flash and has strong performance and reliability. And under the premise of ensuring extremely low resource occupation, the service life of Flash should be extended as much as possible.

[FlashDB](https://github.com/armink/FlashDB) provides two database modes:

- **Key-value database**: It is a non-relational database that stores data as a collection of key-value pairs, where the key is used as a unique identifier. KVDB has simple operation and strong scalability.
- **Time Series Database**: Time Series Database (TSDB), which stores data in **time sequence**. TSDB data has a timestamp, a large amount of data storage, and high insertion and query performance.

## Usage scenario

Nowadays, there are more and more types of IoT products, and the types and total amount of data generated during operation are also increasing. FlashDB provides a variety of data storage solutions, not only has a small resource footprint, but also has a large storage capacity, which is very suitable for IoT products. The following are the main application scenarios:

- **Key-value database**:
  - Product parameter storage
  - User configuration information storage
  - Small file management
- **Time Series Database**: 
  - Store dynamically generated structured data: such as environmental monitoring information collected by temperature and humidity sensors, human health information recorded in real time by smart bracelets, etc.
  - Record operation log: store operation log of product history, record of abnormal alarm, etc.

## Key Features

- Very small footprint, ram usage is almost **0**;
- Support multiple partitions, **multiple instances**. When the amount of data is large, the partition can be refined to reduce the retrieval time;
- Support **wear balance** to extend Flash life;
- Support **Power-off protection** function, high reliability;
- Supports two KV types, string and blob, which is convenient for users to operate;
- Support KV **incremental upgrade**, after product firmware upgrade, KVDB content also supports automatic upgrade;
- Support to modify the status of each TSDB record to facilitate user management;

## Performance and footprint

### TSDB performance test 1 (nor flash W25Q64)

```shell
msh />tsl bench
Append 1250 TSL in 5 seconds, average: 250.00 tsl/S, 4.00 ms/per
Query total spent 2218 (ms) for 1251 TSL, min 1, max 2, average: 1.77 ms/per
```

Insert average: 4 ms, query average: 1.8 ms

### TSDB performance test 2 (stm32f2 onchip flash)

```shell
msh />tsl bench
Append 13421 TSL in 5 seconds, average: 2684.20 tsl/S, 0.37 ms/per
Query total spent 1475 (ms) for 13422 TSL, min 0, max 1, average: 0.11 ms/per
```

Insert average: 0.37 ms, query average: 0.12 ms

### Footprint (stm32f4 IAR8.20)

```shell
    Module                  ro code  ro data  rw data
    ------                  -------  -------  -------
    fdb.o                     276      232        1
    fdb_kvdb.o              4 584      356        1
    fdb_tsdb.o              1 160      236
    fdb_utils.o               418    1 024
```

The above is the map file information of IAR. It can be seen that the footprint of FlashDB is very small.

## License

The project uses the Apache-2.0 open source protocol. For details, please read the contents of the LICENSE file in the project.