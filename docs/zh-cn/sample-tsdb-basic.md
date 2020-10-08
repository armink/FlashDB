# TSDB 基础示例

该示例主要演示了 TSDB 的基础功能，包括 TSL（时序记录）的追加、查询及状态修改功能。

## 代码说明

示例代码位于 `samples/tsdb_sample.c` ，包含追加、查询及状态修改这几个过程，大致代码如下：

```C
void tsdb_sample(fdb_tsdb_t tsdb)
{
    struct fdb_blob blob;

    FDB_INFO("==================== tsdb_sample ====================\n");

    { /* APPEND new TSL (time series log) */
        struct env_status status;

        /* append new log to TSDB */
        status.temp = 36;
        status.humi = 85;
        fdb_tsl_append(tsdb, fdb_blob_make(&blob, &status, sizeof(status)));
        FDB_INFO("append the new status.temp (%d) and status.humi (%d)\n", status.temp, status.humi);

        status.temp = 38;
        status.humi = 90;
        fdb_tsl_append(tsdb, fdb_blob_make(&blob, &status, sizeof(status)));
        FDB_INFO("append the new status.temp (%d) and status.humi (%d)\n", status.temp, status.humi);
    }

    { /* QUERY the TSDB */
        /* query all TSL in TSDB by iterator */
        fdb_tsl_iter(tsdb, query_cb, tsdb);
    }

    { /* QUERY the TSDB by time */
        /* prepare query time (from 1970-01-01 00:00:00 to 2020-05-05 00:00:00) */
        struct tm tm_from = { .tm_year = 1970 - 1900, .tm_mon = 0, .tm_mday = 1, .tm_hour = 0, .tm_min = 0, .tm_sec = 0 };
        struct tm tm_to = { .tm_year = 2020 - 1900, .tm_mon = 4, .tm_mday = 5, .tm_hour = 0, .tm_min = 0, .tm_sec = 0 };
        time_t from_time = mktime(&tm_from), to_time = mktime(&tm_to);
        size_t count;
        /* query all TSL in TSDB by time */
        fdb_tsl_iter_by_time(tsdb, from_time, to_time, query_by_time_cb, tsdb);
        /* query all FDB_TSL_WRITE status TSL's count in TSDB by time */
        count = fdb_tsl_query_count(tsdb, from_time, to_time, FDB_TSL_WRITE);
        FDB_INFO("query count is: %u\n", count);
    }

    { /* SET the TSL status */
        /* Change the TSL status by iterator or time iterator
         * set_status_cb: the change operation will in this callback
         *
         * NOTE: The actions to modify the state must be in orderC.
         *       like: FDB_TSL_WRITE -> FDB_TSL_USER_STATUS1 -> FDB_TSL_DELETED -> FDB_TSL_USER_STATUS2
         *       The intermediate states can also be ignored.
         *       such as: FDB_TSL_WRITE -> FDB_TSL_DELETED
         */
        fdb_tsl_iter(tsdb, set_status_cb, tsdb);
    }

    FDB_INFO("===========================================================\n");
}
```

分别来看下这几个过程

- **追加**：分两次修改结构体对象 `status` 的值然后追加到 TSDB 中；

- **查询**：通过 TSDB 的迭代器 API ，在每次迭代时会自动执行 `query_cb` 回调函数，实现对 TSDB 中所有记录的查询，回调函数内容如下：

  ```C
  static bool query_cb(fdb_tsl_t tsl, void *arg)
  {
      struct fdb_blob blob;
      struct env_status status;
      fdb_tsdb_t db = arg;
  
      fdb_blob_read((fdb_db_t) db, fdb_tsl_to_blob(tsl, fdb_blob_make(&blob, &status, sizeof(status))));
      FDB_INFO("[query_cb] queried a TSL: time: %ld, temp: %d, humi: %d\n", tsl->time, status.temp, status.humi);
  
      return false;
  }
  ```

- **按时间查询**：TSDB 还提供了按时间迭代的 API : `fdb_tsl_iter_by_time` ，可以传入起始和截至时间，此时迭代器会按照传入的时间段，对时序记录进行迭代。每次迭代时会执行  `query_by_time_cb` 回调，在回调中读取当前记录的内容，并打印出来。

  ```C
  static bool query_by_time_cb(fdb_tsl_t tsl, void *arg)
  {
      struct fdb_blob blob;
      struct env_status status;
      fdb_tsdb_t db = arg;
  
      fdb_blob_read((fdb_db_t) db, fdb_tsl_to_blob(tsl, fdb_blob_make(&blob, &status, sizeof(status))));
      FDB_INFO("[query_by_time_cb] queried a TSL: time: %ld, temp: %d, humi: %d\n", tsl->time, status.temp, status.humi);
  
      return false;
  }
  ```

- **修改状态**：每条 TSL 在被追加到 TSDB 后，都可以修改其状态，状态共有 4 种：

  - `FDB_TSL_WRITE`：已写入状态，TSL 被追加到 TSDB 中后的默认状态；

  - `FDB_TSL_USER_STATUS1 `：该状态介于写入与删除之间，用户可自定义其状态含义，比如：数据已被同步至云端；

  - `FDB_TSL_DELETED` ：已删除状态，当 TSL 需要删除时，修改 TSL 的状态为该状态即可；

    > 提示：在 FlashDB 中，为了提升 Flash 寿命，删除动作并不会真正的将数据从 Flash 从擦除，而是将其标记为删除状态，用户可以通过状态对不同的数据记录进行区分。

  - `FDB_TSL_USER_STATUS2`：删除状态之后的自定义状态，预留给用户使用；

  修改状态时只能按照 `FDB_TSL_WRITE -> FDB_TSL_USER_STATUS1 -> FDB_TSL_DELETED -> FDB_TSL_USER_STATUS2` 顺序进行修改，不能逆序修改。也可以跳过中间状态，例如：从 `FDB_TSL_WRITE` 直接修改为`FDB_TSL_DELETED` 状态，跳过 `FDB_TSL_USER_STATUS1` 状态。

  示例中通过迭代器将当前所有的 TSL 都修改为 `FDB_TSL_USER_STATUS1` 状态，迭代器中的回调代码如下：

  ```C
  static bool set_status_cb(fdb_tsl_t tsl, void *arg)
  {
      fdb_tsdb_t db = arg;
  
      FDB_INFO("set the TSL (time %ld) status from %d to %d\n", tsl->time, tsl->status, FDB_TSL_USER_STATUS1);
      fdb_tsl_set_status(db, tsl, FDB_TSL_USER_STATUS1);
  
      return false;
  }
  ```

## 首次运行日志

通过日志可看出，示例首先追加了两条 TSL ，每条 TSL 分别存放了不同的温度及湿度记录。然后再通过普通查询和按时间查询方式，将 TSDB 中的 TSL 获取出来，最后修改其状态，从 `2: FDB_TSL_WRITE` 到 `3: FDB_TSL_USER_STATUS1` 。

```
[FlashDB][sample][tsdb] ==================== tsdb_sample ====================
[FlashDB][sample][tsdb] append the new status.temp (36) and status.humi (85)
[FlashDB][sample][tsdb] append the new status.temp (38) and status.humi (90)
[FlashDB][sample][tsdb] [query_cb] queried a TSL: time: 1, temp: 36, humi: 85
[FlashDB][sample][tsdb] [query_cb] queried a TSL: time: 2, temp: 38, humi: 90
[FlashDB][sample][tsdb] [query_by_time_cb] queried a TSL: time: 1, temp: 36, humi: 85
[FlashDB][sample][tsdb] [query_by_time_cb] queried a TSL: time: 2, temp: 38, humi: 90
[FlashDB][sample][tsdb] query count is: 2
[FlashDB][sample][tsdb] set the TSL (time 1) status from 2 to 3
[FlashDB][sample][tsdb] set the TSL (time 2) status from 2 to 3
[FlashDB][sample][tsdb] ===========================================================
```

## 二次运行日志

第二次运行时，依旧会追加两条 TSL 。再看查询的结果，共计 4 条，包含了 2 条首次运行时追加的 TSL。通过打印的 time 时间戳可以看出，模拟时间戳工作正常。`query count is: 2` 说明了 TSDB 虽然有 4 条记录，但只有 2 条记录是写入状态。

```
[FlashDB][sample][tsdb] ==================== tsdb_sample ====================
[FlashDB][sample][tsdb] append the new status.temp (36) and status.humi (85)
[FlashDB][sample][tsdb] append the new status.temp (38) and status.humi (90)
[FlashDB][sample][tsdb] [query_cb] queried a TSL: time: 1, temp: 36, humi: 85
[FlashDB][sample][tsdb] [query_cb] queried a TSL: time: 2, temp: 38, humi: 90
[FlashDB][sample][tsdb] [query_cb] queried a TSL: time: 3, temp: 36, humi: 85
[FlashDB][sample][tsdb] [query_cb] queried a TSL: time: 4, temp: 38, humi: 90
[FlashDB][sample][tsdb] [query_by_time_cb] queried a TSL: time: 1, temp: 36, humi: 85
[FlashDB][sample][tsdb] [query_by_time_cb] queried a TSL: time: 2, temp: 38, humi: 90
[FlashDB][sample][tsdb] [query_by_time_cb] queried a TSL: time: 3, temp: 36, humi: 85
[FlashDB][sample][tsdb] [query_by_time_cb] queried a TSL: time: 4, temp: 38, humi: 90
[FlashDB][sample][tsdb] query count is: 2
[FlashDB][sample][tsdb] set the TSL (time 1) status from 3 to 3
[FlashDB][sample][tsdb] set the TSL (time 2) status from 3 to 3
[FlashDB][sample][tsdb] set the TSL (time 3) status from 2 to 3
[FlashDB][sample][tsdb] set the TSL (time 4) status from 2 to 3
[FlashDB][sample][tsdb] ===========================================================
```