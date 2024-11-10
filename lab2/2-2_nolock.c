#include <unistd.h> 
#include <signal.h> 
#include <stdio.h> 
#include <sys/wait.h>
#include <stdlib.h>
int pid1,pid2;                              
int main( ) {
    int fd[2]; 
    char InPipe[4010];                    
    char c1='1', c2='2'; 
    pipe(fd);                              
    while((pid1 = fork( )) == -1);      
    if(pid1 == 0) {                                      
        for (int i = 0;i < 2000;i++) {
            write(fd[1], &c1, 1);
        }                                
        sleep(5);                             
        exit(0);          
    } 
    else { 
        while((pid2 = fork()) == -1);                
        if(pid2 == 0) { 
            for (int i = 0;i < 2000;i++) {
                write(fd[1], &c2, 1);
            }                       
            sleep(5); 
            exit(0); 
        } else { 
            wait(0); 
            wait(0); 
            read(fd[0], InPipe, 4000); 
            InPipe[4000] = '\0';
            printf("%s\n", InPipe);
            exit(0); 
        } 
    }
}