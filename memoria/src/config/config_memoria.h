#ifndef CONFIG_MEMORIA_H
#define CONFIG_MEMORIA_H

#include <utils/config/config.h>
#include <string.h>
typedef struct
{
    int puerto_escucha;
    int tam_memoria;
    int tam_pagina;
    int retardo_respuesta;
    char *path_instrucciones;
} t_config_memoria;

void iniciar_config(char *ruta_archivo);
void destruir_config(t_config_memoria *config_memoria);
t_config_memoria *leer_configs(void);

#endif