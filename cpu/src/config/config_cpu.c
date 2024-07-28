#include <config/config_cpu.h>

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

void destruir_config(t_config_cpu *config_cpu)
{
    free(config_cpu);
    destruir_lector_config(config);
}

t_config_cpu *leer_configs()
{
    t_config_cpu *config_cpu = malloc(sizeof(t_config_cpu));
    config_cpu->puerto_memoria = leer_int_config(config, "PUERTO_MEMORIA");
    config_cpu->algoritmo_tlb = leer_string_config(config, "ALGORITMO_TLB");
    config_cpu->ip_memoria = leer_string_config(ip_config, "IP_MEMORIA");
    config_cpu->puerto_escucha_dispatch = leer_int_config(config, "PUERTO_ESCUCHA_DISPATCH");
    config_cpu->puerto_escucha_interrupt = leer_int_config(config, "PUERTO_ESCUCHA_INTERRUPT");
    config_cpu->cantidad_entradas_tlb = leer_int_config(config, "CANTIDAD_ENTRADAS_TLB");
    return config_cpu;
}
