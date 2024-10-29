#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>

int shared = 0;

void* add() {
    printf("thread1 created success!  ");	//如果结尾使用换行符的话会刷新缓冲区，减少add()和sub()对shared的同时访问
    for (int i = 0; i < 10000; i++) {
        shared++;
    }
    return NULL;
}

void* sub() {
    printf("thread2 created success!  ");
    for (int i = 0; i < 10000; i++) {
        shared--;
    }
    return NULL;
}

int main() {
    pthread_t thread1, thread2;

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

    return 0;
}
