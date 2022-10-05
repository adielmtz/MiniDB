# MiniDB
A small database engine written in C for study purposes.

## File structure

| Offset | Size | Name      | Description                                         |
|--------|------|-----------|-----------------------------------------------------|
| 0      | 8    | row_size  | The size of each row (data size).                   |
| 8      | 8    | row_count | Counts how many rows are stored in the database.    |
| 16     | 8    | max_rows  | Maximum number of rows that the database can store. |
