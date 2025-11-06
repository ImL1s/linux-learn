/*
 * 檔案名稱: fifo_writer.c
 * 功能說明: FIFO (命名管道) 寫入端演示
 *
 * 知識點:
 *   1. mkfifo() 創建命名管道
 *   2. 命名管道可用於無親緣關係的進程通訊
 *   3. FIFO 是文件系統中的特殊文件
 *   4. 阻塞與非阻塞模式
 *
 * 編譯方式: gcc -o fifo_writer fifo_writer.c
 * 執行方式: ./fifo_writer
 *
 * 使用說明:
 *   1. 先運行此程式 (writer)
 *   2. 再運行 fifo_reader 接收數據
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#define FIFO_PATH "/tmp/my_fifo"
#define BUFFER_SIZE 256

int main(void)
{
    int fd;
    char buffer[BUFFER_SIZE];

    printf("====== FIFO 寫入端演示 ======\n\n");

    /*
     * mkfifo() - 創建命名管道
     *
     * int mkfifo(const char *pathname, mode_t mode);
     *
     * 參數：
     *   pathname: FIFO 文件的路徑
     *   mode: 文件權限 (類似於 open() 的權限)
     *
     * 返回值：
     *   成功返回 0，失敗返回 -1
     *
     * 注意：
     *   - FIFO 是文件系統中的特殊文件（類型為 p）
     *   - 可以用 ls -l 查看，顯示為 prw-r--r--
     *   - 進程結束後 FIFO 文件不會自動刪除
     */
    if (mkfifo(FIFO_PATH, 0666) == -1) {
        if (errno == EEXIST) {
            printf("[注意] FIFO 已存在，繼續使用: %s\n", FIFO_PATH);
        } else {
            perror("mkfifo failed");
            exit(EXIT_FAILURE);
        }
    } else {
        printf("[成功] 創建 FIFO: %s\n", FIFO_PATH);
    }

    printf("\n提示：\n");
    printf("  1. FIFO 已創建，可用 'ls -l %s' 查看\n", FIFO_PATH);
    printf("  2. 現在需要打開另一個終端運行 fifo_reader\n");
    printf("  3. 讀取端打開後，寫入端才能繼續\n");
    printf("\n等待讀取端打開 FIFO...\n");

    /*
     * open() - 打開 FIFO
     *
     * 重要概念：FIFO 的阻塞特性
     *
     * 1. 以只寫模式打開 FIFO (O_WRONLY)：
     *    - 默認情況下會阻塞，直到有進程以讀模式打開同一個 FIFO
     *    - 可以使用 O_NONBLOCK 標誌避免阻塞
     *
     * 2. 以只讀模式打開 FIFO (O_RDONLY)：
     *    - 默認情況下會阻塞，直到有進程以寫模式打開同一個 FIFO
     *
     * 3. 讀寫模式 (O_RDWR)：
     *    - 不會阻塞，但不符合 POSIX 標準，不推薦使用
     */
    fd = open(FIFO_PATH, O_WRONLY);
    if (fd == -1) {
        perror("open FIFO failed");
        exit(EXIT_FAILURE);
    }

    printf("\n[成功] FIFO 已打開，讀取端已就緒\n");
    printf("============================\n\n");

    /*
     * 交互式寫入
     * 用戶可以輸入多行消息，發送給讀取端
     */
    printf("現在可以輸入消息發送給讀取端\n");
    printf("輸入 'quit' 退出程式\n\n");

    while (1) {
        printf("請輸入消息: ");
        fflush(stdout);

        // 讀取用戶輸入
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            printf("\n讀取輸入失敗\n");
            break;
        }

        // 移除換行符
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[len-1] = '\0';
            len--;
        }

        // 檢查退出命令
        if (strcmp(buffer, "quit") == 0) {
            printf("\n用戶請求退出\n");
            break;
        }

        // 寫入 FIFO
        ssize_t bytes_written = write(fd, buffer, len + 1);  // +1 包含 '\0'

        if (bytes_written == -1) {
            perror("write failed");
            break;
        } else if (bytes_written == 0) {
            printf("讀取端已關閉\n");
            break;
        } else {
            printf("  → 已發送 %ld 字節\n\n", bytes_written);
        }
    }

    /*
     * 清理工作
     */
    printf("\n====== 清理資源 ======\n");

    // 關閉文件描述符
    close(fd);
    printf("已關閉 FIFO 文件描述符\n");

    // 刪除 FIFO 文件
    if (unlink(FIFO_PATH) == 0) {
        printf("已刪除 FIFO 文件: %s\n", FIFO_PATH);
    } else {
        perror("unlink FIFO failed");
    }

    printf("\n程序結束\n");
    return 0;
}

/*
 * 知識點總結：
 *
 * 1. FIFO vs 匿名管道：
 *
 *    ┌──────────────┬─────────────────┬──────────────────┐
 *    │ 特性         │ 匿名管道 (Pipe) │ 命名管道 (FIFO)  │
 *    ├──────────────┼─────────────────┼──────────────────┤
 *    │ 文件名       │ 無              │ 有 (在文件系統中)│
 *    │ 進程關係     │ 必須有親緣關係  │ 可以無親緣關係   │
 *    │ 創建方式     │ pipe()          │ mkfifo()         │
 *    │ 通訊方向     │ 單向            │ 單向             │
 *    │ 生命週期     │ 進程結束即消失  │ 手動刪除         │
 *    └──────────────┴─────────────────┴──────────────────┘
 *
 * 2. FIFO 的使用場景：
 *    - 客戶端-服務器架構的本地通訊
 *    - 無親緣關係進程間的數據傳輸
 *    - Shell 腳本間的數據交換
 *    - 簡單的消息隊列
 *
 * 3. FIFO 的阻塞行為：
 *
 *    寫入端 (O_WRONLY):
 *    - 阻塞直到有讀取端打開
 *    - 或使用 O_NONBLOCK 標誌
 *
 *    讀取端 (O_RDONLY):
 *    - 阻塞直到有寫入端打開
 *    - 或使用 O_NONBLOCK 標誌
 *
 *    示例：非阻塞模式
 *    fd = open(FIFO_PATH, O_WRONLY | O_NONBLOCK);
 *
 * 4. FIFO 的限制：
 *    - 單向通訊（雙向需要兩個 FIFO）
 *    - 無消息邊界（字節流）
 *    - 容量有限（同管道，通常 64KB）
 *    - 原子寫入限制（PIPE_BUF，通常 4KB）
 *
 * 5. 常見錯誤：
 *    - 忘記檢查 EEXIST 錯誤
 *    - 忘記刪除 FIFO 文件（unlink）
 *    - 讀寫端順序不當導致死鎖
 *    - 寫入超過 PIPE_BUF 大小的數據可能不是原子操作
 *
 * 6. Shell 中使用 FIFO：
 *
 *    # 創建 FIFO
 *    mkfifo /tmp/my_fifo
 *
 *    # 終端 1: 寫入
 *    echo "Hello" > /tmp/my_fifo
 *
 *    # 終端 2: 讀取
 *    cat /tmp/my_fifo
 *
 *    # 刪除 FIFO
 *    rm /tmp/my_fifo
 *
 * 7. 查看 FIFO：
 *
 *    ls -l /tmp/my_fifo
 *    # 輸出: prw-r--r-- 1 user user 0 Jan 1 12:00 /tmp/my_fifo
 *    # 第一個字符 'p' 表示這是管道 (pipe)
 *
 * 8. 擴展學習：
 *    - 使用 select/poll/epoll 多路複用 FIFO
 *    - 結合信號實現異步 I/O
 *    - 創建雙向通訊（兩個 FIFO）
 */
