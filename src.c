// src/execute.c
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

typedef struct {
    int job_id;
    pid_t pid;
    char command[MAX_INPUT_SIZE];
    int active;
} job_t;

    job_t jobs[MAX_JOBS];
    int job_count = 0;
    int next_job_id = 1;

// Function to add a job to the jobs list
void add_job(pid_t pid, char *command) {
    jobs[job_count].job_id = next_job_id++;
    jobs[job_count].pid = pid;
    strncpy(jobs[job_count].command, command, MAX_INPUT_SIZE);
    jobs[job_count].active = 1;
    job_count++;
}

// Function to remove a job from the jobs list
void remove_job(pid_t pid) {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].pid == pid) {
            jobs[i].active = 0;
            break;
        }
    }
}

// Function to print the list of background jobs
void print_jobs() {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].active) {
            printf("[%d] %d %s &\n", jobs[i].job_id, jobs[i].pid, jobs[i].command);
        }
    }
}

// Function to check and update background jobs
void check_background_jobs() {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].active) {
            pid_t result = waitpid(jobs[i].pid, NULL, WNOHANG);
            if (result > 0) {
                printf("Completed: [%d] %d %s &\n", jobs[i].job_id, jobs[i].pid, jobs[i].command);
                remove_job(jobs[i].pid);
            }
        }
    }
}

void delete_up_to_null(char *arr[], int size)
{
    int null_pos;

    // Find the null position of a null terminator
    while (null_pos < size && arr[null_pos] != NULL)
    {
        null_pos++;
    }

    // Shift elements after the null terminator to beginning
    int shift_pos = null_pos+1;
    for (int i=0; shift_pos < size; i++, shift_pos++)
    {
        arr[i] = arr[shift_pos];
    }

    // Null-terminate the array at new end
    arr[size - null_pos - 1] = NULL;
}

// Function to parse the input command into tokens
int parse_input(char *input, char **tokens, char **input_file, char **output_file, int *append_output, int *background) {
    int token_count = 0;
    int i = 0;
    *input_file = NULL;
    *output_file = NULL;
    *append_output = 0;
    *background = 0;

    char *token = strtok(input, " ");
    while (token != NULL && token_count < MAX_TOKENS - 1) {
        if (strcmp(token, "<") == 0) {
            // Input redirection
            token = strtok(NULL, " ");
            if (token == NULL) {
                fprintf(stderr, "Syntax error: expected file after '<'\n");
                return -1;
            }
            *input_file = token;
        } else if (strcmp(token, ">") == 0) {
            // Output redirection (truncate)
            token = strtok(NULL, " ");
            if (token == NULL) {
                fprintf(stderr, "Syntax error: expected file after '>'\n");
                return -1;
            }
            *output_file = token;
            *append_output = 0;
        } else if (strcmp(token, ">>") == 0) {
            // Output redirection (append)
            token = strtok(NULL, " ");
            if (token == NULL) {
                fprintf(stderr, "Syntax error: expected file after '>>'\n");
                return -1;
            }
            *output_file = token;
            *append_output = 1;
        } else if (strcmp(token, "&") == 0) {
            // Background execution
            *background = 1;
        } else {
            tokens[token_count++] = token;
        }
        token = strtok(NULL, " ");
        i++;
    }
    tokens[token_count] = NULL;
    return token_count;
}

int main(int argc, char **argv) {
    char input[MAX_INPUT_SIZE];

    while (1) {
        // Print the prompt
        printf("[QUASH]$ ");
        fflush(stdout);

        // Read input
        if (fgets(input, MAX_INPUT_SIZE, stdin) == NULL) {
            break; // EOF
        }

        // Remove trailing newline
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
        }

        // Skip empty input
        if (strlen(input) == 0) {
            continue;
        }

        // Check and update background jobs
        check_background_jobs();

        // Parse the input into tokens and handle redirections
        char *tokens[MAX_TOKENS];
        char *input_file = NULL;
        char *output_file = NULL;
        int append_output = 0;
        int background = 0;
        int token_count = parse_input(input, tokens, &input_file, &output_file, &append_output, &background);

        if (token_count < 0) {
            continue;
        }

        // Check for exit or quit
        if (strcmp(tokens[0], "exit") == 0 || strcmp(tokens[0], "quit") == 0) {
            break;
        }

        // Implement built-in commands
        if (strcmp(tokens[0], "echo") == 0) {
            // Implement echo
            for (int i = 1; i < token_count; i++) {
                if (tokens[i][0] == '$') {
                    // Handle environment variables
                    char *env = getenv(tokens[i] + 1);
                    if (env != NULL) {
                        printf("%s ", env);
                    }
                } else {
                    printf("%s ", tokens[i]);
                }
            }
            printf("\n");
            continue;
        } else if (strcmp(tokens[0], "cd") == 0) {
            // Implement cd
            char *path = tokens[1];
            if (path == NULL) {
                path = getenv("HOME");
            }
            if (chdir(path) != 0) {
                perror("cd failed");
            } else {
                // Update PWD environment variable
                char cwd[1024];
                if (getcwd(cwd, sizeof(cwd)) != NULL) {
                    setenv("PWD", cwd, 1);
                }
            }
            continue;
        } else if (strcmp(tokens[0], "pwd") == 0) {
            // Implement pwd
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("%s\n", cwd);
            } else {
                perror("pwd failed");
            }
            continue;
        } else if (strcmp(tokens[0], "export") == 0) {
            // Implement export
            // Format: export VAR=VALUE
            if (tokens[1] != NULL) {
                char *var = strtok(tokens[1], "=");
                char *value = strtok(NULL, "=");
                if (var != NULL && value != NULL) {
                    // Handle value starting with $
                    if (value[0] == '$') {
                        char *env = getenv(value + 1);
                        if (env != NULL) {
                            setenv(var, env, 1);
                        } else {
                            setenv(var, "", 1);
                        }
                    } else {
                        setenv(var, value, 1);
                    }
                }
            }
            continue;
        } else if (strcmp(tokens[0], "jobs") == 0) {
            // Implement jobs
            print_jobs();
            continue;
        }

        // Check for pipes
        int pipe_present = 0;
        int pipe_indexes[token_count];
        int pipe_count = 0;

        // Initalize pipe_indexes to all 0's
        for (int i=0; i<token_count; i++)
        {
            pipe_indexes[i] = 0;
        }

        // Go through the tokens and count how many pipes their are while getting their index
        for (int i = 0; i < token_count; i++) {
            if (strcmp(tokens[i], "|") == 0) {
                pipe_present = 1;
                pipe_indexes[i] = i;
                pipe_count++;
            }
        }

        if (pipe_present) {
            int in_fd = STDIN_FILENO;

            for (int i=0; i<pipe_count; i++)
            {
                int fd[2];
                if (pipe(fd) == -1)
                {
                    perror("pipe error");
                    continue;
                }
            }


            // // Set all the pipe indexes to NULL for execvp
            // for (int i=0; i<token_count; i++) 
            // {
            //     if (pipe_indexes[i] == 1)
            //     {
            //         tokens[i] = NULL;
            //     }
            // }

            // if (pid == 0)
            // {
            //     // Set up redirection
            // }



            // for (int i=0; i <= pipe_count; i++) // Main loop for all piped functions
            // {
            //     int fd[2];
            //     if (pipe(fd) == -1)
            //     {
            //         perror("pipe error");
            //         continue;
            //     }

            //     pid_t pid = fork();
            //     if (pid < 0)
            //     {
            //         perror("fork");
            //         exit(EXIT_FAILURE);
            //     }

            //     if (pid == 0) // Child Processes
            //     {
            //         if (in_fd != STDIN_FILENO)
            //         {
            //             close(in_fd);
            //         }
            //     }
            //     if (i <= pipe_count-1) // Not the last command
            //     {
            //         dup2(fd[1], STDOUT_FILENO);
            //         close(fd[0]);
            //         close(fd[1]);
            //     }

            //     // Execute the commands
            //     char *current_command[MAX_TOKENS];
            //     int index = 0;
            //     do 
            //     {
            //         current_command[index] = tokens[index];
            //         index++;
            //     }
            //     while (tokens[index] != NULL);

            //     delete_up_to_null(tokens, sizeof(tokens));

            //     if (execvp(current_command[0], current_command) < 0)
            //     {
            //         perror(tokens[0]);
            //         exit(EXIT_FAILURE);
            //     }

            // }

           


        } else {
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

        // Check and update background jobs again
        check_background_jobs();
    }

    return 0;
}
