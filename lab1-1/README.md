## 步骤一：编写并多次运行图 1-1 中代码
### 代码：【1-1.c】
```c
#include<sys/wait.h>
#include<sys/types.h>
#include<stdio.h>
#include<unistd.h>

int main() {
	pid_t pid, pid1;
	
	pid = fork();

	if (pid < 0) {
		fprintf(stderr, "Fork Failed");
		return 1;
	} else if (pid == 0) {
		pid1 = getpid();
		printf("child: pid = %d\n", pid);
		printf("child: pid1 = %d\n", pid1);
	} else {
		pid1 = getpid();
		printf("parent: pid = %d\n", pid);
		printf("parent: pid1 = %d\n", pid1);
		wait(NULL);
	}
	return 0;
}
```
### 运行结果：
```shell
[root@kp-test01 test]# ./1-1
parent: pid = 2649
child: pid = 0
parent: pid1 = 2648
child: pid1 = 2649
[root@kp-test01 test]# ./1-1
parent: pid = 2651
child: pid = 0
parent: pid1 = 2650
child: pid1 = 2651
[root@kp-test01 test]# ./1-1
parent: pid = 2653
child: pid = 0
parent: pid1 = 2652
child: pid1 = 2653
[root@kp-test01 test]# ./1-1
parent: pid = 2655
child: pid = 0
parent: pid1 = 2654
child: pid1 = 2655
[root@kp-test01 test]# ./1-1
parent: pid = 2657
child: pid = 0
parent: pid1 = 2656
child: pid1 = 2657
```
### 问题
- 在编译./1-1（包含
```c
wait(NULL)；
```
的程序）时会报warning。
- 解决方案：添加头文件sys/wait.h

## 删去图 1-1 代码中的 wait()函数并多次运行程序，分析运行结果。
### 代码：【1-1_nowait.c】
```c
#include<sys/types.h>
#include<stdio.h>
#include<unistd.h>

int main() {
        pid_t pid, pid1;

        pid = fork();

        if (pid < 0) {
                fprintf(stderr, "Fork Failed");
                return 1;
        } else if (pid == 0) {
                pid1 = getpid();
                printf("child: pid = %d\n", pid);
                printf("child: pid1 = %d\n", pid1);
        } else {
                pid1 = getpid();
                printf("parent: pid = %d\n", pid);
                printf("parent: pid1 = %d\n", pid1);
//                wait(NULL);
        }
        return 0;
}
```
### 结果：
```shell
[root@kp-test01 test]# ./1-1n
parent: pid = 2659
child: pid = 0
parent: pid1 = 2658
child: pid1 = 2659
[root@kp-test01 test]# ./1-1n
parent: pid = 2661
child: pid = 0
parent: pid1 = 2660
child: pid1 = 2661
[root@kp-test01 test]# ./1-1n
parent: pid = 2663
child: pid = 0
parent: pid1 = 2662
child: pid1 = 2663
[root@kp-test01 test]# ./1-1n
parent: pid = 2665
child: pid = 0
parent: pid1 = 2664
child: pid1 = 2665
[root@kp-test01 test]# ./1-1n
parent: pid = 2667
child: pid = 0
parent: pid1 = 2666
child: pid1 = 2667
[root@kp-test01 test]# ./1-1n
parent: pid = 2669
child: pid = 0
parent: pid1 = 2668
child: pid1 = 2669
[root@kp-test01 test]# ./1-1n
parent: pid = 2671
child: pid = 0
parent: pid1 = 2670
child: pid1 = 2671
[root@kp-test01 test]# ./1-1n
parent: pid = 2673
child: pid = 0
parent: pid1 = 2672
child: pid1 = 2673
[root@kp-test01 test]# ./1-1n
parent: pid = 2675
child: pid = 0
parent: pid1 = 2674
child: pid1 = 2675
[root@kp-test01 test]# ./1-1n
child: pid = 0
parent: pid = 2677
child: pid1 = 2677
parent: pid1 = 2676
[root@kp-test01 test]# ./1-1n
parent: pid = 2679
child: pid = 0
parent: pid1 = 2678
child: pid1 = 2679
[root@kp-test01 test]# ./1-1n
parent: pid = 2681
parent: pid1 = 2680
[root@kp-test01 test]# child: pid = 0
child: pid1 = 2681
 ./1-1n
parent: pid = 2683
child: pid = 0
parent: pid1 = 2682
child: pid1 = 2683
```
### 分析：
注意去掉wait(NULL);的输出更具不确定性。wait(NULL);表示父进程等待子进程结束，若不等待，则父进程会提前返回，可能输出：
```shell
parent: pid = 3031
parent: pid1 = 3030
[root@kp-test01 test]# child: pid = 0
child: pid1 = 3031
```
原因是shell一般在父进程返回时就恢复控制权，而不等待子进程。
这种情况下，若不wait，则可能父进程提前退出，子进程变成孤儿进程；也有可能子进程提前退出，成为僵尸进程。
但无论哪种情况，父进程推出后都会由init进程接管，继而被释放，体现了系统的健壮性。
但僵尸进程在父进程结束前一直存在，不会被自动清理，所以在编写程序时需要注意。

## 修改图 1-1 中代码，增加一个全局变量并在父子进程中对其进行不同的操作（自行设计），观察并解释所做操作和输出结果。

### 代码：【1-1_global.c】
```c
#include<sys/wait.h>
#include<sys/types.h>
#include<stdio.h>
#include<unistd.h>
int global_int = 0;
int main() {
        pid_t pid, pid1;

        pid = fork();

        if (pid < 0) {
                fprintf(stderr, "Fork Failed");
                return 1;
        } else if (pid == 0) {
                pid1 = getpid();
                printf("child: pid = %d\n", pid);
                printf("child: pid1 = %d\n", pid1);
		global_int++;
		printf("child: global_int = %d\n", global_int);
		printf("child: &global_int = %p\n", &global_int);
        } else {
                pid1 = getpid();
                printf("parent: pid = %d\n", pid);
                printf("parent: pid1 = %d\n", pid1);
                global_int--;
                printf("parent: global_int = %d\n", global_int);
		printf("parent: &global_int = %p\n", &global_int);
		wait(NULL);
        }
        return 0;
}
```
### 结果：
```shell
[root@kp-test01 test]# ./1-1g
parent: pid = 2693
child: pid = 0
parent: pid1 = 2692
child: pid1 = 2693
parent: global_int = -1
child: global_int = 1
parent: &global_int = 0x42005c
child: &global_int = 0x42005c
[root@kp-test01 test]# ./1-1g
parent: pid = 2695
parent: pid1 = 2694
parent: global_int = -1
parent: &global_int = 0x42005c
child: pid = 0
child: pid1 = 2695
child: global_int = 1
child: &global_int = 0x42005c
```
### 分析：
在父进程全局变量--，子进程全局变量++。因此最后父进程 = -1 子进程 = 1
### 问题：global_int地址相同，对应的值却不同。
解释：相同的地址是虚拟地址，实际上存储在不同的物理内存空间，所以对应的值不相同。

## 步骤四：在步骤三基础上，在 return 前增加对全局变量的操作（自行设计）并输出结果，观察并解释所做操作和输出结果。

### 代码：【1-1_return.c】
```c
#include<sys/wait.h>
#include<sys/types.h>
#include<stdio.h>
#include<unistd.h>
int global_int = 0;
int main() {
        pid_t pid, pid1;

        pid = fork();

        if (pid < 0) {
                fprintf(stderr, "Fork Failed");
                return 1;
        } else if (pid == 0) {
                pid1 = getpid();
                printf("child: pid = %d\n", pid);
                printf("child: pid1 = %d\n", pid1);
                global_int++;
                printf("child: global_int = %d\n", global_int);
                printf("child: &global_int = %p\n", &global_int);
        } else {
                pid1 = getpid();
                printf("parent: pid = %d\n", pid);
                printf("parent: pid1 = %d\n", pid1);
                global_int--;
                printf("parent: global_int = %d\n", global_int);
                printf("parent: &global_int = %p\n", &global_int);
                wait(NULL);
        }
	global_int += 10000;      
        printf("before return: global_int = %d\n", global_int);
        printf("before_return: &global_int = %p\n", &global_int);
        return 0;
}
```
### 运行结果：
```shell
[root@kp-test01 test]# ./1-1r
parent: pid = 2697
child: pid = 0
parent: pid1 = 2696
child: pid1 = 2697
parent: global_int = -1
child: global_int = 1
parent: &global_int = 0x42005c
child: &global_int = 0x42005c
before return: global_int = 10001
before_return: &global_int = 0x42005c
before return: global_int = 9999
before_return: &global_int = 0x42005c
```
### 分析
操作为返回前全局变量+10000.
最后父进程输出9999，子进程10001，进一步说明了父进程和子进程内变量的独立性。

## 步骤五：修改图 1-1 程序，在子进程中调用 system()与 exec 族函数。编写system_call.c 文件输出进程号 PID，编译后生成 system_call 可执行文件。在子进程中调用 system_call,观察输出结果并分析总结。
system 会创建一个子进程，在子进程中调用系统默认的 shell来执行命令，父进程会等待子进程执行完成后继续。
exec 直接加载指定的程序替换当前进程映像，新的程序开始执行，不会创建子进程，当前进程的执行终止，转而执行新的程序。
### 代码：【1-1_systemcall.c】
```c
#include<stdlib.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<stdio.h>
#include<unistd.h>

int main() {
        pid_t pid, pid1;

        pid = fork();

        if (pid < 0) {
                fprintf(stderr, "Fork Failed");
                return 1;
        } else if (pid == 0) {
                pid1 = getpid();
                printf("child: pid = %d\n", pid);
                printf("child: pid1 = %d\n", pid1);
		int status = system("./system_call");
		if (status == -1) {
			fprintf(stderr, "system Failed");
			return 1;
		}
		execl("./system_call", "system_call", NULL);
		fprintf(stderr, "execl Failed");            
		return 1;
        } else {
                pid1 = getpid();
                printf("parent: pid = %d\n", pid);
                printf("parent: pid1 = %d\n", pid1);
                wait(NULL);
        }
        return 0;
}
```
### 运行结果
```shell
[root@kp-test01 test]# ./1-1s
parent: pid = 2701
child: pid = 0
parent: pid1 = 2700
child: pid1 = 2701
system_call: PID = 2702
system_call: PID = 2701
[root@kp-test01 test]#  ./1-1s
parent: pid = 2704
child: pid = 0
parent: pid1 = 2703
child: pid1 = 2704
system_call: PID = 2705
system_call: PID = 2704
```

### 问题：
若将exec族函数放在system函数前面，子进程将完全被替换为 ./system_call，因此后续的 system 调用将不会被执行。所以不应这么做。