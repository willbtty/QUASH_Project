#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>    // For fork, execvp, getcwd, chdir
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>     // For open
#include <errno.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKENS 128
#define MAX_JOBS 128

// Struct to hold informations for each job
typedef struct {
    int job_id;
    pid_t pid;
    char command[MAX_INPUT_SIZE];
    int active;
} job_t;


job_t jobs[MAX_JOBS];
int job_count = 0;
int next_job_id = 1;



void remove_job(pid_t pid)
// Remove specific job from the job list
{
    for (int i=0; i < job_count; i++)
    {
        if (jobs[i].pid == pid)
        {
            jobs[i].active = 0;
            break;
        }
    }
}

void print_jobs()
{
    for (int i=0; i < job_count; i++)
    {
        printf("[%d], %d %s &\n", jobs[i].job_id, jobs[i].pid, jobs[i].command);
    }
}

void check_background_jobs()
// Check on background jobs and run them
{
    for(int i=0; i<job_count; i++)
    {
        if (jobs[i].active)
        {
            pid_t result = waitpid(jobs[i].pid, NULL, WNOHANG);
            if (result > 0)
            {
                printf("Completed: [%d] %d %s &\n", jobs[i].job_id, jobs[i].pid, jobs[i].command);
                remove_job(jobs[i].pid);
            }
        }
    }
}

void add_job()
{

}



int parse_input(char *input, char **tokens, char **input_file, char **output_file, int *append_output, int *background) 
{
    int token_count = 0;
    int i = 0;
    *input_file = NULL;
    *output_file = NULL;
    *append_output = 0;
    *background = 0;

    char *token = strtok(input, " ");
    while (token != NULL && token_count < MAX_TOKENS - 1) {
        if (strcmp(token, "<") == 0) 
        {
            // Input redirection
            token = strtok(NULL, " ");
            if (token == NULL) {
                fprintf(stderr, "Syntax error: expected file after '<'\n");
                return -1;
            }
            *input_file = token;
        } 
        else if (strcmp(token, ">") == 0) 
        {
            // Output redirection (truncate)
            token = strtok(NULL, " ");
            if (token == NULL) {
                fprintf(stderr, "Syntax error: expected file after '>'\n");
                return -1;
            }
            *output_file = token;
            *append_output = 0;
        } 
        else if (strcmp(token, ">>") == 0) 
        {
            // Output redirection (append)
            token = strtok(NULL, " ");
            if (token == NULL) {
                fprintf(stderr, "Syntax error: expected file after '>>'\n");
                return -1;
            }
            *output_file = token;
            *append_output = 1;
        } 
        else if (strcmp(token, "&") == 0) 
        {
            // Background execution
            *background = 1;
        } 
        else 
        {
            tokens[token_count++] = token;
        }
        token = strtok(NULL, " ");
        i++;
    }
    tokens[token_count] = NULL;
    return token_count;
}

int main() 
{
    char input[MAX_INPUT_SIZE];


    while(1){

        //input = readline("[QUASH]$ ");
        printf("[QUASH]$ ");

        //Read the input
        if (fgets(input, 1024, stdin) == NULL)
            break; // Exit if not sucessful

        // Remove trailing newline character if present
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
        }

        // Skip empty input string
        if (strlen(input) == 0)
            continue;

        // Check on the background jobs
        check_background_jobs();

        char *tokens[MAX_TOKENS];
        char *input_file = NULL;
        char *output_file = NULL;
        int append_output = 0;
        int background = 0;
        int token_count = parse_input(input, tokens, &input_file, &output_file, &append_output, &background);

        if (token_count < 0)
            continue;

        // Check for exit or quit
        if (strncmp(tokens[0], "exit", 4)==0 || strncmp(tokens[0], "quit", 4)==0)
            break;

        
        
        // Built in commands
        if (tokens[0] != NULL && strcmp(tokens[0], "cd") == 0) // See if the command is to cd
        {
            if (tokens[1] == NULL) { // if the argument given was nothing if it was then tell the user
                printf("cd: command not found\n");
            }
            else {
                if (chdir(tokens[1]) != 0) { // Use the change directory function to change the working directory using the argument given
                    perror("cd");
                }
            }
            continue;
        }
        else if (strncmp(tokens[0], "echo", 4)==0)
        {
            for (int i=1; i<token_count; i++)
            {
                if (tokens[i][0] == '$') // Handle the case where the user wants to print an evironment
                {
                    char* env = getenv(tokens[i]+1);
                    if (env != NULL)
                        printf("%s\n ", env);
                }
                else
                {
                    printf("%s\n ", tokens[i]);
                }
            }
            continue;
        }
        else if (strncmp(tokens[0], "pwd", 3) == 0)
        {
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd)) != NULL)
            {
                printf("%s\n", cwd);
            }
            else
            {
                perror("couldn't get cwd");
            }
            
        }


        // Check for pipes
        int isPipes = 0;
        int pipe_indexes[token_count];
        int pipe_count = 0;
        for (int i=0; i< token_count; i++)
        {
            pipe_indexes[i] = 0;
        }

        // Check if their are pipes, then store the indexes of each
        for (int i=0; i<token_count; i++)
        {
            if (strncmp(tokens[i], "|", 1) == 0)
            {
                isPipes = 1;
                pipe_indexes[i] = 1;
                pipe_count++;
            }
        }

        if (isPipes)
        {
            //Split the commands
            for(int i=0; i<token_count; i++)
            {
                if (pipe_indexes[i] == 1)
                {
                    tokens[i] = NULL;
                }
            }

            // Set up pipes
            int pipes[pipe_count][2];
            for (int i=0; i<pipe_count; i++)
            {
                if (pipe(pipes[i]) == -1)
                {
                    perror("Pipe failed");
                    exit(EXIT_FAILURE);
                }
            }

            int command_index = 0;
            for (int i=0; i<pipe_count; i++) // Loop through all the pipes dynamically forking and creating processes.
            {
                pid_t pid = fork();
                if (pid == -1) // When the fork fails
                {
                    perror("Fork failed");
                    exit(EXIT_FAILURE);
                }
                
                else if (pid >= 0) // When the fork is sucessful
                {
                    if (i > 0) // Not the child process
                    {
                        dup2(pipes[i-1][0], STDIN_FILENO);
                    }
                    if (i < pipe_count)
                    {
                        // Not the last command, output to next pipe
                        dup2(pipes[i][1], STDOUT_FILENO);
                    }
                    for (int j = 0; j<pipe_count; j++)
                    {
                        close(pipes[j][0]);
                        close(pipes[j][1]);
                    }
                    // Handle the redirection
                    if (output_file != NULL)
                    {
                        int fd_out;
                        if (append_output)
                        {
                            fd_out = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
                        }
                        else
                        {
                            fd_out = open(output_file, O_WRONLY | O_CREAT |O_TRUNC, 0644);
                        }
                        if (fd_out < 0) // Hello    
                        {
                            perror("open output_file");
                            exit(EXIT_FAILURE);
                        }
                        dup2(fd_out, STDOUT_FILENO);
                        close(fd_out);
                    }
                    execvp(tokens[0], tokens);
                    perror("quash");
                    exit(EXIT_FAILURE);
                    command_index++;
                }
                else
                {
                    // Parent process
                    if (i > 0)
                    {
                        // Close read end of previous pipe
                        close(pipes[i-1][0]);
                    }
                    if (i < pipe_count)
                    {
                        //close the write end of the current pipe
                        close(pipes[i][1]);
                    }

                    // move to the next command
                    while (tokens[command_index] != NULL)
                    {
                        command_index++;
                    }
                    command_index++;
                }
            }
            for (int i = 0; i<=pipe_count; i++)
            {
                wait(NULL);
            }
        }
        else
        {
            // No pipe
            // Fork a child process
            pid_t pid = fork();
            if (pid == 0) {
                // Child process

                // Handle input redirection
                if (input_file != NULL) {
                    int fd_in = open(input_file, O_RDONLY);
                    if (fd_in < 0) {
                        perror("open input_file");
                        exit(EXIT_FAILURE);
                    }
                    dup2(fd_in, STDIN_FILENO);
                    close(fd_in);
                }

                // Handle output redirection
                if (output_file != NULL) {
                    int fd_out;
                    if (append_output) {
                        fd_out = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
                    } else {
                        fd_out = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    }
                    if (fd_out < 0) {
                        perror("open output_file");
                        exit(EXIT_FAILURE);
                    }
                    dup2(fd_out, STDOUT_FILENO);
                    close(fd_out);
                }

                execvp(tokens[0], tokens);
                perror("quash");
                exit(EXIT_FAILURE);
            } else if (pid < 0) {
                perror("fork failed");
            } else {
                if (!background) {
                    // Foreground execution
                    int status;
                    waitpid(pid, &status, 0);
                } else {
                    // Background execution
                    add_job(pid, input);
                    printf("Background job started: [%d] %d %s &\n", jobs[job_count - 1].job_id, pid, input);
                }
            }
        }
    }
    return 0;
}