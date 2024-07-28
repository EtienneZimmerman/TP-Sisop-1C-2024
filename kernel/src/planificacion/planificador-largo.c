#include "main.h"
int ultimo_pid = 1;

char *codigo_estado_string(estado_code codigo)
{
	if (codigo == NEW)
		return "NEW";
	if (codigo == READY)
		return "READY";
	if (codigo == EXEC)
		return "EXEC";
	if (codigo == BLOCKED)
		return "BLOCKED";
	if (codigo == EXITT)
		return "EXIT";
	else
		return "ERROR";
}

void log_cambiar_estado(int pid, estado_code viejo, estado_code nuevo)
{
	log_info(logger_kernel, "PID: %d - Cambio de estado %s -> %s", pid, codigo_estado_string(viejo), codigo_estado_string(nuevo));
}

bool proceso_en_running(t_pcb *pcb)
{
	if(list_size(lista_running) == 0)
	{
		return false;
	}
	else
	{
		t_pcb *pcb_en_exec = list_get(lista_running, 0);
		return pcb->pid == pcb_en_exec->pid;
	}	
}

void nuevo_proceso_a_memoria(uint32_t *pid, char *ruta_archivo)
{
	t_paquete *paquete = serializar_nuevo_proceso_memoria(pid, ruta_archivo);
	enviar_paquete(paquete, socket_memoria, logger_kernel);
}

void *crear_proceso(void *arg)
{
	char *ruta_archivo = (char *)arg;
	t_pcb *pcb_nuevo = crear_pcb();

	// Inicializar estado y mutex para el nuevo proceso
	estado_code *estado_inicial = malloc(sizeof(estado_code));
	*estado_inicial = NEW;
	pthread_mutex_t *mutex_nuevo = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(mutex_nuevo, NULL);

	// Agregar estado y mutex a las listas
	list_add(estados_procesos, estado_inicial);
	list_add(m_estados_procesos, mutex_nuevo);

	pthread_mutex_lock(&m_socket_memoria);
	nuevo_proceso_a_memoria(&pcb_nuevo->pid, ruta_archivo);
	pthread_mutex_unlock(&m_socket_memoria);
	free(ruta_archivo);

	//actualizar_estado_pcb(pcb_nuevo, NEW, lista_new, m_list_NEW);
	agregar_a_lista_con_semaforos(pcb_nuevo, lista_new, m_list_NEW);

	log_info(logger_kernel, "Proceso creado con PID: %d", pcb_nuevo->pid); 
	log_info(logger_obligatorio_kernel, "Se crea el proceso con PID: %d en NEW", pcb_nuevo->pid); //LOGOBLIGATORIO
	return "";
}

t_pcb *crear_pcb()
{
	t_pcb *pcb = malloc(sizeof(t_pcb));
	pcb->estado = NEW;
	pcb->pc = 0;
	pcb->pid = ultimo_pid;
	ultimo_pid++;
	pcb->quantum = 0;
	t_registros registros;
	registros.ax = 0;
	registros.bx = 0;
	registros.cx = 0;
	registros.dx = 0;
	registros.eax = 0;
	registros.ebx = 0;
	registros.ecx = 0;
	registros.edx = 0;
	registros.si = 0;
	registros.di = 0;
	registros.pc = 0;
	pcb->registros = registros;

	pcb->recursos_tomados = list_create();
	pcb->recurso_esperando = NULL;

	pcb->tiempo_inicial = 0;

	pthread_mutex_init(&(pcb->m_proceso), NULL);
	return pcb;
}

void eliminar_proceso_de_memoria(uint32_t *pid)
{
	t_paquete *paquete = serializar_eliminar_proceso_memoria(pid);
	enviar_paquete(paquete, socket_memoria, logger_kernel);
	log_info(logger_kernel, "Se solicitó eliminar el proceso con PID: %d", *pid);
}

void enviar_interrupcion_CPU(uint32_t *pid)
{
	t_paquete *paquete = serializar_interrupcion_exit(pid);
	log_info(logger_kernel, "Interrumpiendo pid: %d", *pid);
	enviar_paquete(paquete, socket_cpu_interrupt, logger_kernel);
}

t_pcb *hayar_pcb(uint32_t *pid)
{
	pthread_mutex_lock(&m_list_NEW);

	int index = obtener_index(lista_new, *pid);
	t_pcb *pcb;
	if (index != -1)
	{
		pcb = list_remove(lista_new, index);
		pthread_mutex_unlock(&m_list_NEW);
		return pcb;
	}
	pthread_mutex_unlock(&m_list_NEW);
	pthread_mutex_lock(&m_list_READY);
	index = obtener_index(lista_ready, *pid);
	if (index != -1)
	{
		pcb = list_remove(lista_ready, index);
		pthread_mutex_unlock(&m_list_READY);
		return pcb;
	}
	pthread_mutex_unlock(&m_list_READY);
	pthread_mutex_lock(&m_list_BLOCKED);
	index = obtener_index(lista_blocked, *pid);
	if (index != -1)
	{
		pcb = list_remove(lista_blocked, index);
		pthread_mutex_unlock(&m_list_BLOCKED);
		return pcb;
	}
	pthread_mutex_unlock(&m_list_BLOCKED);
	pthread_mutex_lock(&m_list_auxiliar);
	index = obtener_index(lista_auxiliar, *pid);
	if (index != -1)
	{
		pcb = list_remove(lista_auxiliar, index);
		pthread_mutex_unlock(&m_list_auxiliar);
		return pcb;
	}
	pthread_mutex_unlock(&m_list_auxiliar);
	pthread_mutex_lock(&m_pid_ejecutando);
	if (pid_ejecutando == *pid)
	{
		enviar_interrupcion_CPU(pid);
		pthread_mutex_unlock(&m_pid_ejecutando);
		return NULL;
	}
	pthread_mutex_unlock(&m_pid_ejecutando);
	return NULL;
}

void *finalizar_proceso(void *arg)
{
	uint32_t *pid = (uint32_t *)arg;
	sem_wait(sem_planificacion_largo);
	t_pcb *pcb = hayar_pcb(pid);
	sem_post(sem_planificacion_largo);
	if (pcb != NULL)
	{
		eliminar_proceso(pcb);
	}
	free(pid);

	return "";
}

void eliminar_proceso(t_pcb *pcb)
{
	if (proceso_en_running(pcb))
	{
		log_info(logger_kernel, "SE ENVIA INTERRUPCION A CPU");
		enviar_interrupcion_CPU(&pcb->pid);
	}
	else
	{
		liberar_recursos_tomados(pcb);

		sem_wait(sem_planificacion_largo);
		actualizar_estado_pcb(pcb, EXITT, lista_exit, m_list_EXIT);
		sem_post(sem_planificacion_largo);

		pthread_mutex_lock(&m_socket_memoria);
		eliminar_proceso_de_memoria(&pcb->pid);
		pthread_mutex_unlock(&m_socket_memoria);
	}
}
