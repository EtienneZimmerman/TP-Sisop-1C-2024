#include <config/config_kernel.h>
#include <main.h>

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

void destruir_config(t_config_kernel *config_kernel)
{
    free(config_kernel->ip_cpu);
    free(config_kernel->ip_memoria);
    free(config_kernel->algoritmo_planificacion);
    char **rec = config_kernel->recursos;
    for (; *config_kernel->recursos != NULL; config_kernel->recursos++)
    {
        free(*config_kernel->recursos);
    }
    free(rec);
    char **rec_inst = config_kernel->instancias_recursos;
    for (; *config_kernel->instancias_recursos != NULL; config_kernel->instancias_recursos++)
    {
        free(*config_kernel->instancias_recursos);
    }
    free(rec_inst);
    free(config_kernel);
}

t_config_kernel *leer_configs()
{
    t_config_kernel *config_kernel = malloc(sizeof(t_config_kernel));
    config_kernel->puerto_escucha = leer_int_config(config, "PUERTO_ESCUCHA");
    config_kernel->puerto_memoria = leer_int_config(config, "PUERTO_MEMORIA");
    config_kernel->ip_cpu = leer_string_config(ip_config, "IP_CPU");
    config_kernel->ip_memoria = leer_string_config(ip_config, "IP_MEMORIA");
    config_kernel->algoritmo_planificacion = leer_string_config(config, "ALGORITMO_PLANIFICACION");
    config_kernel->puerto_cpu_dispatch = leer_int_config(config, "PUERTO_CPU_DISPATCH");
    config_kernel->puerto_cpu_interrupt = leer_int_config(config, "PUERTO_CPU_INTERRUPT");
    config_kernel->quantum = leer_int_config(config, "QUANTUM");
    config_kernel->grado_multiprogramacion = leer_int_config(config, "GRADO_MULTIPROGRAMACION");
    config_kernel->recursos = leer_lista_config(config, "RECURSOS");
    config_kernel->instancias_recursos = leer_lista_config(config, "INSTANCIAS_RECURSOS");

    return config_kernel;
}

int cantidad_recursos(char **recursos)
{
    int n = 0;
    for (int i = 0; recursos[i] != NULL; i++)
        n++;
    return n;
}