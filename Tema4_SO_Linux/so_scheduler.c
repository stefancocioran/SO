#include "utils.h"

unsigned int MAX_THREADS = 1024;

static so_scheduler_t *scheduler;

/**
 * @brief Inserts the given thread in a list containing all threads that have
 * been created, in order to keep track of their current state
 *
 * @param thread
 */
void add_thread_list(so_thread_t *thread)
{
	// If there is not enough space for a new thread, double the list's and
	// queue's size
	if (scheduler->list_size == MAX_THREADS) {
		MAX_THREADS *= 2;

		scheduler->threads = realloc(scheduler->threads, MAX_THREADS);
		DIE(scheduler->threads == NULL, "realloc");

		scheduler->ready_queue =
		    realloc(scheduler->ready_queue, MAX_THREADS);
		DIE(scheduler->ready_queue == NULL, "realloc");
	}

	// Insert thread into list
	scheduler->threads[scheduler->list_size] = thread;
	scheduler->list_size++;
}

/**
 * @brief Removes the first thread from the queue (the biggest priority one)
 *
 */
void pop_queue(void)
{
	// Shift queue elements to the left
	for (int i = 0; i < scheduler->queue_size - 1; i++)
		scheduler->ready_queue[i] = scheduler->ready_queue[i + 1];

	scheduler->queue_size--;
	scheduler->ready_queue[scheduler->queue_size] = NULL;
}

/**
 * @brief Inserts a thread in the priority queue
 *
 * @param thread - the thread that is inserted
 */
void push_queue(so_thread_t *thread)
{
	int index = 0;

	// Whenever a thread is pushed into the queue, it becomes ready to run
	thread->state = READY;

	// Find the corresponding index for the thread based on its priority
	for (int i = 0; i < scheduler->queue_size; i++) {
		if (thread->priority > scheduler->ready_queue[i]->priority)
			break;
		index++;
	}

	scheduler->queue_size++;

	// Shift the other queue elements to the right to make room this one
	for (int i = scheduler->queue_size - 1; i > index; i--)
		scheduler->ready_queue[i] = scheduler->ready_queue[i - 1];

	// Insert new element at index
	scheduler->ready_queue[index] = thread;
}

/**
 * @brief Runs the next top priority thread
 *
 */
void set_next_running_thread(void)
{
	int ret;
	so_thread_t *top_priority_thread;

	if (scheduler->queue_size == 0)
		return;

	// Get first thread from queue
	top_priority_thread = scheduler->ready_queue[0];
	// Set it as the currently running one
	scheduler->running_thread = top_priority_thread;

	// Remove it from the queue and mark it as RUNNING
	pop_queue();
	top_priority_thread->state = RUNNING;

	// Release the semaphore of the thread to allow execution
	ret = sem_post(&top_priority_thread->semaphore);
	DIE(ret != 0, "sem_post");
}

/**
 * @brief Interrupts the thread that is currently running
 *
 * @param running_thread - thread that is interrupted
 */
void halt_running_thread(so_thread_t *running_thread)
{
	int ret;

	ret = sem_wait(&running_thread->semaphore);
	DIE(ret != 0, "sem_wait");
}

/**
 * @brief Run the next top priority thread if the time quantum of the currently
 * running one has expired or a bigger priority thread has been inserted in the
 * priority queue (by executing so_fork or so_signal)
 *
 */
void update_scheduler(void)
{
	so_thread_t *running_thread = scheduler->running_thread;

	if (!running_thread)
		return;

	// Update running thread if time quantum expired
	if (running_thread->time == 0) {
		running_thread->time = scheduler->time_quantum;
		push_queue(running_thread);
		set_next_running_thread();
		halt_running_thread(running_thread);
		return;
	}

	if (scheduler->queue_size == 0)
		return;

	so_thread_t *top_priority_thread = scheduler->ready_queue[0];

	// Run a bigger priority thread if found in queue and halt the
	// current one
	if (top_priority_thread->priority > running_thread->priority) {
		push_queue(running_thread);
		set_next_running_thread();
		halt_running_thread(running_thread);
	}
}

/**
 * @brief The routine where the thread begins, with arg as its only argument.
 * It is executed when pthread_create() function creates a new thread.
 *
 * @param arg - The argument to pass to start_thread (a thread structure).
 */
void start_thread(void *arg)
{
	int ret;
	so_thread_t *thread = (so_thread_t *)arg;

	// Wait for thread to be scheduled
	ret = sem_wait(&thread->semaphore);
	DIE(ret != 0, "sem_wait");

	// Call handler(prio)
	thread->handler(thread->priority);

	// Exit thread
	thread->state = TERMINATED;

	// The current thread finished, run the next one
	set_next_running_thread();
}

/**
 * @brief Allocates memory for a thread structure and initializes it
 *
 * @param func - handler function
 * @param priority - thread priority
 * @return so_thread_t* - the newly allocated thread structure
 */
so_thread_t *init_thread(so_handler *func, unsigned int priority)
{
	int ret;
	so_thread_t *thread;

	thread = malloc(sizeof(so_thread_t));
	DIE(thread == NULL, "malloc");

	thread->tid = INVALID_TID;
	thread->state = NEW;
	thread->priority = priority;
	thread->time = scheduler->time_quantum;
	thread->handler = func;
	thread->io_wait = INVALID_IO;

	ret = sem_init(&thread->semaphore, 0, 0);
	DIE(ret != 0, "sem_init()");

	return thread;
}

int so_init(unsigned int time_quantum, unsigned int io)
{
	if (scheduler || time_quantum == 0 || io > SO_MAX_NUM_EVENTS)
		return SO_ERROR;

	scheduler = malloc(sizeof(so_scheduler_t));
	DIE(scheduler == NULL, "malloc");

	scheduler->time_quantum = time_quantum;
	scheduler->io_max = io;
	scheduler->list_size = 0;
	scheduler->queue_size = 0;
	scheduler->running_thread = NULL;

	scheduler->threads = calloc(MAX_THREADS, sizeof(so_thread_t *));
	DIE(scheduler->threads == NULL, "calloc");

	scheduler->ready_queue = calloc(MAX_THREADS, sizeof(so_thread_t *));
	DIE(scheduler->ready_queue == NULL, "calloc");

	return SO_SUCCESS;
}

tid_t so_fork(so_handler *func, unsigned int priority)
{
	int ret;
	so_thread_t *thread;

	if (!func || priority > SO_MAX_PRIO)
		return INVALID_TID;

	// Initialize thread structure
	thread = init_thread(func, priority);

	add_thread_list(thread);

	// Create new thread that will execute start_thread function
	ret = pthread_create(&thread->tid, NULL, (void *)start_thread,
			     (void *)thread);
	DIE(ret != 0, "pthread_create()");

	// Add new thread to priority queue, now it is in READY state
	push_queue(thread);

	// Set new thread as RUNNING if there is none running
	if (!scheduler->running_thread)
		set_next_running_thread();
	else {
		// Spend time unit for forking if there is a thread running
		scheduler->running_thread->time--;
		// A bigger priority thread might have been added, update
		update_scheduler();
	}

	// Returns the id of the newly created thread
	return thread->tid;
}

void so_exec(void)
{
	so_thread_t *running_thread = scheduler->running_thread;

	if (!running_thread)
		return;

	running_thread->time--;

	// Update running thread if time quantum expired
	update_scheduler();
}

int so_wait(unsigned int io)
{
	so_thread_t *running_thread = scheduler->running_thread;

	// Check if it exceeds the number of supported events
	if (io >= scheduler->io_max)
		return SO_ERROR;

	running_thread->io_wait = io;
	running_thread->state = WAITING;

	// The currently running thread waits, run the next one
	set_next_running_thread();
	halt_running_thread(running_thread);

	return SO_SUCCESS;
}

int so_signal(unsigned int io)
{
	int thread_count = 0;

	// Check if it exceeds the number of supported events
	if (io >= scheduler->io_max)
		return SO_ERROR;

	// Signal all the threads that wait for the given event to wake up
	for (int i = 0; i < scheduler->list_size; i++) {
		if (scheduler->threads[i]->io_wait == io) {
			thread_count++;
			scheduler->threads[i]->io_wait = INVALID_IO;
			push_queue(scheduler->threads[i]);
		}
	}

	// A bigger priority thread might have been signaled, update scheduler
	update_scheduler();

	return thread_count;
}

void so_end(void)
{
	int ret;

	if (!scheduler)
		return;

	for (int i = 0; i < scheduler->list_size; i++) {
		ret = pthread_join(scheduler->threads[i]->tid, NULL);
		DIE(ret != 0, "pthread_join");
	}

	for (int i = 0; i < scheduler->list_size; i++) {
		ret = sem_destroy(&scheduler->threads[i]->semaphore);
		DIE(ret != 0, "sem_destroy");

		free(scheduler->threads[i]);
	}

	free(scheduler->threads);
	free(scheduler->ready_queue);
	free(scheduler);
	scheduler = NULL;
}
