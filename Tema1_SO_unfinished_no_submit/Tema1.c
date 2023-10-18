#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "HashMap.h"

void fatal(char *mesaj_eroare)
{
	perror(mesaj_eroare);
	exit(0);
}

/*
 * Returns next token found after a preprocessor directive
 */
char *get_next_token(char line[256], int i, char *sequence)
{
	char *value;
	int count = 0;

	for (int j = i + 1; j < strlen(line) + 1; j++) {
		if (strchr("\n ", line[j]) == NULL) {
			sequence[count] = line[j];
			count++;
		} else {
			value = (char *)malloc(strlen(sequence) + 1);
			memcpy(value, sequence, strlen(sequence) + 1);
			return value;
		}
	}
	return NULL;
}

/*
 * Replaces given DEFINES with corresponding values
 */
void process_line(char line[256], struct HashMap *hmap_defines, FILE *fp)
{
	char delim[] = "\t []{}<>=+-*/%!&|^.,:;()\\\n";
	char final[256], token[256];
	int count = 0, quotations = 0, print = 0;

	memset(&final, 0, sizeof(final));
	for (int i = 0; i < strlen(line) + 1; i++) {
		if (strchr(delim, line[i]) == NULL) {
			token[count] = line[i];
			count++;
		} else {
			token[count] = '\0';
			if (has_key(hmap_defines, token) && quotations == 0) {
				strcat(final, get(hmap_defines, token));
			} else {
				// Replace defines only if they are not found
				// between quotation marks in printf function
				if (strcmp(token, "printf") == 0) {
					print = 1;
				} else if (strchr(token, '\"') != NULL &&
					   print) {
					if (quotations == 0) {
						quotations++;
					} else {
						quotations--;
						print = 0;
					}
				}
				strcat(final, token);
			}
			strncat(final, &line[i], 1);
			memset(&token, 0, sizeof(token));
			count = 0;
		}
	}
	fprintf(fp, "%s", final);
}

void file_preprocessing(FILE *fp, FILE *fp2, struct HashMap *hmap_defines)
{
	int i, j;
	char delim[] = "\t\r\n []{}<>=+-*/%!&|^.,:;()\\";
	char line[256], sequence[256];
	int count = 0, trueval = 1;

	while (fgets(line, 256, fp) != NULL) {
		// Skip blank line
		if (line[0] == '\n')
			continue;

		memset(&sequence, 0, sizeof(sequence));
		count = 0;

		for (i = 0; i < strlen(line) + 1; i++) {
			if (line[i] != ' ' && line[i] != '\0') {
				sequence[count] = line[i];
				count++;
			} else {
				// Get first token from line
				sequence[count] = '\0';
				if (strstr(sequence, "#define")) {
					memset(&sequence, 0, sizeof(sequence));
					count = 0;
					int aux;
					char *key, *value;

					// Find key
					for (j = i + 1; j < strlen(line) + 1;
					     j++) {
						if (strchr(" ", line[j]) ==
						    NULL) {
							sequence[count] =
							    line[j];
							count++;
						} else {
							key = (char *)malloc(
							    strlen(sequence) +
							    1);
							memcpy(
							    key, sequence,
							    strlen(sequence) +
								1);
							aux = j;
							break;
						}
					}

					memset(&sequence, 0, sizeof(sequence));
					count = 0;

					// Find value
					for (j = aux + 1; j < strlen(line) + 1;
					     j++) {
						if ('\t' == line[j] ||
						    (' ' == line[j] &&
						     line[j + 1] == ' ') ||
						    (' ' == line[j] &&
						     line[j + 1] == '\\')) {
							// Remove useless spaces
							continue;
						} else if ('\n' == line[j]) {
							// Value found
							value = (char *)malloc(
							    strlen(sequence) +
							    1);
							memcpy(
							    value, sequence,
							    strlen(sequence) +
								1);
							break;
						} else if ('\\' == line[j]) {
							// If there is a '\',
							// search on next line
							fgets(line, 256, fp);
							j = 0;
						} else {
							sequence[count] =
							    line[j];
							count++;
						}
					}
					put(hmap_defines, key, strlen(key) + 1,
					    value, strlen(value) + 1);
					free(key);
					free(value);
					break;
				} else if (strstr(sequence, "#undef")) {
					memset(&sequence, 0, sizeof(sequence));
					count = 0;
					char *token =
					    get_next_token(line, i, sequence);

					if (token == NULL)
						exit(12);

					remove_entry(hmap_defines, token);
					free(token);
					break;
				} else if (strcmp(sequence, "#if") == 0 ||
					   (strstr(sequence, "#elif") &&
					    trueval == 0)) {
					memset(&sequence, 0, sizeof(sequence));
					count = 0;
					char *token =
					    get_next_token(line, i, sequence);

					if (token == NULL)
						exit(12);

					int value;

					if (has_key(hmap_defines, token))
						value = atoi(
						    get(hmap_defines, token));
					else
						value = atoi(token);

					if (value)
						trueval = 1;
					else
						trueval = 0;

					free(token);
					break;
				} else if (strstr(sequence, "#else")) {
					if (!trueval) {
						trueval = 1;
						break;
					}
					trueval = 0;
				} else if (strstr(sequence, "#ifdef")) {
					memset(&sequence, 0, sizeof(sequence));
					count = 0;

					char *token =
					    get_next_token(line, i, sequence);

					if (token == NULL)
						exit(12);

					if (has_key(hmap_defines, token))
						trueval = 1;
					else
						trueval = 0;

					free(token);
					break;
				} else if (strstr(sequence, "#ifndef")) {
					memset(&sequence, 0, sizeof(sequence));
					count = 0;
					char *token =
					    get_next_token(line, i, sequence);

					if (token == NULL)
						exit(12);

					if (!has_key(hmap_defines, token))
						trueval = 1;
					else
						trueval = 0;

					free(token);
					break;
				} else if (strstr(sequence, "#include")) {
					break;
				} else if (strstr(sequence, "#endif")) {
					trueval = 1;
					break;
				} else if (!trueval) {
					continue;
				} else {
					// No preprocessor directives found
					process_line(line, hmap_defines, fp2);
					break;
				}
				memset(&sequence, 0, sizeof(sequence));
				count = 0;
			}
		}
	}
}

void free_mem(FILE *fp_input, FILE *fp_output, struct HashMap *hmap_defines)
{
	free_hmap(hmap_defines);
	fclose(fp_input);
	fclose(fp_output);
}

int main(int argc, char *argv[])
{
	FILE *fp_input = stdin, *fp_output = stdout;
	struct HashMap *hmap_defines;
	struct LinkedList *list_headers;

	int current_index = 0;

	int last_arg = 0, current_key, i;
	char *infile_name = NULL;

	hmap_defines = malloc(sizeof(struct HashMap));
	init_hmap(hmap_defines, 10, hash_function, cmp_strings);

	list_headers = malloc(sizeof(struct LinkedList));
	init_list(list_headers);

	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {
			case 'D':
				last_arg = 1;
				break;
			case 'I':
				last_arg = 2;
				break;
			case 'o':
				last_arg = 3;
				break;
			default:
				last_arg = 4;
				break;
			}

			// Remove -D/-I/-o parameter
			if (strlen(argv[i]) > 2)
				argv[i] += 2;
			else
				continue;
		}

		switch (last_arg) {
		case 0:
			if (fp_input == stdin) {
				infile_name = malloc(strlen(argv[i]) + 1);
				memcpy(infile_name, argv[i],
				       strlen(argv[i]) + 1);

				fp_input = fopen(argv[i], "r");
				if (fp_input == NULL) {
					perror("Error opening file");
					exit(12);
				}
			} else if (fp_output == stdout) {
				fp_output = fopen(argv[i], "w");
			} else {
				free_mem(fp_input, fp_output, hmap_defines);
				exit(12);
			}
			break;

		case 1:
			// -D
			char *token = strtok(argv[i], "=");
			char *key = (char *)malloc(strlen(token) + 1);

			memcpy(key, token, strlen(token) + 1);
			token = strtok(NULL, "=");

			if (token != NULL) {
				char *value = (char *)malloc(strlen(token) + 1);

				memcpy(value, token, strlen(token) + 1);
				put(hmap_defines, key, strlen(key) + 1, value,
				    strlen(value) + 1);
				free(value);
			} else {
				put(hmap_defines, key, strlen(key) + 1, "",
				    strlen("") + 1);
			}
			free(key);
			last_arg = 0;
			break;
		case 2:
			// -I
			add_node(list_headers, current_index, argv[i]);
			current_index++;
			last_arg = 0;
			break;
		case 3:
			// -o
			if (fp_output == stdout) {
				fp_output = fopen(argv[i], "w");
			} else {
				free_mem(fp_input, fp_output, hmap_defines);
				exit(12);
			}
			last_arg = 0;
			break;
		default:
			perror("Error wrong parameter");
			free_mem(fp_input, fp_output, hmap_defines);
			exit(12);
			break;
		}
	}

	char *path_file = NULL;

	if (infile_name != NULL) {
		path_file = malloc(strlen(infile_name) + 1);
		memcpy(path_file, infile_name, strlen(infile_name) + 1);

		for (i = strlen(path_file); i >= 0; i--) {
			if (path_file[i] != '/')
				path_file[i] = '\0';
			else
				break;
		}
	} else {
		path_file = malloc(strlen("./") + 1);
		memcpy(path_file, "./", strlen("./") + 1);
	}

	add_node(list_headers, current_index, path_file);
	file_preprocessing(fp_input, fp_output, hmap_defines);

	free(path_file);

	if (infile_name != NULL)
		free(infile_name);

	free_list(&list_headers);
	free_mem(fp_input, fp_output, hmap_defines);

	return 0;
}
