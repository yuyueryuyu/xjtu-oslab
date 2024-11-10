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