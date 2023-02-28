#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

void parse(char *argv[], char command[]){
    int argc = 10;
    char *arg = strtok(command, " ");

    for (int i = 0; i < argc; ++i) {
        if (arg) {
            argv[i] = arg;
            arg = strtok(NULL, " ");
        } else {
            argv[i] = NULL;
        }
    }
}

void cd_command(char *arguments[], char prevdir[]){
    char *curdir, dir[256], tmpdir[256];

    if (!arguments[1] || !strcmp(arguments[1], "~")){
        curdir = getenv("HOME");
    } else if (!strcmp(arguments[1], "..")) {
        getcwd(dir, sizeof(dir));
        char *ptr = strrchr(dir, '/');
        dir[ptr - dir + (dir == ptr ? 1 : 0)] = '\0';
        curdir = dir;
    } else if (!strcmp(arguments[1], "-")) {
        memcpy(dir, prevdir, sizeof(dir));
        curdir = dir;
        printf("%s", curdir);
    } else {
        curdir = arguments[1];
    }

    getcwd(tmpdir, sizeof(tmpdir));
    if (chdir(curdir)) {
        perror("cd");
        return;
    }

    memcpy(prevdir, tmpdir, sizeof(tmpdir));
}

void export_command(char *arguments[]){
    char name[128], value[128];
    char *ptr = strrchr(arguments[1], '='), *end = strchr(arguments[1], '\0');
    memcpy(name, arguments[1], (ptr - arguments[1]) * sizeof(char));
    memcpy(value, ptr+1, (end - (ptr+1)) * sizeof(char));
    name[ptr - arguments[1]] = '\0', value[end - (ptr+1)] = '\0';
    setenv(name, value, 0);
}

void echo_command(char *arguments[]){
    char *variable;
    for(int i = 1; arguments[i] != NULL; i++){
        if (*arguments[i] == '$') {
            arguments[i]++;
            variable = getenv(arguments[i]);
            if (variable)
                printf("%s ", variable);
            else
                printf("\n");
        } else
            printf("%s", arguments[1]);
    }
}

int main() {

    FILE *fp;
    char prevdir[256] = {'\0'};
    fp = fopen ("gShell.log", "w");

    while (1){
        pid_t childPid;
        char command[256];
        char *arguments[10];

        printf("shell %d\n", getpid());
        printf("command: ");
        scanf("%[^\n]%*c", command);

        parse(arguments, command);

        if (!strcmp(arguments[0], "exit")) {
            printf("exiting\n");
            exit(0);
        } else if (!strcmp(arguments[0], "cd")) {
            cd_command(arguments, prevdir);
            continue;
        } else if (!strcmp(arguments[0], "export")) {
            export_command(arguments);
            continue;
        } else if (!strcmp(arguments[0], "echo")) {
            echo_command(arguments);
            continue;
        }

        childPid = fork();
        if (childPid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (childPid == 0) {
            printf("child %d", getpid());
            execvp(arguments[0], arguments);
        } else {
            int state;
            waitpid(childPid, &state, 0);

            printf("State %d\n", state);
            fprintf(fp, "command terminates");
        }
    }
}