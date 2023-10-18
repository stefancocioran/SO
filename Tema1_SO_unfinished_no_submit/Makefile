.PHONY: clean

all: build

build: LinkedList.o HashMap.o Tema1.o
	gcc -g LinkedList.o HashMap.o Tema1.o -o so-cpp

LinkedList.o: LinkedList.c
	gcc -g -c LinkedList.c

HashMap.o: HashMap.c
	gcc -g -c HashMap.c

Tema1.o: Tema1.c
	gcc -g -c Tema1.c

clean:
	rm -rf *.o
	rm -rf so-cpp