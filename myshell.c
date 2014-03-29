#define _BSD_SOURCE
#define _GNU_SOURCE

#include <error.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <readline/history.h>
#include <readline/readline.h>

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

/* Shifts all the characters of the string to the left by times.
 * Useful for removing leading white space.
 */
void shift_string(int from, char string[], int times) {
    int len = (int)strlen(string);
    if (from < len) {
        for (int i = from; i + times < len; i++) {
            string[i] = string[i + times];
        }
        string[len - times] = 0;
    }
}

/* Counts the number of arguments in an argument string,
 * separated by the character split. Ensures correct handling
 * of quotes. It counts a block of split characters as one.
 */
int arg_count(char string[], char split) {
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
        if (c == split && !dquote && !squote) {
            // don't count repeats
            if (i > 0 && string[i - 1] != split) {
                count++;
            }
        }
    }
    return count;
}

/* Removes leading, trailing and duplicates of character c from a string.
 */
void eat(char string[], char c) {
    while (string[0] == c) {
        shift_string(0, string, 1);
    }
    for (int i = (int) strlen(string) - 1; i > 0; i--) {
        if (string[i] == c) {
            string[i] = 0;
        } else {
            break;
        }
    }
    // remove repeating characters 
    for (int i = 1; i < (int)strlen(string) - 1; i++) {
        while (string[i] == c && string[i + 1] == c) {
            shift_string(i + 1, string, 1);
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
void fix_home(char **in) {
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
    fix_home(&newstring); // replace the rest of the tildes
    *in = newstring;
}

/* Removes excess spaces and replaces ~ with the users home path.
 */
void parse_input(char **string) {
    eat(*string, ' ');
    fix_home(string);
}

/* Save current directory to old then changes the current
 * directory to path. If path is empty, it will default back
 * to the users home directory.
 */
int change_dir(char *path, char **old) {
    eat(path, ' ');
    path = ((int)strlen(path) > 0) ? path : getenv("HOME"); 
    char *temp = *old;
    *old = getcwd(NULL, 0);
    return chdir((!strcmp(path, "-")) ? temp : path);
} 

/* Given an input string, creates a space seperated argument vector
 * and saves it to the address arg_vector.
 */
void make_vector(char *input, char *arg_vector[], int arg_size) {
    char **ap, *input_string;
    input_string = input;

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

int next_index(char c, char string[], int after) {
    for (int i = after; i < (int)strlen(string); i++) {
        if (c == string[i]) {
            return i;
        }
    }
    return -1;
}

int save_next_arg(int from, char string[], char **save_to) {
    char *ptr;
    ptr = string;
    if (from < (int)strlen(string)) {
        int offset = from + ((string[from + 1] == ' ') ? 2 : 1);
        int index = next_index(' ', string, offset);
        int size = ((index < 0) ? (int)strlen(string) : index) - offset;
        asprintf(save_to, "%.*s", size, ptr + offset);
        return 0;
    }
    // error occured, nothing saved
    return -1;
}

int check_io(char string[], int fds[]) {
    int error = 0;
    for (int i = 0; i < (int)strlen(string); i++) {
        if (i == (int)strlen(string) - 1 && (string[i] == '<' || string[i]  == '>')) {
            printf("Error: file name expected after %c.\n", string[i]);
            error = 1;
            break;
        }
        int offset = (string[i + 1] == ' ') ? 2 : 1;
        if (string[i] == '<') {
            char *input_file;
            if (!save_next_arg(i, string, &input_file)) {
                shift_string(i, string, (int)strlen(input_file) + offset);
                close(fds[0]);
                if ((fds[0] = open(input_file, O_RDONLY, 0644)) < 0) {
                    printf("Error: %s not found.\n", input_file);
                    error = 1;
                }
                free(input_file);
            } else {
                error = 1;
            }
        }
        if (string[i] == '>') {
            int mode = O_RDWR | O_CREAT;
            if (string[i + 1] == '>') {
                mode |= O_APPEND;
                shift_string(i, string, 1);
            } else {
                mode |= O_TRUNC;
            }
            char *output_file;
            if (!save_next_arg(i, string, &output_file)) {
                shift_string(i, string, (int)strlen(output_file) + offset);
                close(fds[1]);
                if ((fds[1] = open(output_file, mode, 0644)) < 0) {
                    printf("Error: Couldn't open %s.\n", output_file);
                    error = 1;
                }
                free(output_file);
            } else {
                error = 1;
            }
        }
    }
    
    eat(string, ' ');
    return error;
}

void pipe_me(char *string, int fds[]) {
    char *input = string;
    eat(input, '|');
    int count = 0;
    char *array[arg_count(input, '|') + 1];
    //printf("arg count: %d\n", arg_count(input, '|'));

    int index;
    while ((index = index_of(input, '|')) >= 0) {
        //printf("doing something\n");
        asprintf(&array[count], "%.*s", index, input);
        eat(array[count++], ' ');
        shift_string(0, input, index + 1);
        eat(input, ' ');
        eat(input, '|');
        //printf("new input: .%s.\n", input);
    }

    asprintf(&array[count], "%s", input); 
    eat(array[count++], ' ');

    int bg = 0;
    char *last = array[count - 1];
    if (last[(int)strlen(last) - 1] == '&') {
        // run command in background
        bg = 1;
        shift_string((int)strlen(last) - 1, last, 1);
        eat(last, ' ');
    }
    
    int pipe_files[3];
    if (pipe(pipe_files) < 0) {
        printf("Error: Failed to pipe.");
        exit(EXIT_FAILURE);
    }
    
    for (int i = 0; i < count; i++) {
        // remove potentially empty commands
        while (strlen(array[i]) <= 0) {
            shift_args(&count, array, i);
        }

        int arg_size = arg_count(array[i], ' ');
        char *my_args[arg_size + 1];
        make_vector(array[i], my_args, arg_size);

        switch (fork()) {
            case -1:
                // error occured when forking
                printf("Failed to fork a child process.\n");
                exit(EXIT_FAILURE);
                break;
            case 0: // child process
                // first input
                if (i == 0) {
                    dup2(fds[0], fileno(stdin));
                    close(fds[0]);
                } else {
                    dup2(pipe_files[0], fileno(stdin));
                    close(pipe_files[0]);
                }
                // final output
                if (i == count - 1) {
                    dup2(fds[1], fileno(stdout));
                    close(fds[1]);
                } else {
                    dup2(pipe_files[1], fileno(stdout));
                    close(pipe_files[1]);
                }
                // successfully forked child process
                if (execvp(*my_args, my_args) < 0) {
                    printf("%s: command not found\n", *my_args);
                    exit(EXIT_FAILURE);
                }
                break;
            default:
                printf("hello i am the parent.\n");
                close(pipe_files[1]);
                // parent process
                if (!bg) {
                    wait(NULL);
                }
                break;
        }
        //printf(".%s.\n", array[i]);
    }

}

int main(int argc, char *argv[]) {
    char *prompt, *input, *old_dir;
    int quit = 0;

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
            parse_input(&input);

            // manually handle change directory
            if (strstr(input, "cd") == input) {
                if (change_dir(&input[2], &old_dir)) {
                    error(0, errno, "%s", &input[2]);
                }
                continue;
            }

            int fds[3] = {dup(fileno(stdout)), dup(fileno(stderr)), 0};
            
            if (!check_io(input, fds)) {
                pipe_me(input, fds);
            }

            close(fds[0]);
            close(fds[1]);
        }

        free(prompt);
        free(input);
    }

    return 0;
}
