# Project1 Bootloader

## 主要模块

##### /arch/mips/boot/bootblock.S 实现bootloader引导kernel的功能（重定位+大核加载）

##### /test/kernel.c实现一个小的操作系统内核，具有打印字符串和回显输入字符的功能

##### /tools/createimage.c实现将bootblock和kernel合并为一个完整SD卡镜像文件。

## 代码合集(/code)

#### /basic 

task1-3 仅支持小核加载（一个扇区）

#### /bonus

task3 并支持大核加载和重定位

## 运行

1. 进入/test/test-project1/1-x目录（x = 1 or 2）

   - 1-1：task1 & task2
   - 1-2：task3 & bonus

2. make clean：rm -rf bootblock (createimage)? image main *.o

3. make all: 交叉编译

   - task3中权限不够：Makefile中的./createimage --extended bootblock main前添加chmod +x createimage
   - 上述仍不行，报错后手动执行./createimage --extended bootblock main或尝试更改文件所有者

4. 插入SD卡并选中，make floppy：写入SD卡

5. 将SD卡插到板子上，连接开发板串口线，sudo minicom：

   -s：进入minicom设置

6. 开发板按start/reset，听到嘀的一声，终端开始有输出

7. PMON终端输入：loadboot

8. done！

