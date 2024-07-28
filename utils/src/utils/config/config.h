#ifndef UTILS_CONFIG_H
#define UTILS_CONFIG_H

#include <stdlib.h>
#include <stdio.h>
#include <commons/config.h>
#include <commons/string.h>
#include <unistd.h>

t_config *iniciar_lector_config(char *ruta_archivo);
void destruir_lector_config(t_config *lector_config);
char *leer_string_config(t_config *lector_config, char *propiedad);
char **leer_lista_config(t_config *lector_config, char *propiedad);
int leer_int_config(t_config *lector_config, char *propiedad);

#endif