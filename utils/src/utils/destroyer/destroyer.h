#ifndef DESTROYER_H
#define DESTROYER_H

#include <commons/collections/dictionary.h>
#include <commons/collections/list.h>
#include <stdlib.h>
#include "utils/estructura_de_datos_modulos.h"

void element_destroyer(void *str);
void process_destroyer(void *paginas_del_proceso);
void list_destroyer(void *list);
void interface_destroyer(void *interfaz);
void interruption_destroyer(void *inter);

#endif