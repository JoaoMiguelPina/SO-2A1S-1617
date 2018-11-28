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
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>

#define COMANDO_DEBITAR "debitar"
#define COMANDO_CREDITAR "creditar"
#define COMANDO_LER_SALDO "lerSaldo"
#define COMANDO_TRANSFERIR "transferir"
#define COMANDO_SIMULAR "simular"
#define COMANDO_SAIR "sair"
#define COMANDO_ARG_SAIR_AGORA "agora"

#define OP_LERSALDO 0
#define OP_CREDITAR 1
#define OP_DEBITAR 2
#define OP_TRANSFERIR 3
#define OP_SAIR 4
#define OP_SIMULAR 5

#define FILENAME "log.txt"
#define I_BANCO_FIFO "i-banco-pipe"


#define NRMAXPIDS 20  /*Numero maximo de processos*/ 
#define NUM_TRABALHADORAS 3
#define CMD_BUFFER_DIM (NUM_TRABALHADORAS * 2)

extern int exit_flag; /*Variavel global*/
int existem_cmds = 0; /* Numero de comandos existentes no buffer*/
int buff_write_idx = 0, buff_read_idx = 0; /*Indice de escrita e de leitura do buffer*/
int logfile;
int fifoFile;
int terminalFile;

char target[80];

typedef struct {  /*Estrutura para os comandos*/
	int operacao;
	int idConta1;
	int idConta2;
	int valor;
	pid_t pid;
} comando_t;

sem_t canWrite;
sem_t canRead;

pthread_mutex_t read_mutex;
pthread_mutex_t simtex;
pthread_mutex_t comandos;
pthread_cond_t simcond;


void handler(){  /*Funcao que define o que fara o SIGUSR1 a cada processo*/
	exit_flag = 0;
	if (signal(SIGUSR1, handler) == SIG_ERR) {
    	perror("Erro ao definir signal.");
    	exit(EXIT_FAILURE);
  }
}



void pipeHandler(int s) {
	printf("A ligação ao terminal foi cortada\n");
	exit(EXIT_SUCCESS);
}


/*====================================FUNCAO DE LEITURA DE COMANDOS====================================*/

void* processCmd(void *cmd_buffer){ /*Funcao que e passada a pthread_create*/
	
	comando_t *buffer = cmd_buffer;
	comando_t cmd;
	pthread_t tid;
	char s[80];
	char resultado[80];
	while(1){
		sem_wait(&canRead);
		pthread_mutex_lock(&read_mutex);
		cmd = buffer[buff_read_idx];
		buff_read_idx = (buff_read_idx + 1) % CMD_BUFFER_DIM;
		pthread_mutex_unlock(&read_mutex);
		
		tid = pthread_self();
		
		snprintf(target, 79, "terminal-%ld", (long) cmd.pid);
		
		
		if(cmd.operacao == OP_DEBITAR){
			if (debitar(cmd.idConta1, cmd.valor) < 0){
				snprintf(resultado, 79, "%s(%d, %d): Erro\n", COMANDO_DEBITAR, cmd.idConta1, cmd.valor);
			}
			else{
				snprintf(resultado, 79, "%s(%d, %d): OK\n", COMANDO_DEBITAR, cmd.idConta1, cmd.valor);
			}
			snprintf(s, 79, "%lu: %s %d %d\n", tid, COMANDO_DEBITAR, cmd.idConta1, cmd.valor);
		}
		
		else if(cmd.operacao == OP_CREDITAR){
			if (creditar (cmd.idConta1, cmd.valor) < 0){
				snprintf(resultado, 79, "%s(%d, %d): Erro\n", COMANDO_CREDITAR, cmd.idConta1, cmd.valor);
			}
			else{
				snprintf(resultado, 79, "%s(%d, %d): OK\n", COMANDO_CREDITAR, cmd.idConta1, cmd.valor);
			}
			snprintf(s, 79,"%lu: %s %d %d\n", tid, COMANDO_CREDITAR, cmd.idConta1, cmd.valor);
		}
		
		else if(cmd.operacao == OP_LERSALDO){
			int saldo = lerSaldo(cmd.idConta1);
			if (saldo < 0){
				snprintf(resultado, 79, "%s(%d): Erro.\n\n", COMANDO_LER_SALDO, cmd.idConta1);
			}
			else{
				snprintf(resultado, 79, "%s(%d): O saldo da conta é %d.\n", COMANDO_LER_SALDO, cmd.idConta1, saldo);
			}
			snprintf(s, 79, "%lu: %s %d\n", tid, COMANDO_LER_SALDO, cmd.idConta1);
		}
		
		else if(cmd.operacao == OP_TRANSFERIR){
			if (transferir(cmd.idConta1, cmd.idConta2, cmd.valor) < 0){
				snprintf(resultado, 79, "Erro ao transferir %d da conta %d para a conta %d.\n\n", cmd.valor, cmd.idConta1, cmd.idConta2);
			}
			else{
				snprintf(resultado, 79, "%s(%d, %d, %d): OK\n", COMANDO_TRANSFERIR, cmd.idConta1, cmd.idConta2, cmd.valor);
			}
			snprintf(s, 79, "%lu: %s %d %d %d\n", tid, COMANDO_TRANSFERIR, cmd.idConta1, cmd.idConta2, cmd.valor);
			
		}
		
		else if(cmd.operacao == OP_SAIR){
			
			pthread_mutex_lock(&comandos);
			existem_cmds--;  /*Decrementar o numero de comandos presentes no buffer*/
			pthread_mutex_unlock(&comandos);
			sem_post(&canWrite);
			pthread_exit(NULL);
		}
		
		write(logfile, s, strlen(s));
		if ((terminalFile = open (target, O_WRONLY)) < 0){
			exit (-1);
		}
		write(terminalFile, resultado, strlen(resultado));
		if (close (terminalFile) < -1) {
            perror ("Error closing terminal fifo in main");
		}

		
		pthread_mutex_lock(&comandos);
		existem_cmds--;  /*Decrementar o numero de comandos presentes no buffer*/
		pthread_mutex_unlock(&comandos);
		
		pthread_mutex_lock(&simtex);
		pthread_cond_signal(&simcond); /*Libertar a main thread se este for o ultimo comando do buffer antes do simular*/
		pthread_mutex_unlock(&simtex); /*Em caso contrario o signal nao tem efeito*/
		
		sem_post(&canWrite);
	}
	return NULL;
}



/*============================FUNCOES DE INICIALIZACAO=============================*/

void inicializarTarefas(pthread_t buffer[], void* cmd_buffer) { /*Funcao que inicializa as tarefas e as coloca num buffer*/
	int i;
	for(i = 0; i < NUM_TRABALHADORAS; i++){
		if(pthread_create(&buffer[i], NULL, processCmd,(void*) cmd_buffer) != 0){
			perror("Erro na inicialização das threads");
		}
		else{
			printf("Thread trabalhadora nº %d iniciada\n", i+1);
		}
	}
	printf("\n");
}

void inicializarCmdBuffer(comando_t buffer[]){ /*Inicializacao do buffer de comandos*/
	int i;
	for(i=0; i < CMD_BUFFER_DIM; i++){
		buffer[i].operacao = -1;
		buffer[i].idConta1 = -1;
		buffer[i].idConta2 = -1;
		buffer[i].valor = -1;
	}
}






/*====================================FUNCAO MAIN====================================*/

int main (int argc, char** argv) {
	
	pid_t childPid; /*Variavel onde é guardado o pid no fork()*/
	int pcount = 0; /*Variavel que guarda o numero de processos criados pelo parent*/
	pid_t pidv[NRMAXPIDS]; /*Vetor para guardar os Pids dos processos criados no fork()*/
	int bytes_read;
	
	
	
	pthread_t thread_buffer[NUM_TRABALHADORAS];
	comando_t cmd_buffer[CMD_BUFFER_DIM]; /*Cria buffer de comandos*/
	
	if(sem_init(&canWrite, 0, CMD_BUFFER_DIM) < 0){ /*Inicializacao do semaforo de escritura*/
		perror("Erro na inicializacao do semaforo de escritura");
		exit(-1);
	}
	if(sem_init(&canRead, 0, 0) < 0){ /*Inicializacao do semaforo de leitura*/
		perror("Erro na inicializacao do semaforo de leitura");
		exit(-1);
	}
	
	if(pthread_mutex_init(&read_mutex, NULL) != 0){ /*Inicializacao do mutex de leitura*/
		perror("Erro na inicializacao do mutex de leitura");
	}
	
	if(pthread_mutex_init(&simtex, NULL) != 0){ /*Inicializacao do mutex do simular*/
		perror("Erro na inicializacao do mutex de Simulacao");
	}
	
	if(pthread_mutex_init(&comandos, NULL) != 0){ /*Inicializacao do mutex dos comandos*/
		perror("Erro na inicializacao do mutex de Comandos");
	}
	
	if(pthread_cond_init(&simcond, NULL) != 0){ /*Inicializacao do cond do simular*/
		perror("Erro na inicializacao do cond de Simulacao");
	}
	
	
	printf("Bem-vinda/o ao i-banco\n\n");
	
	
	
	inicializarCmdBuffer(cmd_buffer); /*Inicializacao do buffer*/
	
	inicializarTarefas(thread_buffer, (void*) cmd_buffer);  /*Inicializacao das tarefas*/
	
	inicializarContas();
	
	if((logfile = open(FILENAME, O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR)) < 0){
		perror("Error opening the log.txt");
	};
	
	
	unlink(I_BANCO_FIFO);
	
	if ((mkfifo (I_BANCO_FIFO, 0777) < 0)){ /*Criacao do fifo principal do i-banco*/
        perror ("Error in the mkfifo creation");
        exit (1);
    }
    
    if ((fifoFile = open (I_BANCO_FIFO, O_RDONLY)) < 0)
        perror ("Error while opening the fifo");
        
    signal(SIGPIPE, pipeHandler); /*Sigpipe no caso em que o ibanco tenta responder a um terminal fechado*/
	
	
	while (1) {
		comando_t* cmd;
		comando_t new_cmd;
		
		cmd = (comando_t*) malloc(sizeof(comando_t)); /*Alocar memoria para receber o comando*/
		
		memset(cmd, 0, sizeof (comando_t)); /*Inicializar a memoria*/
		
        if ((bytes_read = read (fifoFile, cmd, sizeof (comando_t))) == -1)
            perror ("read");
            
        if (bytes_read == 0)
            continue;
            
		new_cmd = *cmd;
		
		
		if(new_cmd.operacao == OP_SAIR){
			int i, status=0, j=0;
			pid_t pid;
			
			if(new_cmd.valor == 1){
				for(j=0; j<pcount; j++){ /*Enviamos o SIGUSR1  a cada um dos child processes*/
					kill(pidv[j], SIGUSR1); 
				}	
			}
			
			printf("\ni-banco vai terminar.\n");
			printf("--\n");
			
			for(i=0; i < NUM_TRABALHADORAS; i++){ /*Envia 3 comandos sair ao buffer de comandos*/

				sem_wait(&canWrite);
				
				cmd_buffer[buff_write_idx] = new_cmd;
				buff_write_idx = (buff_write_idx + 1) % CMD_BUFFER_DIM;
				
				pthread_mutex_lock(&comandos);
				existem_cmds++; /*Aumentar o numero de comandos presentes no buffer*/
				pthread_mutex_unlock(&comandos);
				
				
				sem_post(&canRead);
			}

			for(i=0; i < NUM_TRABALHADORAS;i++){ /*Esperar que as threads trabalhadoras facam exit*/
				pthread_join(thread_buffer[i], NULL);
			} 

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
			
			sem_destroy(&canWrite); /*Destruicao dos semaforos*/
			sem_destroy(&canRead);
			
			pthread_mutex_destroy(&read_mutex); /*Destruicao do mutex*/
			pthread_mutex_destroy(&simtex); /*Destruicao do mutex*/
			pthread_mutex_destroy(&comandos); /*Destruicao do mutex*/
			pthread_cond_destroy(&simcond);
			
			destroyCMx(); /*Destruicao dos mutexes das contas*/
			
			if(cmd != NULL){
				free(cmd);
			}
			
			close(logfile);
			close(fifoFile);
			unlink(I_BANCO_FIFO);
			
			exit(EXIT_SUCCESS);
		}
		
		else if(new_cmd.operacao == OP_SIMULAR){
			int numAnos, out, pid;
			char str[80];
			
			numAnos = new_cmd.valor;
		
			if (pcount == NRMAXPIDS) {
				printf("Número máximo de processos atingidos.\n");
				continue;
			} 
		
			else {
				pthread_mutex_lock(&simtex);
				
				while(existem_cmds){ /*Espera que todos os comandos que ja la estao sejam feitos*/
					pthread_cond_wait(&simcond, &simtex); /*Poe a thread principal na fila de espera*/
				}
				
				childPid = fork();
				
				pthread_mutex_unlock(&simtex);
				
				pidv[pcount++] = childPid; /*Cada vez que o simular é chamado guarda o pid do fork()*/
	
  
				if(childPid < 0){ /*Caso em que o fork() devolve um erro*/
					perror("fork() error\n");
					exit(EXIT_FAILURE);
				}
		
				else if(childPid == 0){ /*Caso em que estamos no filho*/
				
					if (signal(SIGUSR1, handler) == SIG_ERR) {
    					perror("Erro ao definir signal.");
    					exit(EXIT_FAILURE);
					}
					
					pid = getpid();
					snprintf(str, 79, "i-banco-sim-%ld.txt", (long int) pid);
					
					if((out = open(str, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR)) < 0){
						perror("Error opening the out file");
					};
					
					dup2(out, 1);
					
					simular(numAnos);
					
					close(out);
					
					exit(EXIT_SUCCESS);
				}
			}
			continue;
		}
		
		else {
			sem_wait(&canWrite);
			cmd_buffer[buff_write_idx] = new_cmd;
			buff_write_idx = (buff_write_idx + 1) % CMD_BUFFER_DIM;
			
			pthread_mutex_lock(&comandos);
			existem_cmds++; /*Aumentar o numero de comandos presentes no buffer*/
			pthread_mutex_unlock(&comandos);
			sem_post(&canRead);
		}
	}
}
