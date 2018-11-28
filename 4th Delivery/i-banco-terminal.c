#include "commandlinereader.h"
#include "contas.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>

#define COMANDO_DEBITAR "debitar"
#define COMANDO_CREDITAR "creditar"
#define COMANDO_LER_SALDO "lerSaldo"
#define COMANDO_TRANSFERIR "transferir"
#define COMANDO_SIMULAR "simular"
#define COMANDO_SAIR "sair"
#define COMANDO_ARG_SAIR_AGORA "agora"
#define COMANDO_SAIR_TERMINAL "sair-terminal"

#define OP_LERSALDO 0
#define OP_CREDITAR 1
#define OP_DEBITAR 2
#define OP_TRANSFERIR 3
#define OP_SAIR 4
#define OP_SIMULAR 5
#define OP_SAIR_AGORA 6


#define MAXARGS 4
#define BUFFER_SIZE 100

int fifoFile = 0;
int terminalFile;
char fifoTerminal[80];
char respostaBanco[80];
double timeDiff;

clock_t begin;
clock_t end;

typedef struct Comando {  /*Estrutura para os comandos*/
	int operacao;
	int idConta1;
	int idConta2;
	int valor;
	pid_t pid;
} comando_t;

comando_t* cmd;


void pipeHandler(int s) {
	printf("A conexão ao i-banco foi interrompida, o terminal vai fechar-se.\n");
	exit(EXIT_SUCCESS);
}


/*====================================FUNCAO DE PROCESSAMENTO DE COMANDOS====================================*/

void get_cmd(char *args[], int numargs, char terminal[]){ /*Funcao que le comandos e coloca-os no fifo*/

	int bytes_read;
	pid_t terminalPid = getpid();
	comando_t new_cmd;
	
	
	if (numargs < 0 || (numargs > 0 && (strcmp(args[0], COMANDO_SAIR) == 0))){
		if(numargs > 1 && strcmp(args[1], COMANDO_ARG_SAIR_AGORA)  == 0){
			new_cmd.operacao = OP_SAIR;
			new_cmd.idConta1 = 0;
			new_cmd.idConta2 = 0;
			new_cmd.valor = 1; /*Valor 1 para o sair agora e 0 para o sair*/
			new_cmd.pid = terminalPid;
		}
		else{
		new_cmd.operacao = OP_SAIR;
		new_cmd.idConta1 = 0;
		new_cmd.idConta2 = 0;
		new_cmd.valor = 0;
		new_cmd.pid = terminalPid;
		}
	}
	else if (!strcmp(args[0], COMANDO_CREDITAR)) {
		new_cmd.operacao = OP_CREDITAR;
		new_cmd.idConta1 = atoi(args[1]);
		new_cmd.idConta2 = 0;
		new_cmd.valor = atoi(args[2]);
		new_cmd.pid = terminalPid;
	}
	else if (!strcmp(args[0], COMANDO_DEBITAR)) {
		new_cmd.operacao = OP_DEBITAR;
		new_cmd.idConta1 = atoi(args[1]);
		new_cmd.idConta2 = 0;
		new_cmd.valor = atoi(args[2]);
		new_cmd.pid = terminalPid;
	}
	else if (!strcmp(args[0], COMANDO_LER_SALDO)) {
		new_cmd.operacao = OP_LERSALDO;
		new_cmd.idConta1 = atoi(args[1]);
		new_cmd.idConta2 = 0;
		new_cmd.valor = -1;
		new_cmd.pid = terminalPid;
	}
	else if (!strcmp(args[0], COMANDO_TRANSFERIR)) {
		new_cmd.operacao = OP_TRANSFERIR;
		new_cmd.idConta1 = atoi(args[1]);
		new_cmd.idConta2 = atoi(args[2]);
		new_cmd.valor = atoi(args[3]);
		new_cmd.pid = terminalPid;
	}
	else if (!strcmp(args[0], COMANDO_SIMULAR)) {
		new_cmd.operacao = OP_SIMULAR;
		new_cmd.idConta1 = -1;
		new_cmd.idConta2 = -1;
		new_cmd.valor = atoi(args[1]);
		new_cmd.pid = terminalPid;
	}
	
	else{
		return;
	}
	
	cmd = (comando_t*) malloc(sizeof(comando_t));
	cmd = &new_cmd;
	
	
	if ((fifoFile = open (terminal, O_WRONLY)) < 0){
		exit (-1);
	}
	
	
	time(&begin);
	
	write(fifoFile, cmd, sizeof(comando_t));
	
	close(fifoFile);
	
    if((new_cmd.operacao != OP_SAIR) && (new_cmd.operacao != OP_SIMULAR)){
    	
    	memset (respostaBanco, '\0', sizeof (respostaBanco)); /*Inicializar a memoria para o comando*/
    	
    	if ((terminalFile = open (fifoTerminal, O_RDONLY)) < 0)
           perror ("Erro ao abrir o fifo do terminal");
           
	    if ((bytes_read = read (terminalFile, respostaBanco, sizeof (respostaBanco))) == -1)
	        perror ("read");
		
		time(&end); /*Contagem do tempo do comando*/
		
	    if (bytes_read > 0) {
	        printf ("%s", respostaBanco);
	        timeDiff = difftime(end, begin);
	        printf("Tempo total de execução: %f segundos\n\n", timeDiff);
	    }
    	 if (close (terminalFile) == -1) {
            perror ("close");
    	}
    }
    else{
    	time(&end);
    }
    
      
}





/*====================================FUNCAO MAIN====================================*/

int main (int argc, char** argv) {
	char *args[MAXARGS + 1];
	char buffer[BUFFER_SIZE];
	char terminal[80];
	
	
	printf("Bem-vinda/o ao i-banco\n\n");
	
	snprintf (fifoTerminal, 79, "terminal-%ld", (long) getpid ());
	
	snprintf (terminal, 79, "%s", argv[1]);
	
	if (mkfifo (fifoTerminal, 0777) < 0)  /*Fifo pelo qual o terminal obtem a resposta do i-banco*/
        perror ("Erro na criacao do fifo do terminal");
        
    signal(SIGPIPE, pipeHandler); /*Sigpipe no caso em que o i-banco é fechado*/
    
	while (1) {
		int numargs;
		
		numargs = readLineArguments(args, MAXARGS+1, buffer, BUFFER_SIZE);
		
		/* EOF (end of file) do stdin ou comando "sair" */
		if (numargs < 0 || (numargs > 0 && (strcmp(args[0], COMANDO_SAIR) == 0))) {
			get_cmd(args, numargs, terminal);
		}
		
		else if (numargs == 0)
			/* Nenhum argumento; ignora e volta a pedir */
			continue;
			
			
		/* Debitar */
		else if (strcmp(args[0], COMANDO_DEBITAR) == 0) {
			if (numargs < 3) {
				printf("%s: Sintaxe inválida, tente de novo.\n\n", COMANDO_DEBITAR);
				continue;
			}
			get_cmd(args, numargs, terminal);
		}

		/* Creditar */
		else if (strcmp(args[0], COMANDO_CREDITAR) == 0) {
			if (numargs < 3) {
				printf("%s: Sintaxe inválida, tente de novo.\n\n", COMANDO_CREDITAR);
				continue;
			}
			get_cmd(args, numargs, terminal);
		}

		/* Ler Saldo */
		else if (strcmp(args[0], COMANDO_LER_SALDO) == 0) {
			if (numargs < 2) {
				printf("%s: Sintaxe inválida, tente de novo.\n\n", COMANDO_LER_SALDO);
				continue;
			}
			get_cmd(args, numargs, terminal);
		}
		
		/* Transferir */
		else if (!strcmp(args[0], COMANDO_TRANSFERIR)) {
			if ((numargs < 4) || (atoi(args[1]) == atoi(args[2]))){
				printf("%s: Sintaxe inválida, tente de novo.\n\n", COMANDO_TRANSFERIR);
				continue;
			}
			get_cmd(args, numargs, terminal);
		}

		/* Simular */
		else if (strcmp(args[0], COMANDO_SIMULAR) == 0) {
			int numAnos;
		
			if (numargs < 2) {
				printf("%s: Sintaxe inválida, tente de novo.\n\n", COMANDO_SIMULAR);
				continue;
			}
		
			numAnos = atoi(args[1]);

			if(numAnos < 0){
				printf("%s: Número de anos inválido, tente de novo.\n\n", COMANDO_SIMULAR);
				continue;
			}
			
			get_cmd(args, numargs, terminal);
		}
		
		else if (strcmp(args[0], COMANDO_SAIR_TERMINAL) == 0) {
			
			if(cmd != NULL){
				free(cmd);
			}
			
			unlink(fifoTerminal);
			exit(EXIT_SUCCESS);
		}

		else {
			printf("Comando desconhecido. Tente de novo.\n\n");
		}
	} 
}
