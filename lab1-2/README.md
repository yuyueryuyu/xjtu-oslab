## 步骤一：设计程序，创建两个子线程，两线程分别对同一个共享变量多次操作，观察输出结果。最终在主进程中输出处理后的变量值，可以观察到出现图中结果，尝试分析原因。
### 代码：【1-2.c】
```c
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
```
### 运行结果：
```shell
[root@kp-test01 test]# ./1-2
thread1 created success!  thread2 created success!  variable result: -1164
[root@kp-test01 test]# ./1-2
thread1 created success!  thread2 created success!  variable result: -1716
[root@kp-test01 test]# ./1-2
thread1 created success!  thread2 created success!  variable result: -564
[root@kp-test01 test]# ./1-2
thread1 created success!  thread2 created success!  variable result: 1139
```
### 分析
注意到输出不稳定为0，其原因是两个线程同时访问变量，造成冲突所致。
### 问题
问题1：编译错误：undefined reference to 'pthread_create'等。
解决：需要显式链接pthread库，加上-lpthread。
原因：pthread_create 和其他线程相关的函数属于 POSIX 线程库，不包含在标准 C 库中，因此需要单独链接。

问题2：本步骤的输出稳定为0。经过检查，发现问题在于输出换行符，会使得清空缓存区，降低两个线程冲突的可能性。实际上不输出换行符也可能会发生稳定为0的情况。
如:
```shell
[root@kp-test01 test]# ./1-2
thread2 created success!
thread1 created success!
variable result: 0
[root@kp-test01 test]# ./1-2
thread1 created success!
thread2 created success!
variable result: 0
```
解决方法：尽量减少延迟较大的操作，可以减少输出、判断的操作。

## 步骤二：修改程序，定义信号量 signal，使用 PV 操作实现共享变量的访问与互斥。运行程序，观察最终共享变量的值。引入 PV 操作对变量进行加锁后，多次运行观察到图 1-9 所示结果，分析出现与步骤一不同结果的原因，并尝试灵活运用信号量和 PV 操作实现线程间的同步互斥。

### 代码[1-2_signal.c]
```c
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
```
### 运行结果：
```shell
[root@kp-test01 test]# ./1-2s
thread1 created success!
thread2 created success!
variable result: 0
[root@kp-test01 test]# ./1-2s
thread1 created success!
thread2 created success!
variable result: 0
[root@kp-test01 test]# ./1-2s
thread1 created success!
thread2 created success!
variable result: 0
[root@kp-test01 test]# ./1-2s
thread1 created success!
thread2 created success!
variable result: 0
```
### 分析：
注意到输出稳定为0，这是因为信号量的PV操作。
PV操作指的是：
P操作：当一个线程执行P操作时，它会检查信号量的值。
–如果信号量的值大于0，那么它将减少信号量的值（通常是减1）并继续执行。
–如果信号量的值为0，线程将被阻塞，直到信号量的值变为大于0。
V操作：
V操作增加信号量的值（通常是加1）。
如果有线程因执行P操作而阻塞在这个信号量上，一个或多个线程将被解除阻塞，并被允许减少信号量的值。

所以当线程执行时，在操作共享变量前，会首先进行P操作检查信号值是否为0（有其他线程在对该变量进行操作），若有则等待，若无则减小信号的值，再进行操作，这时其他线程进行P操作时会被阻塞，不会操作此变量。
而操作完成后，线程执行V操作，信号值增加，阻塞的线程恢复为大于0，恢复正常运转，对共享变量进行操作。

## 步骤三：在第一部分实验了解了 system()与 exec 族函数的基础上，将这两个函数的调用改为在线程中实现，输出进程 PID 和线程的 TID 进行分析。

### 代码：【1-2_sys.c】
```c
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
```
### 运行结果
```shell
[root@kp-test01 test]# ./1-2c
thread1 tid: 2748, pid: 2747
thread2 tid: 2749, pid: 2747
system_call: PID = 2747
system_call: PID = 2750
[root@kp-test01 test]# ./1-2c
thread1 tid: 2753, pid: 2752
thread2 tid: 2754, pid: 2752
system_call: PID = 2755
system_call: PID = 2752
[root@kp-test01 test]# ./1-2c
thread1 tid: 2757, pid: 2756
thread2 tid: 2758, pid: 2756
system_call: PID = 2756
system_call: PID = 2759
[root@kp-test01 test]# ./1-2c
thread1 tid: 2761, pid: 2760
thread2 tid: 2762, pid: 2760
system_call: PID = 2763
system_call: PID = 2760
```
### 分析：
注意到线程共享进程的PID，同时也会有自身独立的线程TID。同时TID和PID使用的是同一个自增序列（system fork的进程序号刚好接在TID之后）

### 问题：
问题1：有可能不输出system函数调用system_call的结果。
原因：与之前类似，如果exec族函数运行时system函数还未fork新进程独立，则可能新进程覆盖整个原来的进程，连同另一个线程一同被覆盖，另一个线程受影响不再继续执行。
如：
```shell
thread1 tid: 2611, pid: 2610
thread2 tid: 2612, pid: 2610
system_call: PID = 2610
```
解决方法：使用信号量 signal和 PV 操作实现互斥，控制函数执行顺序。

问题2：有时候部分结果输出完后，shell会先输出[root@kp-test01 test]#，再输出剩余的结果。
原因：在多线程情况下，线程终止和 shell 接管可能会交替导致显示顺序的不一致，程序结束时systemcall可能仍未完成输出。