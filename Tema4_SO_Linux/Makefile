CC = gcc
CFLAGS = -g -Wall -fPIC
LDFLAGS = -Wall

.PHONY: build

all: build

build: libscheduler.so

libscheduler.so: so_scheduler.o
	$(CC) $(LDFLAGS) -shared $^ -o $@

so_scheduler.o: so_scheduler.c
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	-rm -f so_scheduler.o libscheduler.so