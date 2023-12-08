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
        FDB_INFO("Error: db (%s) file path (%s) is too log.\n", file_name, db->storage.dir);
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

static int get_file_from_cache(fdb_db_t db, uint32_t sec_addr)
{
    for (int i = 0; i < FDB_FILE_CACHE_TABLE_SIZE; i++) {
        if (db->cur_file_sec[i] == sec_addr)
            return db->cur_file[i];
    }

    return -1;
}

static void update_file_cache(fdb_db_t db, uint32_t sec_addr, int fd)
{
    int free_index = FDB_FILE_CACHE_TABLE_SIZE;

    for (int i = 0; i < FDB_FILE_CACHE_TABLE_SIZE; i++) {
        if (db->cur_file_sec[i] == sec_addr) {
            db->cur_file[i] = fd;
            return;
        } else if (db->cur_file[i] == -1) {
            free_index = i;
        }
    }

    if (fd > 0) {
        if (free_index < FDB_FILE_CACHE_TABLE_SIZE) {
                db->cur_file[free_index] = fd;
                db->cur_file_sec[free_index] = sec_addr;
        } else {
            /* cache is full, move to end */
            for (int i = FDB_FILE_CACHE_TABLE_SIZE - 1; i > 0; i--) {
                close(db->cur_file[i]);
                memcpy(&db->cur_file[i], &db->cur_file[i - 1], sizeof(db->cur_file[0]));
                memcpy(&db->cur_file_sec[i], &db->cur_file_sec[i - 1], sizeof(db->cur_file_sec[0]));
            }
            /* add to head */
            db->cur_file[0] = fd;
            db->cur_file_sec[0] = sec_addr;
        }
    }
}

static int open_db_file(fdb_db_t db, uint32_t addr, bool clean)
{
    uint32_t sec_addr = FDB_ALIGN_DOWN(addr, db->sec_size);
    int fd = get_file_from_cache(db, sec_addr);
    char path[DB_PATH_MAX];

    if (fd <= 0 || clean) {
        get_db_file_path(db, addr, path, DB_PATH_MAX);

        if (fd > 0) {
            close(fd);
            fd = -1;
            update_file_cache(db, sec_addr, fd);
        }
        if (clean) {
            /* clean the old file */
            int clean_fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0777);
            if (clean_fd <= 0) {
                FDB_INFO("Error: open (%s) file failed.\n", path);
            }
            else {
                close(clean_fd);
                clean_fd = -1;
            }
        }
        if (get_file_from_cache(db, sec_addr) < 0) {
            /* open the database file */
            fd = open(path, O_RDWR, 0777);
            update_file_cache(db, sec_addr, fd);
        }
        db->cur_sec = sec_addr;
    }

    return fd;
}

fdb_err_t _fdb_file_read(fdb_db_t db, uint32_t addr, void *buf, size_t size)
{
    fdb_err_t result = FDB_NO_ERR;
    int fd = open_db_file(db, addr, false);
    if (fd > 0) {
        /* get the offset address is relative to the start of the current file */
        addr = addr % db->sec_size;

        if ((lseek(fd, addr, SEEK_SET) != (int32_t)addr) || (read(fd, buf, size) != (ssize_t)size))
            result = FDB_READ_ERR;
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
        /* get the offset address is relative to the start of the current file */
        addr = addr % db->sec_size;

        if ((lseek(fd, addr, SEEK_SET) != (int32_t)addr) || (write(fd, buf, size) != (ssize_t)size))
            result = FDB_WRITE_ERR;
        if(sync) {
            fsync(fd);
        }
    } else {
        result = FDB_WRITE_ERR;
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

static FILE *get_file_from_cache(fdb_db_t db, uint32_t sec_addr)
{
    for (int i = 0; i < FDB_FILE_CACHE_TABLE_SIZE; i++) {
        if (db->cur_file_sec[i] == sec_addr)
            return db->cur_file[i];
    }

    return NULL;
}

static void update_file_cache(fdb_db_t db, uint32_t sec_addr, FILE *fd)
{
    int free_index = FDB_FILE_CACHE_TABLE_SIZE;

    for (int i = 0; i < FDB_FILE_CACHE_TABLE_SIZE; i++) {
        if (db->cur_file_sec[i] == sec_addr) {
            db->cur_file[i] = fd;
            return;
        }
        else if (db->cur_file[i] == 0) {
            free_index = i;
        }
    }

    if (fd) {
        if (free_index < FDB_FILE_CACHE_TABLE_SIZE) {
            db->cur_file[free_index] = fd;
            db->cur_file_sec[free_index] = sec_addr;
        }
        else {
            /* cache is full, move to end */
            for (int i = FDB_FILE_CACHE_TABLE_SIZE - 1; i > 0; i--) {
                fclose(db->cur_file[i]);
                memcpy(&db->cur_file[i], &db->cur_file[i - 1], sizeof(db->cur_file[0]));
                memcpy(&db->cur_file_sec[i], &db->cur_file_sec[i - 1], sizeof(db->cur_file_sec[0]));
            }
            /* add to head */
            db->cur_file[0] = fd;
            db->cur_file_sec[0] = sec_addr;
        }
    }
}

static FILE *open_db_file(fdb_db_t db, uint32_t addr, bool clean)
{
    uint32_t sec_addr = FDB_ALIGN_DOWN(addr, db->sec_size);
    FILE *fd = get_file_from_cache(db, sec_addr);
    char path[DB_PATH_MAX];

    if (fd == NULL || clean) {
        get_db_file_path(db, addr, path, DB_PATH_MAX);

        if (fd) {
            fclose(fd);
            fd = NULL;
            update_file_cache(db, sec_addr, fd);
        }

        if (clean) {
            /* clean the old file */
            FILE *clean_fd = fopen(path, "wb+");
            if (clean_fd == NULL) {
                FDB_INFO("Error: open (%s) file failed.\n", path);
            } else {
                fclose(clean_fd);
                clean_fd = NULL;
            }
        }
        if (get_file_from_cache(db, sec_addr) == NULL) {
            /* open the database file */
            fd = fopen(path, "rb+");
            update_file_cache(db, sec_addr, fd);
        }
        db->cur_sec = sec_addr;
    }

    return fd;
}

fdb_err_t _fdb_file_read(fdb_db_t db, uint32_t addr, void *buf, size_t size)
{
    fdb_err_t result = FDB_NO_ERR;
    FILE *fp = open_db_file(db, addr, false);
    if (fp) {
        addr = addr % db->sec_size;
        if ((fseek(fp, addr, SEEK_SET) != 0) || (fread(buf, size, 1, fp) != size))
            result = FDB_READ_ERR;
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
        if ((fseek(fp, addr, SEEK_SET) != 0) || (fwrite(buf, size, 1, fp) != size))
            result = FDB_READ_ERR;
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

