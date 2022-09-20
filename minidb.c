#include "minidb.h"

#include <stdio.h>
#include <string.h>

#define RETURN_CASE_AS_STRING(caseval) case caseval: return #caseval

const char* minidb_get_error_str(MiniDBError error)
{
    switch (error) {
        RETURN_CASE_AS_STRING(MINIDB_OK);
        RETURN_CASE_AS_STRING(MINIDB_ERROR_SIN_ESPACIO);
        RETURN_CASE_AS_STRING(MINIDB_ERROR_FUERA_DE_RANGO);
        RETURN_CASE_AS_STRING(MINIDB_ERROR_YA_EXISTE_TUPLA);
        RETURN_CASE_AS_STRING(MINIDB_ERROR_NCONTROL_INCORRECTO);
        RETURN_CASE_AS_STRING(MINIDB_ERROR_TUPLA_NO_ENCONTRADA);
        default: return "MINIDB_ERROR_DESCONOCIDO";
    }
}

void minidb_nueva(MiniDB* db, char* ruta, size_t size)
{
    FILE* fd;
    fopen_s(&fd, ruta, "w+");
    db->file = NULL;
    db->size = 0;

    if (fd != NULL) {
        char zeros[sizeof(Alumno)];
        memset(zeros, 0, sizeof(zeros));
        size_t written;

        for (size_t i = 0; i < size; i += written) {
            written = fwrite(zeros, sizeof(char), sizeof(zeros), fd);
            if (written == 0) {
                break;
            }
        }

        fflush(fd);
        db->file = fd;
        db->size = size;
    }
}

void minidb_abrir(MiniDB* db, char* ruta)
{
    FILE* fd;
    fopen_s(&fd, ruta, "r+");
    db->file = NULL;
    db->size = 0;

    if (fd != NULL) {
        // obtener el tamaño del archivo
        fseek(fd, 0, SEEK_END);
        db->file = fd;
        db->size = ftell(fd);
    }
}

void minidb_cerrar(MiniDB* db)
{
    if (db != NULL && db->file != NULL) {
        fflush(db->file);
        fclose(db->file);
        db->file = NULL;
        db->size = 0;
    }
}

void minidb_resize(MiniDB* db, size_t new_size)
{
    // Incrementar el tamaño de la base de datos
    char zeros[sizeof(Alumno)];
    memset(zeros, 0, sizeof(zeros));
    size_t remaining = new_size - db->size;
    size_t written;

    fseek(db->file, 0, SEEK_END);
    for (size_t i = 0; i < remaining; i += written) {
        written = fwrite(zeros, sizeof(char), sizeof(zeros), db->file);
        if (written == 0) {
            break;
        }
    }

    fflush(db->file);
    db->size = new_size;
}

MiniDBError minidb_select(MiniDB* db, int ncontrol, Alumno* resultado)
{
    if (ncontrol == 0) {
        return MINIDB_ERROR_NCONTROL_INCORRECTO;
    }

    size_t pos = sizeof(Alumno) * (ncontrol - 1);
    if (ncontrol <= 0 || pos >= db->size) {
        return MINIDB_ERROR_FUERA_DE_RANGO;
    }

    fseek(db->file, pos, SEEK_SET);
    fread(resultado, sizeof(Alumno), 1, db->file);
    fflush(db->file);
    return resultado->ncontrol == 0 ? MINIDB_ERROR_TUPLA_NO_ENCONTRADA : MINIDB_OK;
}

MiniDBError minidb_insert(MiniDB* db, Alumno* alumno)
{
    if (alumno->ncontrol == 0) {
        return MINIDB_ERROR_NCONTROL_INCORRECTO;
    }

    size_t pos = sizeof(Alumno) * (alumno->ncontrol - 1);
    if (pos >= db->size) {
        return MINIDB_ERROR_SIN_ESPACIO;
    }

    // Revisar si ya existe una tupla en esa posición
    Alumno check;
    MiniDBError err = minidb_select(db, alumno->ncontrol, &check);
    if (err == MINIDB_OK && check.ncontrol > 0) {
        return MINIDB_ERROR_YA_EXISTE_TUPLA;
    }

    // Escribir la tupla ya que se confirmó que no existe
    fseek(db->file, pos, SEEK_SET);
    fwrite(alumno, sizeof(Alumno), 1, db->file);
    fflush(db->file);
    return MINIDB_OK;
}

MiniDBError minidb_update(MiniDB* db, Alumno* alumno)
{
    if (alumno->ncontrol == 0) {
        return MINIDB_ERROR_NCONTROL_INCORRECTO;
    }

    long pos = (long)sizeof(Alumno) * (alumno->ncontrol - 1);
    if (pos >= db->size) {
        return MINIDB_ERROR_SIN_ESPACIO;
    }

    // Revisar que la tupla exista
    Alumno check;
    MiniDBError err = minidb_select(db, alumno->ncontrol, &check);
    if (err != MINIDB_OK) {
        return MINIDB_ERROR_TUPLA_NO_ENCONTRADA;
    }

    // Escribir la tupla ya que se confirmó que no existe
    fseek(db->file, pos, SEEK_SET);
    fwrite(alumno, sizeof(Alumno), 1, db->file);
    fflush(db->file);
    return MINIDB_OK;
}

MiniDBError minidb_delete(MiniDB* db, int ncontrol)
{
    if (ncontrol == 0) {
        return MINIDB_ERROR_NCONTROL_INCORRECTO;
    }

    long pos = (long)sizeof(Alumno) * (ncontrol - 1);
    if (pos >= db->size) {
        return MINIDB_ERROR_SIN_ESPACIO;
    }

    char zeros[sizeof(Alumno)];
    memset(zeros, 0, sizeof(zeros));
    fseek(db->file, pos, SEEK_SET);
    fwrite(zeros, sizeof(char), sizeof(zeros), db->file);
    fflush(db->file);
    return MINIDB_OK;
}
