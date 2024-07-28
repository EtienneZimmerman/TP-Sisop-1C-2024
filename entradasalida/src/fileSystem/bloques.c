#include "entradasalida.h"

int inicializar_bloques()
{
    char *path_bloques = string_from_format("%s/bloques.dat", config_io->path_base_dialfs);
    int cant_bloques = config_io->block_count;
    int size = cant_bloques * config_io->block_size;
    bloques = malloc(size);
    memset(bloques, 0, size);

    FILE *file = fopen(path_bloques, "rb+");

    if (file == NULL) // Si el archivo no existe, lo creo.
    {
        file = fopen(path_bloques, "wb+");
        if (file == NULL)
        {
            log_error(logger_io, "No se pudo abrir el archivo bloques.dat.");
            free(bloques);
            return 1;
        }
        fwrite(bloques, 1, size, file);
    }
    else // Si el archivo existe, cargo los bloques.
    {
        fread(bloques, 1, size, file);
    }

    fclose(file);

    free(path_bloques);

    return 0;
}

int inicializar_archivos()
{
    archivos = dictionary_create();
    // Encontrar todos los archivos en el FS y cargarlos en el diccionario.
    t_list *lista_archivos = listar_archivos_en_directorio(config_io->path_base_dialfs);
    for (int i = 0; i < lista_archivos->elements_count; i++)
    {
        char *nombre_archivo = list_get(lista_archivos, i);
        t_archivo *archivo = leer_config_archivo(nombre_archivo);
        dictionary_put(archivos, nombre_archivo, archivo);
    }

    // Liberar memoria de la lista de archivos.
    list_destroy_and_destroy_elements(lista_archivos, free);

    return 0;
}