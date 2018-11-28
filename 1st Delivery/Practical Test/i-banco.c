/*
// Projeto SO - exercicio 1, version 1
// Sistemas Operativos, DEI/IST/ULisboa 2016-17
*/

#include "commandlinereader.h"
#include "contas.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define COMANDO_DEBITAR "debitar"
#define COMANDO_CREDITAR "creditar"
#define COMANDO_LER_SALDO "lerSaldo"
#define COMANDO_SIMULAR "simular"
#define COMANDO_SAIR "sair"

#define MAXARGS 3
#define BUFFER_SIZE 100
#define NRMAXPIDS 20  /*Numero maximo de processos*/ 

extern int exit_flag; /*Variavel global*/


void handler(){  /*Funcao que define o que fara o SIGUSR1 a cada processo*/
    exit_flag = 0;
    signal(SIGUSR1, handler);
}

int main (int argc, char** argv) {

    char *args[MAXARGS + 1];
    char buffer[BUFFER_SIZE];
    pid_t childPid; /*Variavel onde é guardado o pid no fork()*/
    int pcount = 0; /*Variavel que guarda o numero de processos criados pelo parent*/
    pid_t pidv[NRMAXPIDS]; /*Vetor para guardar os Pids dos processos criados no fork()*/
    
    
    inicializarContas();
    

    printf("Bem-vinda/o ao i-banco\n\n");
      
    while (1) {
        int numargs;
        
    
        numargs = readLineArguments(args, MAXARGS+1, buffer, BUFFER_SIZE);

        /* EOF (end of file) do stdin ou comando "sair" */
        if (numargs < 0 ||
	        (numargs > 0 && (strcmp(args[0], COMANDO_SAIR) == 0))) {
	            int i, status=0, j=0;
	            pid_t pid;
	           
	            if(numargs == 2 && (strcmp(args[1], "agora") == 0)){ /*Para o caso do "Sair agora"*/
	                for(j=0; j<pcount; j++){ /*Enviamos o SIGUSR1  a cada um dos child processes*/
	                    kill(pidv[j], SIGUSR1); 
	                }
	            }
	            printf("\ni-banco vai terminar.\n");
	            printf("--\n");
	            if(childPid > 0){ /*Estamos no parent*/
	                for(i = 0; i < pcount; i++){
	                    pid = wait(&status); /*Para o "sair", o pai espera que os filhos acabem e guarda o status de saida*/
                        
                        if(WIFEXITED(status)){ /*Se o status indicar que o processo acabou normalmente*/
                            printf("FILHO TERMINADO (PID=%d; terminou normalmente)\n", (int) pid);
                        }
                        else{
                            printf("FILHO TERMINADO (PID=%d; terminou abruptamente)\n", (int) pid);
                        }
	                }
	            }
                printf("--\n");
                printf("i-banco terminou.\n");
                exit(EXIT_SUCCESS);
        }   
                
            
            
    
    
        else if (numargs == 0)
            /* Nenhum argumento; ignora e volta a pedir */
            continue;
            
            
        /* Debitar */
        else if (strcmp(args[0], COMANDO_DEBITAR) == 0) {
            int idConta, valor;
            if (numargs < 3) {
                printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_DEBITAR);
                continue;
            }

            idConta = atoi(args[1]);
            valor = atoi(args[2]);

            if (debitar (idConta, valor) < 0)
	           printf("%s(%d, %d): Erro\n\n", COMANDO_DEBITAR, idConta, valor);
            else
                printf("%s(%d, %d): OK\n\n", COMANDO_DEBITAR, idConta, valor);
    }

    /* Creditar */
    else if (strcmp(args[0], COMANDO_CREDITAR) == 0) {
        int idConta, valor;
        if (numargs < 3) {
            printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_CREDITAR);
            continue;
        }

        idConta = atoi(args[1]);
        valor = atoi(args[2]);

        if (creditar (idConta, valor) < 0)
            printf("%s(%d, %d): Erro\n\n", COMANDO_CREDITAR, idConta, valor);
        else
            printf("%s(%d, %d): OK\n\n", COMANDO_CREDITAR, idConta, valor);
    }

    /* Ler Saldo */
    else if (strcmp(args[0], COMANDO_LER_SALDO) == 0) {
        int idConta, saldo;

        if (numargs < 2) {
            printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_LER_SALDO);
            continue;
        }
        idConta = atoi(args[1]);
        saldo = lerSaldo (idConta);
        if (saldo < 0)
            printf("%s(%d): Erro.\n\n", COMANDO_LER_SALDO, idConta);
        else
            printf("%s(%d): O saldo da conta é %d.\n\n", COMANDO_LER_SALDO, idConta, saldo);
    }

    /* Simular */
    else if (strcmp(args[0], COMANDO_SIMULAR) == 0) {
        int numAnos, numContas;
        
        if (numargs < 3) {
                printf("%s: Sintaxe inválida, tente de novo.\n\n", COMANDO_SIMULAR);
                continue;
            }
        
        numAnos = atoi(args[1]);
        numContas = atoi(args[2]);
        
        if(numAnos < 0){
            printf("%s: Número de anos inválido, tente de novo.\n\n", COMANDO_SIMULAR);
            continue;
        }
            
        if(numContas < 1 || numContas > 10){
            printf("%s: Número de contas inválido, tente de novo.\n\n", COMANDO_SIMULAR);
            continue;
        }
        
        if (pcount == NRMAXPIDS) {
            printf("Número máximo de processos atingidos.\n");
        }
        
        else {
        
            childPid = fork();
            pidv[pcount++] = childPid; /*Cada vez que o simular é chamado guarda o pid do fork()*/
        
  
            if(childPid < 0){ /*Caso em que o fork() devolve um erro*/
                perror("fork() error\n");
                exit(EXIT_FAILURE);
            }
        
            else if(childPid == 0){ /*Caso em que estamos no filho*/
                signal(SIGUSR1, handler);
                simular(numAnos, numContas);
                exit(EXIT_SUCCESS);
            }
        }
    }

    else {
      printf("Comando desconhecido. Tente de novo.\n");
    }

  } 
}

