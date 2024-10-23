#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <IO.h>

static int isPipes = 0;
static int IOredirect = -1;





char **test_get_input(char *input) {
    char **command = malloc(100 * sizeof(char *));
    char *separator = " ";
    char *parsed;
    int index = 0;

    parsed = strtok(input, separator);

    while (parsed != NULL && strncmp(parsed, "|", 1)!=0) { // Loop through the parsed input (each bunch of characters) and check for the NULL terminator and pipes
        command[index] = parsed;
        check_for_IO_redirect(parsed); // Check for IO redirect
        index++; // Increment index to add NULL terminator on the command
        parsed = strtok(NULL, separator); // Keep parsing the string
    }

    if (parsed != NULL && strncmp(parsed, "|", 1) == 0) // Check for same things as while loop execpt looking for a pipe
        // Idea is so pause and re-execute the above code to rerun the command. Might work idk
        isPipes = 1;
    else
        isPipes = 0;

    if (IOredirect != -1) {
        int fd = open(command[index-1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        redirect_IO(input, fd);
        close(fd);
    }
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

        if (command[0] != NULL && strcmp(command[0], "cd") == 0) // See if the command is to cd
        {
            if (command[1] == NULL) { // if the argument given was nothing if it was then tell the user
                printf("cd: command not found\n");
            }
            else {
                if (chdir(command[1]) != 0) { // Use the change directory function to change the working directory using the argument given
                    perror("cd");
                }
            }
        }

        else {
            pid_t child_pid = fork();
            if (child_pid == 0) {
                execvp(command[0], command);
                fprintf(stderr, "Command not found %s \n", command[0]);
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