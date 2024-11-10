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