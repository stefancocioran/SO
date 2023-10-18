#ifndef LIN_UTILS_H_
#define LIN_UTILS_H_ 1

#include <stdio.h>
#include <stdlib.h>

/* Useful macro for handling error codes */
#define DIE(assertion, call_description)                           \
	do {                                                           \
		if (assertion) {                                           \
			fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);     \
			perror(call_description);                              \
			exit(EXIT_FAILURE);                                    \
		}                                                          \
	} while (0)

typedef struct so_page {
	/* address space */
	void *address;
	/* next virtual page */
	struct so_page *next;
} so_page_t;

#endif
