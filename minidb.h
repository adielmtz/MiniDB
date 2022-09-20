#pragma once

#include <stdio.h>

typedef struct Alumno
{
    char nombre[16];
    int ncontrol;
    float promedio;
} Alumno;

typedef struct MiniDB
{
    FILE* file;
    size_t size;
} MiniDB;

typedef enum MiniDBError
{
    MINIDB_OK,
    MINIDB_ERROR_SIN_ESPACIO,
    MINIDB_ERROR_FUERA_DE_RANGO,
    MINIDB_ERROR_YA_EXISTE_TUPLA,
    MINIDB_ERROR_NCONTROL_INCORRECTO,
    MINIDB_ERROR_TUPLA_NO_ENCONTRADA,
} MiniDBError;

const char* minidb_get_error_str(MiniDBError error);

void minidb_nueva(MiniDB* db, char* ruta, size_t size);
void minidb_abrir(MiniDB* db, char* ruta);
void minidb_cerrar(MiniDB* db);
void minidb_resize(MiniDB* db, size_t new_size);

MiniDBError minidb_select(MiniDB* db, int ncontrol, Alumno* resultado);
MiniDBError minidb_insert(MiniDB* db, Alumno* alumno);
MiniDBError minidb_update(MiniDB* db, Alumno* alumno);
MiniDBError minidb_delete(MiniDB* db, int ncontrol);
