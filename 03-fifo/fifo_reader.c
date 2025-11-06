/*
 * 檔案名稱: fifo_reader.c
 * 功能說明: FIFO (命名管道) 讀取端演示
 *
 * 知識點:
 *   1. 打開已存在的 FIFO
 *   2. 從 FIFO 讀取數據
 *   3. 檢測寫入端關閉（EOF）
 *   4. 循環讀取多個消息
 *
 * 編譯方式: gcc -o fifo_reader fifo_reader.c
 * 執行方式: ./fifo_reader
 *
 * 使用說明:
 *   1. 先運行 fifo_writer 創建 FIFO
 *   2. 再運行此程式接收數據
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
    ssize_t bytes_read;
    int msg_count = 0;

    printf("====== FIFO 讀取端演示 ======\n\n");

    /*
     * 檢查 FIFO 是否存在
     *
     * access() 函數：
     * - 檢查文件是否存在及訪問權限
     * - F_OK: 檢查文件是否存在
     * - R_OK: 檢查讀權限
     * - W_OK: 檢查寫權限
     * - X_OK: 檢查執行權限
     */
    if (access(FIFO_PATH, F_OK) == -1) {
        printf("[錯誤] FIFO 不存在: %s\n", FIFO_PATH);
        printf("[提示] 請先運行 fifo_writer 創建 FIFO\n");
        exit(EXIT_FAILURE);
    }

    printf("[檢測] FIFO 存在: %s\n", FIFO_PATH);

    /*
     * 檢查文件類型
     * 確保是 FIFO 而不是普通文件
     */
    struct stat fifo_stat;
    if (stat(FIFO_PATH, &fifo_stat) == -1) {
        perror("stat failed");
        exit(EXIT_FAILURE);
    }

    if (!S_ISFIFO(fifo_stat.st_mode)) {
        printf("[錯誤] %s 不是 FIFO 文件\n", FIFO_PATH);
        exit(EXIT_FAILURE);
    }

    printf("[確認] %s 是有效的 FIFO\n", FIFO_PATH);
    printf("\n等待打開 FIFO...\n");
    printf("提示：需要等待寫入端先打開 FIFO\n");

    /*
     * 打開 FIFO 進行讀取
     *
     * O_RDONLY: 只讀模式
     * - 默認會阻塞，直到有寫入端打開
     *
     * 如果要避免阻塞，可以使用：
     * fd = open(FIFO_PATH, O_RDONLY | O_NONBLOCK);
     */
    fd = open(FIFO_PATH, O_RDONLY);
    if (fd == -1) {
        perror("open FIFO failed");
        exit(EXIT_FAILURE);
    }

    printf("\n[成功] FIFO 已打開，開始接收數據\n");
    printf("============================\n\n");

    /*
     * 循環讀取數據
     *
     * read() 的返回值：
     * > 0: 成功讀取的字節數
     * = 0: EOF（所有寫入端都已關閉）
     * < 0: 發生錯誤
     */
    printf("等待接收消息...\n");
    printf("（按 Ctrl+C 退出）\n\n");

    while (1) {
        // 清空緩衝區
        memset(buffer, 0, BUFFER_SIZE);

        // 從 FIFO 讀取數據
        bytes_read = read(fd, buffer, BUFFER_SIZE - 1);

        if (bytes_read > 0) {
            /*
             * 成功讀取數據
             */
            msg_count++;
            printf("┌─ 消息 #%d ─────────────────\n", msg_count);
            printf("│ 長度: %ld 字節\n", bytes_read);
            printf("│ 內容: %s\n", buffer);
            printf("└────────────────────────────\n\n");

            // 檢查是否是退出消息
            if (strcmp(buffer, "quit") == 0) {
                printf("收到退出消息，準備關閉\n");
                break;
            }
        }
        else if (bytes_read == 0) {
            /*
             * EOF - 所有寫入端都已關閉
             *
             * 當所有寫入端關閉 FIFO 時，read() 返回 0
             * 這是檢測寫入端關閉的標準方法
             */
            printf("\n[檢測到] 所有寫入端已關閉 (EOF)\n");
            printf("總共接收了 %d 條消息\n", msg_count);
            break;
        }
        else {
            /*
             * 讀取錯誤
             */
            if (errno == EINTR) {
                // 被信號中斷，繼續讀取
                printf("讀取被信號中斷，繼續...\n");
                continue;
            } else {
                perror("read failed");
                break;
            }
        }
    }

    /*
     * 清理工作
     */
    printf("\n====== 清理資源 ======\n");

    // 關閉文件描述符
    close(fd);
    printf("已關閉 FIFO 文件描述符\n");

    printf("\n程序結束\n");
    printf("注意：FIFO 文件由 writer 負責刪除\n");

    return 0;
}

/*
 * 進階技巧：
 *
 * 1. 非阻塞讀取：
 *
 *    fd = open(FIFO_PATH, O_RDONLY | O_NONBLOCK);
 *    while (1) {
 *        bytes_read = read(fd, buffer, BUFFER_SIZE);
 *        if (bytes_read == -1 && errno == EAGAIN) {
 *            // 沒有數據可讀，做其他工作
 *            usleep(100000);  // 休眠 100ms
 *            continue;
 *        }
 *        // 處理數據...
 *    }
 *
 * 2. 使用 select() 多路複用：
 *
 *    fd_set readfds;
 *    struct timeval timeout;
 *
 *    while (1) {
 *        FD_ZERO(&readfds);
 *        FD_SET(fd, &readfds);
 *        timeout.tv_sec = 5;   // 5 秒超時
 *        timeout.tv_usec = 0;
 *
 *        int ret = select(fd + 1, &readfds, NULL, NULL, &timeout);
 *        if (ret > 0 && FD_ISSET(fd, &readfds)) {
 *            // 有數據可讀
 *            bytes_read = read(fd, buffer, BUFFER_SIZE);
 *            // 處理數據...
 *        } else if (ret == 0) {
 *            printf("超時，沒有數據\n");
 *        }
 *    }
 *
 * 3. 同時監聽多個 FIFO：
 *
 *    int fd1 = open("/tmp/fifo1", O_RDONLY | O_NONBLOCK);
 *    int fd2 = open("/tmp/fifo2", O_RDONLY | O_NONBLOCK);
 *
 *    fd_set readfds;
 *    while (1) {
 *        FD_ZERO(&readfds);
 *        FD_SET(fd1, &readfds);
 *        FD_SET(fd2, &readfds);
 *
 *        int maxfd = (fd1 > fd2) ? fd1 : fd2;
 *        int ret = select(maxfd + 1, &readfds, NULL, NULL, NULL);
 *
 *        if (FD_ISSET(fd1, &readfds)) {
 *            // 從 fifo1 讀取
 *        }
 *        if (FD_ISSET(fd2, &readfds)) {
 *            // 從 fifo2 讀取
 *        }
 *    }
 *
 * 4. 錯誤處理最佳實踐：
 *
 *    while (1) {
 *        bytes_read = read(fd, buffer, BUFFER_SIZE);
 *
 *        if (bytes_read > 0) {
 *            // 處理數據
 *        } else if (bytes_read == 0) {
 *            // EOF
 *            break;
 *        } else {
 *            // 錯誤
 *            if (errno == EINTR) {
 *                // 被信號中斷，重試
 *                continue;
 *            } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
 *                // 非阻塞模式，暫無數據
 *                continue;
 *            } else {
 *                // 其他錯誤
 *                perror("read error");
 *                break;
 *            }
 *        }
 *    }
 *
 * 5. 雙向通訊實現：
 *
 *    創建兩個 FIFO：
 *    - /tmp/fifo_request  (client -> server)
 *    - /tmp/fifo_response (server -> client)
 *
 *    客戶端：
 *    - 向 fifo_request 寫入
 *    - 從 fifo_response 讀取
 *
 *    服務器：
 *    - 從 fifo_request 讀取
 *    - 向 fifo_response 寫入
 *
 * 6. 實際應用示例：
 *
 *    簡單的日誌收集系統：
 *
 *    // logger.c (多個進程寫入日誌)
 *    fd = open("/tmp/log_fifo", O_WRONLY);
 *    write(fd, log_message, strlen(log_message));
 *
 *    // log_server.c (收集並保存日誌)
 *    fd = open("/tmp/log_fifo", O_RDONLY);
 *    while (read(fd, buffer, SIZE) > 0) {
 *        fprintf(log_file, "%s\n", buffer);
 *        fflush(log_file);
 *    }
 *
 * 常見問題：
 *
 * Q: FIFO 和 socket 相比有什麼優缺點？
 * A: FIFO 優點：簡單、輕量、無需網絡協議
 *    FIFO 缺點：只能本地、單向、無消息邊界
 *    Socket：支持網絡、雙向、更靈活，但更複雜
 *
 * Q: 如何確保寫入的原子性？
 * A: 單次寫入不超過 PIPE_BUF (通常 4096 字節) 可保證原子性
 *    超過此大小可能被分割成多次寫入
 *
 * Q: FIFO 是否支持多個讀者/寫者？
 * A: 支持，但需要注意：
 *    - 多個寫者：數據可能交錯（除非每次寫入 < PIPE_BUF）
 *    - 多個讀者：數據被隨機分配給不同讀者
 */
