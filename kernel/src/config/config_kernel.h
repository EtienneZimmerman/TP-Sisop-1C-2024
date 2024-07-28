#ifndef CONFIG_KERNEL_H
#define CONFIG_KERNEL_H

#include <utils/config/config.h>
#include "t_config_kernel.h"

void iniciar_config(char *ruta_archivo);
void destruir_config(t_config_kernel *config_kernel);
t_config_kernel *leer_configs(void);
int cantidad_recursos(char** recursos);

#endif