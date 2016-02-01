# Makefile

all: par-shell fibonacci par-shell-terminal

par-shell: par-shell.o commandlinereader.o list.o
	gcc -pthread -o par-shell par-shell.o commandlinereader.o list.o

par-shell-terminal: par-shell-terminal.o commandlinereader.o
	gcc -pthread -o par-shell-terminal par-shell-terminal.o commandlinereader.o

fibonacci: fibonacci.o
	gcc -o fibonacci fibonacci.o

par-shell.o: par-shell.c
	gcc -pthread -g -c par-shell.c

par-shell-terminal.o: par-shell-terminal.c
	gcc -g -c par-shell-terminal.c

commandlinereader.o: commandlinereader.c commandlinereader.h
	gcc -g -c commandlinereader.c

fibonacci.o: fibonacci.c
	gcc -g -c fibonacci.c

list.o: list.c list.h
	gcc -g -c list.c

clean:
	rm -f *.o par-shell fibonacci

run:
	./par-shell-terminal par-shell-in < input.txt

run2:
	./par-shell-terminal par-shell-in < input2.txt

run3:
	./par-shell-terminal par-shell-in < input3.txt
