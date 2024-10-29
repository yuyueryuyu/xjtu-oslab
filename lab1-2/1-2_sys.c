#include<semaphore.h>
#include<pthread.h>
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<syscall.h>

sem_t signal;

void *system_func() {
    printf("thread1 tid: %ld, pid: %d\n", syscall(SYS_gettid), getpid());
    system("./system_call");
    sem_post(&signal);
    return NULL;
}

void *exec_func() {
    sem_wait(&signal);
    printf("thread2 tid: %ld, pid: %d\n", syscall(SYS_gettid), getpid());
    execl("./system_call", "system_call", NULL);
    return NULL; 
}

int main() {
    pthread_t system_thread, exec_thread;
    if (sem_init(&signal, 0, 1) == -1) {
        fprintf(stderr, "sem_init Failed");
        return 1;
    }
    if (pthread_create(&system_thread, NULL, system_func, NULL)) {
        fprintf(stderr, "Error creating system thread\n");
        return 1;
    }
    if (pthread_create(&exec_thread, NULL, exec_func, NULL)) {
        fprintf(stderr, "Error creating exec thread\n");
        return 1;
    }

    pthread_join(system_thread, NULL);
    pthread_join(exec_thread, NULL);
    sem_destroy(&signal);
    return 0;
}
