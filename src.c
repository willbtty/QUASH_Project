#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>
#include <sys/wait.h>


static int isPipes = 0;

char **get_input(char *input) {
    char **command = malloc(8 * sizeof(char *));
    char *separator = " ";
    char *parsed;
    int index = 0;

    parsed = strtok(input, separator);
    while (parsed != NULL) {
        command[index] = parsed;
        index++;
        parsed = strtok(NULL, separator);
    }
    command[index] = NULL;
    return command;
}


char **test_get_input(char *input) {
    char **command = malloc(8 * sizeof(char *));
    char *separator = " ";
    char *parsed;
    int index = 0;

    parsed = strtok(input, separator);

    while (parsed != NULL && strncmp(parsed, "|", 1)!=0) { // Try to figure out strncmp (string compaire)
        command[index] = parsed;
        index++;
        parsed = strtok(NULL, separator);
    }
    if (parsed != NULL && strncmp(parsed, "|", 1) == 0)
        isPipes = 1;
    else
        isPipes = 0;

    command[index] = NULL;
    return command;
}

int main() {
    char **command;
    //char *input;
    int size = 1024;
    char input[size];
    //char strinput;
    int stat_loc;


    while(1){

        //input = readline("[QUASH]$ ");
        printf("[QUASH]$ ");
        fgets(input, 1024, stdin);

        // Remove trailing newline character if present
        size_t len = strlen(input);
        if (input[len - 1] == '\n') {
            input[len - 1] = '\0';
        }

        command = test_get_input(input);
        // EX. command = ["ls", "|", "grep", "src"]

        if (command[0] != NULL && strcmp(command[0], "cd") == 0)
        {
            if (command[1] == NULL) {
                printf("cd: command not found\n");
            }
            else {
                if (chdir(command[1]) != 0) {
                    perror("cd");
                }
            }
        }

        else {
            pid_t child_pid = fork();
            if (child_pid == 0) {
                execvp(command[0], command);
                printf("Won't be printed if execvp is succesful\n");
            } else {
                waitpid(child_pid, &stat_loc, WUNTRACED);
            }

            if (isPipes == 1)
            {
                int p[2];
                pipe(p);

                for (int i = 1; i < sizeof(command); i++) // Shift elements to the left deleting the first element
                    command[i-1] = command[i];
            }

            free(command);
        }

    }
    return 0;
}