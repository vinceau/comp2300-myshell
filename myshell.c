#define _BSD_SOURCE
#define _GNU_SOURCE

#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_IN 80

/* Given the argument vector my_args, look for an existing binary in the
 * PATH environmental variable and if it exists, execute it with the
 * arguments found in my_args. If run_in_bg is nonzero, it will not wait
 * for it to return before continuing.
 */
void run_command(char *my_args[], int run_in_bg) {
	switch (fork()) {
		case -1:
			// error occured when forking
			printf("Failed to fork a child process.\n");
			exit(EXIT_FAILURE);
			break;
		case 0:
			// successfully forked child process
			if (execvp(*my_args, my_args) < 0) {
				printf("%s: command not found\n", *my_args);
				exit(EXIT_FAILURE);
			}
			break;
		default:
			// parent process
			if (!run_in_bg) {
				wait(NULL);
			}
			break;
	}
}

/* Counts the occurance of a character in a string.
 */
int char_count(char string[], char c) {
	int count = 0;
	for (int i = 0; i < (int)strlen(string); i++) {
		if (string[i] == c) {
			count++;
		}
	}
	return count;
}

int main(int argc, char *argv[]) {
    char *input;
	int arg_size;	
	int bg;
	int quit = 0;

	char *prompt;
	asprintf(&prompt, "%s: %s$ ", argv[0], getenv("USER"));		
 
    while(!quit) {
        bg = 0;
        input = readline(prompt);

		if (input == NULL) {
			printf("\n");
            quit = 1;
		}

		else if (strlen(input) > 0) {
			if (!strcmp(input, "exit")) {
				quit = 1;
				break;
			}

			add_history(input);
			arg_size = char_count(input, ' ') + 1;

			// split input into an argument vector by space
			// code based on the bsd man strsep
			char **ap, *arg_vector[arg_size + 1], *input_string;
			input_string = input;
			for (ap = arg_vector; (*ap = strsep(&input_string, " ")) != NULL;) {
				if (**ap != '\0') {
					if (++ap >= &arg_vector[arg_size + 1]) {
						break;
					}
				}
			}

			if (!strcmp(arg_vector[arg_size - 1], "&")) {
                // run command in background
				printf("\n");
				arg_vector[arg_size - 1] = NULL;
				arg_size--;
				bg = 1;
			}

			run_command(arg_vector, bg);

		}
 
        free(input);
    }
    
    free(prompt);
    return 0;
}
