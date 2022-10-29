#include "minidb.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifndef is_null
#define is_null(ptr) ((ptr) == NULL)
#endif

#define MINIDB_INDEX_SUFFIX "-index"
#define MINIDB_HEADER_SIZE (sizeof(((MiniDb*)0)->header))
#define RETURN_CASE_AS_STRING(caseval) case caseval: return #caseval
#define SWITCH_UNREACHABLE_DEFAULT_CASE() default: assert(0)

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

static void minidb_header_write(const MiniDb *db)
{
    fseek(db->data_file, 0, SEEK_SET);
    fwrite(&db->header, MINIDB_HEADER_SIZE, 1, db->data_file);
    fflush(db->data_file);
}

static void index_write_node_recurse(const MiniDb *db, const BinaryTreeNode *node)
{
    if (!is_null(node)) {
        index_write_node_recurse(db, node->left);
        fwrite(&node->data, BINARYTREE_NODE_DATA_SIZE, 1, db->index_file);
        index_write_node_recurse(db, node->right);
    }
}

static void minidb_index_write(const MiniDb *db)
{
    fseek(db->index_file, 0, SEEK_SET);
    index_write_node_recurse(db, db->index.root);
    index_write_node_recurse(db, db->freelist.root);
    fflush(db->index_file);
}

static void minidb_initialize_empty(MiniDb *db)
{
    db->header.data_size = 0;
    db->header.row_count = 0;
    db->header.free_count = 0;
    db->data_file = NULL;
    db->index_file = NULL;
    binarytree_init(&db->index);
    binarytree_init(&db->freelist);
}

MiniDbState minidb_create(MiniDb *db, const char *path, int64_t data_size)
{
    minidb_initialize_empty(db);
    db->data_file = fopen(path, "w+");
    if (is_null(db->data_file)) {
        return MINIDB_ERROR;
    }

    char index_path[1024];
    minidb_build_index_file_path(path, index_path, sizeof(index_path));
    db->index_file = fopen(index_path, "w+");

    if (is_null(db->index_file)) {
        fclose(db->data_file);
        db->data_file = NULL;
        return MINIDB_ERROR;
    }

    db->header.data_size = data_size;
    minidb_header_write(db);
    return MINIDB_OK;
}

MiniDbState minidb_open(MiniDb *db, const char *path)
{
    minidb_initialize_empty(db);
    db->data_file = fopen(path, "r+");

    if (is_null(db->data_file)) {
        return MINIDB_ERROR;
    }

    char index_path[1024];
    minidb_build_index_file_path(path, index_path, sizeof(index_path));
    db->index_file = fopen(index_path, "r+");

    if (is_null(db->index_file)) {
        fclose(db->data_file);
        db->data_file = NULL;
        return MINIDB_ERROR;
    }

    // Read database header
    fread(&db->header, MINIDB_HEADER_SIZE, 1, db->data_file);

    // Parse & load index
    if (db->header.row_count > 0) {
        BinaryTreeNode node;
        fseek(db->index_file, 0, SEEK_SET);

        // parse index
        for (int64_t i = 0; i < db->header.row_count; i++) {
            fread(&node.data, BINARYTREE_NODE_DATA_SIZE, 1, db->index_file);
            binarytree_insert(&db->index, node.data.key, node.data.address);
        }

        // parse freelist
        for (int64_t i = 0; i < db->header.free_count; i++) {
            fread(&node.data, BINARYTREE_NODE_DATA_SIZE, 1, db->index_file);
            binarytree_insert(&db->freelist, node.data.key, node.data.address);
        }
    }

    return MINIDB_OK;
}

void minidb_close(MiniDb *db)
{
    if (!is_null(db) && !is_null(db->data_file)) {
        minidb_header_write(db);
        minidb_index_write(db);
        binarytree_destroy(&db->index);
        binarytree_destroy(&db->freelist);
        fflush(db->data_file);
        fflush(db->index_file);
        fclose(db->data_file);
        fclose(db->index_file);
        minidb_initialize_empty(db);
    }
}

MiniDbState minidb_select(const MiniDb *db, int64_t key, void *result)
{
    BinaryTreeNode *node = binarytree_search(&db->index, key);
    if (is_null(node)) {
        return MINIDB_ERROR_ROW_NOT_FOUND;
    }

    fseek(db->data_file, node->data.address, SEEK_SET);
    fread(result, db->header.data_size, 1, db->data_file);
    return MINIDB_OK;
}

static void minidb_index_traverse(const MiniDb *db, BinaryTreeNode *current, void *result, void (*callback)(int64_t, void *))
{
    if (!is_null(current)) {
        minidb_index_traverse(db, current->left, result, callback);
        minidb_select(db, current->data.key, result);
        callback(current->data.key, result);
        minidb_index_traverse(db, current->right, result, callback);
    }
}

MiniDbState minidb_select_all(const MiniDb *db, void (*callback)(int64_t, void *))
{
    void *result = malloc(db->header.data_size);
    minidb_index_traverse(db, db->index.root, result, callback);
    free(result);
    return MINIDB_OK;
}

/**
 * Finds the smallest free address available. Returns NULL if the freelist index is empty.
 */
static const BinaryTreeNode *minidb_freelist_find_node(const MiniDb *db)
{
    const BinaryTreeNode *node = db->freelist.root;
    if (!is_null(node)) {
        while (!is_null(node->left)) {
            node = node->left;
        }
    }

    return node;
}

MiniDbState minidb_insert(MiniDb *db, int64_t key, void *data)
{
    if (binarytree_contains(&db->index, key)) {
        return MINIDB_ERROR_DUPLICATED_KEY_VIOLATION;
    }

    const BinaryTreeNode *free_node = minidb_freelist_find_node(db);
    int64_t address;

    if (is_null(free_node)) {
        address = MINIDB_HEADER_SIZE + db->header.data_size * db->header.row_count;
    } else {
        address = free_node->data.address;
        binarytree_remove(&db->freelist, free_node->data.key, NULL);
        db->header.free_count--;
        assert(db->header.free_count == db->freelist.size);
    }

    fseek(db->data_file, address, SEEK_SET);
    fwrite(data, db->header.data_size, 1, db->data_file);
    db->header.row_count++;

    binarytree_insert(&db->index, key, address);
    minidb_header_write(db);
    minidb_index_write(db);
    return MINIDB_OK;
}

MiniDbState minidb_update(MiniDb *db, int64_t key, void *data)
{
    BinaryTreeNode *node = binarytree_search(&db->index, key);
    if (is_null(node)) {
        return MINIDB_ERROR_ROW_NOT_FOUND;
    }

    fseek(db->data_file, node->data.address, SEEK_SET);
    fwrite(data, db->header.data_size, 1, db->data_file);
    fflush(db->data_file);
    return MINIDB_OK;
}

MiniDbState minidb_delete(MiniDb *db, int64_t key)
{
    if (db->header.row_count > 0) {
        int64_t old_address;
        bool removed = binarytree_remove(&db->index, key, &old_address);
        if (removed) {
            db->header.row_count--;
            assert(db->header.row_count == db->index.size);

            binarytree_insert(&db->freelist, old_address, old_address);
            db->header.free_count++;
            assert(db->header.free_count == db->freelist.size);

            minidb_header_write(db);
            minidb_index_write(db);
        }
    }

    return MINIDB_OK;
}
