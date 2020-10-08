# FlashDB：超轻量级嵌入式数据库

[![Build Status](https://travis-ci.com/armink/FlashDB.svg?branch=master)](https://travis-ci.com/armink/FlashDB) [![license](https://img.shields.io/github/license/armink/FlashDB)](https://raw.githubusercontent.com/armink/FlashDB/master/LICENSE) 

## 简介

[FlashDB](https://github.com/armink/FlashDB) 是一款超轻量级的嵌入式数据库，专注于提供嵌入式产品的数据存储方案。与传统的基于文件系统的数据库不同，[FlashDB](https://github.com/armink/FlashDB) 结合了 Flash 的特性，具有较强的性能及可靠性。并在保证极低的资源占用前提下，尽可能延长 Flash 使用寿命。

[FlashDB](https://github.com/armink/FlashDB) 提供两种数据库模式：

- **键值数据库** ：是一种非关系数据库，它将数据存储为键值（Key-Value）对集合，其中键作为唯一标识符。KVDB 操作简洁，可扩展性强。
- **时序数据库** ：时间序列数据库 （Time Series Database , 简称 TSDB），它将数据按照 **时间顺序存储** 。TSDB 数据具有时间戳，数据存储量大，插入及查询性能高。

## 使用场景

如今，物联网产品种类越来越多，运行时产生的数据种类及总量及也在不断变大。FlashDB 提供了多样化的数据存储方案，不仅资源占用小，并且存储容量大，非常适合用于物联网产品。下面是主要应用场景：

- **键值数据库** ：
  - 产品参数存储
  - 用户配置信息存储
  - 小文件管理
- **时序数据库** ：
  - 存储动态产生的结构化数据：如 温湿度传感器采集的环境监测信息，智能手环实时记录的人体健康信息等
  - 记录运行日志：存储产品历史的运行日志，异常告警的记录等

## 主要特性

- 资源占用极低，内存占用几乎为 **0** ;
- 支持 多分区，**多实例** 。数据量大时，可细化分区，降低检索时间；
- 支持 **磨损平衡** ，延长 Flash 寿命；
- 支持 **掉电保护** 功能，可靠性高；
- 支持 字符串及 blob 两种 KV 类型，方便用户操作；
- 支持 KV **增量升级** ，产品固件升级后， KVDB 内容也支持自动升级；
- 支持 修改每条 TSDB 记录的状态，方便用户进行管理；

## 性能及资源占用

### TSDB 性能测试1 （nor flash W25Q64）

```shell
msh />tsl bench
Append 1250 TSL in 5 seconds, average: 250.00 tsl/S, 4.00 ms/per
Query total spent 2218 (ms) for 1251 TSL, min 1, max 2, average: 1.77 ms/per
```

插入平均：4 ms，查询平均：1.8 ms

### TSDB 性能测试2 （stm32f2 onchip flash）

```shell
msh />tsl bench
Append 13421 TSL in 5 seconds, average: 2684.20 tsl/S, 0.37 ms/per
Query total spent 1475 (ms) for 13422 TSL, min 0, max 1, average: 0.11 ms/per
```

插入平均：0.37 ms，查询平均：0.12 ms

### 资源占用 (stm32f4 IAR8.20)

```shell
    Module                  ro code  ro data  rw data
    ------                  -------  -------  -------
    fdb.o                     276      232        1
    fdb_kvdb.o              4 584      356        1
    fdb_tsdb.o              1 160      236
    fdb_utils.o               418    1 024
```

上面是 IAR 的 map 文件信息，可见 FlashDB 的资源占用非常低

## 支持

 ![support](_media/wechat_support.png)

如果 FlashDB 解决了你的问题，不妨扫描上面二维码请我 **喝杯咖啡**~ 

## 许可

采用 Apache-2.0 开源协议，细节请阅读项目中的 LICENSE 文件内容。