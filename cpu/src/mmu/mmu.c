#include "cpu.h"

int tam_tlb;
int ultimo_index;
char *algo_tlb;

void iniciar_tlb()
{
    tlb = list_create();
    tam_tlb = config_cpu->cantidad_entradas_tlb;
    algo_tlb = config_cpu->algoritmo_tlb;
    ultimo_index = 0;
    if (strcmp(algo_tlb, "LRU") == 0)
    {
        clock_ = temporal_create();
    }
    log_info(logger_cpu, "CREAMOS LA TLB");
}

int get_index_for_lru()
{
    int least_used = temporal_gettime(clock_);
    int index = -1;
    for (int i = 0; i < list_size(tlb); i++)
    {
        t_registro_tlb *tlb_rec = list_get(tlb, i);

        if (tlb_rec->last_access < least_used)
        {
            least_used = tlb_rec->last_access;
            index = i;
        }
    }
    return index;
}

void insertar_en_tlb(t_registro_tlb *registro)
{
    int size_tlb = list_size(tlb);
    log_info(logger_cpu, "tam_tlb: %d", size_tlb);
    if (strcmp("FIFO", algo_tlb) == 0 && tam_tlb > 0)
    {
        if (ultimo_index == tam_tlb)
        {
            ultimo_index = 0;
        }
        registro->last_access = 0;
        log_info(logger_cpu, "Tamaño lista: %d, tamaño_tlb: %d", size_tlb, tam_tlb);
        if (size_tlb < tam_tlb)
        {
            list_add(tlb, registro);
        }
        else if (size_tlb == tam_tlb)
        {
            t_registro_tlb *old = list_remove(tlb, ultimo_index);
            free(old);
            list_add_in_index(tlb, ultimo_index, registro);
            ultimo_index++;
        }
        else
        {
            log_info(logger_cpu, "WTF no deberia pasar esto nunca.");
        }
    }
    else if (strcmp("LRU", algo_tlb) == 0 && tam_tlb > 0)
    {
        if (size_tlb < tam_tlb)
        {
            registro->last_access = temporal_gettime(clock_);
            list_add(tlb, registro);
        }
        else if (size_tlb == tam_tlb)
        {
            int index = get_index_for_lru();
            if (index == -1)
                log_error(logger_cpu, "ESTO NO DEBERIA DE HABER SUCEDIDO");
            t_registro_tlb *old = list_remove(tlb, index);
            free(old);
            registro->last_access = temporal_gettime(clock_);
            list_add(tlb, registro);
        }
    }
    else
    {
        free(registro);
    }
}

uint32_t obtener_marco_en_tlb(uint32_t pid, uint32_t pagina)
{

    for (int i = 0; i < list_size(tlb); i++)
    {
        t_registro_tlb *tlb_rec = list_get(tlb, i);
        log_info(logger_cpu, "[%d | %d | %d]", tlb_rec->pid, tlb_rec->pagina, tlb_rec->marco);
    }

    for (int i = 0; i < list_size(tlb); i++)
    {
        t_registro_tlb *tlb_rec = list_get(tlb, i);
        if (tlb_rec->pid == pid && tlb_rec->pagina == pagina)
        {
            if (strcmp(algo_tlb, "LRU") == 0)
                tlb_rec->last_access = temporal_gettime(clock_);
            log_info(logger_cpu, "TLB HIT se encontro el marco para el pid: %d pagina: %d!!!", pid, pagina);
            log_info(logger_obligatorio_cpu, "PID: <%d> - TLB HIT - Pagina: <%d>", pid, pagina);
            return tlb_rec->marco;
        }
    }
    log_info(logger_cpu, "TLB MISS no se encontro el marco para el pid: %d pagina: %d", pid, pagina);
    log_info(logger_obligatorio_cpu, "PID: <%d> - TLB MISS - Pagina: <%d>", pid, pagina);

    return -1;
}

void limpiar_tlb(uint32_t pid)
{
    for (int i = 0; i < list_size(tlb); i++)
    {
        t_registro_tlb *tlb_rec = list_get(tlb, i);
        if (tlb_rec->pid == pid)
        {
            tlb_rec = list_remove(tlb, i);
            i = 0;
            free(tlb_rec);
        }
    }
}

void *obtener_t_registro(char *registro_direccion)
{
    if (strcmp(registro_direccion, "AX") == 0 || strcmp(registro_direccion, "BX") == 0 ||
        strcmp(registro_direccion, "CX") == 0 || strcmp(registro_direccion, "DX") == 0)
    {
        t_registro_8 *reg = malloc(sizeof(t_registro_8));
        reg->tipo = 1;
        reg->data = *((uint8_t *)obtener_registro(registros, registro_direccion));
        return reg;
    }
    else
    {
        t_registro_32 *reg = malloc(sizeof(t_registro_32));
        reg->tipo = 2;
        reg->data = *((uint32_t *)obtener_registro(registros, registro_direccion));
        return reg;
    }
}

t_direccion_fisica *traducir_direccion_a_fisica(uint32_t pid, char *registro_direccion)
{
    void *reg = obtener_t_registro(registro_direccion);
    uint32_t *desplazamiento = malloc(sizeof(uint32_t));
    *desplazamiento = 0;
    if (((t_registro_abstracto *)reg)->tipo == 1)
    {
        t_registro_8 *reg8 = (t_registro_8 *)reg;
        log_info(logger_cpu, "reg_data: %d", reg8->data);
        while (reg8->data > 0)
        {
            *desplazamiento += 1;
            reg8->data--;
        }
        free(reg8);
        log_info(logger_cpu, "desplazamiento: %u", *desplazamiento);
    }
    else
    {
        t_registro_32 *reg32 = (t_registro_32 *)reg;
        log_info(logger_cpu, "reg_data32 : %u", reg32->data);
        while (reg32->data > 0)
        {
            *desplazamiento += 1;
            reg32->data--;
        }
        free(reg32);
        log_info(logger_cpu, "desplazamiento32 : %u", *desplazamiento);
    }
    uint32_t nro_pagina = *desplazamiento / *tam_pagina;
    uint32_t des_fisico = *desplazamiento - nro_pagina * *tam_pagina;
    free(desplazamiento);
    uint32_t marco = obtener_marco_en_tlb(pid, nro_pagina);
    if (marco == -1)
    {
        t_dir_logica *dir_logica = malloc(sizeof(t_dir_logica));
        dir_logica->pid = pid;
        dir_logica->pagina = nro_pagina;
        t_paquete *paq = serializar_pedir_marco(dir_logica);
        enviar_paquete(paq, socket_memoria, logger_cpu);
        sem_wait(sem_esperar_respuesta);
        t_registro_tlb *registro = (t_registro_tlb *)rta_memoria;
        marco = registro->marco;
        free(dir_logica);
        log_info(logger_cpu, "Llego el registro: PID: %d, pagina: %d, marco: %d, last_access: %ld", registro->pid, registro->pagina, registro->marco, registro->last_access);
        log_info(logger_obligatorio_cpu, "PID: <%d> - OBTENER MARCO - Página: <%d> - Marco: <%d>", registro->pid, registro->pagina, registro->marco);
        insertar_en_tlb(registro);
    }
    t_direccion_fisica *dir = malloc(sizeof(t_direccion_fisica));
    dir->pid = pid;
    dir->marco = marco;
    dir->desplazamiento_fisico = des_fisico;
    return dir;
}