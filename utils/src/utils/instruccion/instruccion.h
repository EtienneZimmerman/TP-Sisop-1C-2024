#ifndef INSTRUCCION_H
#define INSTRUCCION_H

#include <utils/estructura_de_datos.h>
#include <utils/estructura_de_datos_modulos.h>
#include <commons/collections/dictionary.h>
#include "string.h"
#include "stdint.h"
#include "stdlib.h"
#include <commons/string.h>
#include "pthread.h"

char *obtener_instruccion(t_dictionary *dict_instrucciones, uint32_t *pid, uint32_t *pc, pthread_mutex_t m_dict);
char *obtener_sentencia(char *instruccion);
id_instruccion obtener_id_instruccion(char *sentencia);
char **obtener_parametros(char *instruccion);
void restar_registro(t_registros *registros, char *registro_destino, char *registro_origen);
void sumar_registro(t_registros *registros, char *registro_destino, char *registro_origen);
void setear_registro(t_registros *registros, char *registro_destino, char *valor);
void saltar_si_no_es_cero(t_registros *registros, char *registro, char *instruccion);
void *obtener_registro(t_registros *registros, char *registro);
void liberar_params(char **params);

#endif