#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
    int pipefd[2];
    pid_t child1, child2;

    // 1
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "(parent_process>forking...)\n");

    // 2
    child1 = fork();

    if (child1 == -1) {
        perror("fork1");
        exit(EXIT_FAILURE);
    }

    // 3
    if (child1 == 0) {

        fprintf(stderr, "(child1>redirecting stdout to the write end of the pipe...)\n");

        // 3.1
        close(STDOUT_FILENO);
        
        // 3.2
        dup(pipefd[1]);

        // 3.3
        close(pipefd[1]);
        close(pipefd[0]);

        // 3.4
        char* args[] = {"ls", "-lsa", NULL};
        fprintf(stderr, "(child1>going to execute cmd: %s)\n", args[0]);
        execvp(args[0], args);
        
        // in case execvp failed
        perror("execvp child1");
        _exit(1);
    }

    // parent
    fprintf(stderr, "(parent_process>created process with id: %d)\n", child1);

    // 4   
    fprintf(stderr, "(parent_process>closing the write end of the pipe...)\n"); 
    close(pipefd[1]);

    // 5
    child2 = fork();

    if (child2 == -1) {
        perror("fork2");
        exit(EXIT_FAILURE);
    }

    // 6
    if (child2 == 0) {
 
        fprintf(stderr, "(child2>redirecting stdin to the read end of the pipe...)\n");

        // 6.1
        close(STDIN_FILENO);

        // 6.2
        dup(pipefd[0]);

        // 6.3
        close(pipefd[0]);

        // 6.4
        char *args[] = {"tail", "-n", "3", NULL};
        fprintf(stderr, "(child2>going to execute cmd: %s)\n", args[0]);
        execvp(args[0], args);

        // in case execvp failed
        perror("execvp child2");
        _exit(1);
    }

    // parent
    fprintf(stderr, "(parent_process>created process with id: %d)\n", child2);

    // 7
    fprintf(stderr, "(parent_process>closing the read end of the pipe...)\n");
    close(pipefd[0]);

    // 8
    fprintf(stderr, "(parent_process>waiting for child processes to terminate...)\n");
    waitpid(child1, NULL, 0);
    waitpid(child2, NULL, 0);

    fprintf(stderr, "(parent_process>exiting...)\n");
    return 0;
}