/*
 * 檔案名稱: signal_demo.c
 * 功能說明: 信號 (Signal) 處理演示
 *
 * 知識點:
 *   1. signal() 註冊信號處理器
 *   2. 常用信號類型 (SIGINT, SIGTERM, SIGUSR1 等)
 *   3. 自定義信號處理函數
 *   4. kill() 發送信號
 *   5. 信號的異步特性
 *
 * 編譯方式: gcc -o signal_demo signal_demo.c
 * 執行方式: ./signal_demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>

// 全局變量，用於在信號處理器和主程序間通訊
volatile sig_atomic_t keep_running = 1;
volatile sig_atomic_t usr1_count = 0;

/*
 * SIGINT 信號處理器 (Ctrl+C)
 *
 * 重要概念：
 * 1. 信號處理器應該盡量簡短
 * 2. 只使用異步信號安全 (async-signal-safe) 的函數
 * 3. 使用 volatile sig_atomic_t 類型保證原子性
 */
void sigint_handler(int signum)
{
    // 注意：printf 不是異步信號安全的，這裡僅用於演示
    // 生產環境應該使用 write() 系統調用
    printf("\n[信號] 收到 SIGINT (信號 %d)，準備退出...\n", signum);
    keep_running = 0;
}

/*
 * SIGUSR1 信號處理器 (用戶自定義信號 1)
 */
void sigusr1_handler(int signum)
{
    (void)signum;  // 明確標記未使用
    usr1_count++;
    printf("\n[信號] 收到 SIGUSR1 (第 %d 次)\n", usr1_count);
}

/*
 * SIGTERM 信號處理器 (終止信號)
 */
void sigterm_handler(int signum)
{
    (void)signum;  // 明確標記未使用
    printf("\n[信號] 收到 SIGTERM，執行清理工作...\n");
    printf("[信號] 清理完成，退出程序\n");
    exit(EXIT_SUCCESS);
}

/*
 * SIGALRM 信號處理器 (鬧鐘信號)
 */
void sigalrm_handler(int signum)
{
    (void)signum;  // 明確標記未使用
    static int alarm_count = 0;
    alarm_count++;
    printf("[定時器] 鬧鐘響了！(第 %d 次)\n", alarm_count);

    // 重新設置鬧鐘（3 秒後再次觸發）
    if (alarm_count < 5) {
        alarm(3);
    }
}

int main(void)
{
    pid_t pid = getpid();

    printf("====== 信號處理演示 ======\n");
    printf("程序 PID: %d\n\n", pid);

    /*
     * signal() - 註冊信號處理器
     *
     * void (*signal(int signum, void (*handler)(int)))(int);
     *
     * 參數：
     *   signum: 信號編號
     *   handler: 處理函數指針，也可以是：
     *            SIG_DFL - 默認處理
     *            SIG_IGN - 忽略信號
     *
     * 返回值：
     *   成功返回之前的處理器，失敗返回 SIG_ERR
     */

    // 註冊 SIGINT 處理器 (Ctrl+C)
    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        perror("signal(SIGINT) failed");
        exit(EXIT_FAILURE);
    }
    printf("[設置] SIGINT 處理器已註冊 (按 Ctrl+C 測試)\n");

    // 註冊 SIGUSR1 處理器
    if (signal(SIGUSR1, sigusr1_handler) == SIG_ERR) {
        perror("signal(SIGUSR1) failed");
        exit(EXIT_FAILURE);
    }
    printf("[設置] SIGUSR1 處理器已註冊\n");

    // 註冊 SIGTERM 處理器
    signal(SIGTERM, sigterm_handler);
    printf("[設置] SIGTERM 處理器已註冊\n");

    // 註冊 SIGALRM 處理器並設置鬧鐘
    signal(SIGALRM, sigalrm_handler);
    alarm(3);  // 3 秒後觸發 SIGALRM
    printf("[設置] SIGALRM 處理器已註冊，3 秒後觸發\n");

    printf("\n====== 測試說明 ======\n");
    printf("1. 按 Ctrl+C 發送 SIGINT 信號\n");
    printf("2. 在另一個終端執行：kill -USR1 %d\n", pid);
    printf("3. 在另一個終端執行：kill -TERM %d\n", pid);
    printf("4. 等待 3 秒觀察 SIGALRM 定時觸發\n");
    printf("====================\n\n");

    // 主循環
    int count = 0;
    while (keep_running) {
        printf("[主循環] 運行中... (計數: %d)\n", ++count);
        sleep(2);

        // 每 10 秒自己給自己發送一個 SIGUSR1
        if (count % 5 == 0) {
            printf("[自發信號] 發送 SIGUSR1 給自己\n");
            kill(pid, SIGUSR1);
        }
    }

    printf("\n[退出] 主循環結束\n");
    printf("[統計] SIGUSR1 共收到 %d 次\n", usr1_count);
    printf("程序正常結束\n");

    return 0;
}

/*
 * 常用信號列表：
 *
 * ┌────────┬─────────────────┬─────────────────────────┐
 * │ 信號   │ 默認動作        │ 說明                    │
 * ├────────┼─────────────────┼─────────────────────────┤
 * │ SIGHUP │ 終止            │ 終端掛起或控制進程死亡   │
 * │ SIGINT │ 終止            │ 中斷 (Ctrl+C)           │
 * │ SIGQUIT│ 終止+core dump  │ 退出 (Ctrl+\)           │
 * │ SIGILL │ 終止+core dump  │ 非法指令                │
 * │ SIGABRT│ 終止+core dump  │ abort() 調用            │
 * │ SIGFPE │ 終止+core dump  │ 浮點異常                │
 * │ SIGKILL│ 終止            │ 強制終止（不可捕獲）     │
 * │ SIGSEGV│ 終止+core dump  │ 段錯誤                  │
 * │ SIGPIPE│ 終止            │ 管道破裂                │
 * │ SIGALRM│ 終止            │ alarm() 鬧鐘            │
 * │ SIGTERM│ 終止            │ 終止信號                │
 * │ SIGUSR1│ 終止            │ 用戶自定義信號 1        │
 * │ SIGUSR2│ 終止            │ 用戶自定義信號 2        │
 * │ SIGCHLD│ 忽略            │ 子進程狀態改變          │
 * │ SIGSTOP│ 停止            │ 停止進程（不可捕獲）     │
 * │ SIGCONT│ 繼續            │ 繼續執行                │
 * └────────┴─────────────────┴─────────────────────────┘
 *
 * 不可捕獲的信號：SIGKILL (9), SIGSTOP (19)
 *
 * 發送信號的方法：
 *
 * 1. 命令行：
 *    kill -SIGINT <pid>  或  kill -2 <pid>
 *    kill -SIGUSR1 <pid> 或  kill -10 <pid>
 *    kill -9 <pid>       # 強制殺死
 *
 * 2. 代碼：
 *    kill(pid, SIGINT);
 *    raise(SIGINT);  # 給自己發送信號
 *
 * 3. 鍵盤：
 *    Ctrl+C  -> SIGINT
 *    Ctrl+\  -> SIGQUIT
 *    Ctrl+Z  -> SIGTSTP (停止)
 *
 * 信號處理最佳實踐：
 *
 * 1. 使用 sigaction() 而非 signal()（更可靠）
 * 2. 處理器中只使用異步信號安全的函數
 * 3. 使用 volatile sig_atomic_t 類型
 * 4. 處理器應該簡短快速
 * 5. 避免在處理器中調用 malloc, printf 等函數
 *
 * 異步信號安全的函數（部分）：
 * - write()
 * - _exit()
 * - signal()
 * - kill()
 * - getpid()
 *
 * 不安全的函數（部分）：
 * - printf(), fprintf()
 * - malloc(), free()
 * - exit()
 *
 * 信號屏蔽（Signal Masking）：
 *
 * sigset_t set;
 * sigemptyset(&set);
 * sigaddset(&set, SIGINT);
 * sigprocmask(SIG_BLOCK, &set, NULL);    // 阻塞 SIGINT
 * // 執行關鍵代碼...
 * sigprocmask(SIG_UNBLOCK, &set, NULL);  // 解除阻塞
 */
