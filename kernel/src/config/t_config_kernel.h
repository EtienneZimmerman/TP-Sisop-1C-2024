#ifndef T_CONFIG_KERNEL_H
#define T_CONFIG_KERNEL_H
typedef struct
{
    int puerto_escucha;
    char *ip_memoria;
    int puerto_memoria;
    char *ip_cpu;
    int puerto_cpu_dispatch;
    int puerto_cpu_interrupt;
    char *algoritmo_planificacion;
    int quantum;
    char **recursos;
    char **instancias_recursos;
    int grado_multiprogramacion;
} t_config_kernel;
#endif