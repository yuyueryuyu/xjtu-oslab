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