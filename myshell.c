#define _BSD_SOURCE
#define _GNU_SOURCE

#include <readline/history.h>
#include <readline/readline.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

/* Shifts all the arguments in an arg_vector to the left starting at index.
 */
void shift_args(int *arg_size, char *arg_vector[], int index) {
    for (int i = index; i < *arg_size - 1; i++) {
        arg_vector[i] = arg_vector[i + 1];
    }
    if (index < *arg_size) {
        arg_vector[--*arg_size] = NULL;
    }
}


/* Given the argument vector my_args, look for an existing binary in the
 * PATH environmental variable and if it exists, execute it with the
 * arguments found in my_args. If list argument is &, it will not wait
 * for it to return before continuing.
 */
void run_command(int arg_size, char *my_args[], int fds[]) {
    int bg = 0;

    if (!strcmp(my_args[arg_size - 1], "&")) {
        // run command in background
        //printf("\n");
        my_args[arg_size - 1] = NULL;
        arg_size--;
        bg = 1;
    }

    switch (fork()) {
        case -1:
            // error occured when forking
            printf("Failed to fork a child process.\n");
            exit(EXIT_FAILURE);
            break;
        case 0:
            dup2(fds[0], fileno(stdin));
            close(fds[0]);
            dup2(fds[1], fileno(stdout));
            close(fds[1]);
            // successfully forked child process
            if (execvp(*my_args, my_args) < 0) {
                printf("%s: command not found\n", *my_args);
                exit(EXIT_FAILURE);
            }
            break;
        default:
            // parent process
            if (!bg) {
                wait(NULL);
            }
            break;
    }
}

/* Counts the number of arguments in an argument string.
 * Ensures correct handling of quotes and spaces.
 */
int arg_count(char string[]) {
    int dquote = 0; // flag for waiting double quote
    int squote = 0; // flag for waiting single quote
    int count = 1;
    for (int i = 0; i < (int)strlen(string); i++) {
        char c = string[i]; 
        if (c == '\"') {
            dquote = (dquote) ? 0 : 1; 
        }
        if (c == '\'') {
            squote = (squote) ? 0 : 1; 
        }
        if (c == ' ' && !dquote && !squote) {
            // don't count repeated spaces
            if (i > 0 && string[i - 1] != ' ') {
                count++;
            }
        }
    }
    return count;
}

/* Shifts all the characters of the string to the left by one.
 * Useful for removing leading white space.
 */
void shift_string(char string[]) {
    int len = (int)strlen(string) - 1; 
    for (int i = 0; i < len; i++) {
        string[i] = string[i + 1];
    }
    string[len] = 0;
}

/* Removes leading and trailing spaces of a string.
 */
void eat_spaces(char string[]) {
    while (string[0] == ' ') {
        shift_string(string);
    }
    for (int i = (int) strlen(string) - 1; i > 0; i--) {
        if (string[i] == ' ') {
            string[i] = 0;
        } else {
            return;
        }
    }
}

/* Returns the first index of a character in a string.
 * If character can't be found, it returns -1.
 */
int index_of(char input[], char c) {
    char* address = strchr(input, c);
    if (address == NULL) {
        return -1;
    }
    return address - input;
}

/* Replaces all the tildes in a string with the home environmental variable.
 */
void parse_input(char **in) {
    char *input = *in;
    // find index of the ~ in the string
    int index = index_of(input, '~');
    if (index < 0) {
        return;
    }
    // find the length of the remaining characters
    int rest = (int)strlen(input) - index - 1;
    char *newstring;
    asprintf(&newstring, "%.*s%s%.*s", index, input, getenv("HOME"), rest, input + index + 1);
    parse_input(&newstring); // replace the rest of the tildes 
    *in = newstring;
}

/* Save current directory to old then changes the current
 * directory to path.
 */
int change_dir(char *path, char **old) {
    char *temp = *old;
    *old = getcwd(NULL, 0);
    return chdir((!strcmp(path, "-")) ? temp : path);
} 

/* Given an input string, creates a space seperated argument vector.
 */
void make_vector(char *input, char *arg_vector[], int arg_size) {
    char **ap, *input_string;
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
    free(input_string);
}

int main(int argc, char *argv[]) {
    char *prompt, *input, *old_dir;
    int arg_size, quit = 0;

    while(!quit) {
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
            eat_spaces(input);
            arg_size = arg_count(input);
            char *arg_vector[arg_size + 1];
            make_vector(input, arg_vector, arg_size);

            // manually handle change directory 
            if (!strcmp(arg_vector[0], "cd")) {
                char *path = (arg_size == 1) ? getenv("HOME") : arg_vector[1]; 
                if (change_dir(path, &old_dir)) {
                    printf("No such file or directory.\n");
                }
                continue;
            }

            int fds[3] = {dup(fileno(stdout)), dup(fileno(stderr))};
            int error = 0;

            for (int i = 0; i < arg_size - 1; i++) {
                int f = 0;
                if (!strcmp(arg_vector[i], "<")) {
                    f = open(arg_vector[i + 1], O_RDONLY, 0644);
                    if (f > 0) {
                        fds[0] = f;
                    }
                }
                else if (!strcmp(arg_vector[i], ">") || !strcmp(arg_vector[i], ">>")) {
                    int mode = O_RDWR | O_CREAT;
                    mode |= !strcmp(arg_vector[i], ">>") ? O_APPEND : O_TRUNC;
                    
                    f = open(arg_vector[i + 1], mode, 0644);
                    if (f > 0) {
                        fds[1] = f;
                    }
                }
                if (f > 0) {
                    // remove the two arguments from the vector
                    shift_args(&arg_size, arg_vector, i);
                    shift_args(&arg_size, arg_vector, i--); // decrement i because value changed
                } else if (f < 0) {
                    printf("%s: No such file.\n", arg_vector[i + 1]);
                    error = 1;
                    break;
                }
            }

            if (!error) {
                run_command(arg_size, arg_vector, fds);
            }

            close(fds[0]);
            close(fds[1]);
        }

        free(prompt);
        free(input);
    }

    return 0;
}
