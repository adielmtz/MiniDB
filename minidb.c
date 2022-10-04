#include "minidb.h"
#include <assert.h>

#ifndef is_null
#define is_null(ptr) ((ptr) == NULL)
#endif

#define MINIDB_INITIALIZE_EMPTY(db) \
do {                                \
    (db)->header.row_size = 0;      \
    (db)->header.row_count = 0;     \
    (db)->header.max_rows = 0;      \
    (db)->index.size = 0;           \
    (db)->index.root = NULL;        \
    (db)->file = NULL;              \
    (db)->index_begin_offset = 0;   \
} while (0)

#define RETURN_CASE_AS_STRING(caseval) case caseval: return #caseval

const char *minidb_error_get_str(MiniDbError value)
{
    switch (value) {
        RETURN_CASE_AS_STRING(MINIDB_OK);
        RETURN_CASE_AS_STRING(MINIDB_ERROR_INVALID_KEY);
        RETURN_CASE_AS_STRING(MINIDB_ERROR_NOT_FOUND);
        RETURN_CASE_AS_STRING(MINIDB_ERROR_DUPLICATED_KEY_VIOLATION);
        RETURN_CASE_AS_STRING(MINIDB_ERROR_OUT_OF_SPACE);
        default:
            return "UNKNOWN MINIDBERROR";
    }
}

static void minidb_header_write(const MiniDb *db)
{
    fseek(db->file, 0, SEEK_SET);
    fwrite(&db->header, MINIDB_HEADER_SIZE, 1, db->file);
}

static void index_write_node_recurse(const MiniDb *db, const BinaryTreeNode *node)
{
    if (!is_null(node)) {
        index_write_node_recurse(db, node->left);
        fwrite(&node->key, sizeof(int64_t), 1, db->file);
        index_write_node_recurse(db, node->right);
    }
}

static void minidb_write_index(const MiniDb *db)
{
    fseek(db->file, db->index_begin_offset, SEEK_SET);
    index_write_node_recurse(db, db->index.root);
    fflush(db->file);
}

static void minidb_page_erase(const MiniDb *db, size_t row_size, int64_t row_count)
{
    char *zeros = calloc(1, row_size);
    for (int64_t i = 0; i < row_count; i++) {
        size_t written = fwrite(zeros, sizeof(char), row_size, db->file);
        if (written == 0) {
            break;
        }
    }

    fflush(db->file);
    free(zeros);
}

void minidb_create(MiniDb *db, const char *path, size_t data_size, int64_t max_rows)
{
    MINIDB_INITIALIZE_EMPTY(db);
    db->file = fopen(path, "w+");
    if (!is_null(db->file)) {
        db->header.row_size = data_size;
        db->header.max_rows = max_rows;
        db->index_begin_offset = MINIDB_HEADER_SIZE + db->header.row_size * db->header.max_rows;
        minidb_header_write(db);
        minidb_page_erase(db, data_size, max_rows);
    }
}

void minidb_open(MiniDb *db, const char *path)
{
    MINIDB_INITIALIZE_EMPTY(db);
    db->file = fopen(path, "r+");
    if (!is_null(db->file)) {
        fread(&db->header, MINIDB_HEADER_SIZE, 1, db->file);
        db->index_begin_offset = MINIDB_HEADER_SIZE + db->header.row_size * db->header.max_rows;

        // Parse & load index
        if (db->header.row_count > 0) {
            fseek(db->file, db->index_begin_offset, SEEK_SET);
            for (int64_t i = 0; i < db->header.row_count; i++) {
                int64_t key;
                fread(&key, sizeof(int64_t), 1, db->file);
                binarytree_insert(&db->index, key);
            }
        }
    }
}

void minidb_close(MiniDb *db)
{
    if (!is_null(db) && !is_null(db->file)) {
        minidb_header_write(db);
        minidb_write_index(db);
        binarytree_destroy(&db->index);
        fflush(db->file);
        fclose(db->file);
        MINIDB_INITIALIZE_EMPTY(db);
    }
}

bool minidb_resize(MiniDb *db, int64_t max_rows)
{
    if (max_rows > db->header.max_rows) {
        fseek(db->file, db->index_begin_offset, SEEK_SET);
        minidb_page_erase(db, db->header.row_size, max_rows - db->header.max_rows);
        db->header.max_rows = max_rows;
        db->index_begin_offset = MINIDB_HEADER_SIZE + db->header.row_size * db->header.max_rows;
        minidb_header_write(db);
        minidb_write_index(db);
        return true;
    }

    return false;
}

MiniDbError minidb_select(MiniDb *db, int64_t key, void *result)
{
    if (key <= 0) {
        return MINIDB_ERROR_INVALID_KEY;
    }

    BinaryTreeNode *node = binarytree_search(&db->index, key);
    if (is_null(node)) {
        return MINIDB_ERROR_NOT_FOUND;
    }

    size_t position = MINIDB_HEADER_SIZE + db->header.row_size * (node->key - 1);
    fseek(db->file, position, SEEK_SET);
    fread(result, db->header.row_size, 1, db->file);
    return MINIDB_OK;
}

MiniDbError minidb_insert(MiniDb *db, int64_t key, void *data)
{
    if (key <= 0) {
        return MINIDB_ERROR_INVALID_KEY;
    }

    if (binarytree_contains(&db->index, key)) {
        return MINIDB_ERROR_DUPLICATED_KEY_VIOLATION;
    }

    size_t position = MINIDB_HEADER_SIZE + db->header.row_size * (key - 1);
    if (position >= db->index_begin_offset) {
        return MINIDB_ERROR_OUT_OF_SPACE;
    }

    fseek(db->file, position, SEEK_SET);
    fwrite(data, db->header.row_size, 1, db->file);
    db->header.row_count++;

    binarytree_insert(&db->index, key);

    minidb_header_write(db);
    minidb_write_index(db);
    return MINIDB_OK;
}

MiniDbError minidb_update(MiniDb *db, int64_t key, void *data)
{
    if (key <= 0) {
        return MINIDB_ERROR_INVALID_KEY;
    }

    BinaryTreeNode *node = binarytree_search(&db->index, key);
    if (is_null(node)) {
        return MINIDB_ERROR_NOT_FOUND;
    }

    size_t position = MINIDB_HEADER_SIZE + db->header.row_size * (node->key - 1);
    fseek(db->file, position, SEEK_SET);
    fwrite(data, db->header.row_size, 1, db->file);
    fflush(db->file);
    return MINIDB_OK;
}

MiniDbError minidb_delete(MiniDb *db, int64_t key)
{
    if (key <= 0) {
        return MINIDB_ERROR_INVALID_KEY;
    }

    if (db->header.row_count > 0) {
        bool removed = binarytree_remove(&db->index, key);
        if (removed) {
            db->header.row_count--;
            assert(db->header.row_count == db->index.size);
            minidb_header_write(db);
            minidb_write_index(db);
        }
    }

    return MINIDB_OK;
}
