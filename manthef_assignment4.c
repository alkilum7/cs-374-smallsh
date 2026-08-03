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
static int exit_or_term;
static int bgps[1024];
static int foreground_mode;

void toggle_foreground(int errno) {
    if(foreground_mode) {
        foreground_mode = 0;
        write(STDOUT_FILENO, "\nExiting foreground-only mode\n", 30);
        fflush(stdout);
    } else {
        foreground_mode = 1;
        write(STDOUT_FILENO, 
            "\nEntering foreground-only mode (& is now ignored)\n", 50);
        fflush(stdout);
    }
}

void set_interrupt_handler() {
    signal(SIGINT, SIG_IGN);
    struct sigaction tstp_sigaction;
    tstp_sigaction.sa_handler = toggle_foreground;
    sigfillset(&tstp_sigaction.sa_mask);
    sigaction(SIGTSTP, &tstp_sigaction, NULL);
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
    if(exit_or_term == 0) {
        printf("exit status %d\n", status);
    } else if(exit_or_term == 1) {
        printf("terminated by signal %d\n", status);
    }
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
        if(foreground_mode) background = 0;
        int run_pid = fork();
        if(run_pid == 0) {
            // Set input and output redirection
            if(has_input) {
                int input_file = open(input_filename, O_RDONLY);
                if(input_file == -1) {
                    printf("cannot open %s for input\n", input_filename);
                    fflush(stdout);
                    exit(1);
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
                    exit(1);
                } else {
                    dup2(output_file, STDOUT_FILENO);
                }
            }

            // Run process, update status, and die
            execvp(argv[0], argv);
            printf("%s: no such file or directory\n", argv[0]);
            exit(1);
        } else {
            if(background) {
                printf("background pid is %d\n", run_pid);
            } else {
                int command_status = 0;
                waitpid(run_pid, &command_status, 0);
                status = WEXITSTATUS(command_status);
                exit_or_term = 0;
            }
        }
    }
}

int main(int argc, char *argv[]) {
    set_interrupt_handler();
    status = 0;
    exit_or_term = 0;
    foreground_mode = 0;

    // Prepare bgps vector
    for(int i = 0; i < 1024; i++) {
        bgps[i] = -1;
    }

    while(1) {
        char user_line[2048];
        
        // Get line
        do {
            printf(": ");
            fflush(stdout);
        } while(!fgets(user_line, 2048, stdin));

        // Process line
        run_line(user_line);
    }
}