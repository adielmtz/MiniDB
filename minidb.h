#pragma once

#include "tree.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define MINIDB_HEADER_SIZE (sizeof(size_t) + sizeof(int64_t) + sizeof(int64_t))

typedef struct MiniDb
{
    /* Serialized fields */
    size_t row_size;
    int64_t row_count;
    int64_t max_rows;

    /* Runtime fields */
    FILE *file;
    size_t index_begin_offset; // end of data segment; begin of index segment
    BinaryTree index;
} MiniDb;

typedef enum MiniDbError
{
    MINIDB_OK,
    MINIDB_ERROR_INVALID_KEY,
    MINIDB_ERROR_NOT_FOUND,
    MINIDB_ERROR_DUPLICATED_KEY_VIOLATION,
    MINIDB_ERROR_OUT_OF_SPACE,
} MiniDbError;

/**
 * Returns the error value as string.
 *
 * @param value The MiniDbError value.
 */
const char *minidb_error_get_str(MiniDbError value);

/**
 * Creates a new MiniDb database file.
 *
 * @param db The MiniDb object to initialize (stack-allocated).
 * @param path The path to the database file.
 * @param data_size The size of the data to store (sizeof(my_struct)).
 * @param max_rows The maximum number of rows to store.
 */
void minidb_create(MiniDb *db, const char *path, size_t data_size, int64_t max_rows);

/**
 * Opens an existing MiniDb database file.
 *
 * @param db The MiniDb object to initialize and load (stack-allocated).
 * @param path The path to the database file.
 */
void minidb_open(MiniDb *db, const char *path);

/**
 * Flushes the database file, releases memory and closes the MiniDb database.
 *
 * @param db The MiniDb object to close.
 */
void minidb_close(MiniDb *db);

/**
 * Adds more rows to an existing MiniDb database.
 *
 * @param db The MiniDb object to expand.
 * @param max_rows The expanded maximum rows.
 *
 * @return True if max_rows is greater than the old MiniDb.max_rows value; false otherwise.
 */
bool minidb_resize(MiniDb *db, int64_t max_rows);

/**
 * Selects a row that matches the given key.
 *
 * @param db The MiniDb object.
 * @param key The key to search.
 * @param result Where the row will be stored.
 * @return MINIDB_OK on success.
 */
MiniDbError minidb_select(MiniDb *db, int64_t key, void *result);

/**
 * Inserts a new row into the MiniDb database.
 *
 * @param db The MiniDb object.
 * @param key The key of the row to insert.
 * @param data The data to insert.
 *
 * @return MINIDB_OK on success.
 */
MiniDbError minidb_insert(MiniDb *db, int64_t key, void *data);

/**
 * Updates an existing row;
 *
 * @param db The MiniDb object.
 * @param key The key of the row to update.
 * @param data The new data.
 *
 * @return MINIDB_OK on success.
 */
MiniDbError minidb_update(MiniDb *db, int64_t key, void *data);

/**
 * Deletes an existing row from the database.
 *
 * @param db The MiniDb object.
 * @param key The key of the row to delete.
 *
 * @return MINIDB_OK on success.
 */
MiniDbError minidb_delete(MiniDb *db, int64_t key);
