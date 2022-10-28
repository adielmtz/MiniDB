# MiniDB
A small database engine written in C for study purposes.

## File structure


| Offset | Size | Name           | Description                                         |
|--------|------|----------------|-----------------------------------------------------|
| 0      | 8    | data_size       | The size of each row (data size).                   |
| 8      | 8    | row_count      | Counts how many rows are stored in the database.    |
| 16     | 8    | free_count | The number of entries stored in the freelist index. |
