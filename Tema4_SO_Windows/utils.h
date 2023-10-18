#include "so_scheduler.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

/* Useful macro for handling error codes */
#define DIE(assertion, call_description)                                       \
	do {                                                                   \
		if (assertion) {                                               \
			fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);     \
			perror(call_description);                              \
			exit(EXIT_FAILURE);                                    \
		}                                                              \
	} while (0)

#define NEW 0
#define READY 1
#define RUNNING 2
#define WAITING 3
#define TERMINATED 4

#define INVALID_IO -1

#define SO_SUCCESS_RET 0
#define SO_ERROR_RET -1

typedef struct so_thread {
	tid_t tid;
	unsigned int state;
	unsigned int priority;
	unsigned int time;
	unsigned int io_wait;
	so_handler *handler;
	HANDLE semaphore;
	HANDLE hThread;
} so_thread_t;

typedef struct so_scheduler {
	unsigned int time_quantum;
	unsigned int io_max;
	unsigned int list_size;
	unsigned int queue_size;
	so_thread_t *running_thread;
	so_thread_t **ready_queue; // priority queue
	so_thread_t **threads;	   // a list with all created threads
} so_scheduler_t;
