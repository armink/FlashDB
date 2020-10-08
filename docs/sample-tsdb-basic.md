# TSDB basic example

This example mainly demonstrates the basic functions of TSDB, including TSL (time sequence record) addition, query and status modification functions.

## Code description

The sample code is located in `samples/tsdb_sample.c`, including the processes of appending, querying and status modification. The approximate code is as follows:

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

Let's look at these processes separately

- **Append**: Modify the value of the structure object `status` twice and append it to TSDB;

- **Query**: Through TSDB's iterator API, the `query_cb` callback function will be automatically executed during each iteration to query all records in TSDB. The content of the callback function is as follows:

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

- **Query by time**: TSDB also provides an API for iterating by time: `fdb_tsl_iter_by_time`, you can pass in the start and end time, and the iterator will iterate the time series records according to the passed time period. In each iteration, the `query_by_time_cb` callback is executed, and the content of the current record is read in the callback and printed out.

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

- **Modify Status**: After each TSL is added to TSDB, its status can be modified. There are 4 types of status:

  - `FDB_TSL_WRITE`: written state, the default state after TSL is appended to TSDB;

  - `FDB_TSL_USER_STATUS1`: The status is between writing and deleting. Users can customize the meaning of the status, such as: data has been synchronized to the cloud;

  - `FDB_TSL_DELETED`: Deleted state, when TSL needs to be deleted, just modify the state of TSL to this state;

    > Tip: In FlashDB, in order to improve the life of the Flash, the delete action does not actually erase the data from the Flash, but marks it as a deleted state. The user can distinguish different data records by state.

  - `FDB_TSL_USER_STATUS2`: the customized status after the status is deleted, reserved for users;

  When modifying the status, it can only be modified in the order of `FDB_TSL_WRITE -> FDB_TSL_USER_STATUS1 -> FDB_TSL_DELETED -> FDB_TSL_USER_STATUS2`, and cannot be modified in reverse order. It is also possible to skip intermediate states, for example: directly change from `FDB_TSL_WRITE` to `FDB_TSL_DELETED` state, and skip `FDB_TSL_USER_STATUS1` state.

  In the example, all current TSLs are modified to the status of `FDB_TSL_USER_STATUS1` through the iterator. The callback code in the iterator is as follows:

  ```C
  static bool set_status_cb(fdb_tsl_t tsl, void *arg)
  {
      fdb_tsdb_t db = arg;
  
      FDB_INFO("set the TSL (time %ld) status from %d to %d\n", tsl->time, tsl->status, FDB_TSL_USER_STATUS1);
      fdb_tsl_set_status(db, tsl, FDB_TSL_USER_STATUS1);
  
      return false;
  }
  ```

## First run log

It can be seen from the log that the example first adds two TSLs, and each TSL stores different temperature and humidity records. Then obtain the TSL in TSDB through ordinary query and query by time, and finally modify its status from `2: FDB_TSL_WRITE` to `3: FDB_TSL_USER_STATUS1`.

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

## Secondary run log

In the second run, two TSLs will still be added. Look at the results of the query, there are 4 in total, including 2 TSLs that were added at the first run. It can be seen from the printed time stamp that the analog time stamp works normally. `query count is: 2` shows that although TSDB has 4 records, only 2 records are in the write state.

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