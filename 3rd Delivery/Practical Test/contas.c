#include "contas.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define atrasar() sleep(ATRASO)
				 
int contasSaldos[NUM_CONTAS];

pthread_mutex_t mx_contas[NUM_CONTAS]; /* Cria o vetor de mutexes de cada conta*/

int exit_flag = 1; /*Flag usada no "Sair Agora" para acabar de simular o ano atual*/


int contaExiste(int idConta) {
	return (idConta > 0 && idConta <= NUM_CONTAS);
}

void inicializarContas() {
	int i;
	for (i=0; i<NUM_CONTAS; i++){
		contasSaldos[i] = 0;
	}
	for(i=0; i < NUM_CONTAS; i++){  /*Inicializar o vetor de mutexes de cada conta*/
		pthread_mutex_init(&mx_contas[i], NULL);
	}
}


void destroyCMx(){ /*Funcao que destroi os mutexes das contas*/
	int i;
	for(i =0; i< NUM_CONTAS; i++){
		pthread_mutex_destroy(&mx_contas[i]);
	}
}

int debitar(int idConta, int valor) {
	atrasar();
	if (!contaExiste(idConta))
		return -1;
	pthread_mutex_lock(&mx_contas[idConta-1]); /*Fecho do mutex devido ao acesso as contas*/
	if (contasSaldos[idConta - 1] < valor){
		pthread_mutex_unlock(&mx_contas[idConta-1]); /*No caso de a funcao dar return com erro*/
		return -1;
	}
	atrasar();
	contasSaldos[idConta - 1] -= valor;
	pthread_mutex_unlock(&mx_contas[idConta-1]); /*Abertura do mutex em caso de retorno normal*/
	return 0;
}

int creditar(int idConta, int valor) {
	atrasar();
	if (!contaExiste(idConta))
		return -1;
	pthread_mutex_lock(&mx_contas[idConta-1]); /*Abertura do mutex*/
	contasSaldos[idConta - 1] += valor;
	pthread_mutex_unlock(&mx_contas[idConta-1]); /*Fecho do mutex*/
	return 0;
}

int lerSaldo(int idConta) {
    int saldo;
	atrasar();
	if (!contaExiste(idConta))
		return -1;
    pthread_mutex_lock(&mx_contas[idConta-1]);
    saldo = contasSaldos[idConta - 1];
    pthread_mutex_unlock(&mx_contas[idConta-1]);
    return saldo;
}

int transferir(int idConta1, int idConta2, int valor){
	if (!contaExiste(idConta1) || !contaExiste(idConta2) || valor <= 0 || idConta1 == idConta2){
		return -1;
	}
	
	if(idConta1 > idConta2){ /*Para evitar a interblocagem bloqueamos o mutex da conta com maior id primeiro, assim, se a este transferir*/
		pthread_mutex_lock(&mx_contas[idConta1-1]); /*Se seguir o transferir inverso, é sempre o mesmo mutex o primeiro a ser bloqueado*/
		pthread_mutex_lock(&mx_contas[idConta2-1]); /*De forma a evitar que a segunda conta do primeiro transferir seja bloqueada pelo segundo*/
	}
	else if(idConta1 < idConta2){ 
		pthread_mutex_lock(&mx_contas[idConta2-1]);
		pthread_mutex_lock(&mx_contas[idConta1-1]);
	}
	atrasar();
	if (contasSaldos[idConta1 - 1] < valor){
		pthread_mutex_unlock(&mx_contas[idConta1-1]);
		pthread_mutex_unlock(&mx_contas[idConta2-1]);
		return -1;
	}
	atrasar();
	contasSaldos[idConta1 - 1] -= valor;
	
	atrasar();
	contasSaldos[idConta2 - 1] += valor;
	
	pthread_mutex_unlock(&mx_contas[idConta1-1]);
	pthread_mutex_unlock(&mx_contas[idConta2-1]);
	
	return 0;
	
}

int creditar2(int idConta1, int idConta2, int valor){
    atrasar();
    if (!contaExiste(idConta1) || !contaExiste(idConta2) || valor <=0 || idConta1 == idConta2){
		return -1;
    }
    if(idConta1 > idConta2){ /*Para evitar a interblocagem*/
		pthread_mutex_lock(&mx_contas[idConta1-1]); 
		pthread_mutex_lock(&mx_contas[idConta2-1]); 
	}
	else if(idConta1 < idConta2){ 
		pthread_mutex_lock(&mx_contas[idConta2-1]);
		pthread_mutex_lock(&mx_contas[idConta1-1]);
	}
	contasSaldos[idConta1 - 1] += valor;
    contasSaldos[idConta2 - 1] += valor;
    
    pthread_mutex_unlock(&mx_contas[idConta1-1]);
	pthread_mutex_unlock(&mx_contas[idConta2-1]);
	
	return 0;
}



void simular(int numAnos) {
	int i = 0, j=0;
	for(i = 0; i <= numAnos; i++){ /*Ciclo dos anos*/
	
		if (exit_flag == 0){
			printf("Simulacao terminada por signal\n");
			exit(EXIT_SUCCESS);
		}
		
		printf("\nSIMULACAO: Ano %d\n", i);
		printf("=================\n");
		
		for(j = 0; j < NUM_CONTAS; j++){ /*Ciclo das contas*/
			printf("Conta %d, Saldo %d\n", j+1, lerSaldo(j+1));
			creditar(j+1, lerSaldo(j+1)* TAXAJURO );
			debitar(j+1, CUSTOMANUTENCAO);
		}
	}
	printf("\n");
}
