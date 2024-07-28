#include "destroyer.h"
#include <stdlib.h>

void element_destroyer(void *str)
{
    free(str);
}

void process_destroyer(void *paginas_del_proceso)
{
    t_list *list_paginas = (t_list *)paginas_del_proceso;
    if (list_size(list_paginas) > 0)
    {
        list_destroy_and_destroy_elements(list_paginas, element_destroyer);
    }
    else
    {
        list_destroy(list_paginas);
    }
}

void list_destroyer(void *list_void)
{
    t_list *list = (t_list *)list_void;

    if (list_size(list) > 0)
    {
        list_destroy_and_destroy_elements(list, element_destroyer);
    }
    else
    {
        list_destroy(list);
    }
}

void interruption_destroyer(void *inter_void)
{
    t_interrupcion *inter = (t_interrupcion *)inter_void;
    free(inter->pid);
    free(inter);
}

void interface_destroyer(void *interfaz)
{
    t_interfaz *inter = (t_interfaz *)interfaz;
    list_destroy_and_destroy_elements(inter->lista_bloqueados, element_destroyer);
    free(inter->nombre_interfaz);
    free(inter->socket_cliente_io);
    free(inter->tipo_interfaz);
    free(inter);
}