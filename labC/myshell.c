#include "LineParser.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <ctype.h>

#define TERMINATED -1
#define RUNNING 1
#define SUSPENDED 0

#define HISTLEN 10

char* history_queue[HISTLEN];
int hist_head = 0;   
int hist_tail = 0;   
int hist_size = 0;   


typedef struct process {
    cmdLine* cmd;         
    pid_t pid;            
    int status;           
    struct process *next; 
} process;


process* global_process_list = NULL;
int debug_mode = 0;

void add_to_history(const char* input) {
    if (history_queue[hist_tail] != NULL) {
        free(history_queue[hist_tail]);
    }

    history_queue[hist_tail] = strdup(input);
    hist_tail = (hist_tail + 1) % HISTLEN;

    if (hist_size < HISTLEN) {
        hist_size++;
    } else {
        hist_head = (hist_head + 1) % HISTLEN;
    }
}

void printHistory() {
    for (int i = 0; i < hist_size; i++) {
        int index = (hist_head + i) % HISTLEN;
        printf("%d: %s", i + 1, history_queue[index]);
    }
}

void freeHistory() {
    for (int i = 0; i < HISTLEN; i++) {
        if (history_queue[i] != NULL) {
            free(history_queue[i]);
        }
    }
}

void updateProcessStatus(process* process_list, int pid, int status) {
    process* curr = process_list;
    while (curr != NULL) {
        if (curr->pid == pid) {
            curr->status = status;
            return;
        }
        curr = curr->next;
    }
}

void updateProcessList(process** process_list) {
    process* curr = *process_list;
    while (curr != NULL) {
        int status;
        pid_t result = waitpid(curr->pid, &status, WNOHANG | WUNTRACED | WCONTINUED);

        if (result == -1) {
            curr->status = TERMINATED;
        } else if (result > 0) {
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                curr->status = TERMINATED;
            } else if (WIFSTOPPED(status)) {
                curr->status = SUSPENDED;
            } else if (WIFCONTINUED(status)) {
                curr->status = RUNNING;
            }
        }
        curr = curr->next;
    }
} 

void freeProcessList(process* process_list) {
    process* curr = process_list;
    while (curr != NULL) {
        process* next = curr->next;
        if (curr->cmd != NULL) {
            freeCmdLines(curr->cmd);
        }
        free(curr);
        curr = next;
    }
}

void addProcess(process** process_list, cmdLine* cmd, pid_t pid) {
    process* new_proc = (process*)malloc(sizeof(process));

    new_proc->cmd = cmd; 
    new_proc->pid = pid;
    new_proc->status = RUNNING;
    new_proc->next = *process_list;
    
    *process_list = new_proc;
}

void printProcessList(process** process_list) {
    updateProcessList(process_list);

    printf("PID          Command          STATUS\n");
    process* curr = *process_list;
    process* prev = NULL;

    while (curr != NULL) {
        char* status_str;
        if (curr->status == RUNNING) status_str = "Running";
        else if (curr->status == SUSPENDED) status_str = "Suspended";
        else status_str = "Terminated";

        printf("%-12d %-16s %s\n", curr->pid, curr->cmd->arguments[0], status_str);

        if (curr->status == TERMINATED) {
            process* to_delete = curr;
            
            if (prev == NULL) { 
                *process_list = curr->next;
                curr = *process_list;
            } else { 
                prev->next = curr->next;
                curr = curr->next;
            }

            if (to_delete->cmd) {
                freeCmdLines(to_delete->cmd);
            }
            free(to_delete);
        } else {
            prev = curr;
            curr = curr->next;
        }
    }
}

void execute(cmdLine* pCmdLine){

    // Pipeline
    if (pCmdLine->next != NULL) {
        if (pCmdLine->outputRedirect != NULL || pCmdLine->next->inputRedirect != NULL) {
            fprintf(stderr, "Error: Invalid redirection in pipeline\n");
            return;
        }

        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("pipe");
            return;
        }

        pid_t pid1 = fork();
        if (pid1 == 0) { // Child 1 (Writer)
            if (pCmdLine->inputRedirect) {
                int fi = open(pCmdLine->inputRedirect, O_RDONLY);
                dup2(fi, STDIN_FILENO);
                close(fi);
            }
            dup2(pipefd[1], STDOUT_FILENO); 
            close(pipefd[0]);
            close(pipefd[1]);

            execvp(pCmdLine->arguments[0], pCmdLine->arguments);
            perror("execvp child1 failed");
            _exit(1);
        }

        close(pipefd[1]);

        pid_t pid2 = fork();
        if (pid2 == 0) { // Child 2 (Reader)
            if (pCmdLine->next->outputRedirect) {
                int fo = open(pCmdLine->next->outputRedirect, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                dup2(fo, STDOUT_FILENO);
                close(fo);
            }
            dup2(pipefd[0], STDIN_FILENO); 
            close(pipefd[0]);

            execvp(pCmdLine->next->arguments[0], pCmdLine->next->arguments);
            perror("execvp child2 failed");
            _exit(1);
        }

        // Parent
        // the printProcessList procedure recursivly deletes the terminated commands, we split them here to avoid getting the second child deleted because of the first
        cmdLine* nextCmd = pCmdLine->next; 
        pCmdLine->next = NULL;
        addProcess(&global_process_list, pCmdLine, pid1);
        addProcess(&global_process_list, nextCmd, pid2); 

        close(pipefd[0]);
        if (pCmdLine->blocking) {
            waitpid(pid1, NULL, 0);
            waitpid(pid2, NULL, 0);
        }
        return; 
    } // end of Pipeline scenerio

    // normal command
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return; }

    if (pid == 0) { // Child
        if (pCmdLine->inputRedirect != NULL) {
            int fi = open(pCmdLine->inputRedirect, O_RDONLY);
            if (fi == -1) { perror("open input"); _exit(1); }
            dup2(fi, STDIN_FILENO);
            close(fi);
        }
        if (pCmdLine->outputRedirect != NULL) {
            int fo = open(pCmdLine->outputRedirect, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fo == -1) { perror("open output"); _exit(1); }
            dup2(fo, STDOUT_FILENO);
            close(fo);
        }

        execvp(pCmdLine->arguments[0], pCmdLine->arguments);
        perror("execvp failed");
        _exit(1);
    } else { // Parent
        addProcess(&global_process_list, pCmdLine, pid); // Add child
        if (debug_mode) fprintf(stderr, "PID: %d | Command: %s\n", pid, pCmdLine->arguments[0]);
        if (pCmdLine->blocking) waitpid(pid, NULL, 0);
    } // end of normal command
}



int main (int argc, char* argv[]){

    char buffer[PATH_MAX]; 
    char input[2048];

    if(argc > 1 && (strcmp(argv[1], "-d") == 0)){
        debug_mode = 1;
    }
    
    while(1) {
        getcwd(buffer, PATH_MAX);
        usleep(50000);
        printf("%s>", buffer);
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        if (strncmp(input, "quit", 4) == 0) {
            freeProcessList(global_process_list); 
            freeHistory(); 
            break;
        }

        // (!! or !n)
        char* cmd_to_parse = input;
        if (input[0] == '!') {
            int target_idx = -1;
            if (input[1] == '!' && hist_size > 0) {
                target_idx = (hist_tail - 1 + HISTLEN) % HISTLEN;
            } else if (isdigit(input[1])) {
                int n = atoi(&input[1]);
                if (n > 0 && n <= hist_size) {
                    target_idx = (hist_head + n - 1) % HISTLEN;
                }
            }
            if (target_idx != -1) {
                cmd_to_parse = history_queue[target_idx];
                printf("Executing: %s", cmd_to_parse);
            } else {
                fprintf(stderr, "History index out of range\n");
                continue;
            }
        }

        // add to history
        if (input[0] != '!' || strncmp(cmd_to_parse, "history", 7) != 0) {
            add_to_history(cmd_to_parse);
        }

        cmdLine* line = parseCmdLines(cmd_to_parse);

        if(debug_mode) { 
            fprintf(stderr, "The command is: %s", input);
        }
        if(line != NULL) {
            if (strcmp(line->arguments[0], "history") == 0) {
                printHistory();
                freeCmdLines(line);
            }
            else if (strcmp(line->arguments[0], "procs") == 0) {
                printProcessList(&global_process_list);
                freeCmdLines(line); 
            } 
            else if (strcmp(line->arguments[0], "cd") == 0) {
                if (line->argCount < 2) {
                    fprintf(stderr, "cd: missing argument\n");
                } 
                else if (chdir(line->arguments[1]) == -1) {
                    perror("cd failed");
                }
                freeCmdLines(line); 
            }
            else if (strcmp(line->arguments[0], "zzzz") == 0) {
                if (line->argCount < 2) {
                    fprintf(stderr, "zzzz: missing PID\n");
                } else {
                    int target_pid = atoi(line->arguments[1]);
                    if (kill(target_pid, SIGSTOP) == 0) {
                        printf("Signal SIGSTOP sent to process %d\n", target_pid);
                        updateProcessStatus(global_process_list, target_pid, SUSPENDED);
                    } else {
                        perror("zzzz failed");
                    }
                }
                freeCmdLines(line);
            }
            else if (strcmp(line->arguments[0], "blast") == 0) {
                if (line->argCount < 2) {
                    fprintf(stderr, "blast: missing PID\n");
                } else {
                    int target_pid = atoi(line->arguments[1]);
                    if (kill(target_pid, SIGINT) == 0) {
                        printf("Signal SIGINT sent to process %d\n", target_pid);
                        updateProcessStatus(global_process_list, target_pid, TERMINATED);
                    } else {
                        perror("blast failed");
                    }
                }
                freeCmdLines(line);
            }
            else if (strcmp(line->arguments[0], "kuku") == 0) {
                if (line->argCount < 2) {
                    fprintf(stderr, "kuku: missing PID\n");
                } else {
                    int target_pid = atoi(line->arguments[1]);
                    if (kill(target_pid, SIGCONT) == 0) {
                        printf("Signal SIGCONT sent to process %d\n", target_pid);
                        updateProcessStatus(global_process_list, target_pid, RUNNING);
                    } else {
                        perror("kuku failed");
                    }
                }
                freeCmdLines(line);
            }
            else {
                execute(line);
            }
        }  
    }
    return 0;
}