## 步骤一：根据实验内容要求，编写模拟自旋锁程序代码 spinlock.c，待补充主函数的示例代码如下：
```c
/**
*spinlock.c
*in xjtu
*2023.8
*/
#include <stdio.h>
#include <pthread.h>
// 定义自旋锁结构体
typedef struct {
 int flag;
} spinlock_t;
// 初始化自旋锁
void spinlock_init(spinlock_t *lock) {
 lock->flag = 0;
}
// 获取自旋锁
void spinlock_lock(spinlock_t *lock) {
 while (__sync_lock_test_and_set(&lock->flag, 1)) {
 // 自旋等待
 }
}
// 释放自旋锁
void spinlock_unlock(spinlock_t *lock) {
 __sync_lock_release(&lock->flag);
}
// 共享变量
int shared_value = 0;
// 线程函数
void *thread_function(void *arg) {
 spinlock_t *lock = (spinlock_t *)arg;
 
 for (int i = 0; i < 5000; ++i) {
 spinlock_lock(lock);
 shared_value++;
 spinlock_unlock(lock);
 }
 
 return NULL;
}
int main() {
 pthread_t thread1, thread2;
 spinlock_t lock;
// 输出共享变量的值
 // 初始化自旋锁
 
 // 创建两个线程
 
 // 等待线程结束
 
 // 输出共享变量的值
 return 0;
}
```
### 填充后如下：[spinlock.c]
```c
#include <stdio.h>
#include <pthread.h>
// 定义自旋锁结构体
typedef struct {
 int flag;
} spinlock_t;
// 初始化自旋锁
void spinlock_init(spinlock_t *lock) {
 lock->flag = 0;
}
// 获取自旋锁
void spinlock_lock(spinlock_t *lock) {
 while (__sync_lock_test_and_set(&lock->flag, 1)) {
 // 自旋等待
 }
}
// 释放自旋锁
void spinlock_unlock(spinlock_t *lock) {
 __sync_lock_release(&lock->flag);
}
// 共享变量
int shared_value = 0;
// 线程函数
void *thread_function(void *arg) {
 spinlock_t *lock = (spinlock_t *)arg;
 
 for (int i = 0; i < 5000; ++i) {
 spinlock_lock(lock);
 shared_value++;
 spinlock_unlock(lock);
 }
 
 return NULL;
}
int main() {
 pthread_t thread1, thread2;
 spinlock_t lock;
// 输出共享变量的值
 printf("shared_value = %d\n", shared_value);
 // 初始化自旋锁
 spinlock_init(&lock);
 // 创建两个线程
 if (pthread_create(&thread1, NULL, thread_function, &lock) != 0) {
   fprintf(stderr, "Failed to create thread1!\n");
   return 1;
 }
 if (pthread_create(&thread2, NULL, thread_function, &lock) != 0) {
   fprintf(stderr, "Failed to create thread2!\n");
   return 1;
 }
 // 等待线程结束
 pthread_join(thread1, NULL);
 pthread_join(thread2, NULL);
 
 // 输出共享变量的值
 printf("shared_value = %d\n", shared_value);
 return 0;
}
```
## 步骤二：补充完成代码后，编译并运行程序，分析运行结果
### 运行结果：
```shell
[root@kp-test01 test]# ./spinlock
shared_value = 0
shared_value = 10000
[root@kp-test01 test]# ./spinlock
shared_value = 0
shared_value = 10000
[root@kp-test01 test]# ./spinlock
shared_value = 0
shared_value = 10000
```
### 分析：
自旋锁使用了while循环实现了自旋等待，控制了同时访问，使得每次操作都成功进行，进行完成后释放自旋锁使得其他线程可以进行操作。故而输出均显示为10000。

### 与信号量PV操作的区别：
自旋锁：
自旋锁在线程在尝试获取锁时如果发现锁被占用，就会不断地检查（“自旋”）锁的状态，直到锁被释放。（通过自旋等待进行实现）

优点：
无需进行线程状态切换，适用于短时间等待，时间性能高效。

缺点：
反复检查条件，一直占用CPU资源。

信号量：
信号量是一种计数器，用于管理对共享资源的访问，通过两个操作实现：
P 操作：试图获取信号量，如果信号量值大于零，减少信号量值；否则阻塞线程。
V 操作：释放信号量，增加信号量值，并唤醒被阻塞的线程（如果有）。
（通过线程阻塞和唤醒实现）

优点：
阻塞线程，释放CPU资源。

缺点：
需要切换线程状态，时间性能较差。

