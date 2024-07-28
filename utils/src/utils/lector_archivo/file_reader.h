#ifndef FILE_READER_H
#define FILE_READER_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <stdlib.h>

t_list *leer_archivo(char *path, char *nombre_de_archivo);
t_list *listar_archivos_en_directorio(char *path);

#endif