# MiniDB

A small database engine written in C for study purposes.

## File structure

| Offset | Size | Name       | Description                                         |
|--------|------|------------|-----------------------------------------------------|
| 0      | 8    | data_size  | The size of each row (data size).                   |
| 8      | 8    | row_count  | Counts how many rows are stored in the database.    |
| 16     | 8    | free_count | The number of entries stored in the freelist index. |

## Usage

#### Define a structure

```c
typedef struct Human
{
    char name[50];
    int age;
} Human;
```

`minidb_create`: Creates a new database file and opens a connection.

```c
MiniDb db;
MiniDbState state = minidb_create(&db, "./mini.db", sizeof(Human));

if (state != MINIDB_OK) {
    printf("Error: %s\n", minidb_error_get_str(state));
}
```

`minidb_open`: Opens a connection to an existing database file.

```c
MiniDb db;
MiniDbState state = minidb_open(&db, "./mini.db");

if (state != MINIDB_OK) {
    printf("Error: %s\n", minidb_error_get_str(state));
}
```

`minidb_close`: Closes a connection to the database.

```c
MiniDb db;
MiniDbState state = minidb_open(&db, "./mini.db");

if (state != MINIDB_OK) {
    printf("Error: %s\n", minidb_error_get_str(state));
}

minidb_close(&db);
```

## Commands

### select

```c
int64_t key = 1;
Human result;

MiniDbState state = minidb_select(&db, key, &result);
if (state == MINIDB_OK) {
    printf("Name: %s\n", result.name);
    printf("Age: %d\n", result.age);
}
```

### insert

```c
int64_t key = 1;
Human result;

strncpy(result.name, "John Doe", sizeof(result.name));
result.age = 20;

MiniDbState state = minidb_insert(&db, key, &result);
if (state != MINIDB_OK) {
    printf("Error: %s\n", minidb_error_get_str(state));
}
```

### update

```c
int64_t key = 1;
Human result;

strncpy(result.name, "A dragon", sizeof(result.name));
result.age = 5000;

MiniDbState state = minidb_update(&db, key, &result);
if (state != MINIDB_OK) {
    printf("Error: %s\n", minidb_error_get_str(state));
}
```

### delete

```c
int64_t key = 1;

MiniDbState state = minidb_delete(&db, key);
if (state != MINIDB_OK) {
    printf("Error: %s\n", minidb_error_get_str(state));
}
```
