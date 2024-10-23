#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>
#include <sys/wait.h>
#include <fcntl.h>

typedef struct {
    char* input_file;
    char* output_file;
    int append; // 0: > to overwrite, 1: >> to append
} IO_redirect;

void check_for_IO_redirect(const char *input) {
    if (strncmp(input, "<", 1) == 0) {
        // Check to see if the user is trying to put input into a file
        IOredirect = 1;
    }
    else if (strncmp(input, ">", 1) == 0) {
        IOredirect = 0;
    }
    else if (strncmp(input, ">>", 2) == 0) {
        IOredirect = 2;
    }
}

void redirect_IO(IO_redirect *io_red) {

    // Handle input redirection >
    if (io_red->input_file != NULL) {
        int fd_in = open(io_red->input_file, O_RDONLY);
        if (fd_in < 0){
            perror("Error opening input file");
            exit(EXIT_FAILURE);
        }
        if(dup2(fd_in, STDIN_FILENO) < 0){
            perror("Error duplicating descriptor file for input");
            close(fd_in);
            exit(EXIT_FAILURE);
        }
        close(fd_in);
    }

    // Handle output redirection
    if (io_red->output_file != NULL){
        int fd_out;
        if (io_red->append){
            int fd_out = open(io_red->output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
        }
        else{
            int fd_out = open(io_red->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        }

        if (fd_out < 0){
            perror("Error opening ouput file");
            exit(EXIT_FAILURE);
        }

        if (dup2(fd_out, STDOUT_FILENO) < 0){
            perror("Error duplicating file desciptor for output");
            close(fd_out);
            exit(EXIT_FAILURE);
        }
        close(fd_out);
    }
}