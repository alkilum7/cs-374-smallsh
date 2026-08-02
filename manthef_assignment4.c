#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

static int status;
static int bgps[1024];

void set_interrupt_handler() {
    signal(SIGINT, SIG_IGN);

}

void exit_shell() {
    exit(0);
    // WIP: kill children
}

void cd_command(int argc, char *argv[]) {
    char *path;
    if(argc == 1) {
        path = getenv("HOME");
    } else {
        path = argv[1];
    }
    chdir(path);
    return;
}

void print_status() {
    printf("exit status %d", status);
    return;
}

void run_line(char *user_line) {
    char *argv[512];
    int argc = 0;
    char *token = strtok(user_line, " \n");
    char *input_filename;
    int has_input = 0;
    char *output_filename;
    int has_output = 0;
    int background = 0;

    // Ignore comments and blanks
    if(token == NULL) return;
    if(*token == '#') return;

    for(;token != NULL; token = strtok(NULL, " \n")) {
        if(strcmp(token, "<") == 0) {
            token = strtok(NULL, " \n");
            input_filename = strdup(token);
            has_input = 1;
        } else if (strcmp(token, ">") == 0) {
            token = strtok(NULL, " \n");
            output_filename = strdup(token);
            has_output = 1;
        } else if (strcmp(token, "&") == 0) {
            background = 1;
        } else {
            argv[argc] = strdup(token); //FREETHIS
            argc++;
        }
    }
    argv[argc] = NULL;

    // Check for shell commands
    if(strcmp(argv[0], "exit") == 0) {
        exit_shell();
    } else if(strcmp(argv[0], "cd") == 0) {
        cd_command(argc, argv);
    } else if(strcmp(argv[0], "status") == 0) {
        print_status();
    } else {
        // Run arbitrary command
        int run_pid = fork();
        if(run_pid == 0) {
            // Set input and output redirection
            if(has_input) {
                int input_file = open(input_filename, O_RDONLY);
                if(input_file == -1) {
                    printf("cannot open %s for input\n", input_filename);
                    fflush(stdout);
                } else {
                    dup2(input_file, STDIN_FILENO);
                }
            }
            // Output
            if(has_output) {
                int output_file = open(output_filename, O_WRONLY | O_CREAT);
                if(output_file == -1) {
                    printf("Could not create %s", output_filename);
                    fflush(stdout);
                } else {
                    dup2(output_file, STDOUT_FILENO);
                }
            }

            // Run process, update status, and die
            execvp(argv[0], argv);
            printf("%s: no such file or directory\n", argv[0]);
            exit(1);
        } else {
            if(!background) {
                waitpid(run_pid, &status, 0);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    set_interrupt_handler();
    status = 0;
    int no_loop = 0;

    while(no_loop < 100) {
        char user_line[2048];
        
        // Get line
        printf(": ");
        fflush(stdout);
        fgets(user_line, 2048, stdin);

        // Process line
        run_line(user_line);
        no_loop++;
    }
}