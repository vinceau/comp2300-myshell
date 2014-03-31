#define _GNU_SOURCE

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <readline/history.h>
#include <readline/readline.h>

#include "myshell.h"

/* Shifts all the characters of the string to the left n times.
 */
void shift_string(int from, char string[], int n) {
    int len = (int)strlen(string);
    if (from < len) {
        for (int i = from; i + n < len; i++) {
            string[i] = string[i + n];
        }
        string[len - n] = 0;
    }
}

/* Counts the number of arguments in an argument string, separated by the split
 * character. It counts a block of split characters as one.
 */
int arg_count(char string[], char split) {
    int count = 1;
    for (int i = 0; i < (int)strlen(string); i++) {
        if (string[i] == split) {
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
void strip(char string[], char c) {
    // remove leading characters
    while (string[0] == c) {
        shift_string(0, string, 1);
    }
    // remove trailing characters
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

/* Given a string, and starting at the index from, this will find the next
 * occurance of the character c and return its index. If not found, it will
 * return -1.
 */
int next_index(char c, char string[], int from) {
    for (int i = from; i < (int)strlen(string); i++) {
        if (c == string[i]) {
            return i;
        }
    }
    return -1;
}

/* Replaces all the tildes in a string with the home environmental variable.
 */
void fix_home(char **in) {
    char *input = *in;
    // find index of the ~ in the string
    int index = next_index('~', input, 0);
    if (index < 0) {
        return;
    }
    // find the length of the remaining characters
    int rest = (int)strlen(input) - index - 1;
    char *newstring;
    asprintf(&newstring, "%.*s%s%.*s", index, input, getenv("HOME"), rest,\
            input + index + 1);
    fix_home(&newstring); // replace the rest of the tildes
    free(*in);
    *in = newstring;
}

/* Save current directory to old then changes the current directory to path.
 * If path is empty, it will default back to the users home directory.
 */
int change_dir(char *path, char **old) {
    strip(path, ' ');
    path = ((int)strlen(path) > 0) ? path : getenv("HOME"); 
    char *temp = *old;
    *old = getcwd(NULL, 0);
    return chdir((!strcmp(path, "-")) ? temp : path);
} 

/* Given an input string, creates a space seperated argument vector and saves
 * it to the address arg_vector.
 */
void make_arg_vector(char *input, char *arg_vector[], int arg_size) {
    char **ap, *input_string;
    input_string = input;

    // split input into an argument vector by space
    // code based on the bsd man page of strsep
    for (ap = arg_vector; (*ap = strsep(&input_string, " ")) != NULL;) {
        if (**ap != '\0') {
            if (++ap >= &arg_vector[arg_size + 1]) {
                break;
            }
        }
    }
    free(input_string);
}

/* Given a string, and starting at the index from, this finds the next argument
 * and saves it to the address save_to. Returns the number of characters saved
 * or -1 if an error occured.
 */
int save_next_arg(int from, char string[], char **save_to) {
    char *ptr;
    ptr = string;
    if (from < (int)strlen(string)) {
        int offset = from + ((string[from + 1] == ' ') ? 2 : 1);
        int index = next_index(' ', string, offset);
        int size = ((index < 0) ? (int)strlen(string) : index) - offset;
        return asprintf(save_to, "%.*s", size, ptr + offset);
    }
    return -1;
}

/* Checks an input string for input and output specifiers.
 * e.g. < input, > output, >> append.
 * If found, it will open a file and save the file number to fds. fds[0] for
 * input, and fds[1] for output. Returns -1 if an error was generated and 0
 * otherwise.
 */
int check_io(char string[], int fds[]) {
    for (int i = 0; i < (int)strlen(string); i++) {
        if (i == (int)strlen(string) - 1 && (string[i] == '<' ||\
                    string[i] == '>')) {
            fprintf(stderr, "Error: file name expected after %c.\n", string[i]);
            return -1;
            break;
        }
        if (string[i] == '<') {
            char *input_file;
            if (save_next_arg(i, string, &input_file) > 0) {
                int offset = (string[i + 1] == ' ') ? 2 : 1;
                shift_string(i, string, (int)strlen(input_file) + offset);
                close(fds[0]);
                if ((fds[0] = open(input_file, O_RDONLY, 0644)) < 0) {
                    fprintf(stderr, "Error: %s not found.\n", input_file);
                    return -1;
                }
                free(input_file);
            } else {
                return -1;
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
            if (save_next_arg(i, string, &output_file) > 0) {
                int offset = (string[i + 1] == ' ') ? 2 : 1;
                shift_string(i, string, (int)strlen(output_file) + offset);
                close(fds[1]);
                if ((fds[1] = open(output_file, mode, 0644)) < 0) {
                    fprintf(stderr, "Error: Couldn't open %s.\n", output_file);
                    return -1;
                }
                free(output_file);
            } else {
                return -1;
            }
        }
    }
    
    strip(string, ' ');
    return 0;
}

/* Takes an input string of arguments separated by a pipe and splits them into
 * arguments. e.g. if input was "ls -la | wc", then:
 * cmd_array[0] = "ls -la"
 * cmd_array[1] = "wc"
 * It then executes each command in the cmd_array and passes the input of one
 * into the other. fds[] is an array of a file descriptors. fds[0] is the inital
 * input file and fds[1] is the final output.
 */
void run_pipe(char *input, int fds[]) {
    strip(input, '|');
    int count = 0;
    char *cmd_array[arg_count(input, '|') + 1];

    // process input and push commands into the cmd_array
    int index;
    while ((index = next_index('|', input, 0)) >= 0) {
        asprintf(&cmd_array[count], "%.*s", index, input);
        strip(cmd_array[count], ' '); // clean up any additional spaces
        if (strlen(cmd_array[count]) > 0) {
            count++;
        }
        shift_string(0, input, index + 1); // push the string backwards
        // clean the string
        strip(input, ' ');
        strip(input, '|');
    }

    // save the final command manually
    asprintf(&cmd_array[count], "%s", input); 
    strip(cmd_array[count++], ' ');

    int bg = 0; // flag for if command should be run in background
    char *last = cmd_array[count - 1];
    if (last[(int)strlen(last) - 1] == '&') {
        bg = 1;
        shift_string((int)strlen(last) - 1, last, 1);
        strip(last, ' ');
    }

    int pipe_in[2] = {fds[0], -1};
    
    for (int i = 0; i < count; i++) {
        int pipe_out[2];
        if (pipe(pipe_out) < 0) {
            err(EXIT_FAILURE, "Failed to pipe");
        }
        int arg_size = arg_count(cmd_array[i], ' ');
        char *my_args[arg_size + 1];
        make_arg_vector(cmd_array[i], my_args, arg_size);

        switch (fork()) {
            case -1:
                // error occured when forking
                err(EXIT_FAILURE, "Failed to fork a child process");
                break;
            case 0: // successfully forked child process
                if (pipe_in[1] != -1) {
                    close(pipe_in[1]);
                }
                dup2(pipe_in[0], fileno(stdin));
                close(pipe_in[0]);
                // final output
                if (i == count - 1) {
                    dup2(fds[1], fileno(stdout));
                    close(fds[1]);
                } else {
                    dup2(pipe_out[1], fileno(stdout));
                    close(pipe_out[1]);
                }
                if (execvp(*my_args, my_args) < 0) {
                    err(EXIT_FAILURE, "%s", *my_args);
                }
                break;
            default: // parent process
                if (pipe_in[1] != -1) {
                    close(pipe_in[1]);
                }
                close(pipe_in[0]);
                // save output for next pipe
                pipe_in[0] = pipe_out[0];
                pipe_in[1] = pipe_out[1];
                if (!bg) {
                    wait(NULL);
                }
                break;
        }
        free(cmd_array[i]);
    }

}

int main(int argc, char *argv[]) {
    char *prompt, *input, *old_dir;

    for (;;) {
        asprintf(&prompt, "%s: %s$ ", argv[0], getcwd(NULL, 0));
        input = readline(prompt);

        // quit if EOF or CTRL+D
        if (input == NULL) {
            printf("\n");
            free(prompt);
            free(input);
            break;
        }

        else if (strlen(input) > 0) {
            if (!strcmp(input, "exit")) {
                free(prompt);
                free(input);
                break;
            }

            add_history(input);
            strip(input, ' ');
            fix_home(&input);

            // manually handle change directory
            if (strstr(input, "cd") == input) {
                if (change_dir(&input[2], &old_dir)) {
                    err(0, "%s", &input[2]);
                }
                free(prompt);
                free(input);
                continue;
            }

            int fds[2] = {dup(fileno(stdin)), dup(fileno(stdout))};
            
            if (check_io(input, fds) != -1) {
                run_pipe(input, fds);
            }

            close(fds[0]);
            close(fds[1]);
        }

        free(prompt);
        free(input);
    }

    return 0;
}
