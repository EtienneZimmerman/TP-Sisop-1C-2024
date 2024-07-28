#include "entradasalida.h"

t_config *config;
t_config *ip_config;

void iniciar_config(char *ruta_archivo)
{
    ip_config = iniciar_lector_config("ip.config");
    char **split = string_split(ruta_archivo, "/");
    if (strcmp(*split, ruta_archivo) != 0)
    {

        char **split2 = split;
        if (*split != NULL)
        {
            free(*split);
            split++;
        }
        char *path_append = string_new();
        char *last_read;
        while (*split != NULL)
        {
            last_read = malloc(strlen(*split) + 1);
            memcpy(last_read, *split, strlen(*split) + 1);
            string_append(&path_append, last_read);
            free(*split);
            free(last_read);
            split++;
            last_read = malloc(strlen(*split) + 1);
            memcpy(last_read, *split, strlen(*split) + 1);
            free(*split);
            split++;
        }
        string_append(&path_append, "/");

        chdir(path_append);
        free(path_append);
        config = iniciar_lector_config(last_read);
        chdir("../");
        free(last_read);
        free(split2);
    }
    else
    {
        config = iniciar_lector_config(ruta_archivo);
        free(*split);
        free(split);
    }
}

void destruir_config(t_config_io *config_io)
{
    free(config_io);
    destruir_lector_config(config);
}

t_config_io *leer_configs()
{
    t_config_io *config_io = malloc(sizeof(t_config_io));
    config_io->tipo_interfaz = leer_string_config(config, "TIPO_INTERFAZ");
    config_io->tiempo_unidad_trabajo = leer_int_config(config, "TIEMPO_UNIDAD_TRABAJO");
    config_io->ip_kernel = leer_string_config(ip_config, "IP_KERNEL");
    config_io->puerto_kernel = leer_int_config(config, "PUERTO_KERNEL");
    config_io->ip_memoria = leer_string_config(ip_config, "IP_MEMORIA");
    config_io->puerto_memoria = leer_int_config(config, "PUERTO_MEMORIA");
    config_io->path_base_dialfs = leer_string_config(config, "PATH_BASE_DIALFS");
    config_io->block_size = leer_int_config(config, "BLOCK_SIZE");
    config_io->block_count = leer_int_config(config, "BLOCK_COUNT");
    config_io->retraso_compactacion = leer_int_config(config, "RETRASO_COMPACTACION");
    return config_io;
}

// ------------------------------------------------------------------------------------------
// -- manejo de archivos --
// -----------------------------------------------------------------------------------------

t_archivo *leer_config_archivo(char *path_archivo)
{
    char *path = string_from_format("%s/%s", config_io->path_base_dialfs, path_archivo);
    t_config *config_archivo = iniciar_lector_config(path);
    t_archivo *archivo = malloc(sizeof(t_archivo));
    archivo->bloque_inicial = leer_int_config(config_archivo, "BLOQUE_INICIAL");
    archivo->tam_archivo = leer_int_config(config_archivo, "TAM_ARCHIVO");
    destruir_lector_config(config_archivo);
    free(path);
    return archivo;
}