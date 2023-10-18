.PHONY: clean

all: build

build: so_stdio.o
	gcc -shared -Wall so_stdio.o -o libso_stdio.so

so_stdio.o: so_stdio.c
	gcc -g -Wall -fPIC -c so_stdio.c

clean:
	rm -rf *.so
	rm -rf *.o
	rm -rf so_stdio