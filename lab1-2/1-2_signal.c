#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<semaphore.h>

int shared = 0;
sem_t signal;

void* add() {
    printf("thread1 created success!\n"); 
    for (int i = 0; i < 10000; i++) {
        sem_wait(&signal);
	    shared++;
	    sem_post(&signal);
    }
    return NULL;
}

void* sub() {
    printf("thread2 created success!\n"); 
    for (int i = 0; i < 10000; i++) {
        sem_wait(&signal);                                                                      
	    shared--;
        sem_post(&signal); 
    }
    return NULL;
}

int main() {
    pthread_t thread1, thread2;
    
    if (sem_init(&signal, 0, 1) == -1) {
        fprintf(stderr, "sem_init Failed");
        return 1;
    }

    if (pthread_create(&thread1, NULL, add, NULL) != 0) {
        fprintf(stderr, "Failed to create thread1");
        return 1;
    }
    if (pthread_create(&thread2, NULL, sub, NULL) != 0) {
        fprintf(stderr, "Failed to create thread2");
        return 1;
    }

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    printf("variable result: %d\n", shared);
    sem_destroy(&signal);

    return 0;
}
