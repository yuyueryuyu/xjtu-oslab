# 2.1 进程的软中断通信
## 步骤一：使用 man 命令查看 fork 、kill 、signal、sleep、exit 系统调用的帮助手册。
- 输入man fork / man kill等命令依次进行了查看。详略。
## 步骤二：根据流程图编制实现软中断通信的程序。
### 代码：[2-1.c]
```c
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
```
### 需要思考的问题：
#### (1)父进程向子进程发送信号时，如何确保子进程已经准备好接收信号？
答：在进入子进程的时候首先调用signal函数，确保父进程的信号得到正确处理。
#### (2)如何阻塞住子进程，让子进程等待父进程发来信号？
答：在子进程中调用signal()函数，使得子进程收到SIGINT等信号后触发waiting()函数，重新设置pause()，直到收到父进程的16/17信号。
## 步骤三： 多次运行所写程序，比较 5s 内按下 Ctrl+\或 Ctrl+Delete 发送中断，或 5s 内不进行任何操作发送中断，分别会出现什么结果？分析原因。
### 5s内按下Ctrl+C：
```shell
[yuyueryuyu@Yu lab2]$ ./2-1
^CReceived signal SIGINT
Received signal id = 16
Child process1 is killed by parent!!
Received signal id = 17
Child process2 is killed by parent!!
Parent process is killed!!
[yuyueryuyu@Yu lab2]$ ./2-1
^CReceived signal SIGINT
Received signal id = 17
Child process2 is killed by parent!!
Received signal id = 16
Child process1 is killed by parent!!
Parent process is killed!!
[yuyueryuyu@Yu lab2]$ ./2-1
^CReceived signal SIGINT
Received signal id = 16
Child process1 is killed by parent!!
Received signal id = 17
Child process2 is killed by parent!!
Parent process is killed!!
```
- 父进程收到了SIGINT信号，随后分别发给两个子进程16和17信号，子进程退出后父进程退出。注意到子进程接收到信号的顺序并不固定。
### 5s内按下ctrl + \:
```shell
[yuyueryuyu@Yu lab2]$ ./2-1
^\Received signal SIGQUIT
Received signal id = 16
Child process1 is killed by parent!!
Received signal id = 17
Child process2 is killed by parent!!
Parent process is killed!!
[yuyueryuyu@Yu lab2]$ ./2-1
^\Received signal SIGQUIT
Received signal id = 16
Received signal id = 17
Child process1 is killed by parent!!
Child process2 is killed by parent!!
Parent process is killed!!
[yuyueryuyu@Yu lab2]$ ./2-1
^\Received signal SIGQUIT
Received signal id = 17
Received signal id = 16
Child process2 is killed by parent!!
Child process1 is killed by parent!!
Parent process is killed!!
```
- 父进程收到了SIGQUIT信号，随后分别发给两个子进程16和17信号，子进程退出后父进程退出。注意到子进程接收到信号的顺序并不固定。
###  5s 内不进行任何操作发送中断：
```shell
[yuyueryuyu@Yu lab2]$ ./2-1
Received signal SIGALRM
Received signal id = 16
Child process1 is killed by parent!!
Received signal id = 17
Child process2 is killed by parent!!
Parent process is killed!!
[yuyueryuyu@Yu lab2]$ ./2-1
Received signal SIGALRM
Received signal id = 16
Child process1 is killed by parent!!
Received signal id = 17
Child process2 is killed by parent!!
Parent process is killed!!
[yuyueryuyu@Yu lab2]$ ./2-1
Received signal SIGALRM
Received signal id = 17
Child process2 is killed by parent!!
Received signal id = 16
Child process1 is killed by parent!!
Parent process is killed!!
```
- 5s后，父进程按照alarm()函数的约定收到了SIGALRM信号，随后分别发给两个子进程16和17信号，子进程退出后父进程退出。注意到子进程接收到信号的顺序并不固定。
## 步骤四：（4）将本实验中通信产生的中断通过 14 号信号值进行闹钟中断，体会不同中断的执行样式，从而对软中断机制有一个更好的理解。
为了进行闹钟中断，我们将程序中的
```c
signal(SIGALRM, inter_handler);
```
语句注释。
### 运行结果：
```shell
[yuyueryuyu@Yu lab2]$ ./2-1
Alarm clock
```
### 分析：在没有进行特殊处理的情况下，闹钟中断会输出Alarm clock，直接终止程序，不继续执行。

## 报告中运行结果与分析部分，请回答下列问题。
### （1）你最初认为运行结果会怎么样？写出你猜测的结果。
- 父进程首先收到对应信号（SIGALRM/SIGINT/SIGQUIT），输出"Received signal..."
- 父进程使用kill()发送信号给子进程，两个子进程分别收到信号，输出"Received signal 16/17"
- 子进程结束，输出"Child process1/2 is killed by parent!!"
- 父进程结束，输出"Parent process is killed!!"
### （2）实际的结果什么样？有什么特点？在接收不同中断前后有什么差别？请将 5 秒内中断和 5 秒后中断的运行结果截图，试对产生该现象的原因进行分析。
- 大体上按以上顺序执行，但是子进程结束的顺序并不稳定。五秒后已经收到信号SIGALRM，因子按照SIGALRM进行中断。
### （3）改为闹钟中断后，程序运行的结果是什么样子？与之前有什么不同？
- 输出Alarm clock。
### （4）kill 命令在程序中使用了几次？每次的作用是什么？执行后的现象是什么？
- 2次。执行后子进程收到信号，开始执行对应函数，同时子进程的pause()函数返回。
### （5）使用 kill 命令可以在进程的外部杀死进程。进程怎样能主动退出？这两种退出方式哪种更好一些？
- 主动退出：使用return或exit或abort。其中abort主要用于调试。
- 两种方式各有各的优势：
- kill命令：强制退出，适用于进程处于各种原因无法响应、无法主动退出的情况；但是kill命令具有突然性，无法进行保存和数据清理，可能造成安全问题。
- 主动退出：适用于程序正常运行的情况。这种退出正常可控，并且可以返回状态码，可供调试。但是如果程序处于某些原因无法触及这些语句，则需要外部强制退出。

# 2.2 进程的管道通信
## （1）学习 man 命令的用法，通过它查看管道创建、同步互斥系统调用的在线帮助，并阅读参考资料。
- 输入man pipe / man write / man read等命令依次进行了查看。详略。
## （2）根据流程图和所给管道通信程序，按照注释里的要求把代码补充完整，运行程序，体会互斥锁的作用，比较有锁和无锁程序的运行结果，分析管道通信是如何实现同步与互斥的。
## （3）修改上述程序，让其中两个子进程各向管道写入 2000 个字符，父进程从管道中读出，分有锁和无锁的情况。运行程序，分别观察有锁和无锁情况下的写入读出情况。
### 代码：[2-2.c]
```c
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
        lockf(fd[1],1,0);                    
        for (int i = 0;i < 2000;i++) {
            write(fd[1], &c1, 1);
        }                                
        sleep(5);                          
        lockf(fd[1],0,0);    
        exit(0);          
    } 
    else { 
        while((pid2 = fork()) == -1);                
        if(pid2 == 0) { 
            lockf(fd[1],1,0); 
            for (int i = 0;i < 2000;i++) {
                write(fd[1], &c2, 1);
            }                       
            sleep(5); 
            lockf(fd[1],0,0); 
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
```
### 有锁运行结果：
```shell
[yuyueryuyu@Yu lab2]$ ./2-2
1111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111122222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222
```
### 无锁运行结果：
- 将lockf语句注释。
```shell
[yuyueryuyu@Yu lab2]$ ./2-2
1111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111211111112121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111121111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111212112121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212121212122222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222
```
### 比较结果
- 注意到有锁程序1严格于2之前写入管道，无锁程序则在中段出现了1、2交替写入的情况。
### 分析
- lockf(fd[1],1,0);在写入前调用，第二个参数置为1，锁定fd[1]，写入完成后调用lockf(fd[1],0,0); 第二个参数置为0，用于解锁。在锁定后，由于第三个参数len为0，说明长度是正无穷，整个管道都被锁住，防止其他进程写入。解锁后，其他进程恢复正常写入的权限。

## 报告中运行结果与分析部分，请回答下列问题。
### (1)你最初认为运行结果会怎么样？
- 对于有锁，先输出1后输出2；对于无锁，1和2乱序输出。
### (2)实际的结果什么样？有什么特点？试对产生该现象的原因进行分析。
- 有锁和预测一样，这是进程锁的同步和互斥在起作用。对于无锁情况实际只有在中段乱序输出。原因是两个子进程执行的时间差，导致子进程1的头部分和子进程2的尾部分两个进程并没有同时写入管道。在中间部分，两个进程同时写入，因此出现了乱序。
### (3)实验中管道通信是怎样实现同步与互斥的？如果不控制同步与互斥会发生什么后果？
- 因为管道可以看作是一个文件，所以是通过文件锁lockf进行实现互斥。如果不控制同步和互斥就会出现两个进程同时对管道进行写入，造成写入混乱的情况。

# 2.3 内存的分配与回收
## （1）理解内存分配 FF，BF，WF 策略及实现的思路。
### FF（First Fit）策略： 
- 即首次适应算法。在采用空闲分区链作为数据结构时，该算法要求空闲分区链表以地址递增的次序链接。在进行内存分配时，从链首开始顺序查找，直至找到一个能满足进程大小要求的空闲分区为止。然后，再按照进程请求内存的大小，从该分区中划出一块内存空间分配给请求进程，余下的空闲分区仍留在空闲链中。
### BF（Best Fit）策略： 
- 即最佳适应算法。将空闲分区链表按分区大小由小到大排序，在链表中查找第一个满足要求的分区。
### WF（Worst Fit）策略： 
- 即最差匹配算法。将空闲分区链表按分区大小由大到小排序，在链表中找到第一个满足要求的空闲分区
## （2）参考给出的代码思路，定义相应的数据结构，实现上述 3 种算法。每种算法要实现内存分配、回收、空闲块排序以及合并、紧缩等功能。
### 代码 :[2-3.c]
```c
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>

#define PROCESS_NAME_LEN 32 /** 进程名长度 */
#define MIN_SLICE 10 /** 最小碎片的大小 */
#define DEFAULT_MEM_SIZE 1024 /** 内存大小 */
#define DEFAULT_MEM_START 0 /** 起始位置 */

/* 内存分配算法 */
#define MA_FF 1
#define MA_BF 2
#define MA_WF 3

/*描述每一个空闲块的数据结构*/
struct free_block_type {
    int size;
    int start_addr;
    struct free_block_type * next;
};

/*指向内存中空闲块链表的首指针*/
struct free_block_type * free_block;

/*每个进程分配到的内存块的描述*/
struct allocated_block {
    int pid;
    int size;
    int start_addr;
    char process_name[PROCESS_NAME_LEN];
    struct allocated_block * next;
};

/*进程分配内存块链表的首指针*/
struct allocated_block * allocated_block_head = NULL;

/* 内存大小 */
int mem_size = DEFAULT_MEM_SIZE; 
/* 当前分配算法 */
int ma_algorithm = MA_FF; 
/* 初始 pid */
static int pid = 0; 
/* 设置内存大小标志 */
int flag = 0; 

/* 函数声明 */
struct free_block_type * init_free_block(int mem_size);
int allocate_mem(struct allocated_block *ab);
void kill_process();
struct allocated_block *find_process(int pid);
int free_mem(struct allocated_block *ab);
int dispose(struct allocated_block *free_ab);
void kill_block(struct allocated_block *ab);
int display_mem_usage();
int set_mem_size();
void set_algorithm();
void display_menu();
int new_process();
void rearrange(int algorithm);
void rearrange_FF();
void rearrange_BF();
void rearrange_WF();
void memory_merge();
void memory_sort();
void memory_compaction();
void do_exit();

int main() {
    int choice;
    pid = 0;
    free_block = init_free_block(mem_size); //初始化空闲区
    while (1) {
        display_menu(); //显示菜单
        scanf("%d", &choice);
        switch (choice) {
            case 1:
                set_mem_size();
                break; //设置内存大小
            case 2:
                set_algorithm();
                flag = 1;
                break; //设置算法
            case 3:
                new_process();
                flag = 1;
                break; //创建新进程
            case 4:
                kill_process();
                flag = 1;
                break; //删除进程
            case 5:
                display_mem_usage();
                flag = 1;
                break; //显示内存使用
            case 0:
                do_exit();
                exit(0);
                //释放链表并退出
            default:
                break;
        }
    }
}

/*初始化空闲块，默认为一块，可以指定大小及起始地址*/
struct free_block_type * init_free_block(int mem_size) {
    struct free_block_type * fb;
    fb = (struct free_block_type * ) malloc(sizeof(struct free_block_type));
    if (fb == NULL) {
        printf("No mem\n");
        return NULL;
    }
    fb -> size = mem_size;
    fb -> start_addr = DEFAULT_MEM_START;
    fb -> next = NULL;
    return fb;
}

/*显示菜单*/
void display_menu() {
    printf("\n");
    printf("1 - Set memory size (default=%d)\n", DEFAULT_MEM_SIZE);
    printf("2 - Select memory allocation algorithm\n");
    printf("3 - New process \n");
    printf("4 - Terminate a process \n");
    printf("5 - Display memory usage \n");
    printf("0 - Exit\n");
}

struct allocated_block * find_process(int pid) {
    struct allocated_block *ab = allocated_block_head;
    while (ab != NULL) {
        if (ab->pid == pid) {
            return ab;
        }
        ab = ab->next;
    }
    return NULL;
}

/*设置内存的大小*/
int set_mem_size() {
    int size;
    if (flag != 0) { //防止重复设置
        printf("Cannot set memory size again\n");
        return 0;
    }
    printf("Total memory size =");
    scanf("%d", & size);
    if (size > 0) {
        mem_size = size;
        free_block -> size = mem_size;
    }
    flag = 1;
    return 1;
}

/* 设置当前的分配算法 */
void set_algorithm() {
    int algorithm;
    printf("\t1 - First Fit\n");
    printf("\t2 - Best Fit \n");
    printf("\t3 - Worst Fit \n");
    scanf("%d", & algorithm);
    if (algorithm >= 1 && algorithm <= 3)
        ma_algorithm = algorithm;
    rearrange(ma_algorithm);
}

void rearrange(int algorithm) {
    switch (algorithm) {
        case MA_FF:
            rearrange_FF();
            break;
        case MA_BF:
            rearrange_BF();
            break;
        case MA_WF:
            rearrange_WF();
            break;
    }
}

/*按 FF 算法重新整理内存空闲块链表*/
void rearrange_FF() {
    struct free_block_type *i, *j;
    int temp_size, temp_addr;
    for (i = free_block; i != NULL; i = i->next) {
        for (j = i->next; j != NULL; j = j->next) {
            if (i->start_addr > j->start_addr) { // 地址从小到大排列
                // 交换节点数据
                temp_size = i->size;
                temp_addr = i->start_addr;
                i->size = j->size;
                i->start_addr = j->start_addr;
                j->size = temp_size;
                j->start_addr = temp_addr;
            }
        }
    }
}

/*按 BF 算法重新整理内存空闲块链表*/
void rearrange_BF() {
    struct free_block_type *i, *j;
    int temp_size, temp_addr;
    for (i = free_block; i != NULL; i = i->next) {
        for (j = i->next; j != NULL; j = j->next) {
            if (i->size > j->size) { // 小块优先
                // 交换节点数据
                temp_size = i->size;
                temp_addr = i->start_addr;
                i->size = j->size;
                i->start_addr = j->start_addr;
                j->size = temp_size;
                j->start_addr = temp_addr;
            }
        }
    }
}

/*按 WF 算法重新整理内存空闲块链表*/
void rearrange_WF() {
    struct free_block_type *i, *j;
    int temp_size, temp_addr;
    for (i = free_block; i != NULL; i = i->next) {
        for (j = i->next; j != NULL; j = j->next) {
            if (i->size < j->size) { // 大块优先
                // 交换节点数据
                temp_size = i->size;
                temp_addr = i->start_addr;
                i->size = j->size;
                i->start_addr = j->start_addr;
                j->size = temp_size;
                j->start_addr = temp_addr;
            }
        }
    }
}

/*创建新的进程，主要是获取内存的申请数量*/
int new_process() {
    struct allocated_block * ab;
    int size;
    int ret;
    ab = (struct allocated_block * ) malloc(sizeof(struct allocated_block));
    if (!ab) exit(-5);
    ab -> next = NULL;
    pid++;
    sprintf(ab -> process_name, "PROCESS-%02d", pid);
    ab -> pid = pid;
    printf("Memory for %s:", ab -> process_name);
    scanf("%d", & size);
    if (size > 0) ab -> size = size;
    ret = allocate_mem(ab); /* 从空闲区分配内存，ret==1 表示分配 ok*/
    /*如果此时 allocated_block_head 尚未赋值，则赋值*/
    if ((ret == 1) && (allocated_block_head == NULL)) {
        allocated_block_head = ab;
        return 1;
    }
    /*分配成功，将该已分配块的描述插入已分配链表*/
    else if (ret == 1) {
        ab -> next = allocated_block_head;
        allocated_block_head = ab;
        return 2;
    } else if (ret == -1) {
        /*分配不成功*/
        printf("Allocation fail\n");
        free(ab);
        return -1;
    }
    return 3;
}

/*分配内存模块*/
int allocate_mem(struct allocated_block * ab) {
    struct free_block_type *fbt, *pre;
    int request_size = ab -> size;
    fbt = free_block; pre = NULL;

    // 尝试寻找可满足空闲分区
    while (fbt != NULL) {
        if (fbt->size >= request_size) {
            // 找到可满足空闲分区且但分配后剩余空间比较小，则一起分配
            if (fbt->size - request_size < MIN_SLICE) {
                ab->start_addr = fbt->start_addr;
                ab->size = fbt->size;
                if (pre == NULL) free_block = fbt->next;
                else pre->next = fbt->next;
                free(fbt);
            } 
            // 找到可满足空闲分区且分配后剩余空间足够大，则分割
            else {
                ab->start_addr = fbt->start_addr;
                fbt->start_addr += request_size;
                fbt->size -= request_size;
            }
            rearrange(ma_algorithm); // 重新排序
            return 1; // 分配成功
            
        }
        pre = fbt;
        fbt = fbt->next;
    }
    // 使用内存紧缩技术进行空闲分区的合并，然后再分配
    memory_compaction();
    fbt = free_block;
    // 内存紧缩后，仍然空间不够，分配失败
    if (fbt->size < request_size) 
        return -1;
    // 分配后剩余空间比较小，则一起分配
    if (fbt->size - request_size < MIN_SLICE) {
        ab->start_addr = fbt->start_addr;
        ab->size = fbt->size;
        if (pre == NULL) free_block = fbt->next;
        else pre->next = fbt->next;
        free(fbt);
    } 
    // 分配后剩余空间足够大，则分割
    else {
        ab->start_addr = fbt->start_addr;
        fbt->start_addr += request_size;
        fbt->size -= request_size;
    }
    rearrange(ma_algorithm); // 重新排序
    return 1; // 分配成功
}

void memory_compaction() {
    int new_start_addr = 0;
    struct allocated_block *now = allocated_block_head;
    while (now) {
        now->start_addr = new_start_addr;
        new_start_addr += now->size;
        now = now->next;
    }

    struct free_block_type *fb = free_block;
    while (fb) {
        struct free_block_type *tmp = fb;
        fb = fb->next;
        free(tmp);
    }

    free_block = (struct free_block_type *)malloc(sizeof(struct free_block_type));
    free_block->start_addr = new_start_addr;
    free_block->size = mem_size - new_start_addr; 
    free_block->next = NULL;
}

/*删除进程，归还分配的存储空间，并删除描述该进程内存分配的节点*/
void kill_process() {
    struct allocated_block * ab;
    int pid;
    printf("Kill Process, pid=");
    scanf("%d", & pid);
    ab = find_process(pid);
    if (ab != NULL) {
        free_mem(ab); /*释放 ab 所表示的分配区*/
        dispose(ab); /*释放 ab 数据结构节点*/
    }
}

/*将 ab 所表示的已分配区归还，并进行可能的合并*/
int free_mem(struct allocated_block * ab) {
    int algorithm = ma_algorithm;
    struct free_block_type * fbt, * pre, * work;
    fbt = (struct free_block_type * ) malloc(sizeof(struct free_block_type));
    if (!fbt) return -1;
    fbt->size = ab->size;
    fbt->start_addr = ab->start_addr;
    fbt->next = free_block;
    free_block = fbt;

    // 对空闲块按照地址排序
    memory_sort();

    // 合并相邻空闲区
    memory_merge();
    rearrange(ma_algorithm); // 按算法重新排序
    return 1;
}

void memory_sort() {
    struct free_block_type *i = free_block, *j = NULL;
    int temp_size, temp_addr;
    for (; i != NULL; i = i->next) {
        for (j = i->next; j != NULL; j = j->next) {
            if (i->start_addr > j->start_addr) {
                // 交换节点数据
                temp_size = i->size;
                temp_addr = i->start_addr;
                i->size = j->size;
                i->start_addr = j->start_addr;
                j->size = temp_size;
                j->start_addr = temp_addr;
            }
        }
    }
}

void memory_merge() {
    struct free_block_type *fbt = free_block;
    while (fbt && fbt->next) {
        if (fbt->start_addr + fbt->size == fbt->next->start_addr) {
            fbt->size += fbt->next->size;
            struct free_block_type *temp = fbt->next;
            fbt->next = temp->next;
            free(temp);
        } else {
            fbt = fbt->next;
        }
    }
}

/*释放 ab 数据结构节点*/
int dispose(struct allocated_block * free_ab) {
    struct allocated_block * pre, * ab;
    if (free_ab == allocated_block_head) {
        /*如果要释放第一个节点*/
        allocated_block_head = allocated_block_head -> next;
        free(free_ab);
        return 1;
    }
    pre = allocated_block_head;
    ab = allocated_block_head -> next;
    while (ab != free_ab) {
        pre = ab;
        ab = ab -> next;
    }
    pre -> next = ab -> next;
    free(ab);
    return 2;
}

/* 显示当前内存的使用情况，包括空闲区的情况和已经分配的情况 */
int display_mem_usage() {
    struct free_block_type * fbt = free_block;
    struct allocated_block * ab = allocated_block_head;
    if (fbt == NULL) return (-1);
    printf("----------------------------------------------------------\n");
    /* 显示空闲区 */
    printf("Free Memory:\n");
    printf("%20s %20s\n", " start_addr", " size");
    while (fbt != NULL) {
        printf("%20d %20d\n", fbt -> start_addr, fbt -> size);
        fbt = fbt -> next;
    }
    /* 显示已分配区 */
    printf("\nUsed Memory:\n");
    printf("%10s %20s %10s %10s\n", "PID", "ProcessName", "start_addr", " size");
    while (ab != NULL) {
        printf("%10d %20s %10d %10d\n", ab -> pid, ab -> process_name,
            ab -> start_addr, ab -> size);
        ab = ab -> next;
    }
    printf("----------------------------------------------------------\n");
    return 0;
}

void do_exit() {
    struct allocated_block *ab = allocated_block_head;
    while (ab) {
        allocated_block_head = ab->next;
        free(ab);
        ab = allocated_block_head;
    }
    struct free_block_type *fb = free_block;
    while (fb) {
        free_block = fb->next;
        free(fb);
        fb = free_block;
    }
}
```
## （3）充分模拟三种算法的实现过程，并通过对比，分析三种算法的优劣
### 模拟FF算法：
```shell
1 - Set memory size (default=1024)
2 - Select memory allocation algorithm
3 - New process
4 - Terminate a process
5 - Display memory usage
0 - Exit
3
Memory for PROCESS-01:100

1 - Set memory size (default=1024)
2 - Select memory allocation algorithm
3 - New process
4 - Terminate a process
5 - Display memory usage
0 - Exit
3
Memory for PROCESS-02:200

1 - Set memory size (default=1024)
2 - Select memory allocation algorithm
3 - New process
4 - Terminate a process
5 - Display memory usage
0 - Exit
3 400
Memory for PROCESS-03:
```
- 首先新建了三个进程，分别分配了100，200，400的空间。
```shell
1 - Set memory size (default=1024)
2 - Select memory allocation algorithm
3 - New process
4 - Terminate a process
5 - Display memory usage
0 - Exit
4
Kill Process, pid=2

1 - Set memory size (default=1024)
2 - Select memory allocation algorithm
3 - New process
4 - Terminate a process
5 - Display memory usage
0 - Exit
5
----------------------------------------------------------
Free Memory:
          start_addr                 size
                 100                  200
                 700                  324

Used Memory:
       PID          ProcessName start_addr       size
         3           PROCESS-03        300        400
         1           PROCESS-01          0        100
----------------------------------------------------------
```
- 终止第二个进程，释放200空间，空闲空间分成两段
```shell
1 - Set memory size (default=1024)
2 - Select memory allocation algorithm
3 - New process
4 - Terminate a process
5 - Display memory usage
0 - Exit
3
Memory for PROCESS-04:50

1 - Set memory size (default=1024)
2 - Select memory allocation algorithm
3 - New process
4 - Terminate a process
5 - Display memory usage
0 - Exit
5
----------------------------------------------------------
Free Memory:
          start_addr                 size
                 150                  150
                 700                  324

Used Memory:
       PID          ProcessName start_addr       size
         4           PROCESS-04        100         50
         3           PROCESS-03        300        400
         1           PROCESS-01          0        100
----------------------------------------------------------
```
- 分配50内存给新进程，注意到按地址序进行了分配。
```shell
----------------------------------------------------------
Free Memory:
          start_addr                 size
                 900                  124

Used Memory:
       PID          ProcessName start_addr       size
         6           PROCESS-06        550        350
         4           PROCESS-04          0         50
         3           PROCESS-03         50        400
         1           PROCESS-01        450        100
----------------------------------------------------------
```
- 分配350内存给新进程，运行了内存紧缩算法，注意到1，3，4的内存起始地址被重新排列。

### 模拟BF算法
```shell
----------------------------------------------------------
Free Memory:
          start_addr                 size
                 900                  124
                 400                  150

Used Memory:
       PID          ProcessName start_addr       size
         9           PROCESS-09        250        150
         7           PROCESS-07         50        200
         6           PROCESS-06        550        350
         4           PROCESS-04          0         50
----------------------------------------------------------
```
- 构造了如上状态，按照BF算法进行内存分配
```shell
1 - Set memory size (default=1024)
2 - Select memory allocation algorithm
3 - New process
4 - Terminate a process
5 - Display memory usage
0 - Exit
3
Memory for PROCESS-10:50

1 - Set memory size (default=1024)
2 - Select memory allocation algorithm
3 - New process
4 - Terminate a process
5 - Display memory usage
0 - Exit
5
----------------------------------------------------------
Free Memory:
          start_addr                 size
                 950                   74
                 400                  150

Used Memory:
       PID          ProcessName start_addr       size
        10           PROCESS-10        900         50
         9           PROCESS-09        250        150
         7           PROCESS-07         50        200
         6           PROCESS-06        550        350
         4           PROCESS-04          0         50
----------------------------------------------------------
```
- 注意到BF算法没有按照地址顺序，而是按照内存空间大小，小块优先进行分配。

### 模拟WF算法
```shell
----------------------------------------------------------
Free Memory:
          start_addr                 size
                 970                   54
                 200                   50
                 500                   50

Used Memory:
       PID          ProcessName start_addr       size
        14           PROCESS-14          0        200
        13           PROCESS-13        400        100
        11           PROCESS-11        950         20
        10           PROCESS-10        900         50
         9           PROCESS-09        250        150
         6           PROCESS-06        550        350
----------------------------------------------------------
```
- 分配内存空间如上，采用WF算法进行分配。
```shell
1 - Set memory size (default=1024)
2 - Select memory allocation algorithm
3 - New process
4 - Terminate a process
5 - Display memory usage
0 - Exit
3
Memory for PROCESS-15:20

1 - Set memory size (default=1024)
2 - Select memory allocation algorithm
3 - New process
4 - Terminate a process
5 - Display memory usage
0 - Exit
5
----------------------------------------------------------
Free Memory:
          start_addr                 size
                 200                   50
                 500                   50
                 990                   34

Used Memory:
       PID          ProcessName start_addr       size
        15           PROCESS-15        970         20
        14           PROCESS-14          0        200
        13           PROCESS-13        400        100
        11           PROCESS-11        950         20
        10           PROCESS-10        900         50
         9           PROCESS-09        250        150
         6           PROCESS-06        550        350
----------------------------------------------------------
```
- 分配20内存空间给新进程，注意到WF算法没有按照地址顺序，而是按照内存空间大小，大块优先进行分配。

### 三种算法的优劣：
- FF算法：
优势：按照内存地址排序，查找速度相对较快。
劣势：容易导致内存碎片，特别是外部碎片。
- BF算法：
优势：按照空闲内存大小排序，减少浪费。
劣势：查找速度较慢，且容易造成小碎片。
- WF算法：
优势：分配给空闲最大的空间，尽可能避免从小空闲的内存空间进行分配，可以减少小碎片。
劣势：查找速度较慢，且容易形成新的大碎片。

## 问题：
### （1）对涉及的 3 个算法进行比较，包括算法思想、算法的优缺点、在实现上如何提高算法的查找性能。
- FF（First Fit）策略： 
- 即首次适应算法。在采用空闲分区链作为数据结构时，该算法要求空闲分区链表以地址递增的次序链接。在进行内存分配时，从链首开始顺序查找，直至找到一个能满足进程大小要求的空闲分区为止。然后，再按照进程请求内存的大小，从该分区中划出一块内存空间分配给请求进程，余下的空闲分区仍留在空闲链中。
优势：按照内存地址排序，查找速度相对较快。
劣势：容易导致内存碎片，特别是外部碎片。
优化策略：可以采用索引等方法优化查找性能。
- BF（Best Fit）策略： 
- 即最佳适应算法。将空闲分区链表按分区大小由小到大排序，在链表中查找第一个满足要求的分区。
优势：按照空闲内存大小排序，减少浪费。
劣势：查找速度较慢，且容易造成小碎片。
优化策略：可以使用小根堆进行保存。
- WF（Worst Fit）策略： 
- 即最差匹配算法。将空闲分区链表按分区大小由大到小排序，在链表中找到第一个满足要求的空闲分区。
优势：分配给空闲最大的空间，尽可能避免从小空闲的内存空间进行分配，可以减少小碎片。
劣势：查找速度较慢，且容易形成新的大碎片。
优化策略：可以使用大根堆进行保存。
### （2）3 种算法的空闲块排序分别是如何实现的。
- FF（First Fit）策略： 
按照内存地址从小到大进行排序。具体有冒泡排序、归并排序等算法。
- BF（Best Fit）策略： 
- 即最佳适应算法。将空闲分区链表按分区大小由小到大排序，在链表中查找第一个满足要求的分区。
按照内存空间从小到大进行排序。具体有冒泡排序、归并排序等算法。
- WF（Worst Fit）策略：
按照内存地址从大到小进行排序。具体有冒泡排序、归并排序等算法。
### （3）结合实验，举例说明什么是内碎片、外碎片，紧缩功能解决的是什么碎片。
- 内碎片
是指分配给进程的内存块中未被实际使用的部分。这些未使用的空间属于已分配块的内部，因此无法被其他进程利用。
举例：在分配空间时，若剩余空间小于最小碎片大小，则将整片空间分配。这时便产生了没有被实际使用的空间，这就是内碎片。
- 外碎片
是指未被分配的空闲内存区块，散布在已分配的内存块之间，虽然这些空闲块总量足够大，但因不连续而无法满足较大内存请求。
举例：如分配200，50，100，50，200的空间，释放两个50的空间，这时两个50的空间就是两个外碎片。
假设这时空闲的只有这两个50空间，若不进行内存紧缩，则即使现在一共剩余100内存，也无法分配75内存。

- 紧缩功能：解决的是外碎片。
### （4）在回收内存时，空闲块合并是如何实现的？
将空闲块按照地址进行排序，依次检查相邻空闲块，若地址连续则把两个空闲块进行合并，合并为一个空闲块。

# 2.4 页面的置换
## （1）理解页面置换算法 FIFO、LRU 的思想及实现的思路。
- FIFO算法：
在分配内存页面数（AP）小于进程页面数（PP）时，当然是最先运行的 AP 个页面放入内存；
这时又需要处理新的页面，则将原来放的内存中的 AP 个页中最先进入的调出（FIFO），再将新页面放入；
总是淘汰在内存中停留时间最长的一页，即先进入内存的一页，先被替换出。
以后如果再有新页面需要调入，则都按上述规则进行。

实现思路：所使用的内存页面构成一个队列，先进先出。

- LRU算法：
当内存分配页面数（AP）小于进程页面数（PP）时，把最先执行的 AP 个页面放入内存。
当需调页面进入内存，而当前分配的内存页面全部不空闲时，选择将其中最长时间没有用到的那一页调出，以空出内存来放置新调入的页面（LRU）。

实现思路：有多种方法，这里我为了方便同样使用的是队列进行实现，每次CPU用到页面，则将其放到队列末尾。需要覆盖时仍然是从队首出队。


## （2）参考给出的代码思路，定义相应的数据结构，在一个程序中实现上述 2 种算法，运行时可以选择算法。算法的页面引用序列要至少能够支持随机数自动生成、手动输入两种生成方式；算法要输出页面置换的过程和最终的缺页率。
### 帮助结构体和函数定义：[queue.h]
```c
#ifndef QUEUE_H
#define QUEUE_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

typedef struct Node {
    int id;
    int data;
    struct Node *next;
} Node;

typedef struct Queue {
    Node *head;
    Node *tail;
    int length;
    int maxLength;
} Queue;

Queue* newQueue(int maxLength);
void pushQueue(Queue *queue, int value);
Node* popQueue(Queue *queue);
void pushQueueWithId(Queue *queue, int data, int id);
bool inQueue(Queue *queue, int value);
bool switchQueue(Queue *queue, int value);
void printQueue(Queue *queue);
void freeQueue(Queue *queue);

Queue *newQueue(int mLength){
    if (mLength <= 0)
        return NULL;
    Queue *q = malloc(sizeof(Queue));
    q->head = malloc(sizeof(Node));
    q->head->id = 0;
    q->head->data = 0;
    q->head->next = NULL;
    q->tail = q->head;
    q->length = 0;
    q->maxLength = mLength;
    return q;
}
Queue *copyQueue(Queue *q){
    if (q == NULL) {
        return NULL;
    }
    Queue * nq = newQueue(q->maxLength);
    for (Node *node = q->head->next;node != NULL;node = node->next) {
        pushQueueWithId(nq, node->data, node->id);
    }
    return nq;
}
bool isQueueEmpty(Queue *q) {
    if (q == NULL || q->head == q->tail) 
        return true;
    return false;
}
void pushQueue(Queue *q, int data) {
    Node *node = malloc(sizeof(Node));
    node->data = data;
    node->next = NULL;
    if (q->length == q->maxLength) {
        Node *nodeToReplace = popQueue(q);
        node->id = nodeToReplace->id;
        printf("In Frame %d : %d -> %d ; ", node->id, nodeToReplace->data, data);
        free(nodeToReplace);
    } else {
        node->id = q->length + 1;
        printf("In Frame %d : X -> %d ; ", node->id, data);
    }
    q->tail->next = node;
    q->tail = node;
    q->length++;
}
void pushQueueWithId(Queue *q, int data, int id) {
    Node *node = malloc(sizeof(Node));
    node->data = data;
    node->next = NULL;
    node->id = id;
    q->tail->next = node;
    q->tail = node;
    q->length++;
}
Node *popQueue(Queue *q) {
    if (isQueueEmpty(q)) 
        return NULL;
    Node *node = q->head->next;
    q->head->next = node->next;
    if (q->tail == node) 
        q->tail = q->head; 
    node->next = NULL;
    q->length--;
    
    return node;
}
void freeQueue(Queue *q) {
    if (q == NULL)
        return;
    while (!isQueueEmpty(q)) {
        Node *node = popQueue(q);
        free(node);
    }
    free(q->head);
    free(q);
}
bool inQueue(Queue *q, int data) {
    if (isQueueEmpty(q)) 
        return false;
    for (Node *node = q->head->next; node != NULL ; node = node->next) {
        if (node->data == data)
            return true;
    }
    return false;
}
bool switchQueue(Queue *q, int data) {
    if (isQueueEmpty(q)) 
        return false;
    Node *previous = q->head;
    for (Node *node = q->head->next; node != NULL ; node = node->next) {
        if (node->data == data) {
            previous->next = node->next;
            if (q->tail == node) 
                q->tail = previous; 
            node->next = NULL;
            q->tail->next = node;
            q->tail = node;
            return true;
        }
        previous = node;
    }
    return false;
}
Queue* sortQueue(Queue *q) {
    if (q == NULL || q->length < 2) {
        return copyQueue(q);  
    }
    Queue *nq = copyQueue(q);
    Node *sorted = NULL;
    while (!isQueueEmpty(nq)) {
        Node *current = popQueue(nq);

        // 插入 current 节点到新链表 sorted 中
        if (sorted == NULL || sorted->id >= current->id) {
            // 插入到新链表的头部
            current->next = sorted;
            sorted = current;
        } else {
            // 查找插入位置
            Node *temp = sorted;
            while (temp->next != NULL && temp->next->id < current->id) {
                temp = temp->next;
            }
            // 插入到找到的位置
            current->next = temp->next;
            temp->next = current;
        }
    }

    nq->head->next = sorted;
    nq->tail = nq->head;
    nq->length = 0;

    Node *temp = nq->head->next;
    while (temp != NULL) {
        nq->tail->next = temp;
        nq->tail = temp;
        nq->length++;
        temp = temp->next;
    }
    nq->tail->next = NULL;  // 确保尾部为空
    return nq;
}
void printQueue(Queue *q) {
    Queue *nq = sortQueue(q);
    if (nq == NULL) 
        return;
    for (Node *node = nq->head->next;node != NULL;node = node->next) {
        printf("%d ", node->data);
    }
    freeQueue(nq);
}

#endif
```
- 这个文件定义了队列结构体，同时实现了入队、出队，检查队列是否为空等多个函数。
### 程序代码：[2-4.c]
```c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include "queue.h"

#define MAX_PAGES 100    // 最大页面引用序列长度
#define MAX_FRAMES 10    // 最大内存帧数

void simulateFIFO(int pages[], int pageCount, int frameCount);
void simulateLRU(int pages[], int pageCount, int frameCount);
void generateRandomPages(int pages[], int pageCount);

int main() {
    int choice, frameCount, pageCount, pages[MAX_PAGES];

    // 输入页面数量和内存帧数
    printf("Enter the number of pages (<= %d): ", MAX_PAGES);
    scanf("%d", &pageCount);
    printf("Enter the number of frames (<= %d): ", MAX_FRAMES);
    scanf("%d", &frameCount);

    // 页面序列生成方式
    printf("Choose page reference sequence generation method:\n");
    printf("1 - Randomly generate page sequence\n");
    printf("2 - Manually enter page sequence\n");
    scanf("%d", &choice);
    
    if (choice == 1) {
        generateRandomPages(pages, pageCount);
    } else if (choice == 2) {
        printf("Enter the page sequence:\n");
        for (int i = 0; i < pageCount; i++) {
            scanf("%d", &pages[i]);
        }
    } else {
        printf("Invalid choice!\n");
        return 1;
    }

    // 选择页面置换算法
    printf("Choose page replacement algorithm:\n");
    printf("1 - FIFO\n");
    printf("2 - LRU\n");
    scanf("%d", &choice);

    if (choice == 1) {
        simulateFIFO(pages, pageCount, frameCount);
    } else if (choice == 2) {
        simulateLRU(pages, pageCount, frameCount);
    } else {
        printf("Invalid choice!\n");
    }

    return 0;
}

// 生成随机页面引用序列
void generateRandomPages(int pages[], int pageCount) {
    srand(time(NULL));
    printf("Randomly generated page sequence: ");
    for (int i = 0; i < pageCount; i++) {
        pages[i] = rand() % 10;  // 假设页面编号在 0-9 范围
        printf("%d ", pages[i]);
    }
    printf("\n");
}

// FIFO 页面置换算法
void simulateFIFO(int pages[], int pageCount, int frameCount) {
    Queue *frames = newQueue(frameCount);
    int faults = 0;
    printf("\nFIFO Page Replacement:\n");
    for (int i = 0; i < pageCount; i++) {
        int page = pages[i];
        bool found = inQueue(frames, page);  

        // 页面不在内存，产生缺页中断
        if (!found) {
            faults++;
            printf("Page %d -> ", page);
            pushQueue(frames, page);
            
            printQueue(frames);
            for (int j = frames->length;j < frameCount;j++) {
                printf("X ");
            }
            printf("(Page Fault)\n");
        } else {
            printf("Page %d -> No page fault\n", page);
        }
    }
    printf("\nTotal Page Faults: %d\n", faults);
    printf("Page Fault Rate: %.2f%%\n", (faults * 100.0) / pageCount);
    freeQueue(frames);
}

// LRU 页面置换算法
void simulateLRU(int pages[], int pageCount, int frameCount) {
    Queue *frames = newQueue(frameCount);
    int faults = 0;

    printf("\nLRU Page Replacement:\n");
    for (int i = 0; i < pageCount; i++) {
        int page = pages[i];
        bool found = switchQueue(frames, page);
              
        // 页面不在内存，产生缺页中断
        if (!found) {
            faults++;
            printf("Page %d -> ", page);
            pushQueue(frames, page);
            printQueue(frames);
            for (int j = frames->length;j < frameCount;j++) {
                printf("X ");
            }
            printf("(Page Fault)\n");
        } else {
            printf("Page %d -> No page fault\n", page);
        }
    }
    printf("\nTotal Page Faults: %d\n", faults);
    printf("Page Fault Rate: %.2f%%\n", (faults * 100.0) / pageCount);
    freeQueue(frames);
}
```
## （3）运行所实现的算法，并通过对比，分析 2 种算法的优劣。
### FIFO算法：
```shell
[yuyueryuyu@Yu lab2]$ ./2-4
Enter the number of pages (<= 100): 100
Enter the number of frames (<= 10): 4
Choose page reference sequence generation method:
1 - Randomly generate page sequence
2 - Manually enter page sequence
1
Randomly generated page sequence: 0 1 6 1 7 0 6 5 2 6 8 8 0 6 6 9 0 1 0 3 1 4 6 1 6 5 8 2 8 0 5 0 3 1 2 0 4 0 8 8 8 6 7 8 5 3 9 7 6 0 0 9 6 8 3 4 3 3 6 4 3 3 4 6 4 8 9 0 0 9 1 8 5 8 7 0 3 6 7 2 8 9 1 6 7 4 0 1 7 8 7 2 1 3 9 8 4 0 8 4
Choose page replacement algorithm:
1 - FIFO
2 - LRU
1

FIFO Page Replacement:
Page 0 -> In Frame 1 : X -> 0 ; 0 X X X (Page Fault)
Page 1 -> In Frame 2 : X -> 1 ; 0 1 X X (Page Fault)
Page 6 -> In Frame 3 : X -> 6 ; 0 1 6 X (Page Fault)
Page 1 -> No page fault
Page 7 -> In Frame 4 : X -> 7 ; 0 1 6 7 (Page Fault)
Page 0 -> No page fault
Page 6 -> No page fault
Page 5 -> In Frame 1 : 0 -> 5 ; 5 1 6 7 (Page Fault)
Page 2 -> In Frame 2 : 1 -> 2 ; 5 2 6 7 (Page Fault)
Page 6 -> No page fault
Page 8 -> In Frame 3 : 6 -> 8 ; 5 2 8 7 (Page Fault)
Page 8 -> No page fault
Page 0 -> In Frame 4 : 7 -> 0 ; 5 2 8 0 (Page Fault)
Page 6 -> In Frame 1 : 5 -> 6 ; 6 2 8 0 (Page Fault)
Page 6 -> No page fault
Page 9 -> In Frame 2 : 2 -> 9 ; 6 9 8 0 (Page Fault)
Page 0 -> No page fault
Page 1 -> In Frame 3 : 8 -> 1 ; 6 9 1 0 (Page Fault)
Page 0 -> No page fault
Page 3 -> In Frame 4 : 0 -> 3 ; 6 9 1 3 (Page Fault)
Page 1 -> No page fault
Page 4 -> In Frame 1 : 6 -> 4 ; 4 9 1 3 (Page Fault)
Page 6 -> In Frame 2 : 9 -> 6 ; 4 6 1 3 (Page Fault)
Page 1 -> No page fault
Page 6 -> No page fault
Page 5 -> In Frame 3 : 1 -> 5 ; 4 6 5 3 (Page Fault)
Page 8 -> In Frame 4 : 3 -> 8 ; 4 6 5 8 (Page Fault)
Page 2 -> In Frame 1 : 4 -> 2 ; 2 6 5 8 (Page Fault)
Page 8 -> No page fault
Page 0 -> In Frame 2 : 6 -> 0 ; 2 0 5 8 (Page Fault)
Page 5 -> No page fault
Page 0 -> No page fault
Page 3 -> In Frame 3 : 5 -> 3 ; 2 0 3 8 (Page Fault)
Page 1 -> In Frame 4 : 8 -> 1 ; 2 0 3 1 (Page Fault)
Page 2 -> No page fault
Page 0 -> No page fault
Page 4 -> In Frame 1 : 2 -> 4 ; 4 0 3 1 (Page Fault)
Page 0 -> No page fault
Page 8 -> In Frame 2 : 0 -> 8 ; 4 8 3 1 (Page Fault)
Page 8 -> No page fault
Page 8 -> No page fault
Page 6 -> In Frame 3 : 3 -> 6 ; 4 8 6 1 (Page Fault)
Page 7 -> In Frame 4 : 1 -> 7 ; 4 8 6 7 (Page Fault)
Page 8 -> No page fault
Page 5 -> In Frame 1 : 4 -> 5 ; 5 8 6 7 (Page Fault)
Page 3 -> In Frame 2 : 8 -> 3 ; 5 3 6 7 (Page Fault)
Page 9 -> In Frame 3 : 6 -> 9 ; 5 3 9 7 (Page Fault)
Page 7 -> No page fault
Page 6 -> In Frame 4 : 7 -> 6 ; 5 3 9 6 (Page Fault)
Page 0 -> In Frame 1 : 5 -> 0 ; 0 3 9 6 (Page Fault)
Page 0 -> No page fault
Page 9 -> No page fault
Page 6 -> No page fault
Page 8 -> In Frame 2 : 3 -> 8 ; 0 8 9 6 (Page Fault)
Page 3 -> In Frame 3 : 9 -> 3 ; 0 8 3 6 (Page Fault)
Page 4 -> In Frame 4 : 6 -> 4 ; 0 8 3 4 (Page Fault)
Page 3 -> No page fault
Page 3 -> No page fault
Page 6 -> In Frame 1 : 0 -> 6 ; 6 8 3 4 (Page Fault)
Page 4 -> No page fault
Page 3 -> No page fault
Page 3 -> No page fault
Page 4 -> No page fault
Page 6 -> No page fault
Page 4 -> No page fault
Page 8 -> No page fault
Page 9 -> In Frame 2 : 8 -> 9 ; 6 9 3 4 (Page Fault)
Page 0 -> In Frame 3 : 3 -> 0 ; 6 9 0 4 (Page Fault)
Page 0 -> No page fault
Page 9 -> No page fault
Page 1 -> In Frame 4 : 4 -> 1 ; 6 9 0 1 (Page Fault)
Page 8 -> In Frame 1 : 6 -> 8 ; 8 9 0 1 (Page Fault)
Page 5 -> In Frame 2 : 9 -> 5 ; 8 5 0 1 (Page Fault)
Page 8 -> No page fault
Page 7 -> In Frame 3 : 0 -> 7 ; 8 5 7 1 (Page Fault)
Page 0 -> In Frame 4 : 1 -> 0 ; 8 5 7 0 (Page Fault)
Page 3 -> In Frame 1 : 8 -> 3 ; 3 5 7 0 (Page Fault)
Page 6 -> In Frame 2 : 5 -> 6 ; 3 6 7 0 (Page Fault)
Page 7 -> No page fault
Page 2 -> In Frame 3 : 7 -> 2 ; 3 6 2 0 (Page Fault)
Page 8 -> In Frame 4 : 0 -> 8 ; 3 6 2 8 (Page Fault)
Page 9 -> In Frame 1 : 3 -> 9 ; 9 6 2 8 (Page Fault)
Page 1 -> In Frame 2 : 6 -> 1 ; 9 1 2 8 (Page Fault)
Page 6 -> In Frame 3 : 2 -> 6 ; 9 1 6 8 (Page Fault)
Page 7 -> In Frame 4 : 8 -> 7 ; 9 1 6 7 (Page Fault)
Page 4 -> In Frame 1 : 9 -> 4 ; 4 1 6 7 (Page Fault)
Page 0 -> In Frame 2 : 1 -> 0 ; 4 0 6 7 (Page Fault)
Page 1 -> In Frame 3 : 6 -> 1 ; 4 0 1 7 (Page Fault)
Page 7 -> No page fault
Page 8 -> In Frame 4 : 7 -> 8 ; 4 0 1 8 (Page Fault)
Page 7 -> In Frame 1 : 4 -> 7 ; 7 0 1 8 (Page Fault)
Page 2 -> In Frame 2 : 0 -> 2 ; 7 2 1 8 (Page Fault)
Page 1 -> No page fault
Page 3 -> In Frame 3 : 1 -> 3 ; 7 2 3 8 (Page Fault)
Page 9 -> In Frame 4 : 8 -> 9 ; 7 2 3 9 (Page Fault)
Page 8 -> In Frame 1 : 7 -> 8 ; 8 2 3 9 (Page Fault)
Page 4 -> In Frame 2 : 2 -> 4 ; 8 4 3 9 (Page Fault)
Page 0 -> In Frame 3 : 3 -> 0 ; 8 4 0 9 (Page Fault)
Page 8 -> No page fault
Page 4 -> No page fault

Total Page Faults: 59
Page Fault Rate: 59.00%
```
### LRU算法：
```shell
[yuyueryuyu@Yu lab2]$ ./2-4
Enter the number of pages (<= 100): 100
Enter the number of frames (<= 10): 4
Choose page reference sequence generation method:
1 - Randomly generate page sequence
2 - Manually enter page sequence
2
Enter the page sequence:
0 1 6 1 7 0 6 5 2 6 8 8 0 6 6 9 0 1 0 3 1 4 6 1 6 5 8 2 8 0 5 0 3 1 2 0 4 0 8 8 8 6 7 8 5 3 9 7 6 0 0 9 6 8 3 4 3 3 6 4 3 3 4 6 4 8 9 0 0 9 1 8 5 8 7 0 3 6 7 2 8 9 1 6 7 4 0 1 7 8 7 2 1 3 9 8 4 0 8 4
Choose page replacement algorithm:
1 - FIFO
2 - LRU
2

LRU Page Replacement:
Page 0 -> In Frame 1 : X -> 0 ; 0 X X X (Page Fault)
Page 1 -> In Frame 2 : X -> 1 ; 0 1 X X (Page Fault)
Page 6 -> In Frame 3 : X -> 6 ; 0 1 6 X (Page Fault)
Page 1 -> No page fault
Page 7 -> In Frame 4 : X -> 7 ; 0 1 6 7 (Page Fault)
Page 0 -> No page fault
Page 6 -> No page fault
Page 5 -> In Frame 2 : 1 -> 5 ; 0 5 6 7 (Page Fault)
Page 2 -> In Frame 4 : 7 -> 2 ; 0 5 6 2 (Page Fault)
Page 6 -> No page fault
Page 8 -> In Frame 1 : 0 -> 8 ; 8 5 6 2 (Page Fault)
Page 8 -> No page fault
Page 0 -> In Frame 2 : 5 -> 0 ; 8 0 6 2 (Page Fault)
Page 6 -> No page fault
Page 6 -> No page fault
Page 9 -> In Frame 4 : 2 -> 9 ; 8 0 6 9 (Page Fault)
Page 0 -> No page fault
Page 1 -> In Frame 1 : 8 -> 1 ; 1 0 6 9 (Page Fault)
Page 0 -> No page fault
Page 3 -> In Frame 3 : 6 -> 3 ; 1 0 3 9 (Page Fault)
Page 1 -> No page fault
Page 4 -> In Frame 4 : 9 -> 4 ; 1 0 3 4 (Page Fault)
Page 6 -> In Frame 2 : 0 -> 6 ; 1 6 3 4 (Page Fault)
Page 1 -> No page fault
Page 6 -> No page fault
Page 5 -> In Frame 3 : 3 -> 5 ; 1 6 5 4 (Page Fault)
Page 8 -> In Frame 4 : 4 -> 8 ; 1 6 5 8 (Page Fault)
Page 2 -> In Frame 1 : 1 -> 2 ; 2 6 5 8 (Page Fault)
Page 8 -> No page fault
Page 0 -> In Frame 2 : 6 -> 0 ; 2 0 5 8 (Page Fault)
Page 5 -> No page fault
Page 0 -> No page fault
Page 3 -> In Frame 1 : 2 -> 3 ; 3 0 5 8 (Page Fault)
Page 1 -> In Frame 4 : 8 -> 1 ; 3 0 5 1 (Page Fault)
Page 2 -> In Frame 3 : 5 -> 2 ; 3 0 2 1 (Page Fault)
Page 0 -> No page fault
Page 4 -> In Frame 1 : 3 -> 4 ; 4 0 2 1 (Page Fault)
Page 0 -> No page fault
Page 8 -> In Frame 4 : 1 -> 8 ; 4 0 2 8 (Page Fault)
Page 8 -> No page fault
Page 8 -> No page fault
Page 6 -> In Frame 3 : 2 -> 6 ; 4 0 6 8 (Page Fault)
Page 7 -> In Frame 1 : 4 -> 7 ; 7 0 6 8 (Page Fault)
Page 8 -> No page fault
Page 5 -> In Frame 2 : 0 -> 5 ; 7 5 6 8 (Page Fault)
Page 3 -> In Frame 3 : 6 -> 3 ; 7 5 3 8 (Page Fault)
Page 9 -> In Frame 1 : 7 -> 9 ; 9 5 3 8 (Page Fault)
Page 7 -> In Frame 4 : 8 -> 7 ; 9 5 3 7 (Page Fault)
Page 6 -> In Frame 2 : 5 -> 6 ; 9 6 3 7 (Page Fault)
Page 0 -> In Frame 3 : 3 -> 0 ; 9 6 0 7 (Page Fault)
Page 0 -> No page fault
Page 9 -> No page fault
Page 6 -> No page fault
Page 8 -> In Frame 4 : 7 -> 8 ; 9 6 0 8 (Page Fault)
Page 3 -> In Frame 3 : 0 -> 3 ; 9 6 3 8 (Page Fault)
Page 4 -> In Frame 1 : 9 -> 4 ; 4 6 3 8 (Page Fault)
Page 3 -> No page fault
Page 3 -> No page fault
Page 6 -> No page fault
Page 4 -> No page fault
Page 3 -> No page fault
Page 3 -> No page fault
Page 4 -> No page fault
Page 6 -> No page fault
Page 4 -> No page fault
Page 8 -> No page fault
Page 9 -> In Frame 3 : 3 -> 9 ; 4 6 9 8 (Page Fault)
Page 0 -> In Frame 2 : 6 -> 0 ; 4 0 9 8 (Page Fault)
Page 0 -> No page fault
Page 9 -> No page fault
Page 1 -> In Frame 1 : 4 -> 1 ; 1 0 9 8 (Page Fault)
Page 8 -> No page fault
Page 5 -> In Frame 2 : 0 -> 5 ; 1 5 9 8 (Page Fault)
Page 8 -> No page fault
Page 7 -> In Frame 3 : 9 -> 7 ; 1 5 7 8 (Page Fault)
Page 0 -> In Frame 1 : 1 -> 0 ; 0 5 7 8 (Page Fault)
Page 3 -> In Frame 2 : 5 -> 3 ; 0 3 7 8 (Page Fault)
Page 6 -> In Frame 4 : 8 -> 6 ; 0 3 7 6 (Page Fault)
Page 7 -> No page fault
Page 2 -> In Frame 1 : 0 -> 2 ; 2 3 7 6 (Page Fault)
Page 8 -> In Frame 2 : 3 -> 8 ; 2 8 7 6 (Page Fault)
Page 9 -> In Frame 4 : 6 -> 9 ; 2 8 7 9 (Page Fault)
Page 1 -> In Frame 3 : 7 -> 1 ; 2 8 1 9 (Page Fault)
Page 6 -> In Frame 1 : 2 -> 6 ; 6 8 1 9 (Page Fault)
Page 7 -> In Frame 2 : 8 -> 7 ; 6 7 1 9 (Page Fault)
Page 4 -> In Frame 4 : 9 -> 4 ; 6 7 1 4 (Page Fault)
Page 0 -> In Frame 3 : 1 -> 0 ; 6 7 0 4 (Page Fault)
Page 1 -> In Frame 1 : 6 -> 1 ; 1 7 0 4 (Page Fault)
Page 7 -> No page fault
Page 8 -> In Frame 4 : 4 -> 8 ; 1 7 0 8 (Page Fault)
Page 7 -> No page fault
Page 2 -> In Frame 3 : 0 -> 2 ; 1 7 2 8 (Page Fault)
Page 1 -> No page fault
Page 3 -> In Frame 4 : 8 -> 3 ; 1 7 2 3 (Page Fault)
Page 9 -> In Frame 2 : 7 -> 9 ; 1 9 2 3 (Page Fault)
Page 8 -> In Frame 3 : 2 -> 8 ; 1 9 8 3 (Page Fault)
Page 4 -> In Frame 1 : 1 -> 4 ; 4 9 8 3 (Page Fault)
Page 0 -> In Frame 4 : 3 -> 0 ; 4 9 8 0 (Page Fault)
Page 8 -> No page fault
Page 4 -> No page fault

Total Page Faults: 57
Page Fault Rate: 57.00%
```
### 对比：
可以注意到，LRU算法发生页错误的概率比FIFO低。
### 分析：
- FIFO算法：
FIFO 算法按照页面进入内存的顺序来进行替换，即最早进入内存的页面会被优先置换掉。
优点：实现简单；只需维护一个队列，开销小。
缺陷：不考虑页面的使用频率，没有考虑局部性，容易造成缺页；会发生BLEADY现象，详见第四部分。

- LRU算法：
LRU 算法替换最久未使用的页面，即最近最少使用的页面优先被置换。
优点：考虑局部性，性能更好，更不容易出现页错误。
缺陷：实现较为复杂；对于长时期未使用的数据容易造成缺页。
## （4）设计测试数据，观察 FIFO 算法的 BLEADY 现象；设计具有局部性特点的测试数据，分别运行实现的 2 种算法，比较缺页率，并进行分析。
### BLEADY 现象：
测试数据：1 2 3 4 1 2 5 1 2 3 4 5
```shell
[yuyueryuyu@Yu lab2]$ ./2-4
Enter the number of pages (<= 100): 12
Enter the number of frames (<= 10): 3
Choose page reference sequence generation method:
1 - Randomly generate page sequence
2 - Manually enter page sequence
2
Enter the page sequence:
1 2 3 4 1 2 5 1 2 3 4 5
Choose page replacement algorithm:
1 - FIFO
2 - LRU
1

FIFO Page Replacement:
Page 1 -> In Frame 1 : X -> 1 ; 1 X X (Page Fault)
Page 2 -> In Frame 2 : X -> 2 ; 1 2 X (Page Fault)
Page 3 -> In Frame 3 : X -> 3 ; 1 2 3 (Page Fault)
Page 4 -> In Frame 1 : 1 -> 4 ; 4 2 3 (Page Fault)
Page 1 -> In Frame 2 : 2 -> 1 ; 4 1 3 (Page Fault)
Page 2 -> In Frame 3 : 3 -> 2 ; 4 1 2 (Page Fault)
Page 5 -> In Frame 1 : 4 -> 5 ; 5 1 2 (Page Fault)
Page 1 -> No page fault
Page 2 -> No page fault
Page 3 -> In Frame 2 : 1 -> 3 ; 5 3 2 (Page Fault)
Page 4 -> In Frame 3 : 2 -> 4 ; 5 3 4 (Page Fault)
Page 5 -> No page fault

Total Page Faults: 9
Page Fault Rate: 75.00%
[yuyueryuyu@Yu lab2]$ ./2-4
Enter the number of pages (<= 100): 12
Enter the number of frames (<= 10): 4
Choose page reference sequence generation method:
1 - Randomly generate page sequence
2 - Manually enter page sequence
2
Enter the page sequence:
1 2 3 4 1 2 5 1 2 3 4 5
Choose page replacement algorithm:
1 - FIFO
2 - LRU
1

FIFO Page Replacement:
Page 1 -> In Frame 1 : X -> 1 ; 1 X X X (Page Fault)
Page 2 -> In Frame 2 : X -> 2 ; 1 2 X X (Page Fault)
Page 3 -> In Frame 3 : X -> 3 ; 1 2 3 X (Page Fault)
Page 4 -> In Frame 4 : X -> 4 ; 1 2 3 4 (Page Fault)
Page 1 -> No page fault
Page 2 -> No page fault
Page 5 -> In Frame 1 : 1 -> 5 ; 5 2 3 4 (Page Fault)
Page 1 -> In Frame 2 : 2 -> 1 ; 5 1 3 4 (Page Fault)
Page 2 -> In Frame 3 : 3 -> 2 ; 5 1 2 4 (Page Fault)
Page 3 -> In Frame 4 : 4 -> 3 ; 5 1 2 3 (Page Fault)
Page 4 -> In Frame 1 : 5 -> 4 ; 4 1 2 3 (Page Fault)
Page 5 -> In Frame 2 : 1 -> 5 ; 4 5 2 3 (Page Fault)

Total Page Faults: 10
Page Fault Rate: 83.33%
```
可以看到，对于同样的数据，可供分配的内存帧数增加，页错误发生率却增高了，出现了BLEADY现象。

### 局部性数据：
测试数据：
1 2 3 1 2   //局部区域1构建
4           //局部区域外
1 5 2 1 5   //局部区域1测试+局部区域2构建
4           //局部区域外    
1           //局部区域2测试
3           //局部区域外
5           //局部区域2测试

测试：
#### FIFO：
```shell
[yuyueryuyu@Yu lab2]$ ./2-4
Enter the number of pages (<= 100): 15
Enter the number of frames (<= 10): 3
Choose page reference sequence generation method:
1 - Randomly generate page sequence
2 - Manually enter page sequence
2
Enter the page sequence:
1 2 3 1 2 4 1 5 2 1 5 4 1 3 5
Choose page replacement algorithm:
1 - FIFO
2 - LRU
1

FIFO Page Replacement:
Page 1 -> In Frame 1 : X -> 1 ; 1 X X (Page Fault)
Page 2 -> In Frame 2 : X -> 2 ; 1 2 X (Page Fault)
Page 3 -> In Frame 3 : X -> 3 ; 1 2 3 (Page Fault)
Page 1 -> No page fault
Page 2 -> No page fault
Page 4 -> In Frame 1 : 1 -> 4 ; 4 2 3 (Page Fault)
Page 1 -> In Frame 2 : 2 -> 1 ; 4 1 3 (Page Fault)
Page 5 -> In Frame 3 : 3 -> 5 ; 4 1 5 (Page Fault)
Page 2 -> In Frame 1 : 4 -> 2 ; 2 1 5 (Page Fault)
Page 1 -> No page fault
Page 5 -> No page fault
Page 4 -> In Frame 2 : 1 -> 4 ; 2 4 5 (Page Fault)
Page 1 -> In Frame 3 : 5 -> 1 ; 2 4 1 (Page Fault)
Page 3 -> In Frame 1 : 2 -> 3 ; 3 4 1 (Page Fault)
Page 5 -> In Frame 2 : 4 -> 5 ; 3 5 1 (Page Fault)

Total Page Faults: 11
Page Fault Rate: 73.33%
```
#### LRU:
```shell
[yuyueryuyu@Yu lab2]$ ./2-4
Enter the number of pages (<= 100): 15
Enter the number of frames (<= 10): 3
Choose page reference sequence generation method:
1 - Randomly generate page sequence
2 - Manually enter page sequence
2
Enter the page sequence:
1 2 3 1 2 4 1 5 2 1 5 4 1 3 5
Choose page replacement algorithm:
1 - FIFO
2 - LRU
2

LRU Page Replacement:
Page 1 -> In Frame 1 : X -> 1 ; 1 X X (Page Fault)
Page 2 -> In Frame 2 : X -> 2 ; 1 2 X (Page Fault)
Page 3 -> In Frame 3 : X -> 3 ; 1 2 3 (Page Fault)
Page 1 -> No page fault
Page 2 -> No page fault
Page 4 -> In Frame 3 : 3 -> 4 ; 1 2 4 (Page Fault)
Page 1 -> No page fault
Page 5 -> In Frame 2 : 2 -> 5 ; 1 5 4 (Page Fault)
Page 2 -> In Frame 3 : 4 -> 2 ; 1 5 2 (Page Fault)
Page 1 -> No page fault
Page 5 -> No page fault
Page 4 -> In Frame 3 : 2 -> 4 ; 1 5 4 (Page Fault)
Page 1 -> No page fault
Page 3 -> In Frame 2 : 5 -> 3 ; 1 3 4 (Page Fault)
Page 5 -> In Frame 3 : 4 -> 5 ; 1 3 5 (Page Fault)

Total Page Faults: 9
Page Fault Rate: 60.00%
```
### 比较：LRU缺页率远低于FIFO

### 分析：
对于这个测试数据：
```shell
1 2 3 1 2   //局部区域1构建
4           //局部区域外
1 5 2 1 5   //局部区域1测试+局部区域2构建
4           //局部区域外    
1           //局部区域2测试
3           //局部区域外
5           //局部区域2测试
```

假设内存帧数为3，
FIFO的情况：
```shell
Page 1 -> In Frame 1 : X -> 1 ; 1 X X (Page Fault)
Page 2 -> In Frame 2 : X -> 2 ; 1 2 X (Page Fault)
Page 3 -> In Frame 3 : X -> 3 ; 1 2 3 (Page Fault)
Page 1 -> No page fault
Page 2 -> No page fault
Page 4 -> In Frame 1 : 1 -> 4 ; 4 2 3 (Page Fault)
Page 1 -> In Frame 2 : 2 -> 1 ; 4 1 3 (Page Fault)
Page 5 -> In Frame 3 : 3 -> 5 ; 4 1 5 (Page Fault)
Page 2 -> In Frame 1 : 4 -> 2 ; 2 1 5 (Page Fault)
Page 1 -> No page fault
Page 5 -> No page fault
Page 4 -> In Frame 2 : 1 -> 4 ; 2 4 5 (Page Fault)
Page 1 -> In Frame 3 : 5 -> 1 ; 2 4 1 (Page Fault)
Page 3 -> In Frame 1 : 2 -> 3 ; 3 4 1 (Page Fault)
Page 5 -> In Frame 2 : 4 -> 5 ; 3 5 1 (Page Fault)
```
注意到访问Page 4时按照FIFO原则将1进行置换，但局部性较好的程序中，1由于在刚刚被访问，很有可能在接下来被访问。
FIFO算法进行了置换，则在接下来访问1的时候又会产生页错误，缺页率较高。

LRU的情况：
```shell
Page 1 -> In Frame 1 : X -> 1 ; 1 X X (Page Fault)
Page 2 -> In Frame 2 : X -> 2 ; 1 2 X (Page Fault)
Page 3 -> In Frame 3 : X -> 3 ; 1 2 3 (Page Fault)
Page 1 -> No page fault
Page 2 -> No page fault
Page 4 -> In Frame 3 : 3 -> 4 ; 1 2 4 (Page Fault)
Page 1 -> No page fault
Page 5 -> In Frame 2 : 2 -> 5 ; 1 5 4 (Page Fault)
Page 2 -> In Frame 3 : 4 -> 2 ; 1 5 2 (Page Fault)
Page 1 -> No page fault
Page 5 -> No page fault
Page 4 -> In Frame 3 : 2 -> 4 ; 1 5 4 (Page Fault)
Page 1 -> No page fault
Page 3 -> In Frame 2 : 5 -> 3 ; 1 3 4 (Page Fault)
Page 5 -> In Frame 3 : 4 -> 5 ; 1 3 5 (Page Fault)
```
对于同样的数据，访问Page 4的时候按照LRU原则选择了3进行置换，这样随后访问1的时候就不会缺页，缺页率较低。

## 问题：
### （1）从实现和性能方面，比较分析 FIFO 和 LRU 算法。
- FIFO算法：
只需维护队列，实现简单；需要置换时队首出队尾进即可，性能较好。
- LRU算法：
需要记录最近访问的页面，实现较为复杂；同时也需要维护，性能较差。
### （2）LRU 算法是基于程序的局部性原理而提出的算法，你模拟实现的 LRU 算法有没有体现出该特点？如果有，是如何实现的？
有体现程序的局部性原理。我模拟实现的算法在维护队列时优先保留了最近访问的页面。当一个页面在队列中被访问时，会被重新移动到队列的末尾，从而表示它是“最近使用”的页面；而当一个新页面需要插入时，最旧的页面（队列头）被移除。这种操作方式符合局部性。
### （3）在设计内存管理程序时,应如何提高内存利用率。
1. 应该使用合适的页面置换算法。
一般来说，LRU 是基于局部性原理的算法，在程序访问数据具有时空局部性时表现更佳。但也有在部分情况FIFO算法的表现优秀，需要根据需求使用对应的算法。
2. 使用合适的内存分配策略。
应该根据需要选择合适的内存分配策略，减少碎片。
3. 使用内存紧缩。
可以在内存紧张的时候，使用内存紧缩算法，压缩内存空间。
4. 设计合理的垃圾回收机制。
可以采用Mark-Sweep（标记清除算法）、Stop-Copy（停止复制算法）、Reference-Counting（引用计数算法）等算法进行自动的垃圾回收，防止内存泄漏。
其中前两种算法因为内存使用效率、性能等问题在系统级环境中很少被使用；引用计数则在Linux内核和一些系统级编程语言(如C++、Rust)的特殊数据类型中得到了应用。
6. 采用合理的数据结构。
可以设计合理的数据结构，减小程序的内存使用量。
