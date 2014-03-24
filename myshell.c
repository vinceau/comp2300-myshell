#define _BSD_SOURCE
#define _GNU_SOURCE

#include <readline/history.h>
#include <readline/readline.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_IN 80

/* Given the argument vector my_args, look for an existing binary in the
 * PATH environmental variable and if it exists, execute it with the
 * arguments found in my_args. If run_in_bg is nonzero, it will not wait
 * for it to return before continuing.
 */
void run_command(int arg_size, char *my_args[], int run_in_bg) {
    // manually handle change directory 
    if (!strcmp(my_args[0], "cd")) {
        char *path = (arg_size == 1) ? getenv("HOME") : my_args[1]; 
        if (chdir(path)) {
            printf("No such file or directory.\n");
        }
        return;
    }
    
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

/* Removes trailing spaces of a string.
 */
void remove_spaces(char *string) {
    for (int i = (int) strlen(string) - 1; i > 0; i--) {
        if (string[i] == ' ') {
            string[i] = 0;
        } else {
            return;
        }
    }
}

/* Replaces all the tildes in a string with the home environmental variable.
 */
void parse_input(char **in) {
    char *input = *in;
    // get the address of the first ~ character
    char* tilde = strchr(input, '~');
    if (tilde == NULL) {
        return;
    }
    // find index of the ~ in the string
    int index = tilde - input;
    // find the length of the remaining characters
    int rest = (int)strlen(input) - index - 1;
    char *newstring;
    asprintf(&newstring, "%.*s%s%.*s", index, input, getenv("HOME"), rest, input + index + 1);
    parse_input(&newstring); 
    *in = newstring;
}

int main(int argc, char *argv[]) {
    char *input;
    int arg_size;
    int bg;
    int quit = 0;

    while(!quit) {
        bg = 0;
    
        char *prompt;
        asprintf(&prompt, "%s: %s$ ", argv[0], getcwd(NULL, 0));
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
            remove_spaces(input);
            arg_size = char_count(input, ' ') + 1;
            
            char **ap, *arg_vector[arg_size + 1], *input_string;
            input_string = input;
            parse_input(&input_string);

            // split input into an argument vector by space
            // code based on the bsd man strsep
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

            run_command(arg_size, arg_vector, bg);

            free(input_string);
        }

        free(prompt);
        free(input);
    }

    return 0;
}
