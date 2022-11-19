#pragma once

#include "minidb.h"
#include "btree.h"
#include <stdio.h>
#include <stdint.h>

typedef struct MiniDbIndex
{
    BTree search;
    BTree freelist;
    FILE *fd;
} MiniDbIndex;

void minidb_index_init(MiniDbIndex *index);

MiniDbState minidb_index_open(MiniDbIndex *index, const char *path, int64_t row_count, int64_t freelist_count);

void minidb_index_close(MiniDbIndex *index);

void minidb_index_write(const MiniDbIndex *index);
