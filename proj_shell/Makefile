shell: shell.o
	gcc -g -Wall -I./include/ -o ./bin/shell ./src/shell.o -L./lib/ -lpthread

shell.o: ./src/shell.c
	gcc -g -Wall -I./include/ -c -o ./src/shell.o ./src/shell.c

clean:
	rm -f ./bin/shell ./src/shell.o 