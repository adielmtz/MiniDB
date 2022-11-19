#include "minidb.h"
#include "index.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MINIDB_INDEX_SUFFIX "-index"
#define RETURN_CASE_AS_STRING(caseval) case caseval: return #caseval
#define SWITCH_UNREACHABLE_DEFAULT_CASE() default: assert(0)

typedef struct MiniDbHeader
{
    size_t data_size;
    int64_t row_count;
    int64_t free_count;
} MiniDbHeader;

struct MiniDb
{
    MiniDbHeader header;
    MiniDbIndex index;
    FILE *fd;
};

const char *minidb_error_get_str(MiniDbState value)
{
    switch (value) {
        RETURN_CASE_AS_STRING(MINIDB_OK);
        RETURN_CASE_AS_STRING(MINIDB_ERROR);
        RETURN_CASE_AS_STRING(MINIDB_ERROR_ROW_NOT_FOUND);
        RETURN_CASE_AS_STRING(MINIDB_ERROR_DUPLICATED_KEY_VIOLATION);
        SWITCH_UNREACHABLE_DEFAULT_CASE();
    }
}

static void minidb_build_index_file_path(const char *base_path, char *output, size_t output_size)
{
    size_t len = strlen(base_path);
    size_t total_len = len + sizeof(MINIDB_INDEX_SUFFIX) - 1;
    if (total_len >= output_size) {
        total_len = output_size - 1;
    }

    memcpy(output, base_path, len);
    memcpy(output + len, MINIDB_INDEX_SUFFIX, sizeof(MINIDB_INDEX_SUFFIX) - 1);
    output[total_len] = '\0';
}

static void minidb_header_write(const MiniDb *mini)
{
    fseek(mini->fd, 0, SEEK_SET);
    fwrite(&mini->header, sizeof(MiniDbHeader), 1, mini->fd);
    fflush(mini->fd);
}

static void minidb_initialize_empty(MiniDb *mini)
{
    mini->header.data_size = UINT64_C(0);
    mini->header.row_count = INT64_C(0);
    mini->header.free_count = INT64_C(0);
    minidb_index_init(&mini->index);
}

MiniDbState minidb_create(MiniDb **db, const char *path, size_t data_size)
{
    *db = NULL;
    FILE *fd = fopen(path, "w+");
    if (is_null(fd)) {
        return MINIDB_ERROR_CANNOT_OPEN_FILE;
    }

    MiniDb *mini = malloc(sizeof(MiniDb));
    if (is_null(mini)) {
        fclose(fd);
        return MINIDB_ERROR_MALLOC_FAIL;
    }

    minidb_initialize_empty(mini);
    mini->header.data_size = data_size;
    mini->fd = fd;

    char index_path[1024];
    minidb_build_index_file_path(path, index_path, sizeof(index_path));
    MiniDbState state = minidb_index_open(&mini->index, index_path, mini->header.row_count, mini->header.free_count);
    if (state != MINIDB_OK) {
        fclose(fd);
        free(mini);
        return state;
    }

    minidb_header_write(mini);
    *db = mini;
    return MINIDB_OK;
}

MiniDbState minidb_open(MiniDb **db, const char *path)
{
    *db = NULL;
    FILE *fd = fopen(path, "r+");
    if (is_null(fd)) {
        return MINIDB_ERROR_CANNOT_OPEN_FILE;
    }

    MiniDb *mini = malloc(sizeof(MiniDb));
    if (is_null(mini)) {
        fclose(fd);
        return MINIDB_ERROR_MALLOC_FAIL;
    }

    minidb_initialize_empty(mini);
    mini->fd = fd;
    fread(&mini->header, sizeof(MiniDbHeader), 1, fd);

    char index_path[1024];
    minidb_build_index_file_path(path, index_path, sizeof(index_path));
    MiniDbState state = minidb_index_open(&mini->index, index_path, mini->header.row_count, mini->header.free_count);
    if (state != MINIDB_OK) {
        fclose(fd);
        free(mini);
    }

    *db = mini;
    return MINIDB_OK;
}

void minidb_close(MiniDb **db)
{
    if (!is_null(db)) {
        MiniDb *mini = *db;
        minidb_index_close(&mini->index);
        minidb_header_write(mini);
        fflush(mini->fd);
        fclose(mini->fd);
        free(mini);
        *db = NULL;
    }
}

void minidb_get_info(const MiniDb *db, MiniDbInfo *result)
{
    result->data_size = db->header.data_size;
    result->row_count = db->header.row_count;
    result->free_count = db->header.free_count;
}

MiniDbState minidb_select(const MiniDb *db, int64_t key, void *result)
{
    BTreeNode *node = btree_search(&db->index.search, key);
    if (is_null(node)) {
        return MINIDB_ERROR_ROW_NOT_FOUND;
    }

    fseek(db->fd, node->value, SEEK_SET);
    fread(result, db->header.data_size, 1, db->fd);
    return MINIDB_OK;
}

static void minidb_index_traverse(const MiniDb *db, BTreeNode *current, void *result, void (*callback)(int64_t, void *))
{
    if (!is_null(current)) {
        minidb_index_traverse(db, current->left, result, callback);
        minidb_select(db, current->key, result);
        callback(current->key, result);
        minidb_index_traverse(db, current->right, result, callback);
    }
}

MiniDbState minidb_select_all(const MiniDb *db, void (*callback)(int64_t, void *))
{
    void *result = malloc(db->header.data_size);
    if (is_null(result)) {
        return MINIDB_ERROR_MALLOC_FAIL;
    }

    minidb_index_traverse(db, db->index.search.root, result, callback);
    free(result);
    return MINIDB_OK;
}

/**
 * Finds the smallest free address available. Returns NULL if the freelist index is empty.
 */
static const BTreeNode *minidb_freelist_find_node(const MiniDb *db)
{
    const BTreeNode *node = db->index.freelist.root;
    if (!is_null(node)) {
        while (!is_null(node->left)) {
            node = node->left;
        }
    }

    return node;
}

MiniDbState minidb_insert(MiniDb *db, int64_t key, void *data)
{
    if (btree_contains(&db->index.search, key)) {
        return MINIDB_ERROR_DUPLICATED_KEY_VIOLATION;
    }

    const BTreeNode *free_node = minidb_freelist_find_node(db);
    int64_t address;

    if (is_null(free_node)) {
        address = sizeof(MiniDbHeader) + db->header.data_size * db->header.row_count;
    } else {
        address = free_node->value;
        btree_remove(&db->index.freelist, free_node->key, NULL);
        db->header.free_count--;
        assert(db->header.free_count == db->index.freelist.size);
    }

    fseek(db->fd, address, SEEK_SET);
    fwrite(data, db->header.data_size, 1, db->fd);
    db->header.row_count++;

    btree_insert(&db->index.search, key, address);
    minidb_header_write(db);
    minidb_index_write(&db->index);
    return MINIDB_OK;
}

MiniDbState minidb_update(MiniDb *db, int64_t key, void *data)
{
    BTreeNode *node = btree_search(&db->index.search, key);
    if (is_null(node)) {
        return MINIDB_ERROR_ROW_NOT_FOUND;
    }

    fseek(db->fd, node->value, SEEK_SET);
    fwrite(data, db->header.data_size, 1, db->fd);
    fflush(db->fd);
    return MINIDB_OK;
}

MiniDbState minidb_delete(MiniDb *db, int64_t key)
{
    if (db->header.row_count > 0) {
        int64_t old_address;
        bool removed = btree_remove(&db->index.search, key, &old_address);
        if (removed) {
            db->header.row_count--;
            assert(db->header.row_count == db->index.search.size);

            btree_insert(&db->index.freelist, old_address, old_address);
            db->header.free_count++;
            assert(db->header.free_count == db->index.freelist.size);

            minidb_header_write(db);
            minidb_index_write(&db->index);
        }
    }

    return MINIDB_OK;
}
