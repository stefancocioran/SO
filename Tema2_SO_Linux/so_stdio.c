#include "so_stdio.h"
#include <fcntl.h> /* O_RDWR, O_CREAT, O_TRUNC, O_WRONLY */
#include <string.h>
#include <sys/stat.h>  /* open */
#include <sys/types.h> /* open */
#include <sys/wait.h>
#include <unistd.h> /* close */

#define BUF_SIZE 4096

#define R_MODE 1
#define R_MODE_PLUS 2
#define W_MODE 3
#define W_MODE_PLUS 4
#define A_MODE 5
#define A_MODE_PLUS 6

#define READ 1
#define WRITE 2

/**
 * @brief It is the type of data that describes a file and is used by all
 * other functions. A pointer to such a structure is obtained when opening a
 * file, using the so_fopen function
 */
typedef struct _so_file {
	int fd;	  // file descriptor
	int mode; // mode in which the file will be opened
	int eof;  // flag that marks if the end of the file has been reached
	int err;  // flag that marks if an error occurred with a file operation
	int last_op;	// last file operation, read or write
	int read_curr;	// indicates where the cursor is in the read buffer
	int read_total; // indicates how many bytes were read in buffer
	int write_curr; // indicates where the cursor is in the write buffer
	long offset;	// indicates where the file cursor is placed
	pid_t pid;	// process identifier
	char read_buffer[BUF_SIZE];  // stores data read from file
	char write_buffer[BUF_SIZE]; // stores data that will be written in file
} SO_FILE;

/**
 * @brief Initialize SO_FILE struct fields
 *
 * @param stream - pointer to the file for which operations are performed
 * @param fd - file descriptor associated with the stream
 * @param mode - mode in which the file will be opened
 */
void init_so_file(SO_FILE *stream, int fd, int mode)
{
	stream->fd = fd;
	stream->mode = mode;
	stream->eof = 0;
	stream->err = 0;
	stream->last_op = 0;
	stream->offset = 0;
	stream->read_curr = 0;
	stream->read_total = 0;
	stream->write_curr = 0;
	stream->read_buffer[0] = '\0';
	stream->write_buffer[0] = '\0';
}

/**
 * @brief Opens the file whose name is specified in the parameter filename and
 associates it with a stream that can be identified in future operations by the
 SO_FILE pointer returned
 *
 * @param pathname - the path to a file
 * @param mode - determines the mode in which the file will be opened
 * @return SO_FILE* - In case of error, the function returns NULL
 */
SO_FILE *so_fopen(const char *pathname, const char *mode)
{
	int fd, file_mode;

	if (strcmp(mode, "r") == 0) {
		fd = open(pathname, O_RDONLY, 0644);
		file_mode = R_MODE;
	} else if (strcmp(mode, "r+") == 0) {
		fd = open(pathname, O_RDWR, 0644);
		file_mode = R_MODE_PLUS;
	} else if (strcmp(mode, "w") == 0) {
		fd = open(pathname, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		file_mode = W_MODE;
	} else if (strcmp(mode, "w+") == 0) {
		fd = open(pathname, O_RDWR | O_CREAT | O_TRUNC, 0644);
		file_mode = W_MODE_PLUS;
	} else if (strcmp(mode, "a") == 0) {
		fd = open(pathname, O_WRONLY | O_CREAT | O_APPEND, 0644);
		file_mode = A_MODE;
	} else if (strcmp(mode, "a+") == 0) {
		fd = open(pathname, O_RDWR | O_CREAT | O_APPEND, 0644);
		file_mode = A_MODE_PLUS;
	} else {
		fd = -1;
		file_mode = 0;
	}

	if (fd < 0) {
		return NULL;
	}

	SO_FILE *fp = malloc(sizeof(SO_FILE));

	if (fp == NULL)
		return NULL;

	// Initialize SO_FILE struct fields
	init_so_file(fp, fd, file_mode);
	return fp;
}

/**
 * @brief Closes the file received as a parameter and frees the memory used by
 * the SO_FILE structure
 *
 * @param stream - pointer to the file for which operations are performed
 * @return int - Returns 0 on success or SO_EOF on error
 */
int so_fclose(SO_FILE *stream)
{
	// Write what is left in the write_buffer before closing the file
	int r = so_fflush(stream);

	if (r < 0) {
		close(stream->fd);
		free(stream);
		return SO_EOF;
	}

	r = close(stream->fd);
	free(stream);
	if (r < 0)
		return SO_EOF;

	return 0;
}

/**
 * @brief Returns the integer file descriptor associated with the file stream
 * pointed to by stream
 *
 * @param stream - pointer to the file for which operations are performed
 * @return int - In case of error, the function returns -1
 */
int so_fileno(SO_FILE *stream)
{
	if (stream->mode == 0) {
		stream->err = 1;
		return -1;
	}
	return stream->fd;
}

/**
 * @brief Writes the entire content of write_buffer into the file stream then
 * the buffer is emptied
 *
 * @param stream - pointer to the file for which operations are performed
 * @return int - If successful it returns 0 otherwise it returns SO_EOF on error
 */
int so_fflush(SO_FILE *stream)
{
	int w, total_bytes = stream->write_curr, bytes_written = 0;

	if (stream->write_curr == 0)
		return 0;

	// Function may not write all the data from write_buffer so it must
	// retry until its entire content has been written to the file
	while (bytes_written < total_bytes) {
		w = write(stream->fd, stream->write_buffer + bytes_written,
			  total_bytes - bytes_written);
		if (w < 0) {
			stream->err = 1;
			return SO_EOF;
		}
		bytes_written += w;
	}

	memset(stream->write_buffer, 0, BUF_SIZE);
	stream->write_curr = 0;
	stream->last_op = WRITE;

	return 0;
}

/**
 * @brief Sets the position indicator associated with the stream to a new
 position obtained by adding the offset value to the position specified by
 whence
 *
 * @param stream - pointer to the file for which operations are performed
 * @param offset - number of bytes to offset from origin (whence)
 * @param whence - position used as reference for the offset
 * @return int - Returns 0 on success and -1 on error
 */
int so_fseek(SO_FILE *stream, long offset, int whence)
{
	// If the last operation performed on the file was a READ operation, the
	// entire buffer must be invalidated
	if (stream->last_op == READ) {
		memset(&stream->read_buffer, 0, BUF_SIZE);
		stream->read_curr = 0;
		stream->read_total = 0;
		// If the last operation performed was a WRITE, the contents of
		// the buffer must be written to the file
	} else if (stream->last_op == WRITE)
		if (so_fflush(stream) < 0)
			return -1;

	int r = lseek(stream->fd, offset, whence);

	if (r < 0) {
		stream->err = 1;
		return -1;
	}

	stream->offset = r;
	return 0;
}

/**
 * @brief Returns the current value of the position indicator of the stream
 *
 * @param stream - pointer to the file for which operations are performed
 * @return long - In case of error, the function returns -1
 */
long so_ftell(SO_FILE *stream)
{
	if (stream->mode == 0) {
		stream->err = 1;
		return -1;
	}

	return stream->offset;
}

/**
 * @brief Reads an array of nmemb elements, each one with a size of size bytes,
 * from the stream and stores them in the block of memory specified by ptr
 *
 * @param ptr - pointer to a block of memory
 * @param size - size of each element to be read
 * @param nmemb - number of elements
 * @param stream - pointer to the file for which operations are performed
 * @return size_t - In case of error or if the end of the file has been reached,
 * the function returns 0
 */
size_t so_fread(void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
	int i, j, c;
	size_t ret = 0;

	for (i = 0; i < nmemb; i++) {
		for (j = 0; j < size; j++) {
			c = so_fgetc(stream);

			if (c < 0) {
				stream->offset += ret * size;
				return ret;
			}

			*(char *)ptr = c;
			ptr++;
		}
		ret++;
	}

	stream->offset += ret * size;
	return ret;
}

/**
 * @brief - Writes an array of count elements, each one with a size of size
 * bytes, from the block of memory pointed by ptr to the current position in the
 * stream
 *
 * @param ptr - pointer to the array of elements to be written
 * @param size - size of each element to be written
 * @param nmemb - number of elements
 * @param stream - pointer to the file for which operations are performed
 * @return size_t - Returns the number of elements written or 0 in case of error
 */
size_t so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
	int i, j, c;
	size_t count = 0, ret = 0;
	char *ptr_cpy = (char *)malloc(size * nmemb + 1);

	memcpy(ptr_cpy, ptr, size * nmemb + 1);

	for (i = 0; i < nmemb; i++) {
		for (j = 0; j < size; j++) {
			c = so_fputc((int)ptr_cpy[count], stream);

			if (c < 0) {
				free(ptr_cpy);
				stream->offset += ret * size;
				return ret;
			}
			count++;
		}
		ret++;
	}
	stream->offset += ret * size;

	free(ptr_cpy);
	return ret;
}

/**
 * @brief Invalidates read_buffer's content and inserts new data in it
 *
 * @param stream - pointer to the file for which operations are performed
 * @return int - Returns number of bytes read or -1 in case of error
 */
int fill_buffer(SO_FILE *stream)
{
	memset(&stream->read_buffer, 0, BUF_SIZE);
	int r = read(stream->fd, stream->read_buffer, BUF_SIZE);

	if (r == 0) {
		stream->eof = 1;
		return -1;
	} else if (r < 0) {
		stream->err = 1;
		return -1;
	}

	stream->read_curr = 0;
	stream->read_total = r;
	stream->last_op = READ;

	return r;
}

/**
 * @brief Reads a character from the file stream
 *
 * @param stream - pointer to the file for which operations are performed
 * @return int - Returns the character as an unsigned char extended to int or
 * SO_EOF in case of error.
 */
int so_fgetc(SO_FILE *stream)
{
	// If the read_buffer is empty or the read cursor has reached the end,
	// new data will be inserted in the buffer
	if (stream->read_curr == stream->read_total) {
		if (fill_buffer(stream) < 0)
			return SO_EOF;
	}

	unsigned char c = stream->read_buffer[stream->read_curr];

	stream->read_curr++;
	return c;
}

/**
 * @brief Writes a character to the stream
 *
 * @param c - the int promotion of the character to be written, internally
 * converted to an unsigned char
 * @param stream - pointer to the file for which operations are performed
 * @return int - Returns the written character or SO_EOF in case of error
 */
int so_fputc(int c, SO_FILE *stream)
{
	// Writing operations to a file opened in "a" mode is done as if each
	// operation were preceded by a seek at the end of the file
	if (stream->mode == A_MODE || stream->mode == A_MODE_PLUS)
		so_fseek(stream, 0, SEEK_END);

	// Flush the buffer when it is full
	if (stream->write_curr == BUF_SIZE) {
		if (so_fflush(stream) < 0)
			return -1;
	}

	unsigned char ch = (unsigned char)c;

	stream->write_buffer[stream->write_curr] = ch;
	stream->write_curr++;

	return ch;
}

/**
 * @brief Returns a value other than 0 if the end of the file has been reached
 * or 0 otherwise.
 *
 * @param stream - pointer to the file for which operations are performed
 */
int so_feof(SO_FILE *stream) { return stream->eof; }

/**
 * @brief Returns a value other than 0 if an error occurred with a file
 * operation or 0 otherwise.
 *
 * @param stream - pointer to the file for which operations are performed
 */
int so_ferror(SO_FILE *stream) { return stream->err; }

/**
 * @brief Launches a new process which will execute the command specified by
 * the command parameter. "sh -c command" will be executed, using a pipe to
 * redirect the standard input / standard output of the new process
 *
 * @param command - the command that will be executed
 * @param type - specifies only reading or writing
 * @return SO_FILE* - Returns a SO_FILE structure on which the usual
 * read / write operations can then be performed, as if it were a regular file
 * or NULL if the fork/pipe call fails
 */
SO_FILE *so_popen(const char *command, const char *type)
{
	int filedes[2];
	int fd, mode;

	if (pipe(filedes) < 0)
		return NULL;

	pid_t pid = fork();

	switch (pid) {
	case -1:
		// Error forking
		close(filedes[0]);
		close(filedes[1]);
		return NULL;
	case 0:
		// Child process
		if (*type == 'r') {
			close(filedes[0]);
			if (filedes[1] != STDOUT_FILENO) {
				dup2(filedes[1], STDOUT_FILENO);
				close(filedes[1]);
			}
		} else {
			close(filedes[1]);
			if (filedes[0] != STDIN_FILENO) {
				dup2(filedes[0], STDIN_FILENO);
				close(filedes[0]);
			}
		}

		execl("/bin/sh", "sh", "-c", command, NULL);
		// Only if exec failed
		return NULL;
	default:
		if (*type == 'r') {
			mode = R_MODE;
			fd = filedes[0];
			close(filedes[1]);
		} else {
			fd = filedes[1];
			mode = W_MODE;
			close(filedes[0]);
		}
		break;
	}

	// Initialize SO_FILE struct fields
	SO_FILE *fp = malloc(sizeof(SO_FILE));

	fp->pid = pid;
	init_so_file(fp, fd, mode);
	return fp;
}

/**
 * @brief Only called for files opened with so_popen. Waits for the process
 * launched by so_popen to end and frees up the memory occupied by the SO_FILE
 * structure
 *
 * @param stream - pointer to the file for which operations are performed
 * @return int - Returns waitpid process exit code. If the waitpid call fails,
 * it returns -1
 */
int so_pclose(SO_FILE *stream)
{
	int status;
	pid_t pid = stream->pid;

	if (so_fclose(stream) < 0 || waitpid(pid, &status, 0) < 0)
		return -1;

	return status;
}
