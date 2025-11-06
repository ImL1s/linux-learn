/*
 * 檔案名稱: fork_basic.c
 * 功能說明: 演示 fork() 系統調用的基本用法
 *
 * 知識點:
 *   1. fork() 創建子進程
 *   2. 父子進程的區分
 *   3. 進程 ID (PID) 的獲取
 *   4. 父子進程的執行順序
 *   5. wait() 避免殭屍進程
 *
 * 編譯方式: gcc -o fork_basic fork_basic.c
 * 執行方式: ./fork_basic
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void)
{
    pid_t pid;
    int count = 0;  // 用於觀察父子進程的變量獨立性

    printf("====== Fork 基本演示 ======\n");
    printf("程序開始，當前進程 PID: %d\n", getpid());
    printf("準備調用 fork()...\n\n");

    /*
     * fork() 系統調用：
     * - 創建一個新進程（子進程）
     * - 子進程是父進程的副本
     * - 返回值：
     *   > 0: 在父進程中，返回子進程的 PID
     *   = 0: 在子進程中
     *   < 0: fork 失敗
     */
    pid = fork();

    if (pid < 0) {
        // fork 失敗
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) {
        /*
         * 子進程執行的代碼
         *
         * 重要概念：
         * - 子進程擁有父進程的副本（包括代碼、數據、堆棧等）
         * - 子進程和父進程有獨立的內存空間
         * - 修改變量不會影響對方
         */
        printf("【子進程】\n");
        printf("  - 我的 PID: %d\n", getpid());
        printf("  - 我的父進程 PID: %d\n", getppid());
        printf("  - fork() 返回值: %d\n", pid);
        printf("  - count 初始值: %d\n", count);

        // 修改 count，觀察變量獨立性
        for (int i = 0; i < 3; i++) {
            count++;
            printf("  - [子進程] count = %d\n", count);
            sleep(1);
        }

        printf("  - [子進程] 執行完畢，準備退出\n");
        exit(EXIT_SUCCESS);  // 子進程退出
    }
    else {
        /*
         * 父進程執行的代碼
         *
         * pid > 0，存儲的是子進程的 PID
         */
        printf("【父進程】\n");
        printf("  - 我的 PID: %d\n", getpid());
        printf("  - 我的父進程 PID: %d\n", getppid());
        printf("  - fork() 返回值（子進程 PID）: %d\n", pid);
        printf("  - count 初始值: %d\n", count);

        // 修改 count，觀察變量獨立性
        for (int i = 0; i < 3; i++) {
            count += 10;
            printf("  - [父進程] count = %d\n", count);
            sleep(1);
        }

        /*
         * wait() 或 waitpid() 的重要性：
         *
         * 1. 回收子進程資源
         * 2. 獲取子進程的退出狀態
         * 3. 避免產生殭屍進程
         *
         * 如果父進程不調用 wait()，子進程結束後會變成殭屍進程，
         * 佔用系統資源（進程表項），直到父進程結束。
         */
        printf("  - [父進程] 等待子進程結束...\n");
        int status;
        pid_t child_pid = wait(&status);

        // 檢查子進程退出狀態
        if (WIFEXITED(status)) {
            printf("  - [父進程] 子進程 %d 正常退出，退出碼: %d\n",
                   child_pid, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("  - [父進程] 子進程 %d 被信號 %d 終止\n",
                   child_pid, WTERMSIG(status));
        }

        printf("  - [父進程] 最終 count = %d\n", count);
        printf("  - [父進程] 執行完畢\n");
    }

    /*
     * 這行代碼會被父子進程都執行
     * 因為 fork() 後，兩個進程都會繼續執行後續代碼
     */
    printf("\n[PID %d] 到達程序末尾\n", getpid());

    return 0;
}

/*
 * 輸出分析：
 *
 * 1. fork() 之前的代碼只執行一次
 * 2. fork() 之後的代碼會被父子進程各執行一次
 * 3. 父子進程的 count 變量互不影響，證明了內存獨立性
 * 4. 父子進程的執行順序是不確定的（由操作系統調度）
 *
 * 關鍵概念：
 *
 * 1. 寫時複製 (Copy-On-Write, COW)：
 *    - fork() 並不立即複製整個內存空間
 *    - 父子進程共享相同的物理內存頁面
 *    - 只有在寫入時才真正複製（節省內存和時間）
 *
 * 2. 文件描述符繼承：
 *    - 子進程繼承父進程打開的文件描述符
 *    - 父子進程共享文件偏移量
 *
 * 3. 進程屬性：
 *    子進程繼承：           子進程不繼承：
 *    - 代碼段              - PID
 *    - 堆棧                - 父進程 PID (ppid 不同)
 *    - 數據段              - 文件鎖
 *    - 環境變量            - 掛起的信號
 *    - 打開的文件描述符    - 定時器
 *
 * 常見錯誤：
 *
 * 1. 忘記檢查 fork() 返回值
 * 2. 忘記調用 wait() 回收子進程
 * 3. 誤以為父子進程共享變量
 *
 * 擴展實驗：
 *
 * 1. 連續調用多次 fork()，觀察進程數量的變化
 * 2. 在 fork() 前後打開文件，觀察文件描述符的共享
 * 3. 使用 getpid() 和 getppid() 構建進程樹
 */
