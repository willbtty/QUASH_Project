#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>
#include <sys/wait.h>


static int pipes;

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

    while (parsed != NULL && strncmp("|", input)!=0) { // Try to figure out strncmp (string compaire)
        command[index] = parsed;
        index++;
        parsed = strtok(NULL, separator);
    }
    command[index] = NULL;
    return command;
}

int main() {
    char **command;
    char *input;
    //char input[1024];
    //char strinput;
    pid_t child_pid;
    int stat_loc;

    while(1){
        input = readline("[QUASH]$ ");
        //printf("[QUASH]$ ");
        //fgets(input, 1024, stdin);
        command = test_get_input(input);
        
        // EX. command = ["ls", "|", "grep", "src"]

        for (int i=0; i < sizeof(command); i++)
        {
            printf(command[i]);
            printf("\n");
        }

        if (pipes != 0)
        // Try and split the commands
        {

        }

        pid_t child_pid = fork();
        if (child_pid == 0) {
            execvp(command[0], command);
            printf("Won't be printed if execvp is succesful\n");
        } else {
            waitpid(child_pid, &stat_loc, WUNTRACED);
        }

        free(command);
        free(input);
    }
    return 0;
}