#include "contas.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define atrasar() sleep(ATRASO)
		     
int contasSaldos[NUM_CONTAS];

int exit_flag = 1; /*Flag usada no "Sair Agora" para acabar de simular o ano atual*/


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
  if (contasSaldos[idConta - 1] < valor)
    return -1;
  atrasar();
  contasSaldos[idConta - 1] -= valor;
  return 0;
}

int creditar(int idConta, int valor) {
  atrasar();
  if (!contaExiste(idConta))
    return -1;
  contasSaldos[idConta - 1] += valor;
  return 0;
}

int lerSaldo(int idConta) {
  atrasar();
  if (!contaExiste(idConta))
    return -1;
  return contasSaldos[idConta - 1];
}

void simular(int numAnos, int numContas) {
  int i = 0, j=0;
  for(i = 0; i <= numAnos; i++){ /*Ciclo dos anos*/
  
    if (exit_flag == 0){
      printf("Simulacao terminada por signal\n");
      exit(EXIT_SUCCESS);
    }
    
    printf("\nSIMULACAO: Ano %d\n", i);
    printf("=================\n");
    
    for(j = 0; j < numContas; j++){ /*Ciclo das contas*/
      printf("Conta %d, Saldo %d\n", j+1, lerSaldo(j+1));
      creditar(j+1, lerSaldo(j+1)* TAXAJURO );
      debitar(j+1, CUSTOMANUTENCAO);
    }
  }
  printf("\n");
}
