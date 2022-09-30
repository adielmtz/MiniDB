#include "minidb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define FATAL(retval)                                          \
do {                                                           \
    if ((retval) != MINIDB_OK) {                               \
        printf("Error: %s\n\n", minidb_get_error_str(retval)); \
        minidb_close(&db);                                     \
        exit(1);                                               \
    }                                                          \
} while (0)

#define prompt_string(text, outbuff)            \
do {                                            \
    printf((text));                             \
    fflush(stdout);                             \
    fgets((outbuff), sizeof((outbuff)), stdin); \
    outbuff[strlen((outbuff)) - 1] = '\0';      \
} while (0)

#define prompt_int(text, intvar)    \
do {                                \
    char buff[20];                  \
    prompt_string((text), buff);    \
    sscanf_s(buff, "%d", (intvar)); \
} while (0)

#define prompt_float(text, floatvar)  \
do {                                  \
    char buff[20];                    \
    prompt_string((text), buff);      \
    sscanf_s(buff, "%f", (floatvar)); \
} while (0)

typedef struct Alumno
{
    char nombre[16];
    int32_t ncontrol;
    float promedio;
} Alumno;

static void print_pretty_table(const Alumno *alumno)
{
    puts("+------------------+------------+----------+");
    puts("|      Nombre      | N. Control | Promedio |");
    puts("+------------------+------------+----------+");
    printf("| %-16s | % 10d | %8.1f |\n", alumno->nombre, alumno->ncontrol, alumno->promedio);
    puts("+------------------+------------+----------+");
}

static void select_dump_all_recursive(MiniDb *db, BinaryTreeNode *node)
{
    if (node != NULL) {
        select_dump_all_recursive(db, node->left);

        Alumno alumno;
        MiniDbError error = minidb_select(db, node->key, &alumno);
        if (error != MINIDB_OK) {
            printf("Panik! %s\n", minidb_error_get_str(error));
            return;
        }

        print_pretty_table(&alumno);
        puts("");

        select_dump_all_recursive(db, node->right);
    }
}

int main(void)
{
    MiniDb db;
    MiniDbError error;

    char command[255];
    char archivo[255];

    // Preamble
    printf("MiniDb x%zu bits\n\n", sizeof(int*) * 8);

    while (1) {
        prompt_string("MiniDb > ", command);

        if (strcmp(command, "salir") == 0 || strcmp(command, "exit") == 0) {
            return 0;
        } else if (strcmp(command, "nueva") == 0) {
            prompt_string("Physical name: ", archivo);

            int n_paginas;
            prompt_int("Size: ", &n_paginas);

            printf("Creando base de datos... ");
            fflush(stdout);
            minidb_create(&db, archivo, sizeof(Alumno), n_paginas);
            puts("Ok!\n");
            fflush(stdout);
        } else if (strcmp(command, "abrir") == 0) {
            prompt_string("Physical name: ", archivo);
            minidb_open(&db, archivo);
            if (db.file == NULL) {
                printf("Error fatal: archivo no encontrado.\n");
                exit(1);
            }

            puts("La base de datos se ha abierto correctamente.\n");
        } else {
            printf(
                    "Error: commando \"%s\" no es valido.\n"
                    "Debe crear una nueva base de datos utilizando el comando 'nueva'\n"
                    "o abrir una base de datos existente con el comando 'abrir'.\n"
                    "Para terminar el programa, escriba el comando 'salir'.\n\n",
                    command
            );

            continue;
        }

        break;
    }

    const char *nombre_db = archivo;
    for (int64_t i = (int64_t) strlen(archivo) - 1; i >= 0; i--) {
        if (archivo[i] == '\\' || archivo[i] == '/') {
            nombre_db = archivo + i + 1;
            break;
        }
    }

    while (1) {
        printf("MiniDb\\%s > ", nombre_db);
        fflush(stdout);
        fgets(command, sizeof(command), stdin);
        command[strlen(command) - 1] = '\0';

        Alumno alumno;
        if (strcmp(command, "salir") == 0 || strcmp(command, "exit") == 0) {
            break;
        } else if (strcmp(command, "dbinfo") == 0) {
            puts("== DATABASE INFO ==");
            printf("Physical name : %s\n", archivo);
            printf("Row Size      : %zu\n", db.row_size);
            printf("Row Count     : %zu\n", db.row_count);
            printf("Max Row Count : %zu\n", db.max_rows);
            printf("Db Data Size  : %zu bytes\n", db.max_rows * db.row_size);
            puts("");
        } else if (strcmp(command, "select") == 0) {
            int ncontrol;
            prompt_int("N. control: ", &ncontrol);
            puts(""); // A�adir espaciado

            error = minidb_select(&db, ncontrol, &alumno);
            if (error != MINIDB_OK) {
                printf("Error: %s\n\n", minidb_error_get_str(error));
                continue;
            }

            print_pretty_table(&alumno);
            printf("\n");
        } else if (strcmp(command, "select *") == 0) {
            select_dump_all_recursive(&db, db.index.root);
        } else if (strcmp(command, "insert") == 0) {
            prompt_string("Nombre[16]: ", alumno.nombre);
            prompt_int("N. control: ", &alumno.ncontrol);
            prompt_float("Promedio:   ", &alumno.promedio);

            error = minidb_insert(&db, alumno.ncontrol, &alumno);
            if (error != MINIDB_OK) {
                printf("Error: %s\n\n", minidb_error_get_str(error));
                continue;
            }

            puts("Tupla insertada correctamente\n");
        } else if (strcmp(command, "update") == 0) {
            prompt_string("Nombre[16]: ", alumno.nombre);
            prompt_int("N. control: ", &alumno.ncontrol);
            prompt_float("Promedio:   ", &alumno.promedio);

            error = minidb_update(&db, alumno.ncontrol, &alumno);
            if (error != MINIDB_OK) {
                printf("Error: %s\n\n", minidb_error_get_str(error));
                continue;
            }

            puts("Tupla actualizada correctamente\n");
        } else if (strcmp(command, "delete") == 0) {
            int ncontrol;
            prompt_int("N. control: ", &ncontrol);

            error = minidb_delete(&db, ncontrol);
            if (error != MINIDB_OK) {
                printf("Error: %s\n\n", minidb_error_get_str(error));
                continue;
            }

            puts("Tupla eliminada correctamente\n");
        } else if (strcmp(command, "resize") == 0) {
            int nuevo_tamano;
            prompt_int("Size: ", &nuevo_tamano);

            if (minidb_resize(&db, nuevo_tamano)) {
                puts("Base de datos redimensionada correctamente.\n");
            } else {
                puts("Error: no se puede reducir el tamano la base de datos.\n");
            }
        } else {
            if (command[0] != '\0') {
                puts("Error: comando no reconocido.\n");
            }
        }
    }

    minidb_close(&db);
    printf("Programa finalizado\n");
    return 0;
}
