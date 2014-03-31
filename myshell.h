int arg_count(char string[], char split);
int change_dir(char *path, char **old);
int check_io(char string[], int fds[]);
int index_of(char input[], char c);
int next_index(char c, char string[], int from);
int save_next_arg(int from, char string[], char **save_to);
void fix_home(char **in);
void make_arg_vector(char *input, char *arg_vector[], int arg_size);
void run_pipe(char *input, int fds[]);
void shift_string(int from, char string[], int n);
void strip(char string[], char c);

int main(int argc, char *argv[]);
