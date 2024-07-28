#include "cpu.h"

void mostrar_registros(t_registros *registros)
{
    log_info(logger_cpu, "ax: %d, bx: %d, cx: %d, dx: %d, eax: %d, ebx: %d, ecx: %d, edx: %d, di: %d, si: %d, pc: %d",
             registros->ax, registros->bx, registros->cx, registros->dx,
             registros->eax, registros->ebx, registros->ecx, registros->edx,
             registros->di, registros->si, registros->pc);
}

void iniciar_registros()
{
    registros->pc = 0;
    registros->ax = 0;
    registros->bx = 0;
    registros->cx = 0;
    registros->dx = 0;
    registros->eax = 0;
    registros->ebx = 0;
    registros->ecx = 0;
    registros->edx = 0;
    registros->di = 0;
    registros->si = 0;
}

void asignar_registros(t_registros *registros, t_registros *news)
{
    registros->ax = news->ax;
    registros->bx = news->bx;
    registros->cx = news->cx;
    registros->dx = news->dx;
    registros->eax = news->eax;
    registros->ebx = news->ebx;
    registros->ecx = news->ecx;
    registros->edx = news->edx;
    registros->si = news->si;
    registros->di = news->di;
    registros->pc = news->pc;
}
