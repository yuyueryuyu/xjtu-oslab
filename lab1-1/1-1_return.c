#include<sys/wait.h>
#include<sys/types.h>
#include<stdio.h>
#include<unistd.h>
int global_int = 0;
int main() {
        pid_t pid, pid1;

        pid = fork();

        if (pid < 0) {
                fprintf(stderr, "Fork Failed");
                return 1;
        } else if (pid == 0) {
                pid1 = getpid();
                printf("child: pid = %d\n", pid);
                printf("child: pid1 = %d\n", pid1);
                global_int++;
                printf("child: global_int = %d\n", global_int);
                printf("child: &global_int = %p\n", &global_int);
        } else {
                pid1 = getpid();
                printf("parent: pid = %d\n", pid);
                printf("parent: pid1 = %d\n", pid1);
                global_int--;
                printf("parent: global_int = %d\n", global_int);
                printf("parent: &global_int = %p\n", &global_int);
                wait(NULL);
        }
	global_int += 10000;      
        printf("before return: global_int = %d\n", global_int);
        printf("before_return: &global_int = %p\n", &global_int);
        return 0;
}
