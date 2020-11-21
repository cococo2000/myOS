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
#endif
struct task_info task16 = {"multcore", (uint64_t)&test_multicore, USER_PROCESS};
static struct task_info *test_tasks[NUM_MAX_TASK] = {
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
};

#endif

static void disable_interrupt()
{
    uint32_t cp0_status = get_cp0_status();
    cp0_status &= 0xfffffffe;
    set_cp0_status(cp0_status);
}

static void enable_interrupt()
{
    uint32_t cp0_status = get_cp0_status();
    cp0_status |= 0x01;
    set_cp0_status(cp0_status);
}

static char read_uart_ch(void)
{
    char ch = 0;
    char *read_port = (char *)0xffffffffbfe00000;
    volatile char *state = (char *)0xffffffffbfe00005;  // serial port state register
    while ((*state) & 0x01) {
        ch = *read_port;
    }
    return ch;
}

int my_atoi(char s[])
{
    int i = 0;
    int value = 0;
    while (s[i] >= '0' && s[i] <= '9') {
        value = value * 10 + (s[i] - '0');
        i++;
    }
    return value;
}

void execute(uint32_t argc, char argv[][10])
{
    if (argc == 1) {
        if (!strcmp(argv[0], "ps")) {
            sys_process_show();
        }
        else if (!strcmp(argv[0], "clear")){
            sys_screen_clear();
        }
        else {
            printf("Command '%s' not found!\n", argv[0]);
        }
    }
    else if (argc == 2) {
        int pid = my_atoi(argv[1]);
        if (!strcmp(argv[0], "exec")) {
            printf("exec process[%d].\n", pid);
            sys_spawn(test_tasks[pid]);
        }
        else if (!strcmp(argv[0], "kill")) {
            printf("kill process pid = %d.\n", pid);
            sys_kill(pid);
        }
        else {
            printf("Command '%s' not found!\n", argv[0]);
        }
    }
    else if (argc) {
        printf("Command '%s' not found!\n", argv[0]);
    }
}

void test_shell()
{
    char command[20];
    uint32_t argc;
    char argv[3][10];
    uint32_t i = 0;
    uint32_t j, k;
    sys_move_cursor(0, SCREEN_HEIGHT / 2);
    printf("-------------------------    COMMAND    --------------------------\n");
    printf("> root@UCAS_OS: ");
    while (1) {
        disable_interrupt();
        char ch = read_uart_ch();
        enable_interrupt();
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
