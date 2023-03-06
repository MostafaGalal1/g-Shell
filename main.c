#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

// environment constants
#define MAX_LENGTH 256
#define MAX_ARGS 10
#define SLEEPING 200
#define LOG_FILE_DIR "/home/mostafa-galal/CLionProjects/os_lab1/g-Shell.log"
#define HISTORY_FILE_DIR "/home/mostafa-galal/CLionProjects/os_lab1/History.log"

// input_type enum
typedef enum {
    shell_builtin,
    executable_or_error
} input_type;

// command_type enum
typedef enum {
    cd,
    echo,
    export,
    history
} command_type;

/*
 * error_message: displays an error message showing command caused error and the code of the error
 */
void error_message(char error[]){
    perror(error);
    usleep(SLEEPING * 1000);
}

/*
 * cd_command: built-in cd command for handling user command to navigate between different directories.
 */
void cd_command(char *argv[]){
    static char prevdir[MAX_LENGTH];
    char *curdir, dir[MAX_LENGTH], tmpdir[MAX_LENGTH];

    if (!argv[1] || !strcmp(argv[1], "~")){
        curdir = getenv("HOME");
    } else if (!strcmp(argv[1], "-")) {
        memcpy(dir, prevdir, sizeof(dir));
        curdir = dir;
        printf("%s\n", curdir);
    } else {
        curdir = argv[1];
    }

    getcwd(tmpdir, sizeof(tmpdir));
    if (chdir(curdir)) {
        error_message("cd");
        return;
    }

    memcpy(prevdir, tmpdir, sizeof(tmpdir));
}

/*
 * export_command: built-in export command for defining environment variables and using them in child processes.
 */
void export_command(char *argv[]){
    char *name, *value;
    for (int i = 1; argv[i] != NULL; ++i) {
        if ((value = strchr(argv[i], '=')) != NULL) {
            *value = '\0';
            name = argv[i], value++;
            setenv(name, value, 1);
        }
    }
}

/*
 * echo_command: built-in echo command for printing input line as string of text or value of defined environment variable.
 */
void echo_command(char *argv[]){
    for (int i = 1; argv[i] != NULL; ++i)
        printf("%s ", argv[i]);
    printf("\n");
}

/*
 * history_command: display commands entered by the user
 */
void history_command(){
    FILE *hist;
    char * line = NULL;
    size_t len = 0;

    hist = fopen(HISTORY_FILE_DIR, "r");
    if (hist == NULL)
        return;

    while (getline(&line, &len, hist) != -1)
        printf("%s", line);
    fclose(hist);

    if (line)
        free(line);
}

/*
 * reap_child_zombie: removes the process which sent the signal from the processes table and deallocate its resources.
 * In case of a zombie process this method is responsible for reaping it.
 */
void reap_child_zombie(){
    while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
}

/*
 * write_to_log_file: keeps track of child processes termination and write them in a log file
 */
void write_to_log_file(char line[]){
    FILE *log;

    log = fopen(LOG_FILE_DIR, "a");
    if (log == NULL)
        return;

    fprintf(log, "%s", line);
    fclose(log);
}

/*
 * on_child_exit: responsible for  calling functions to handle process termination and writing to log file
 */
void on_child_exit(){
    reap_child_zombie();
    write_to_log_file("Child terminated\n");
}

/*
 * register_child_signal: registers parent process to signal handler responsible for process termination housekeeping
 */
void register_child_signal(void (*on_child_exit)(int)) {
    signal(SIGCHLD, on_child_exit);
}

/*
 * setup_environment: changing working directory to the current one
 */
void setup_environment() {
    chdir(getenv("PWD"));
}

/*
 * read_input: takes command input entered by the user
 */
void read_input(char *command){
    printf("g-Shell > ");
    fgets(command, MAX_LENGTH, stdin);
    command[strcspn(command, "\n")] = '\0';
}

/*
 * record_input: save command entered by the user in a history command file for retrieval
 */
void record_input(char command[]){
    FILE *hist;

    hist = fopen(HISTORY_FILE_DIR, "a+");
    if (hist == NULL)
        return;

    fprintf(hist, "%s\n", command);
    fclose(hist);
}

/*
 * parse_input: parses command entered by the user.
 * Since max number of arguments allowed in terminal the arguments array is of size 10.
 */
void parse_input(char command[], char *argv[]){
    int argc = 0;
    int len = (int)strlen(command);
    char *arg = strtok(command, " "), *ind;

    while (argc < MAX_ARGS && arg != NULL) {
        argv[argc++] = arg;
        arg = strtok(NULL, " ");
        if (arg && (ind = strchr(arg, '"')) != NULL) {
            *(arg + strlen(arg)) = ' ';
            memmove(ind, ind+1, len - (ind - command));
            arg = strtok(arg, "\"");
        }
    }
}

/*
 * check_input: checks the input type whether a shell builtin command or executable command 
 */
input_type check_input(char *input){
    if (input && (!strcmp(input, "cd") || !strcmp(input, "export") || !strcmp(input, "echo")  || !strcmp(input, "history")))
        return shell_builtin;
    return executable_or_error;
}

/*
 * check_command: specify which shell builtin command is being called
 */
command_type check_command(char *input){
    if (!strcmp(input, "cd"))
        return cd;
    else if (!strcmp(input, "export"))
        return export;
    else if (!strcmp(input, "echo"))
        return echo;
    else
        return history;
}

/*
 * evaluate_expression: checks arguments whether they are text or environment variables
 */
void evaluate_expression(char *argv[]){
    for(int i = 1; argv[i] != NULL; i++){
        if (*argv[i] == '$') {
            *argv[i] = '\n';
            argv[i]++;
            argv[i] = getenv(argv[i]);
        }
    }
}

/*
 * compose_arguments: used to parse string arguments into fine arguments which will be used in executing commands
 */
void compose_arguments(char *args[], char *argv[]) {
    int argc = 1;
    char *arg;

    args[0] = argv[0];
    for (int i = 1; i < MAX_ARGS && argv[i]; ++i) {
        arg = strtok(argv[i], " ");
        while (argc < MAX_LENGTH && arg != NULL) {
            args[argc++] = arg;
            arg = strtok(NULL, " ");
        }
    }
}

/*
 * execute_shell_builtin: executes shell builtin commands after specifying which one to be executed
 */
void execute_shell_builtin(char *argv[]) {
    command_type type;
    type = check_command(argv[0]);

    switch(type) {
        case cd:
            cd_command(argv);
            break;
        case echo:
            echo_command(argv);
            break;
        case export:
            export_command(argv);
            break;
        case history:
            history_command();
            break;
    }
}

/*
 * execute_command: executes system commands by forking a child process so that the corresponding program is loaded into it
 */
void execute_command(char *argv[]) {
    pid_t childPid = fork();

    if (childPid < 0) {
        error_message("fork");
    } else if (childPid == 0) {
        char *args[MAX_LENGTH] = {};

        if (argv[1] && !strcmp(argv[1], "&")) {
            argv[1] = NULL;
            printf("process: %d", getpid());
        }

        compose_arguments(args, argv);

        execvp(args[0], args);
        error_message("execvp");
        exit(0);
    } else {
        if (argv[1] && !strcmp(argv[1], "&"))
            return;

        waitpid(childPid, 0, 0);
    }
}

/*
 * shell: main function containing the program loop and calling functions responsible for command parsing and execution
 */
void shell(){
    int _exit = 1;

    do {
        input_type type;
        char command[MAX_LENGTH] = {};
        char *argv[MAX_ARGS] = {};

        read_input(command);
        record_input(command);
        parse_input(command, argv);
        evaluate_expression(argv);

        type = check_input(argv[0]);

        if (argv[0] && !strcmp(argv[0], "exit")) {
            _exit = 0;
            continue;
        }

        switch (type) {
            case shell_builtin:
                execute_shell_builtin(argv);
                break;
            case executable_or_error:
                execute_command(argv);
                break;
        }
    } while (_exit);

    exit(0);
}

/*
 * main: initialize the program and set some stuff
 */
int main() {
    register_child_signal(on_child_exit);
    setup_environment();
    shell();
}
