#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LENGTH 256
#define MAX_ARGS 10
#define SLEEPING 200
#define LOG_FILE_NAME "/home/mostafa-galal/CLionProjects/os_lab1/g-Shell.log"
#define HISTORY_FILE_NAME "/home/mostafa-galal/CLionProjects/os_lab1/History.log"

typedef enum {
    shell_builtin,
    executable_or_error
} input_type;

typedef enum {
    cd,
    echo,
    export,
    history
} command_type;

void error_message(char error[]){
    perror(error);
    usleep(SLEEPING * 1000);
    exit(0);
}

void cd_command(char *argv[]){
    static char prevdir[MAX_LENGTH];
    char *curdir, dir[MAX_LENGTH], tmpdir[MAX_LENGTH];

    if (!argv[1] || !strcmp(argv[1], "~")){
        curdir = getenv("HOME");
    } else if (!strcmp(argv[1], "..")) {
        getcwd(dir, sizeof(dir));
        char *ptr = strrchr(dir, '/');
        dir[ptr - dir + (dir == ptr ? 1 : 0)] = '\0';
        curdir = dir;
    } else if (!strcmp(argv[1], "-")) {
        memcpy(dir, prevdir, sizeof(dir));
        curdir = dir;
        printf("%s\n", curdir);
    } else {
        curdir = argv[1];
    }

    getcwd(tmpdir, sizeof(tmpdir));
    if (chdir(curdir)) {
        perror("cd");
        usleep(SLEEPING * 1000);
        return;
    }

    memcpy(prevdir, tmpdir, sizeof(tmpdir));
}
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

void echo_command(char *argv[]){
    for (int i = 1; argv[i] != NULL; ++i)
        printf("%s ", argv[i]);
    printf("\n");
}

void history_command(){
    FILE *hist;
    char * line = NULL;
    size_t len = 0;

    hist = fopen(HISTORY_FILE_NAME, "r");
    if (hist == NULL)
        return;

    while (getline(&line, &len, hist) != -1)
        printf("%s", line);
    fclose(hist);

    if (line)
        free(line);
}

void reap_child_zombie(){
    while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
}
void write_to_log_file(char line[]){
    FILE *log;

    log = fopen(LOG_FILE_NAME, "a");
    if (log == NULL)
        return;

    fprintf(log, "%s", line);
    fclose(log);
}

void on_child_exit(){
    reap_child_zombie();
    write_to_log_file("Child terminated\n");
}

void register_child_signal(void (*on_child_exit)(int)) {
    signal(SIGCHLD, on_child_exit);
}

void setup_environment() {
    char dir[MAX_LENGTH];

    getcwd(dir, sizeof(dir));
    chdir(dir);
}

void read_input(char *command){
    printf("g-Shell > ");
    fgets(command, MAX_LENGTH, stdin);
    command[strcspn(command, "\n")] = '\0';
}
void record_input(char command[]){
    FILE *hist;

    hist = fopen(HISTORY_FILE_NAME, "a+");
    if (hist == NULL)
        return;

    fprintf(hist, "%s\n", command);
    fclose(hist);
}
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

input_type evaluate_input(char *input){
    if (input && (!strcmp(input, "cd") || !strcmp(input, "export") || !strcmp(input, "echo")  || !strcmp(input, "history")))
        return shell_builtin;
    return executable_or_error;
}

command_type evaluate_command(char *input){
    if (!strcmp(input, "cd"))
        return cd;
    else if (!strcmp(input, "export"))
        return export;
    else if (!strcmp(input, "echo"))
        return echo;
    else
        return history;
}

void evaluate_expression(char *argv[]){
    for(int i = 1; argv[i] != NULL; i++){
        if (*argv[i] == '$') {
            *argv[i] = '\n';
            argv[i]++;
            argv[i] = getenv(argv[i]);
        }
    }
}

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

void execute_shell_builtin(char *argv[]) {
    command_type type;
    type = evaluate_command(argv[0]);

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
    } else {
        if (argv[1] && !strcmp(argv[1], "&"))
            return;

        waitpid(childPid, 0, 0);
    }
}

void shell(){
    int done = 1;

    do {
        input_type type;
        char command[MAX_LENGTH] = {};
        char *argv[MAX_ARGS] = {};

        read_input(command);
        record_input(command);
        parse_input(command, argv);
        evaluate_expression(argv);

        type = evaluate_input(argv[0]);

        if (argv[0] && !strcmp(argv[0], "exit")) {
            done = 0;
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
    } while (done);

    exit(0);
}

int main() {
    register_child_signal(on_child_exit);
    setup_environment();
    shell();
}
