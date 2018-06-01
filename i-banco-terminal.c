#include "commandlinereader.h"
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#define COMANDO_SAIR_TERMINAL "sair-terminal"
#define COMANDO_DEBITAR "debitar"
#define COMANDO_CREDITAR "creditar"
#define COMANDO_LER_SALDO "lerSaldo"
#define COMANDO_TRANSFERIR "transferir"
#define COMANDO_SIMULAR "simular"
#define COMANDO_SAIR "sair"
#define COMANDO_SAIR_AGORA "agora"
#define INT_SAIR_TERMINAL -3
#define INT_SAIR_AGORA -2
#define INT_SAIR -1
#define INT_SIMULAR 0
#define INT_DEBITAR 1
#define INT_CREDITAR 2
#define INT_LER_SALDO 3
#define INT_TRANSFERIR 4

#define MAXARGS 4
#define BUFFER_SIZE 100
#define FEEDBACK_MAX 40
#define PATHNAME_MAX 40
#define PATHNAME_WRITER "/tmp/i-banco-pipe"

void trataFifoSignal(int s);

typedef struct
{
	char pathname[PATHNAME_MAX];
	int operacao;
	int idConta;
    int idContaD;
	int valor;
} comando_t;

int main(int argc, char** argv) {
	char *args[MAXARGS + 1], pathname[PATHNAME_MAX];
    char buffer[BUFFER_SIZE], feedback[FEEDBACK_MAX];
    int i_pipe_writer, i_pipe_reader;

    if(argc != 2) {
		perror("Pathname de pipe invalido.");
		exit(-1);
    }
    strcpy(pathname, argv[1]);

	if (signal(SIGPIPE, trataFifoSignal)) {
        perror("Erro ao definir signal no terminal.");
        exit(EXIT_FAILURE);
    }

	unlink(pathname);
	if((mkfifo(pathname, 0777)) < 0)
    	perror("Erro ao criar pipe no terminal.");

    if((i_pipe_writer = open(PATHNAME_WRITER, O_WRONLY)) < 0)
    	perror("Erro ao abrir pipe no terminal.");

    while(1) {

    	comando_t c;
    	int numargs = readLineArguments(args, MAXARGS+1, buffer, BUFFER_SIZE);

    	double exec_time;
    	time_t end, start = time(NULL);

        strcpy(c.pathname, pathname);

        /* EOF (end of file) do stdin ou comando "sair" */
        if (numargs < 0 ||
	        (numargs > 0 && (strcmp(args[0], COMANDO_SAIR) == 0))) {

        	c.operacao = INT_SAIR;
        	
        	if(numargs == 2 && strcmp(args[1], COMANDO_SAIR_AGORA) == 0)
        		c.operacao = INT_SAIR_AGORA;
        }

        else if (numargs == 0)
            /* Nenhum argumento; ignora e volta a pedir */
            continue;

        /* Sair do terminal */
        else if (strcmp(args[0], COMANDO_SAIR_TERMINAL) == 0)
        	return 0;

        /* Simular */
        else if (strcmp(args[0], COMANDO_SIMULAR) == 0) {
            if(numargs < 2) {
                printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_SIMULAR);
                continue;
            }

            c.operacao = INT_SIMULAR;
            c.valor = atoi(args[1]);
        }

        /* Debitar */
        else if (strcmp(args[0], COMANDO_DEBITAR) == 0) {
			if (numargs < 3) {
                printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_DEBITAR);
                continue;
            }

            c.operacao = INT_DEBITAR;
            c.idConta = atoi(args[1]);
            c.valor = atoi(args[2]);
        }

        /* Transferir */
        else if (strcmp(args[0], COMANDO_TRANSFERIR) == 0) {
          	if (numargs < 4) {
                printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_TRANSFERIR);
                continue;
            }

            c.operacao = INT_TRANSFERIR;
            c.idConta = atoi(args[1]);
            c.idContaD = atoi(args[2]);
            c.valor = atoi(args[3]);
        }

        /* Creditar */
        else if (strcmp(args[0], COMANDO_CREDITAR) == 0) {
            if (numargs < 3) {
                printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_CREDITAR);
                continue;
            }

            c.operacao = INT_CREDITAR;
            c.idConta = atoi(args[1]);
            c.valor = atoi(args[2]);
        }

        /* Ler Saldo */
        else if (strcmp(args[0], COMANDO_LER_SALDO) == 0) {
			if (numargs < 2) {
                printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_LER_SALDO);
                continue;
            }

            c.operacao = INT_LER_SALDO;
            c.idConta = atoi(args[1]);
        }

        else {
            printf("Comando desconhecido. Tente de novo.\n");
        	continue;
        }

        if(write(i_pipe_writer, &c, sizeof(comando_t)) < 0)
    		perror("Erro ao escrever no pipe.");

    	if(c.operacao == INT_SIMULAR || c.operacao == INT_SAIR || c.operacao == INT_SAIR_AGORA)
    		continue;

    	if((i_pipe_reader = open(pathname, O_RDONLY)) < 0)
    		perror("Erro ao abrir pipe no terminal.");
    	while(read(i_pipe_reader, feedback, FEEDBACK_MAX) < 0);
    	if(close(i_pipe_reader) < 0)
        	perror("Erro ao fechar pipe de resposta no terminal.");

        end = time(NULL);
        exec_time = difftime(end, start);

        printf("%s%lf\n", feedback, exec_time);
    }
}

void trataFifoSignal(int s) {
	signal(SIGPIPE, trataFifoSignal);
}