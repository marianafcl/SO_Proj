/*
// Projeto SO - exercicio 1, version 1
// Sistemas Operativos, DEI/IST/ULisboa 2016-17
*/

#include "contas.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define COMANDO_DEBITAR "debitar"
#define COMANDO_CREDITAR "creditar"
#define COMANDO_LER_SALDO "lerSaldo"
#define COMANDO_TRANSFERIR "transferir"
#define COMANDO_SIMULAR "simular"
#define COMANDO_SAIR "sair"
#define COMANDO_SAIR_AGORA "agora"
#define INT_SAIR_AGORA -2
#define INT_SAIR -1
#define INT_SIMULAR 0
#define INT_DEBITAR 1
#define INT_CREDITAR 2
#define INT_LER_SALDO 3
#define INT_TRANSFERIR 4

#define NUM_TRABALHADORAS 3
#define CMD_BUFFER_DIM (NUM_TRABALHADORAS * 2)
#define PATHNAME_READER "/tmp/i-banco-pipe"
#define PATHNAME_MAX 40
#define FEEDBACK_MAX 40
#define FILENAME_SIM_MAX 40

typedef struct
{
	char pathname[PATHNAME_MAX];
	int operacao;
	int idConta;
    int idContaD;
	int valor;
} comando_t;

void *tarefa();
void trataFifoSignal(int s);

int stopSim = 0, buff_write_idx = 0, buff_read_idx = 0, threadCount = 0;
comando_t cmd_buffer[CMD_BUFFER_DIM];
sem_t writerSem, readerSem;
pthread_mutex_t cmd_buffer_mutex, cond_mutex;
pthread_cond_t condSim;
FILE* log_pointer;
int i_pipe;

int main (int argc, char** argv) {
    int pid, status, simOutput, nfilhos = 0;
    char filename[FILENAME_SIM_MAX];
    pthread_t tid[NUM_TRABALHADORAS];

    if (signal(SIGUSR1, signalChange) == SIG_ERR || signal(SIGPIPE, trataFifoSignal)) {
        perror("Erro ao definir signal.");
        exit(EXIT_FAILURE);
    }

    inicializarContas();
    inicializarTrincos();

    if(pthread_mutex_init(&cmd_buffer_mutex, NULL) != 0)
        perror("Erro inicializar trinco\n");
    if(pthread_cond_init(&condSim, NULL) != 0)
        perror("Erro ao inicializar trinco\n");
    if(sem_init(&writerSem, 0, CMD_BUFFER_DIM) != 0)
    	perror("Erro ao criar o semaforo dos escritores.");
    if(sem_init(&readerSem, 0, 0) != 0)
    	perror("Erro ao criar o semaforo dos leitores.");

    for(int i = 0; i < NUM_TRABALHADORAS; i++) {
        if(pthread_create(&tid[i], 0, tarefa, NULL) != 0)
            perror("Erro ao criar thread.");
    }

    if((log_pointer = fopen("log.txt", "w")) == NULL)
    	perror("Erro ao abrir ficheiro log.txt.");

    printf("Bem-vinda/o ao i-banco\n\n");

    unlink(PATHNAME_READER);
    if((mkfifo(PATHNAME_READER, 0777)) < 0)
    	perror("Erro ao criar pipe.");
    if((i_pipe = open(PATHNAME_READER, O_RDONLY)) < 0)
    	perror("Erro ao abrir pipe.");
      
    while (1) {
    	comando_t *cp = (comando_t*) malloc(sizeof(comando_t));
    	int size_read = read(i_pipe, cp, sizeof(comando_t))

    	if(read_size < 0)
    		perror("Erro ao ler do fincheiro.")

    	switch(cp->operacao) {
        	
        	case INT_SAIR_AGORA:
            	kill(0, SIGUSR1);

            case INT_SAIR:
        		printf("i-banco vai terminar.\n--\n");

            	for(; nfilhos > 0; nfilhos--) {
                	pid = wait(&status);
                	if(pid == -1)
                		perror("Erro ao terminar filho.");
                	else if(WIFEXITED(status))
                    	printf("FILHO TERMINADO (PID = %d; terminou normalmente)\n", pid);
                	else
                    	printf("FILHO TERMINADO (PID = %d; terminou abruptamente)\n", pid);
            	}

            	for (int i = 0; i < NUM_TRABALHADORAS; i++) {
                	sem_wait(&writerSem);
                	cmd_buffer[buff_write_idx] = *cp;
                	buff_write_idx = (buff_write_idx + 1) % CMD_BUFFER_DIM;
                	sem_post(&readerSem);
            	}
            
            	for(int i = 0; i < NUM_TRABALHADORAS; i++) {
                	if(pthread_join(tid[i], NULL) != 0)
                    	perror("Erro ao terminar thread.");
            	}

            	pthread_mutex_destroy(&cmd_buffer_mutex);
            	destroiTrincos();
            	sem_destroy(&writerSem);
            	sem_destroy(&readerSem);
            	pthread_cond_destroy(&condSim);

            	fclose(log_pointer);
            	close(i_pipe);
            	printf("--\ni-banco terminou.\n");

            	exit(EXIT_SUCCESS);

        	case INT_SIMULAR:
            	while(threadCount != 0)
                	pthread_cond_wait(&condSim, &cond_mutex);
            	fflush(log_pointer);
				pthread_mutex_unlock(&cond_mutex);

            	pid = fork();
            	if(pid == -1) {
                	printf("Unable to fork.");
                	break;
            	}
            	if(pid == 0) {
  					sprintf(filename, "i-banco-sim-%d.txt", getpid());
                	if((simOutput = open(filename, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IWOTH | S_IROTH)) < 0)
    					perror("Erro ao abrir ficheiro de simulacao.");
  					dup2(simOutput, 1);
  					log_pointer = NULL;
                	simular(cp->valor);
                	exit(EXIT_SUCCESS);
            	}
            	else {
                	nfilhos++;
                	break;
            	}
        
        	default:
            	sem_wait(&writerSem);
            	cmd_buffer[buff_write_idx] = *cp;
            	buff_write_idx = (buff_write_idx + 1) % CMD_BUFFER_DIM;
            	pthread_mutex_lock(&cond_mutex);
            	threadCount++;
            	pthread_mutex_unlock(&cond_mutex);
            	sem_post(&readerSem);
            	break;
    	}
    } 
}


void *tarefa() {
    comando_t c;
    int i_pipe_writer;
    char feedback[FEEDBACK_MAX];
	while(1) {
		sem_wait(&readerSem);
        pthread_mutex_lock(&cmd_buffer_mutex);
    	c = cmd_buffer[buff_read_idx];
    	buff_read_idx = (buff_read_idx + 1) % CMD_BUFFER_DIM;
        pthread_mutex_unlock(&cmd_buffer_mutex);
    	sem_post(&writerSem);

    	if (c.operacao == INT_LER_SALDO) {
	    	int saldo = lerSaldo(c.idConta);
    		if (saldo < 0)
        	   	sprintf(feedback, "%s(%d): Erro.\n\n", COMANDO_LER_SALDO, c.idConta);
       		else
           		sprintf(feedback, "%s(%d): O saldo da conta é %d.\n\n", COMANDO_LER_SALDO, c.idConta, saldo);
      	}

    	else if (c.operacao == INT_DEBITAR) {
    		if (debitar(c.idConta, c.valor) < 0)
            	sprintf(feedback, "%s(%d, %d): Erro\n\n", COMANDO_DEBITAR, c.idConta, c.valor);
        	else
            	sprintf(feedback, "%s(%d, %d): OK\n\n", COMANDO_DEBITAR, c.idConta, c.valor);
    	}

    	else if (c.operacao == INT_CREDITAR) {
    		if (creditar(c.idConta, c.valor) < 0)
            	sprintf(feedback, "%s(%d, %d): Erro\n\n", COMANDO_CREDITAR, c.idConta, c.valor);
        	else
            	sprintf(feedback, "%s(%d, %d): OK\n\n", COMANDO_CREDITAR, c.idConta, c.valor);
    	}

        else if (c.operacao == INT_TRANSFERIR) {
            if (transferir(c.idConta, c.idContaD, c.valor) < 0)
                sprintf(feedback, "%s(%d, %d, %d): Erro\n\n", COMANDO_TRANSFERIR, c.idConta, c.idContaD, c.valor);
            else
                sprintf(feedback, "%s(%d, %d, %d): OK\n\n", COMANDO_TRANSFERIR, c.idConta, c.idContaD, c.valor);
        }

        else if(c.operacao == INT_SAIR)
            return NULL;

        else
        	continue;

        if((i_pipe_writer = open(c.pathname, O_WRONLY)) < 0)
    		perror("Erro ao abrir pipe de resposta.");
        if(write(i_pipe_writer, feedback, FEEDBACK_MAX) < 0)
    		perror("Erro ao escrever resposta no pipe.");
    	if(close(i_pipe_writer) < 0)
        	perror("Erro ao fechar pipe de resposta.");

        pthread_mutex_lock(&cond_mutex);
        threadCount--;
        pthread_cond_signal(&condSim);
        pthread_mutex_unlock(&cond_mutex); 
	}
}

void trataFifoSignal(int s) {
	signal(SIGPIPE, trataFifoSignal);
}