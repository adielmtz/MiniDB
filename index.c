#include "index.h"

#include <stdbool.h>
#include <stdlib.h>

void minidb_index_init(MiniDbIndex *index)
{
    btree_init(&index->search);
    btree_init(&index->freelist);
    index->fd = NULL;
}

MiniDbState minidb_index_open(MiniDbIndex *index, const char *path, int64_t row_count, int64_t freelist_count)
{
    bool is_new_file = row_count == INT64_C(0) && freelist_count == INT64_C(0);
    FILE *fd = fopen(path, is_new_file ? "w+" : "r+");
    if (is_null(fd)) {
        return MINIDB_ERROR_CANNOT_OPEN_FILE;
    }

    index->fd = fd;
    if (!is_new_file) {
        // Load index from file
        int64_t i;
        BTreeNode node;
        fseek(fd, 0, SEEK_SET);

        // search index
        for (i = 0; i < row_count; i++) {
            fread(&node.key, sizeof(node.key), 1, fd);
            fread(&node.value, sizeof(node.value), 1, fd);
            btree_insert(&index->search, node.key, node.value);
        }

        // freelist index
        for (i = 0; i < freelist_count; i++) {
            fread(&node.key, sizeof(node.key), 1, fd);
            fread(&node.value, sizeof(node.value), 1, fd);
            btree_insert(&index->freelist, node.key, node.value);
        }
    }

    return MINIDB_OK;
}

void minidb_index_close(MiniDbIndex *index)
{
    minidb_index_write(index);
    fflush(index->fd);
    fclose(index->fd);
    btree_destroy(&index->search);
    btree_destroy(&index->freelist);
}

static void minidb_index_write_node_recursive(const BTreeNode *node, FILE *fd)
{
    if (!is_null(node)) {
        minidb_index_write_node_recursive(node->left, fd);
        fwrite(&node->key, sizeof(node->key), 1, fd);
        fwrite(&node->value, sizeof(node->value), 1, fd);
        minidb_index_write_node_recursive(node->right, fd);
    }
}

void minidb_index_write(const MiniDbIndex *index)
{
    fseek(index->fd, 0, SEEK_SET);
    minidb_index_write_node_recursive(index->search.root, index->fd);
    minidb_index_write_node_recursive(index->freelist.root, index->fd);
    fflush(index->fd);
}
