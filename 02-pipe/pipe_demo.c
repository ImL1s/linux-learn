/*
 * 檔案名稱: pipe_demo.c
 * 功能說明: 演示 Linux 匿名管道 (Anonymous Pipe) 的基本用法
 *
 * 知識點:
 *   1. pipe() 系統調用創建管道
 *   2. fork() 創建子進程
 *   3. 父子進程通過管道進行單向通訊
 *   4. 管道的讀寫端正確關閉
 *
 * 編譯方式: gcc -o pipe_demo pipe_demo.c
 * 執行方式: ./pipe_demo
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 1024

int main(void)
{
    pid_t pid;
    char buf[BUFFER_SIZE];
    int pipefd[2];  // pipefd[0]: 讀端, pipefd[1]: 寫端
    const char *message = "Hello from parent process! 來自父進程的問候！\n";

    /*
     * 創建管道
     * pipe() 成功返回 0，失敗返回 -1
     * pipefd[0] 是讀取端的文件描述符
     * pipefd[1] 是寫入端的文件描述符
     */
    if (pipe(pipefd) == -1) {
        perror("pipe error");
        exit(EXIT_FAILURE);
    }

    printf("====== 管道創建成功 ======\n");
    printf("讀端文件描述符: %d\n", pipefd[0]);
    printf("寫端文件描述符: %d\n", pipefd[1]);
    printf("========================\n\n");

    /*
     * 創建子進程
     * fork() 返回值:
     *   > 0: 在父進程中，返回子進程的 PID
     *   = 0: 在子進程中
     *   < 0: 創建失敗
     */
    pid = fork();

    if (pid < 0) {
        perror("fork error");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) {  // 子進程代碼
        /*
         * 子進程：負責從管道讀取數據
         *
         * 重要概念：
         * 1. 關閉不需要的寫端，避免資源浪費
         * 2. read() 會阻塞直到有數據可讀
         * 3. 讀取完成後關閉讀端
         */
        printf("[子進程 PID=%d] 開始運行\n", getpid());

        // 關閉寫端（子進程只讀取）
        close(pipefd[1]);

        // 清空緩衝區
        memset(buf, 0, BUFFER_SIZE);

        // 從管道讀取數據
        printf("[子進程] 等待從管道讀取數據...\n");
        ssize_t bytes_read = read(pipefd[0], buf, BUFFER_SIZE);

        if (bytes_read > 0) {
            printf("[子進程] 成功讀取 %ld 字節\n", bytes_read);
            printf("[子進程] 收到的內容: %s", buf);
        } else if (bytes_read == 0) {
            printf("[子進程] 管道已關閉\n");
        } else {
            perror("[子進程] 讀取錯誤");
        }

        // 關閉讀端
        close(pipefd[0]);
        printf("[子進程] 退出\n\n");

        exit(EXIT_SUCCESS);
    }
    else {  // 父進程代碼
        /*
         * 父進程：負責向管道寫入數據
         *
         * 重要概念：
         * 1. 關閉不需要的讀端
         * 2. write() 將數據寫入管道
         * 3. wait() 等待子進程結束，避免殭屍進程
         */
        printf("[父進程 PID=%d] 開始運行，子進程 PID=%d\n", getpid(), pid);

        // 關閉讀端（父進程只寫入）
        close(pipefd[0]);

        // 向管道寫入數據
        printf("[父進程] 準備向管道寫入數據...\n");
        ssize_t bytes_written = write(pipefd[1], message, strlen(message));

        if (bytes_written > 0) {
            printf("[父進程] 成功寫入 %ld 字節\n", bytes_written);
        } else {
            perror("[父進程] 寫入錯誤");
        }

        // 關閉寫端（重要：這樣子進程的 read() 才能返回）
        close(pipefd[1]);

        // 等待子進程結束
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status)) {
            printf("[父進程] 子進程正常退出，退出碼: %d\n", WEXITSTATUS(status));
        }

        printf("[父進程] 退出\n");
    }

    printf("\n====== 程序執行完畢 ======\n");
    return 0;
}

/*
 * 學習要點總結：
 *
 * 1. 管道特性：
 *    - 半雙工通訊（單向）
 *    - 只能用於有親緣關係的進程
 *    - 數據先進先出 (FIFO)
 *    - 內核緩衝區有限（通常 64KB）
 *
 * 2. 使用步驟：
 *    a) 調用 pipe() 創建管道
 *    b) 調用 fork() 創建子進程
 *    c) 父子進程各自關閉不需要的端
 *    d) 進行讀寫操作
 *    e) 關閉使用的端
 *
 * 3. 注意事項：
 *    - 必須在 fork() 之前創建管道
 *    - 及時關閉不使用的端，避免死鎖
 *    - 寫端全部關閉後，讀端 read() 返回 0
 *    - 讀端關閉後，寫端 write() 會收到 SIGPIPE 信號
 *
 * 4. 常見問題：
 *    Q: 為什麼要關閉不使用的端？
 *    A: 避免資源浪費，並且確保讀端能正確檢測到寫端關閉
 *
 *    Q: 如果不調用 wait() 會怎樣？
 *    A: 子進程結束後會變成殭屍進程，浪費系統資源
 *
 * 5. 擴展學習：
 *    - 如果需要雙向通訊，需要創建兩個管道
 *    - 對於無親緣關係的進程，應該使用命名管道 (FIFO)
 *    - popen() 函數封裝了管道的創建和進程控制
 */
