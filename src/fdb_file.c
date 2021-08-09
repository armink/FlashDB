/*
 * Copyright (c) 2020, Armink, <armink.ztl@gmail.com>
 * Copyright (c) 2020, enkiller, <462747508@qq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <flashdb.h>
#include <fdb_low_lvl.h>

#define FDB_LOG_TAG "[file]"

#ifdef FDB_USING_FILE_MODE

#define DB_PATH_MAX            256

static void get_db_file_path(fdb_db_t db, uint32_t addr, char *path, size_t size)
{
#define DB_NAME_MAX            8

    /* from db_name.fdb.0 to db_name.fdb.n */
    char file_name[DB_NAME_MAX + 4 + 10];
    uint32_t sec_addr = FDB_ALIGN_DOWN(addr, db->sec_size);
    int index = sec_addr / db->sec_size;

    snprintf(file_name, sizeof(file_name), "%.*s.fdb.%d", DB_NAME_MAX, db->name, index);
    if (strlen(db->storage.dir) + 1 + strlen(file_name) >= size) {
        /* path is too long */
        FDB_ASSERT(0)
    }
    snprintf(path, size, "%s/%s", db->storage.dir, file_name);
}

#if defined(FDB_USING_FILE_POSIX_MODE)
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if !defined(_MSC_VER)
#include <unistd.h>
#endif

static int open_db_file(fdb_db_t db, uint32_t addr, bool clean)
{
    uint32_t sec_addr = FDB_ALIGN_DOWN(addr, db->sec_size);
    int fd = db->cur_file;
    char path[DB_PATH_MAX];

    if (sec_addr != db->cur_sec || fd <= 0 || clean) {
        get_db_file_path(db, addr, path, DB_PATH_MAX);

        if (fd > 0) {
            close(fd);
            fd = -1;
        }
        if (clean) {
            /* clean the old file */
            fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0777);
            if (fd <= 0) {
                FDB_INFO("Error: open (%s) file failed.\n", path);
            }
            else {
                close(fd);
                fd = -1;
            }
        }
        /* open the database file */
        fd = open(path, O_RDWR, 0777);
        db->cur_sec = sec_addr;
    }
    db->cur_file = fd;

    return db->cur_file;
}

fdb_err_t _fdb_file_read(fdb_db_t db, uint32_t addr, void *buf, size_t size)
{
    fdb_err_t result = FDB_NO_ERR;
    int fd = open_db_file(db, addr, false);
    if (fd > 0) {
        addr = addr % db->sec_size;
        lseek(fd, addr, SEEK_SET);
        read(fd, buf, size);
    } else {
        result = FDB_READ_ERR;
    }
    return result;
}

fdb_err_t _fdb_file_write(fdb_db_t db, uint32_t addr, const void *buf, size_t size, bool sync)
{
    fdb_err_t result = FDB_NO_ERR;
    int fd = open_db_file(db, addr, false);
    if (fd > 0) {
        addr = addr % db->sec_size;
        lseek(fd, addr, SEEK_SET);
        write(fd, buf, size);
        if(sync) {
            fsync(fd);
        }
    } else {
        result = FDB_READ_ERR;
    }

    return result;
}

fdb_err_t _fdb_file_erase(fdb_db_t db, uint32_t addr, size_t size)
{
    fdb_err_t result = FDB_NO_ERR;
    int fd = open_db_file(db, addr, true);
    if (fd > 0) {
#define BUF_SIZE 32
        uint8_t buf[BUF_SIZE];
        size_t i;
        lseek(fd, 0, SEEK_SET);
        for (i = 0; i * BUF_SIZE < size; i++)
        {
            memset(buf, 0xFF, BUF_SIZE);
            write(fd, buf, BUF_SIZE);
        }
        memset(buf, 0xFF, BUF_SIZE);
        write(fd, buf, size - i * BUF_SIZE);
        fsync(fd);
    } else {
        result = FDB_ERASE_ERR;
    }
    return result;
}
#elif defined(FDB_USING_FILE_LIBC_MODE)
static FILE *open_db_file(fdb_db_t db, uint32_t addr, bool clean)
{
    uint32_t sec_addr = FDB_ALIGN_DOWN(addr, db->sec_size);

    if (sec_addr != db->cur_sec || db->cur_file == NULL || clean) {
        char path[DB_PATH_MAX];

        get_db_file_path(db, addr, path, DB_PATH_MAX);

        if (db->cur_file) {
            fclose(db->cur_file);
        }

        if (clean) {
            /* clean the old file */
            db->cur_file = fopen(path, "wb+");
            if (db->cur_file == NULL) {
                FDB_INFO("Error: open (%s) file failed.\n", path);
            } else {
                fclose(db->cur_file);
            }
        }

        /* open the database file */
        db->cur_file = fopen(path, "rb+");
        db->cur_sec = sec_addr;
    }

    return db->cur_file;
}

fdb_err_t _fdb_file_read(fdb_db_t db, uint32_t addr, void *buf, size_t size)
{
    fdb_err_t result = FDB_NO_ERR;
    FILE *fp = open_db_file(db, addr, false);
    if (fp) {
        addr = addr % db->sec_size;
        fseek(fp, addr, SEEK_SET);
        fread(buf, size, 1, fp);
    } else {
        result = FDB_READ_ERR;
    }
    return result;
}

fdb_err_t _fdb_file_write(fdb_db_t db, uint32_t addr, const void *buf, size_t size, bool sync)
{
    fdb_err_t result = FDB_NO_ERR;
    FILE *fp = open_db_file(db, addr, false);
    if (fp) {
        addr = addr % db->sec_size;
        fseek(fp, addr, SEEK_SET);
        fwrite(buf, size, 1, fp);
        if(sync) {
            fflush(fp);
        }
    } else {
        result = FDB_READ_ERR;
    }

    return result;
}

fdb_err_t _fdb_file_erase(fdb_db_t db, uint32_t addr, size_t size)
{
    fdb_err_t result = FDB_NO_ERR;

    FILE *fp = open_db_file(db, addr, true);
    if (fp != NULL) {
#define BUF_SIZE 32
        uint8_t buf[BUF_SIZE];
        size_t i;
        fseek(fp, 0, SEEK_SET);
        for (i = 0; i * BUF_SIZE < size; i++)
        {
            memset(buf, 0xFF, BUF_SIZE);
            fwrite(buf, BUF_SIZE, 1, fp);
        }
        memset(buf, 0xFF, BUF_SIZE);
        fwrite(buf, size - i * BUF_SIZE, 1, fp);
        fflush(fp);
    } else {
        result = FDB_ERASE_ERR;
    }
    return result;
}
#endif /* defined(FDB_USING_FILE_LIBC_MODE) */

#endif /* FDB_USING_FILE_MODE */

