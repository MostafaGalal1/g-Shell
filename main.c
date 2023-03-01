#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LENGTH 256
#define MAX_ARGS 10
#define LOG_FILE_NAME "g-Shell.log"
#define HISTORY_FILE_NAME "History.log"

// recorder: keeps track of the commands entered by the user and save them in a history file.
void recorder(char command[]){
    FILE *hist;

    hist = fopen(HISTORY_FILE_NAME, "a+");
    if (hist == NULL)
        return;

    fprintf(hist, "%s\n", command);
    fclose(hist);
}

/*
parser: parses command entered by the user.
Since max number of arguments allowed in terminal the arguments array is of size 10.
*/
void parser(char command[], char *argv[]){
    char *arg = strtok(command, " ");

    int argc = 0;
    while (argc < MAX_ARGS && arg != NULL) {
        argv[argc++] = arg;
        arg = strtok(NULL, " ");
    }
}

/*
handler: handles SIGCHLD signals generated by the child process when terminated.
The parent process then removes the process which initiated the signal from the processes table.
In case of a zombie process this method is responsible for reaping it.
*/
void handler() {
    FILE *log;

    log = fopen(LOG_FILE_NAME, "a");
    if (log == NULL)
        return;

    while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
    fprintf(log, "Child process was terminated\n");
    fclose(log);
}

// cd_command: built-in cd command for handling user command to navigate between different directories.
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
        return;
    }

    memcpy(prevdir, tmpdir, sizeof(tmpdir));
}

// export_command: built-in export command for defining environment variables and using them in child processes.
void export_command(char *argv[]){
    char name[MAX_LENGTH/2], value[MAX_LENGTH/2];
    char *ptr = strrchr(argv[1], '='), *end = strchr(argv[1], '\0');
    memcpy(name, argv[1], (ptr - argv[1]) * sizeof(char));
    memcpy(value, ptr+1, (end - (ptr+1)) * sizeof(char));
    name[ptr - argv[1]] = '\0', value[end - (ptr+1)] = '\0';
    setenv(name, value, 0);
}

// echo_command: built-in echo command for printing input line as string of text or value of defined environment variable.
void echo_command(char *argv[]){
    char *variable;
    for(int i = 1; argv[i] != NULL; i++){
        if (*argv[i] == '$') {
            argv[i]++;
            variable = getenv(argv[i]);
            if (variable)
                printf("%s\n", variable);
            else
                printf("\n");
        } else
            printf("%s\n", argv[1]);
    }
}

// Driver code.
int main() {
    signal(SIGCHLD, handler);

    while (1){
        pid_t childPid;
        char command[MAX_LENGTH] = {};
        char *argv[MAX_ARGS] = {};

        printf("g-Shell > ");
        fgets(command, MAX_LENGTH, stdin);
        command[strcspn(command, "\n")] = '\0';

        recorder(command);
        parser(command, argv);

        if (!strcmp(argv[0], "exit"))
            exit(0);

        if (!strcmp(argv[0], "cd"))
            cd_command(argv);
        else if (!strcmp(argv[0], "export"))
            export_command(argv);
        else if (!strcmp(argv[0], "echo"))
            echo_command(argv);
        else {
            childPid = fork();
            if (childPid < 0) {
                perror("fork");
                exit(EXIT_FAILURE);
            } else if (childPid == 0) {
                if (argv[1] && !strcmp(argv[1], "&")) {
                    argv[1] = NULL;
                    printf("process: %d", getpid());
                }

                execvp(argv[0], argv);
                perror("exec");
                exit(0);
            } else {
                if (argv[1] && !strcmp(argv[1], "&"))
                    continue;

                waitpid(childPid, 0, 0);
            }
        }
    }
}
