#include <utils/config/config.h>

t_config *iniciar_lector_config(char *ruta_archivo)
{
    t_config *nuevo_config;
    nuevo_config = config_create(ruta_archivo);
    return nuevo_config;
}

void destruir_lector_config(t_config *config)
{
    config_destroy(config);
}

char **leer_lista_config(t_config *config, char *key)
{
    char **array = config_get_array_value(config, key);
    return array;
}

char *leer_string_config(t_config *config, char *key)
{
    return config_get_string_value(config, key);
}

int leer_int_config(t_config *config, char *key)
{
    int valor = config_get_int_value(config, key);
    return valor;
}
