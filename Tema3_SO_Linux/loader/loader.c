/*
 * Loader Implementation
 *
 * 2018, Operating Systems
 */

#include "exec_parser.h"
#include "utils.h"
#include <fcntl.h> /* O_RDWR, O_CREAT, O_TRUNC, O_WRONLY */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>  /* open */
#include <sys/types.h> /* open */
#include <unistd.h>

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

so_page_t *loader; /*head of mapped pages list*/
static so_exec_t *exec;
static int page_size;
static struct sigaction old_action;
const char *filepath;

void map_page(char *addr)
{
	so_page_t *new_page = malloc(sizeof(so_page_t));

	new_page->address = addr;
	new_page->next = loader;
	loader = new_page;
}

void read_from_file(char *buffer, int offset, int bytes_to_read)
{
	int rc;
	int fd;
	int bytes_read = 0;

	fd = open(filepath, O_RDONLY);
	DIE(fd < 0, "open");

	rc = lseek(fd, offset, SEEK_SET);
	DIE(rc < 0, "lseek");

	memset(buffer, 0, page_size);

	while (bytes_read < bytes_to_read) {
		rc = read(fd, buffer + bytes_read, bytes_to_read - bytes_read);
		if (rc < 0)
			DIE(rc < 0, "read");

		bytes_read += rc;
	}

	rc = close(fd);
	DIE(rc < 0, "close");
}

int check_page_mapped(char *addr)
{
	so_page_t *page = loader;

	while (page) {
		if (addr - (char *)page->address >= 0 &&
		    addr - (char *)page->address < page_size)
			return 1;

		page = page->next;
	}
	return 0;
}

so_seg_t *get_page_fault_segment(char *addr)
{
	for (int i = 0; i < exec->segments_no; i++) {
		char *segment_vaddr = (char *)exec->segments[i].vaddr;
		unsigned int mem_size = exec->segments[i].mem_size;

		if (addr >= segment_vaddr && addr <= segment_vaddr + mem_size)
			return &(exec->segments[i]);
	}
	return NULL;
}

static void segv_handler(int signum, siginfo_t *info, void *context)
{
	char *addr, *buffer;
	int rc;

	/*
	 * Check if the signal is SIGSEGV
	 * Run page fault default handler
	 */
	if (signum != SIGSEGV) {
		old_action.sa_sigaction(signum, info, context);
		return;
	}

	/*
	 * Obtain from siginfo_t the memory location which caused the page fault
	 */
	addr = (char *)info->si_addr;

	/* Obtain the segment which caused the page fault */
	so_seg_t *page_fault_segment = get_page_fault_segment(addr);

	/*
	 * If not found, it is an invalid memory access
	 * Run page fault default handler
	 */
	if (page_fault_segment == NULL) {
		old_action.sa_sigaction(signum, info, context);
		return;
	}

	/*
	 * If already mapped, it is an unauthorized memory access attempt
	 * The segment does not have the necessary permissions
	 * Run page fault default handler
	 */
	if (check_page_mapped(addr)) {
		old_action.sa_sigaction(signum, info, context);
		return;
	}

	/* Obtain the page which caused the page fault */
	int page = (addr - (char *)page_fault_segment->vaddr) / page_size;
	int file_offset = page_fault_segment->offset + page * page_size;
	int bytes_to_read = 0;

	/* Check if page surpassed the file size - there is data left to read*/
	if (page * page_size < page_fault_segment->file_size) {
		int next_page = (page + 1) * page_size;
		int diff = next_page - page_fault_segment->file_size;
		bytes_to_read = MIN(page_size, page_size - diff);
	}

	buffer = malloc(page_size * sizeof(char));
	DIE(buffer == NULL, "malloc");

	if (bytes_to_read)
		read_from_file(buffer, file_offset, bytes_to_read);

	/* Fill the buffer */
	memset(buffer + bytes_to_read, 0, page_size - bytes_to_read);

	void *page_start = (void *)page_fault_segment->vaddr + page * page_size;

	/* Map the page to the corresponding address */
	char *mem = mmap(page_start, page_size, PROT_WRITE,
			 MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
	DIE(mem == MAP_FAILED, "mmap");

	/* Write buffer data in the mapped memory */
	memcpy(mem, buffer, page_size);

	/* Change with the permissions of the given segment */
	rc = mprotect(mem, page_size, page_fault_segment->perm);
	DIE(rc < 0, "mprotect");

	/* Add page to loader */
	map_page(mem);

	free(buffer);
}

static void set_signal(void)
{
	struct sigaction action;
	int rc;

	action.sa_sigaction = segv_handler;
	sigemptyset(&action.sa_mask);
	sigaddset(&action.sa_mask, SIGSEGV);
	action.sa_flags = SA_SIGINFO;

	rc = sigaction(SIGSEGV, &action, &old_action);
	DIE(rc == -1, "sigaction");
}

int so_init_loader(void)
{
	/* Initialize on-demand loader */
	loader = NULL;
	page_size = getpagesize();

	set_signal();

	return -1;
}

int so_execute(char *path, char *argv[])
{

	filepath = path;

	exec = so_parse_exec(path);
	if (!exec)
		return -1;

	so_start_exec(exec, argv);

	return -1;
}
