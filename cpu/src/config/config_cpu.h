#ifndef CONFIG_CPU_H
#define CONFIG_CPU_H

#include <utils/config/config.h>
#include <string.h>

typedef struct
{
    char *ip_memoria;
    int puerto_memoria;
    int puerto_escucha_dispatch;
    int puerto_escucha_interrupt;
    char *algoritmo_tlb;
    int cantidad_entradas_tlb;
} t_config_cpu;

void iniciar_config(char *ruta_archivo);
void destruir_config(t_config_cpu *config_cpu);
t_config_cpu *leer_configs(void);

#endif
