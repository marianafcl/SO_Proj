#include "contas.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

extern int stopSim;
extern int threadCount;
extern FILE* log_pointer;

#define atrasar() sleep(ATRASO)
		     
int contasSaldos[NUM_CONTAS];
pthread_mutex_t mutexContas[NUM_CONTAS];


void inicializarTrincos() {
  for (int i = 0; i < NUM_CONTAS; i++) {
    if(pthread_mutex_init(&mutexContas[i], NULL) != 0)
      perror("Erro ao inicializar trinco.");
  }
}

void destroiTrincos() {
  for (int i = 0; i < NUM_CONTAS; i++) {
    if(pthread_mutex_destroy(&mutexContas[i]) != 0)
      perror("Erro ao destruir trinco.");
  }
}

int contaExiste(int idConta) {
  return (idConta > 0 && idConta <= NUM_CONTAS);
}

void inicializarContas() {
  int i;
  for (i=0; i<NUM_CONTAS; i++)
    contasSaldos[i] = 0;
}

int debitar(int idConta, int valor) {
  atrasar();
  if (!contaExiste(idConta))
    return -1;
  pthread_mutex_lock(&mutexContas[idConta - 1]);
  if (contasSaldos[idConta - 1] < valor) {
  	pthread_mutex_unlock(&mutexContas[idConta - 1]);
    return -1;
  }
  atrasar();
  contasSaldos[idConta - 1] -= valor;
  if(log_pointer != NULL)
    fprintf(log_pointer, "%lu: debitar\n", pthread_self());
  pthread_mutex_unlock(&mutexContas[idConta - 1]);
  return 0;
}

int creditar(int idConta, int valor) {
  atrasar();
  if (!contaExiste(idConta))
    return -1;
  pthread_mutex_lock(&mutexContas[idConta - 1]);
  contasSaldos[idConta - 1] += valor;
  if(log_pointer != NULL)
    fprintf(log_pointer, "%lu: creditar\n", pthread_self());
  pthread_mutex_unlock(&mutexContas[idConta - 1]);
  return 0;
}

int lerSaldo(int idConta) {
  int saldo;
  atrasar();
  if (!contaExiste(idConta))
    return -1;
  pthread_mutex_lock(&mutexContas[idConta - 1]);
  saldo = contasSaldos[idConta - 1];
  if(log_pointer != NULL)
    fprintf(log_pointer, "%lu: lerSaldo\n", pthread_self());
  pthread_mutex_unlock(&mutexContas[idConta - 1]);
  return saldo;
}

int transferir(int idConta, int idContaD, int valor) {
  atrasar();
  if (!contaExiste(idConta) || !contaExiste(idContaD))
    return -1;
  if (idConta < idContaD) {
  	pthread_mutex_lock(&mutexContas[idConta - 1]);
  	pthread_mutex_lock(&mutexContas[idContaD - 1]);
  }
  else {
  	pthread_mutex_lock(&mutexContas[idContaD - 1]);
  	pthread_mutex_lock(&mutexContas[idConta - 1]);
  }
  atrasar();
  if (contasSaldos[idConta - 1] < valor) {
    pthread_mutex_unlock(&mutexContas[idConta - 1]);
  	pthread_mutex_unlock(&mutexContas[idContaD - 1]);
    return -1;
  }
  atrasar();
  contasSaldos[idConta - 1] -= valor;
  contasSaldos[idContaD - 1] += valor;
  if(log_pointer != NULL)
    fprintf(log_pointer, "%lu: transferir\n", pthread_self());
  pthread_mutex_unlock(&mutexContas[idConta - 1]);
  pthread_mutex_unlock(&mutexContas[idContaD - 1]);
  return 0;
}


void simular(int numAnos) {
  int i, j, credito;

  for (j=0; j <= numAnos; j++) {
  
    printf("SIMULACAO: Ano %d\n", j);
    printf("=================\n");
  
    for (i=1; i <= NUM_CONTAS; i++){
      if(j==0)
        printf("Conta %d, Saldo %d\n", i, lerSaldo(i));

      else {
        credito = lerSaldo(i) * TAXAJURO - CUSTOMANUTENCAO;
        if(credito > 0)
          creditar(i, credito);
        printf("Conta %d, Saldo %d\n", i, lerSaldo(i));
      }
    }
    printf("\n");

    if(stopSim) {
      printf("Simulacao terminada por sinal\n");
      exit(0);
    }
  }
}

void signalChange(int s) {
  stopSim = 1;
  signal(SIGUSR1, signalChange);
}