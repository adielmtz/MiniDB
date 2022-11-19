#pragma once

#include <stdint.h>
#include <stddef.h>

#ifndef is_null
#define is_null(ptr) ((ptr) == NULL)
#endif

typedef struct MiniDb MiniDb;

typedef struct MiniDbInfo
{
    size_t data_size;
    int64_t row_count;
    int64_t free_count;
} MiniDbInfo;

typedef enum MiniDbState
{
    MINIDB_OK,
    MINIDB_ERROR,
    MINIDB_ERROR_MALLOC_FAIL,
    MINIDB_ERROR_CANNOT_OPEN_FILE,
    MINIDB_ERROR_NULL_POINTER,
    MINIDB_ERROR_ROW_NOT_FOUND,
    MINIDB_ERROR_DUPLICATED_KEY_VIOLATION,
} MiniDbState;

/**
 * Returns the error value as string.
 *
 * @param value The MiniDbError value.
 */
const char *minidb_error_get_str(MiniDbState value);

/**
 * Creates a new MiniDb database file.
 *
 * @param db The MiniDb object to initialize (stack-allocated).
 * @param path The path to the database file.
 * @param data_size The size of the data to store (sizeof(my_struct)).
 *
 * @return MINIDB_OK on success.
 */
MiniDbState minidb_create(MiniDb **db, const char *path, size_t data_size);

/**
 * Opens an existing MiniDb database file.
 *
 * @param db The MiniDb object to initialize and load (stack-allocated).
 * @param path The path to the database file.
 *
 * @return MINISB_OK on success.
 */
MiniDbState minidb_open(MiniDb **db, const char *path);

/**
 * Flushes the database file, releases memory and closes the MiniDb database.
 *
 * @param db The MiniDb object to close.
 */
void minidb_close(MiniDb **db);

void minidb_get_info(const MiniDb *db, MiniDbInfo *result);

/**
 * Selects a row that matches the given key.
 *
 * @param db The MiniDb object.
 * @param key The key to search.
 * @param result Where the row will be stored.
 *
 * @return MINIDB_OK on success.
 */
MiniDbState minidb_select(const MiniDb *db, int64_t key, void *result);

/**
 * Selects all rows in the database.
 *
 * @param db The MiniDb object.
 * @param callback The callback function that will be executed on for each row.
 *
 * @return MINIDB_OK on success.
 */
MiniDbState minidb_select_all(const MiniDb *db, void (*callback)(int64_t, void *));

/**
 * Inserts a new row into the MiniDb database.
 *
 * @param db The MiniDb object.
 * @param key The key of the row to insert.
 * @param data The data to insert.
 *
 * @return MINIDB_OK on success.
 */
MiniDbState minidb_insert(MiniDb *db, int64_t key, void *data);

/**
 * Updates an existing row.
 *
 * @param db The MiniDb object.
 * @param key The key of the row to update.
 * @param data The new data.
 *
 * @return MINIDB_OK on success.
 */
MiniDbState minidb_update(MiniDb *db, int64_t key, void *data);

/**
 * Deletes an existing row from the database.
 *
 * @param db The MiniDb object.
 * @param key The key of the row to delete.
 *
 * @return MINIDB_OK on success.
 */
MiniDbState minidb_delete(MiniDb *db, int64_t key);
