# UCAS OS 2020

A repository for OSLab-MIPS 2020 in UCAS.

## Tasks Progress 

- [x] Project1 Bootloader
- [x] Project2 Simple Kernel
- [ ] Project3 Interactive OS and Process Management 
    - [x] S-core
    - [ ] A-core: Synchronization done.
    - [ ] C-core: to do
- [ ] Project 4 Virtual Memory
    - [x] S-core
    - [x] A-core
    - [ ] C-core: to do
- [ ] Project 5 Device Driver
    - [x] S-core
    - [x] A-core
    - [ ] C-core: to do
- [ ] Project 6 File System
    - [x] S-core
    - [x] A-core
    - [ ] C-core: to do

## 目录结构

#### Design Review & Report

design review 的 PPT 与实验设计文档见 `/report`目录。

```shell
│  .gitignore
│  ld.script
│  README.md
│
├─.vscode
│      c_cpp_properties.json
│      launch.json
│
├─arch
│  └─mips
│      ├─boot
│      │      bootblock.S
│      │
│      ├─include
│      │      asm.h
│      │      common.h
│      │      regs.h
│      │      smp.h
│      │
│      ├─kernel
│      │      entry.S
│      │      smp.c
│      │      syscall.S
│      │
│      └─pmon
│              common.c
│
├─drivers
│      mac.c
│      mac.h
│      screen.c
│      screen.h
│
├─fs
│      fs.c
│      fs.h
│
├─include
│  │  stdarg.h
│  │  stdio.h
│  │  type.h
│  │
│  ├─os
│  │      barrier.h
│  │      cond.h
│  │      irq.h
│  │      lock.h
│  │      mm.h
│  │      queue.h
│  │      sched.h
│  │      sem.h
│  │      string.h
│  │      sync.h
│  │      time.h
│  │
│  └─sys
│          syscall.h
│
├─init
│      main.c
│
├─ipc
│      barrier.c
│      cond.c
│      sem.c
│
├─kernel
│  ├─irq
│  │      irq.c
│  │
│  ├─locking
│  │      lock.c
│  │
│  ├─sched
│  │      queue.c
│  │      sched.c
│  │      time.c
│  │
│  └─syscall
│          syscall.c
│
├─libs
│      libepmon.a
│      libepmon.a.bck
│      mailbox.c
│      mailbox.h
│      printk.c
│      string.c
│
├─mm
│      memory.c
│
├─pktRxTx
│  │  libpcap-1.9.1.tar.gz
│  │  libpcap.so.1
│  │  pktRxTx
│  │  tcpdump-4.9.3.tar.gz
│  │
│  ├─libpcap-1.9.1
│  │
│  └─pktRxTx-Linux
│      │  libpcap.so.1
│      │  Makefile
│      │
│      ├─Include
│      │
│      └─Source
│
├─report
│  ├─prj1
│  │      Design Review prj1.pptx
│  │      Project1 Bootloader设计文档.pdf
│  │
│  ├─prj2
│  │      Design Review prj2 part1.pptx
│  │      Design Review prj2 part2.pptx
│  │      Project2 A Simple Kernel-part1-设计文档.pdf
│  │      Project2 A Simple Kernel-part2-设计文档.pdf
│  │
│  ├─prj3
│  │      Design Review prj3.pptx
│  │      Project3-Interactive OS and Process Management设计文档.pdf
│  │
│  ├─prj4
│  │      Design Review prj4.pptx
│  │      Project4 Virtual Memory设计文档.pdf
│  │
│  ├─prj5
│  │      Design Review prj5.pptx
│  │      Project5 Device Driver设计文档.pdf
│  │
│  └─prj6
│          Design Review prj6.pptx
│          Project6 File System设计文档.pdf
│
├─test
│  │  test.c
│  │  test.h
│  │  test_shell.c
│  │
│  ├─test_project1
│  │  │  kernel.c
│  │  │  ld.script
│  │  │
│  │  ├─1-1
│  │  │      createimage
│  │  │      disk
│  │  │      ld.script
│  │  │      Makefile
│  │  │
│  │  └─1-2
│  │          disk
│  │          ld.script
│  │          Makefile
│  │
│  ├─test_project2
│  │      kernel.txt
│  │      Makefile
│  │      test.c
│  │      test.h
│  │      test_lock1.c
│  │      test_lock2.c
│  │      test_scheduler1.c
│  │      test_scheduler2.c
│  │      test_sleep.c
│  │      test_timer.c
│  │
│  ├─test_project3
│  │      disk
│  │      kernel.txt
│  │      Makefile
│  │      test3.h
│  │      test_affinity.c
│  │      test_barrier.c
│  │      test_condition.c
│  │      test_kill.c
│  │      test_multicore.c
│  │      test_sanguo.c
│  │      test_semaphore.c
│  │      test_wait.c
│  │
│  ├─test_project4
│  │      disk
│  │      kernel.txt
│  │      Makefile
│  │      process1.c
│  │      process2.c
│  │      shm_test0.c
│  │      shm_test1.c
│  │      test4.h
│  │
│  ├─test_project5
│  │      kernel.txt
│  │      Makefile
│  │      test5.h
│  │      test_mac.c
│  │
│  └─test_project6
│          kernel.txt
│          Makefile
│          rand.c
│          test_fs.c
│          test_fs.h
│
└─tools
        createimage.c
```

## Project1 Bootloader

##### 主要模块

`/arch/mips/boot/bootblock.S` 实现 `bootloader` 引导 `kernel` 的功能（重定位+大核加载）

`/test/kernel.c`实现一个小的操作系统内核，具有打印字符串和回显输入字符的功能

`/tools/createimage.c` 实现将 `bootblock` 和 `kernel` 合并为一个完整SD卡镜像文件。

##### 运行

1. ##### 进入/test/test-project1/1-x目录（x = 1 or 2）

    - 1-1：task1 & task2
    - 1-2：task3 & bonus

2. ##### make clean：rm -rf bootblock (createimage)? image main *.o

3. ##### make all: 交叉编译

    - task3中权限不够：Makefile中的./createimage --extended bootblock main前添加chmod +x createimage
    - 上述仍不行，报错后手动执行./createimage --extended bootblock main或尝试更改文件所有者

4. ##### 插入SD卡并选中，make floppy：写入SD卡

5. ##### 将SD卡插到板子上，连接开发板串口线，sudo minicom：

    -s：进入minicom设置

6. ##### 开发板按start/reset，听到嘀的一声，终端开始有输出

7. ##### PMON终端输入：loadboot

8. ##### done！

## Project2 Simple Kernel



## Project3 Interactive OS and Process Management 



## Project 4 Virtual Memory



## Project 5 Device Driver



## Project 6 File System

本文件系统支持的命令有：（注：   仅 `cd`和`ls`指令支持多级目录和相对路径。）

-   statfs
-   mkfs
-   cd [path]
-   ls
-   ls [path]
-   mkdir [name]
-   rmdir [name]
-   touch [file]
-   cat [file]
-   ln [src] [dest]（硬链接）
-   ln -s [src] [dest] （软链接）

完成了基本的文件操作，并实现二级索引。