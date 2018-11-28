FLAGS = -g -Wall -pedantic -lpthread

i-banco: i-banco.o contas.o commandlinereader.o
	gcc -pthread i-banco.o contas.o commandlinereader.o -o i-banco
	
i-banco.o: i-banco.c contas.h commandlinereader.h
	gcc $(FLAGS) -c i-banco.c
	
contas.o: contas.c contas.h
	gcc $(FLAGS) -c contas.c
	
commandlinereader.o: commandlinereader.c commandlinereader.h
	gcc $(FLAGS) -c commandlinereader.c

clean:
	rm -f *.o i-banco