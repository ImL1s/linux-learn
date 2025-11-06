/*
 * 檔案名稱: zombie_orphan.c
 * 功能說明: 演示殭屍進程 (Zombie) 和孤兒進程 (Orphan)
 *
 * 知識點:
 *   1. 殭屍進程的產生和危害
 *   2. 孤兒進程的產生和特點
 *   3. 如何避免殭屍進程
 *   4. init 進程的作用
 *
 * 編譯方式: gcc -o zombie_orphan zombie_orphan.c
 * 執行方式: ./zombie_orphan
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void demo_zombie(void);
void demo_zombie_fix(void);
void demo_orphan(void);

int main(void)
{
    int choice;

    printf("====== 殭屍進程與孤兒進程演示 ======\n\n");
    printf("選擇要演示的情況：\n");
    printf("1. 殭屍進程 (Zombie Process) - 問題演示\n");
    printf("2. 殭屍進程 - 正確處理方式\n");
    printf("3. 孤兒進程 (Orphan Process)\n");
    printf("\n請輸入選項 (1-3): ");

    if (scanf("%d", &choice) != 1) {
        printf("無效輸入\n");
        return 1;
    }

    switch (choice) {
        case 1:
            demo_zombie();
            break;
        case 2:
            demo_zombie_fix();
            break;
        case 3:
            demo_orphan();
            break;
        default:
            printf("無效選項\n");
            return 1;
    }

    return 0;
}

/*
 * 殭屍進程演示 - 問題場景
 *
 * 什麼是殭屍進程？
 * - 子進程已經結束，但父進程還沒有調用 wait/waitpid 回收
 * - 進程已死，但進程表項（PCB）還保留
 * - 狀態顯示為 Z (Zombie)
 * - 無法被 kill 命令終止
 *
 * 危害：
 * - 佔用進程表空間
 * - 大量殭屍進程會耗盡系統資源
 * - 可能導致無法創建新進程
 */
void demo_zombie(void)
{
    pid_t pid;

    printf("\n【殭屍進程演示 - 問題場景】\n\n");

    pid = fork();

    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) {
        // 子進程：快速退出
        printf("[子進程 PID=%d] 開始執行\n", getpid());
        printf("[子進程] 執行一些簡單的任務...\n");
        sleep(2);
        printf("[子進程] 任務完成，準備退出\n");
        printf("[子進程] 退出後將變成殭屍進程！\n");
        exit(EXIT_SUCCESS);
    }
    else {
        // 父進程：不調用 wait()，讓子進程變成殭屍
        printf("[父進程 PID=%d] 創建了子進程 %d\n", getpid(), pid);
        printf("[父進程] 注意：我不會調用 wait() 來回收子進程！\n");
        printf("[父進程] 子進程退出後會變成殭屍進程\n\n");

        printf("提示：在另一個終端執行以下命令觀察殭屍進程：\n");
        printf("  ps aux | grep Z\n");
        printf("  或\n");
        printf("  ps -eo pid,ppid,stat,cmd | grep %d\n\n", pid);

        // 父進程繼續運行，不回收子進程
        printf("[父進程] 睡眠 30 秒，期間子進程會變成殭屍...\n");
        sleep(30);

        printf("\n[父進程] 睡眠結束\n");
        printf("[父進程] 現在調用 wait() 回收子進程...\n");

        int status;
        pid_t zombie_pid = wait(&status);
        printf("[父進程] 成功回收殭屍進程 %d\n", zombie_pid);
        printf("[父進程] 殭屍進程已被清除\n");
    }
}

/*
 * 殭屍進程 - 正確處理方式
 *
 * 避免殭屍進程的方法：
 * 1. 父進程及時調用 wait() 或 waitpid()
 * 2. 使用信號處理器處理 SIGCHLD 信號
 * 3. 兩次 fork()，讓孫進程成為孤兒（由 init 回收）
 * 4. 設置 SA_NOCLDWAIT 標誌忽略子進程退出
 */
void demo_zombie_fix(void)
{
    pid_t pid;

    printf("\n【殭屍進程 - 正確處理方式】\n\n");

    pid = fork();

    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) {
        // 子進程
        printf("[子進程 PID=%d] 開始執行\n", getpid());
        printf("[子進程] 執行任務...\n");
        sleep(3);
        printf("[子進程] 任務完成，準備退出\n");
        exit(EXIT_SUCCESS);
    }
    else {
        // 父進程：正確使用 wait() 回收子進程
        printf("[父進程 PID=%d] 創建了子進程 %d\n", getpid(), pid);
        printf("[父進程] 繼續做其他工作...\n");
        sleep(1);

        printf("[父進程] 準備等待子進程結束...\n");

        /*
         * 方法 1: 阻塞等待（wait）
         * - 優點：簡單直接
         * - 缺點：父進程會阻塞
         */
        int status;
        pid_t child_pid = wait(&status);

        printf("[父進程] 子進程 %d 已結束並被回收\n", child_pid);

        if (WIFEXITED(status)) {
            printf("[父進程] 子進程正常退出，退出碼: %d\n", WEXITSTATUS(status));
        }

        printf("[父進程] 沒有殭屍進程產生！\n");

        // 演示方法 2: 非阻塞等待
        printf("\n--- 演示非阻塞等待 (WNOHANG) ---\n");

        pid = fork();
        if (pid == 0) {
            printf("[子進程2] 睡眠 5 秒...\n");
            sleep(5);
            exit(0);
        } else {
            printf("[父進程] 使用 WNOHANG 輪詢子進程狀態...\n");
            int count = 0;
            while (1) {
                pid_t result = waitpid(pid, &status, WNOHANG);
                if (result == 0) {
                    // 子進程還在運行
                    printf("[父進程] 輪詢 %d: 子進程還在運行，繼續做其他工作...\n", ++count);
                    sleep(1);
                } else if (result == pid) {
                    // 子進程已結束
                    printf("[父進程] 子進程已結束並回收\n");
                    break;
                } else {
                    perror("waitpid error");
                    break;
                }
            }
        }
    }
}

/*
 * 孤兒進程演示
 *
 * 什麼是孤兒進程？
 * - 父進程先於子進程退出
 * - 子進程失去了父進程
 * - 會被 init 進程（PID=1）收養
 *
 * 特點：
 * - 不會造成資源浪費
 * - init 會自動回收孤兒進程
 * - PPID 變為 1
 */
void demo_orphan(void)
{
    pid_t pid;

    printf("\n【孤兒進程演示】\n\n");

    pid = fork();

    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) {
        // 子進程：父進程退出後繼續運行
        printf("[子進程 PID=%d] 開始執行\n", getpid());
        printf("[子進程] 當前父進程 PPID=%d\n", getppid());

        printf("[子進程] 睡眠 3 秒，期間父進程會退出...\n");
        sleep(3);

        /*
         * 當父進程退出後，子進程的 PPID 會變為 1
         * 表示被 init 進程收養
         */
        printf("\n[子進程] 醒來！檢查父進程...\n");
        printf("[子進程] 當前父進程 PPID=%d\n", getppid());

        if (getppid() == 1) {
            printf("[子進程] 我已經是孤兒進程了！\n");
            printf("[子進程] 我被 init 進程（PID=1）收養\n");
        }

        printf("[子進程] 繼續執行任務...\n");
        sleep(2);
        printf("[子進程] 任務完成，退出\n");
        printf("[子進程] init 會自動回收我，不會變成殭屍\n");

        exit(EXIT_SUCCESS);
    }
    else {
        // 父進程：快速退出，讓子進程成為孤兒
        printf("[父進程 PID=%d] 創建了子進程 %d\n", getpid(), pid);
        printf("[父進程] 我將立即退出，子進程會成為孤兒\n");
        printf("[父進程] 退出後，子進程的 PPID 會變為 1\n");

        sleep(1);
        printf("[父進程] 現在退出...\n");

        // 父進程退出，不等待子進程
        exit(EXIT_SUCCESS);
    }
}

/*
 * 知識點總結：
 *
 * ╔═══════════════════════════════════════════════════════════╗
 * ║                    殭屍進程 vs 孤兒進程                   ║
 * ╠═══════════════════════════════════════════════════════════╣
 * ║ 特性         │ 殭屍進程 (Zombie)  │ 孤兒進程 (Orphan)    ║
 * ╠══════════════╪════════════════════╪═════════════════════╣
 * ║ 定義         │ 子進程已退出       │ 父進程先退出         ║
 * ║              │ 但未被父進程回收   │                      ║
 * ╠══════════════╪════════════════════╪═════════════════════╣
 * ║ 狀態         │ Z (Zombie)         │ 正常運行狀態         ║
 * ╠══════════════╪════════════════════╪═════════════════════╣
 * ║ PPID         │ 原父進程 PID       │ 1 (init/systemd)     ║
 * ╠══════════════╪════════════════════╪═════════════════════╣
 * ║ 是否運行     │ 否，已結束         │ 是，繼續運行         ║
 * ╠══════════════╪════════════════════╪═════════════════════╣
 * ║ 資源佔用     │ 進程表項（PCB）    │ 正常進程資源         ║
 * ╠══════════════╪════════════════════╪═════════════════════╣
 * ║ 危害         │ 耗盡進程表         │ 無危害               ║
 * ╠══════════════╪════════════════════╪═════════════════════╣
 * ║ 能否 kill    │ 否，已經死了       │ 可以                 ║
 * ╠══════════════╪════════════════════╪═════════════════════╣
 * ║ 如何清除     │ 父進程調用 wait()  │ init 自動回收        ║
 * ║              │ 或殺死父進程       │                      ║
 * ╚═══════════════════════════════════════════════════════════╝
 *
 * 避免殭屍進程的最佳實踐：
 *
 * 1. 及時調用 wait/waitpid：
 *    pid_t pid = wait(&status);
 *
 * 2. 使用信號處理器（適合不確定子進程何時結束的情況）：
 *    signal(SIGCHLD, SIG_IGN);  // 忽略子進程退出信號
 *
 * 3. 在信號處理器中調用 waitpid：
 *    void sigchld_handler(int sig) {
 *        while (waitpid(-1, NULL, WNOHANG) > 0);
 *    }
 *    signal(SIGCHLD, sigchld_handler);
 *
 * 4. 兩次 fork() 技巧：
 *    fork()       // 第一次
 *    if (pid == 0) {
 *        fork()   // 第二次
 *        if (pid > 0) exit(0);  // 第一個子進程退出
 *        // 孫進程成為孤兒，由 init 回收
 *    }
 *    wait();      // 回收第一個子進程
 *
 * 查看殭屍進程的命令：
 *
 * # 方法1: 查看進程狀態
 * ps aux | grep Z
 *
 * # 方法2: 查看具體信息
 * ps -eo pid,ppid,stat,cmd | grep Z
 *
 * # 方法3: 統計殭屍進程數量
 * ps aux | awk '{if($8=="Z") print $0}' | wc -l
 *
 * 清除殭屍進程：
 *
 * 1. 讓父進程調用 wait()（最好的方式）
 * 2. 殺死父進程：kill -9 <parent_pid>
 *    （父進程被殺後，殭屍進程會被 init 收養並清除）
 * 3. 重啟系統（最後的手段，不推薦）
 */
