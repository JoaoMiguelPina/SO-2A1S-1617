FLAGS = -g -Wall -pedantic -lpthread

all: i-banco i-banco-terminal

i-banco: i-banco.o contas.o commandlinereader.o
	gcc -pthread i-banco.o contas.o commandlinereader.o -o i-banco
	
i-banco-terminal: i-banco-terminal.o contas.o commandlinereader.o
	gcc -pthread i-banco-terminal.o contas.o commandlinereader.o -o i-banco-terminal
	
i-banco.o: i-banco.c contas.h commandlinereader.h
	gcc $(FLAGS) -c i-banco.c

i-banco-terminal.o: i-banco-terminal.c contas.h commandlinereader.h
	gcc $(FLAGS) -c i-banco-terminal.c
	
contas.o: contas.c contas.h
	gcc $(FLAGS) -c contas.c
	
commandlinereader.o: commandlinereader.c commandlinereader.h
	gcc $(FLAGS) -c commandlinereader.c

clean:
	rm -f *.o *.txt terminal-* i-banco i-banco-terminal i-banco-pipe