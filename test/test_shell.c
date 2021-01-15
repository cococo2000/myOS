/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                  The shell acts as a task running in user mode. 
 *       The main function is to make system calls through the user's output.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this 
 * software and associated documentation files (the "Software"), to deal in the Software 
 * without restriction, including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit 
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#include "screen.h"
#include "stdio.h"
#include "syscall.h"
#include "test.h"
#include "fs.h"

#ifdef P3_TEST

struct task_info task1 = {"task1", (uint64_t)&ready_to_exit_task, USER_PROCESS};
struct task_info task2 = {"task2", (uint64_t)&wait_lock_task, USER_PROCESS};
struct task_info task3 = {"task3", (uint64_t)&wait_exit_task, USER_PROCESS};

struct task_info task4 = {"task4", (uint64_t)&semaphore_add_task1, USER_PROCESS};
struct task_info task5 = {"task5", (uint64_t)&semaphore_add_task2, USER_PROCESS};
struct task_info task6 = {"task6", (uint64_t)&semaphore_add_task3, USER_PROCESS};

struct task_info task7 = {"task7", (uint64_t)&producer_task, USER_PROCESS};
struct task_info task8 = {"task8", (uint64_t)&consumer_task1, USER_PROCESS};
struct task_info task9 = {"task9", (uint64_t)&consumer_task2, USER_PROCESS};

struct task_info task10 = {"task10", (uint64_t)&barrier_task1, USER_PROCESS};
struct task_info task11 = {"task11", (uint64_t)&barrier_task2, USER_PROCESS};
struct task_info task12 = {"task12", (uint64_t)&barrier_task3, USER_PROCESS};

struct task_info task13 = {"SunQuan", (uint64_t)&SunQuan, USER_PROCESS};
struct task_info task14 = {"LiuBei", (uint64_t)&LiuBei, USER_PROCESS};
struct task_info task15 = {"CaoCao", (uint64_t)&CaoCao, USER_PROCESS};

// struct task_info task16 = {"multcore", (uint64_t)&test_multicore, USER_PROCESS};
#endif

#ifdef P4_TEST
struct task_info task16 = {"mem_test1", (uint64_t)&rw_task1, USER_PROCESS};
struct task_info task17 = {"plan", (uint64_t)&drawing_task1, USER_PROCESS};
#endif

#ifdef P5_TEST
struct task_info task18 = {"mac_send", (uint64_t)&mac_send_task, USER_PROCESS};
struct task_info task19 = {"mac_recv", (uint64_t)&mac_recv_task, USER_PROCESS};
#endif

#ifdef P6_TEST
struct task_info task19 = {"fs_test", (uint64_t)&test_fs, USER_PROCESS};
struct task_info task20 = {"big_file", (uint64_t)&test_big_file, USER_PROCESS};
#endif

static struct task_info *test_tasks[NUM_MAX_TASK] = {
#ifdef P3_TEST
    &task1,
    &task2,
    &task3,
    &task4,
    &task5,
    &task6,
    &task7,
    &task8,
    &task9,
    &task10,
    &task11,
    &task12,
    &task13,
    &task14,
    &task15,
#endif
#ifdef P4_TEST
    &task16,
    &task17,
#endif
#ifdef P5_TEST
    &task18,
    &task19,
#endif
#ifdef P6_TEST
    &task19,
    &task20,
#endif
};

char read_shell_buff()
{
    char ch = 0;
    char *read_port = (char *)0xffffffffbfe00000;
    volatile char *state = (char *)0xffffffffbfe00005;  // serial port state register
    while ((*state) & 0x01) {
        ch = *read_port;
    }
    return ch;
}

int atoi(char *str)
{
    int ret = 0;
    int base = 10;
    if ((str[0] == '0' && str[1] == 'x') || (str[0] == '0' && str[1] == 'X')) {
        base = 16;
        str += 2;
    }
    while (*str != '\0') {
        if ('0' <= *str && *str <= '9') {
            ret = ret * base + (*str - '0');
        } else if (base == 16) {
            if ('a' <= *str && *str <= 'f'){
                ret = ret * base + (*str - 'a' + 10);
            } else if ('A' <= *str && *str <= 'F') {
                ret = ret * base + (*str - 'A' + 10);
            } else {
                return 0;
            }
        } else {
            return 0;
        }
        ++str;
    }
    return ret;
}

void execute(uint32_t argc, char argv[][LEN_ARGV])
{
    if (argc == 1) {
        if (!strcmp(argv[0], "ps")) {
            sys_process_show();
        }
        else if (!strcmp(argv[0], "clear")) {
            sys_screen_clear();
        }
        else if (!strcmp(argv[0], "mkfs")) {
            sys_mkfs();
        }
        else if (!strcmp(argv[0], "statfs")) {
            sys_print_fs();
        }
        else if (!strcmp(argv[0], "ls")) {
            sys_readdir("\0");
        }
        else {
            printf("Command '%s' not found!\n", argv[0]);
        }
    }
    else if (argc == 2) {
        int pid = atoi(argv[1]);
        if (!strcmp(argv[0], "exec")) {
            printf("exec process[%d].\n", pid);
            sys_spawn(test_tasks[pid]);
        }
        else if (!strcmp(argv[0], "kill")) {
            if (!sys_kill(pid)) {
                printf("kill process pid = %d.\n", pid);
            }
        }
        else if (!strcmp(argv[0], "mkdir")) {
            if (!sys_mkdir(argv[1])) {
                printf("Successfully create dir: %s\n", argv[1]);
            }
        }
        else if (!strcmp(argv[0], "rmdir")) {
            if (!sys_rmdir(argv[1])) {
                printf("Successfully remove dir: %s\n", argv[1]);
            }
        }
        else if (!strcmp(argv[0], "touch")) {
            if (!sys_mknod(argv[1])) {
                printf("Successfully create file: %s\n", argv[1]);
            }
        }
        else if (!strcmp(argv[0], "cat")) {
            if (!sys_cat(argv[1])) {
                printf("Successfully cat file: %s\n", argv[1]);
            }
        }
        else if (!strcmp(argv[0], "cd")) {
            if (!sys_enterdir(argv[1])) {
                printf("Successfully enter dir: %s\n", argv[1]);
            }
        }
        else if (!strcmp(argv[0], "ls")) {
            sys_readdir(argv[1]);
        }
        else {
            printf("Command '%s' not found!\n", argv[0]);
        }
    }
    else if (argc) {
        if (argc == 5 && !strcmp(argv[0], "exec")) {
            int pid = atoi(argv[1]);
            printf("exec process[%d].\n", pid);
            int i = sys_spawn(test_tasks[pid]);
            pcb[i].user_context.regs[4] = atoi(argv[2]);
            pcb[i].user_context.regs[5] = atoi(argv[3]);
            pcb[i].user_context.regs[6] = atoi(argv[4]);
        }
        else if (argc == 4 && !strcmp(argv[0], "ln") && !strcmp(argv[1], "-s")) {
            if (!sys_link(argv[2], argv[3], 1)) {
                printf("Successfully create soft link: %s -> %s\n", argv[3], argv[2]);
            }
        }
        else if (argc == 3 && !strcmp(argv[0], "ln")) {
            if (!sys_link(argv[1], argv[2], 0)) {
                printf("Successfully create hard link: %s -> %s\n", argv[2], argv[1]);
            }
        }
        else {
            printf("Command '%s' not found!\n", argv[0]);
        }
    }
}

char argv[MAX_ARGC][LEN_ARGV];

void test_shell()
{
    char command[MAX_ARGC * LEN_ARGV];
    uint32_t argc;
    uint32_t i = 0;
    uint32_t j, k;
    sys_move_cursor(0, SCREEN_HEIGHT / 2);
    printf("-------------------------    COMMAND    --------------------------\n");
    printf("> root@UCAS_OS: ");
    while (1) {
        char ch = sys_read_shell_buff();
        // NULL or (BackSpace or Delete) at begin
        if (ch == 0 || (i == 0 && (ch == 0x8 || ch == 0x7f))) {
            continue;
        }
        // delete or backspace
        else if (i && (ch == 0x8 || ch == 0x7f)) {
            i--;
            screen_cursor_x --;
            sys_move_cursor(screen_cursor_x, screen_cursor_y);
            printf(" ");
            screen_cursor_x --;
            continue;
        }
        printf("%c", ch);
        // Enter '\r'
        if (ch != '\r') {
            // backspace and delete(TODO)
            if (ch == 0x8 || ch == 0x7f) {
                i--;
            } else {
                command[i++] = ch;
            }
            continue;
        } else {
            command[i] = ' ';
            command[++i] = '\0';
            // to generate argc and argv[]
            // argc: number of arguments, argv[]: arguments
            j = 0;
            // skip blank (' ')
            while (command[j] == ' ' && j < i) {
                j++;
            }
            for (argc = 0, k = 0; j < i; j++) {
                if (command[j] == ' ') {
                    // skip blank (' '), a new argv begin
                    while (command[j] == ' ' && j < i) {
                        j++;
                    }
                    argv[argc][k] = '\0';
                    argc++;
                    k = 0;
                }
                argv[argc][k++] = command[j];
            }
            execute(argc, argv);
            // one cammand done, reset i
            i = 0;
            printf("> root@UCAS_OS: ");
        }
    }

}
