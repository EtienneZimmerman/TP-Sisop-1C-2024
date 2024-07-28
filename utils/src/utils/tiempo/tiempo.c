#include "tiempo.h"

void esperar(int tiempo)
{
    t_temporal *temporal = temporal_create();
    while (temporal_gettime(temporal) < tiempo)
        ;
    temporal_destroy(temporal);
}