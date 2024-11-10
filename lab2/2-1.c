#include <stdio.h> 
#include <unistd.h> 
#include <sys/wait.h> 
#include <stdlib.h> 
#include <signal.h> 

void inter_handler(int signum) { 
    switch (signum) {
        case SIGINT:
            printf("Received signal SIGINT\n");
            break;
        case SIGALRM:
            printf("Received signal SIGALRM\n");
            break;
        case SIGQUIT:
            printf("Received signal SIGQUIT\n");
            break;
        default:
            printf("Received signal id = %d\n", signum);
            break;
    }
} 
void waiting(int signum) {
    pause();
}
int main() { 
    pid_t pid1 = -1, pid2 = -1; 
    while (pid1 == -1)  pid1 = fork(); 
    if (pid1 > 0) { 
        while (pid2 == -1)  pid2 = fork(); 
        if (pid2 > 0) { 
            signal(SIGINT, inter_handler);
            signal(SIGQUIT, inter_handler);
            signal(SIGALRM, inter_handler);
            alarm(5);
            pause();
            kill(pid1, 16);
            kill(pid2, 17);
            wait(NULL);
            wait(NULL);
            printf("Parent process is killed!!\n"); 
        } else { 
            signal(17, inter_handler);
            signal(SIGINT, waiting);
            signal(SIGQUIT, waiting);
            signal(SIGALRM, waiting);
            pause();
            printf("Child process2 is killed by parent!!\n"); 
            return 0; 
        } 
    } else { 
        signal(16, inter_handler);
        signal(SIGINT, waiting);
        signal(SIGQUIT, waiting);
        signal(SIGALRM, waiting);
        pause();
        printf("Child process1 is killed by parent!!\n"); 
        return 0; 
    } 
    return 0; 
} 