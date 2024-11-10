#include <unistd.h> 
#include <signal.h> 
#include <stdio.h> 
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
int pid1,pid2;                              
int main( ) {
    int fd[2]; 
    char InPipe[410];   
    char OutPipe[200];                 
    pipe(fd);                              
    while((pid1 = fork( )) == -1);      
    if(pid1 == 0) {                     
        strcpy(OutPipe, "Child Process 1 is sending message!");                    
        write(fd[1], OutPipe, 35);                               
        sleep(5);                             
        exit(0);          
    } 
    else { 
        while((pid2 = fork()) == -1);                
        if(pid2 == 0) { 
            strcpy(OutPipe, "Child Process 2 is sending message!");                    
            write(fd[1], OutPipe, 35);                    
            sleep(5); 
            exit(0); 
        } else { 
            wait(0); 
            wait(0); 
            read(fd[0], InPipe, 70); 
            InPipe[70] = '\0';
            printf("%s\n", InPipe);
            exit(0); 
        } 
    }
}