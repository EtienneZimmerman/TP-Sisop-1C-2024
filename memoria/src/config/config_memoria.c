#include <config/config_memoria.h>

t_config *config;

void iniciar_config(char *ruta_archivo)
{
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

void destruir_config(t_config_memoria *config_memoria)
{
    free(config_memoria);
    destruir_lector_config(config);
}

t_config_memoria *leer_configs()
{
    t_config_memoria *config_memoria = malloc(sizeof(t_config_memoria));
    config_memoria->puerto_escucha = leer_int_config(config, "PUERTO_ESCUCHA");
    config_memoria->path_instrucciones = leer_string_config(config, "PATH_INSTRUCCIONES");
    config_memoria->tam_memoria = leer_int_config(config, "TAM_MEMORIA");
    config_memoria->tam_pagina = leer_int_config(config, "TAM_PAGINA");
    config_memoria->retardo_respuesta = leer_int_config(config, "RETARDO_RESPUESTA");
    return config_memoria;
}