CFLAGS = -Wall -g -pedantic

all: i-banco i-banco-terminal

i-banco: i-banco.o contas.o
	gcc -pthread -o i-banco i-banco.o contas.o

i-banco-terminal: i-banco-terminal.o commandlinereader.o
	gcc -o i-banco-terminal i-banco-terminal.o commandlinereader.o

i-banco.o: i-banco.c contas.h
	gcc $(CFLAGS) -c i-banco.c

i-banco-terminal.o: i-banco-terminal.c commandlinereader.h
	gcc $(CFLAGS) -c i-banco-terminal.c

contas.o: contas.c contas.h
	gcc $(CFLAGS) -c contas.c

commandlinereader.o: commandlinereader.c commandlinereader.h
	gcc $(CFLAGS) -c commandlinereader.c

clean:
	rm -f *.o i-banco i-banco-terminal

zip:
	zip i-banco i-banco.c i-banco-terminal.c contas.c commandlinereader.c contas.h commandlinereader.h Makefile
