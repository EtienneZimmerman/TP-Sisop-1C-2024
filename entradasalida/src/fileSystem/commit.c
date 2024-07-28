// Funciones para actualizar los archivos con los cambios realizados en el FS.

#include "entradasalida.h"

int actualizar_bitmap()
{
    char *path_bitmap = string_from_format("%s/bitmap.dat", config_io->path_base_dialfs);
    FILE *file = fopen(path_bitmap, "wb+");
    if (file == NULL)
    {
        log_error(logger_io, "No se pudo abrir el archivo bitmap.dat.");
        return 1;
    }
    log_trace(logger_io, "Actualizando archivo bitmap.dat");
    fwrite(bitmap->bitarray, 1, bitmap->size, file);
    fclose(file);
    free(path_bitmap);
    return 0;
}

int actualizar_bloques()
{
    char *path_bloques = string_from_format("%s/bloques.dat", config_io->path_base_dialfs);
    FILE *file = fopen(path_bloques, "wb+");
    if (file == NULL)
    {
        log_error(logger_io, "No se pudo abrir el archivo bloques.dat.");
        return 1;
    }
    log_trace(logger_io, "Actualizando archivo bloques.dat");
    int bloques_size = config_io->block_count * config_io->block_size;
    fwrite(bloques, 1, bloques_size, file);
    fclose(file);
    free(path_bloques);
    return 0;
}

int actualizar_archivo(char *nombre_archivo)
{
    char *path_archivo = string_from_format("%s/%s", config_io->path_base_dialfs, nombre_archivo);
    FILE *file = fopen(path_archivo, "wb+");
    if (file == NULL)
    {
        log_error(logger_io, "No se pudo abrir el archivo %s.", nombre_archivo);
        return 1;
    }
    log_trace(logger_io, "Actualizando archivo %s", nombre_archivo);

    char *datos = malloc(100);
    int size = 0;
    formatear_datos_archivo(nombre_archivo, datos, &size);

    fwrite(datos, 1, size, file);
    fclose(file);

    free(datos);
    free(path_archivo);
    return 0;
}

void formatear_datos_archivo(char *nombre_archivo, char *datos, int *size)
{
    t_archivo *archivo = dictionary_get(archivos, nombre_archivo);

    char *bloque_inicial = string_itoa(archivo->bloque_inicial);
    char *tam_archivo = string_itoa(archivo->tam_archivo);

    strcpy(datos, "BLOQUE_INICIAL=");
    strcat(datos, bloque_inicial);
    strcat(datos, "\n");
    strcat(datos, "TAM_ARCHIVO=");
    strcat(datos, tam_archivo);

    *size = strlen(datos);

    free(bloque_inicial);
    free(tam_archivo);
}

int actualizar_archivos()
{
    t_list *nombres_archivos = dictionary_keys(archivos);
    for (int i = 0; i < list_size(nombres_archivos); i++)
    {
        char *nombre_archivo = list_get(nombres_archivos, i);
        actualizar_archivo(nombre_archivo);
    }
    list_destroy(nombres_archivos);
    return 0;
}

void eliminar_archivo_txt(char *nombre_archivo)
{
    char *path_archivo = string_from_format("%s/%s", config_io->path_base_dialfs, nombre_archivo);
    if (remove(path_archivo) == 0)
    {
        log_info(logger_io, "Archivo %s eliminado.", nombre_archivo);
    }
    else
    {
        log_error(logger_io, "No se pudo eliminar el archivo %s.", nombre_archivo);
    }
    free(path_archivo);
}
