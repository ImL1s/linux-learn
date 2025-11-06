/*
 * 檔案名稱: ipc_benchmark.c
 * 功能說明: IPC 性能對比工具 - 幫助選擇合適的 IPC 機制
 *
 * 測試的 IPC 機制:
 *   1. 匿名管道 (Pipe)
 *   2. FIFO (命名管道)
 *   3. System V 共享內存 (Shared Memory)
 *   4. System V 消息隊列 (Message Queue)
 *   5. Unix Domain Socket
 *
 * 性能指標:
 *   - 吞吐量 (Throughput): MB/s
 *   - 延遲 (Latency): 微秒
 *   - CPU 使用率
 *
 * 編譯方式: gcc -o ipc_benchmark ipc_benchmark.c -lrt
 * 執行方式:
 *   ./ipc_benchmark                    # 運行所有測試
 *   ./ipc_benchmark pipe               # 只測試管道
 *   ./ipc_benchmark --size 1M          # 自定義數據大小
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/time.h>

/*
 * 配置參數
 */
#define DEFAULT_DATA_SIZE (1024 * 1024)  // 1MB
#define DEFAULT_BLOCK_SIZE 4096          // 4KB
#define FIFO_PATH "/tmp/ipc_benchmark_fifo"
#define SOCKET_PATH "/tmp/ipc_benchmark_sock"

/*
 * 測試結果結構
 */
typedef struct {
    char name[32];
    double throughput_mbps;  // MB/s
    double latency_us;       // 微秒
    double time_sec;         // 秒
} benchmark_result_t;

/*
 * 獲取當前時間（微秒）
 */
static inline long long get_time_us(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000000 + tv.tv_usec;
}

/*
 * 格式化大小
 */
void format_size(char *buf, size_t bufsize, size_t bytes)
{
    if (bytes < 1024) {
        snprintf(buf, bufsize, "%zu B", bytes);
    } else if (bytes < 1024 * 1024) {
        snprintf(buf, bufsize, "%.2f KB", bytes / 1024.0);
    } else {
        snprintf(buf, bufsize, "%.2f MB", bytes / 1024.0 / 1024.0);
    }
}

/*
 * 打印測試結果
 */
void print_result(benchmark_result_t *result)
{
    printf("%-20s | %8.2f MB/s | %8.2f us | %6.3f s\n",
           result->name,
           result->throughput_mbps,
           result->latency_us,
           result->time_sec);
}

/*
 * 1. 匿名管道測試
 */
benchmark_result_t test_pipe(size_t data_size, size_t block_size)
{
    benchmark_result_t result = {0};
    strcpy(result.name, "Anonymous Pipe");

    int pipefd[2];
    if (pipe(pipefd) < 0) {
        perror("pipe");
        return result;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
        return result;
    }

    if (pid == 0) {
        // 子進程：讀取數據
        close(pipefd[1]);
        char *buffer = malloc(block_size);
        size_t total = 0;

        while (total < data_size) {
            ssize_t n = read(pipefd[0], buffer, block_size);
            if (n <= 0) break;
            total += n;
        }

        free(buffer);
        close(pipefd[0]);
        exit(0);
    } else {
        // 父進程：寫入數據
        close(pipefd[0]);
        char *buffer = malloc(block_size);
        memset(buffer, 'A', block_size);

        long long start = get_time_us();
        size_t total = 0;

        while (total < data_size) {
            size_t to_write = (data_size - total < block_size) ?
                             (data_size - total) : block_size;
            ssize_t n = write(pipefd[1], buffer, to_write);
            if (n <= 0) break;
            total += n;
        }

        long long end = get_time_us();
        close(pipefd[1]);
        free(buffer);

        waitpid(pid, NULL, 0);

        double time_sec = (end - start) / 1000000.0;
        result.time_sec = time_sec;
        result.throughput_mbps = (data_size / 1024.0 / 1024.0) / time_sec;
        result.latency_us = (end - start) / (double)(data_size / block_size);
    }

    return result;
}

/*
 * 2. FIFO 測試
 */
benchmark_result_t test_fifo(size_t data_size, size_t block_size)
{
    benchmark_result_t result = {0};
    strcpy(result.name, "FIFO (Named Pipe)");

    // 創建 FIFO
    unlink(FIFO_PATH);
    if (mkfifo(FIFO_PATH, 0666) < 0) {
        perror("mkfifo");
        return result;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        unlink(FIFO_PATH);
        return result;
    }

    if (pid == 0) {
        // 子進程：讀取
        int fd = open(FIFO_PATH, O_RDONLY);
        if (fd < 0) {
            perror("open fifo");
            exit(1);
        }

        char *buffer = malloc(block_size);
        size_t total = 0;

        while (total < data_size) {
            ssize_t n = read(fd, buffer, block_size);
            if (n <= 0) break;
            total += n;
        }

        free(buffer);
        close(fd);
        exit(0);
    } else {
        // 父進程：寫入
        int fd = open(FIFO_PATH, O_WRONLY);
        if (fd < 0) {
            perror("open fifo");
            waitpid(pid, NULL, 0);
            unlink(FIFO_PATH);
            return result;
        }

        char *buffer = malloc(block_size);
        memset(buffer, 'B', block_size);

        long long start = get_time_us();
        size_t total = 0;

        while (total < data_size) {
            size_t to_write = (data_size - total < block_size) ?
                             (data_size - total) : block_size;
            ssize_t n = write(fd, buffer, to_write);
            if (n <= 0) break;
            total += n;
        }

        long long end = get_time_us();
        close(fd);
        free(buffer);

        waitpid(pid, NULL, 0);
        unlink(FIFO_PATH);

        double time_sec = (end - start) / 1000000.0;
        result.time_sec = time_sec;
        result.throughput_mbps = (data_size / 1024.0 / 1024.0) / time_sec;
        result.latency_us = (end - start) / (double)(data_size / block_size);
    }

    return result;
}

/*
 * 3. System V 共享內存測試
 */
benchmark_result_t test_shm(size_t data_size, size_t block_size)
{
    benchmark_result_t result = {0};
    strcpy(result.name, "Shared Memory");

    key_t key = ftok("/tmp", 'S');
    int shmid = shmget(key, data_size, IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget");
        return result;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        shmctl(shmid, IPC_RMID, NULL);
        return result;
    }

    if (pid == 0) {
        // 子進程：讀取
        char *shm = (char*)shmat(shmid, NULL, 0);
        if (shm == (char*)-1) {
            perror("shmat");
            exit(1);
        }

        // 等待父進程寫入完成（簡單的同步）
        sleep(1);

        // 驗證數據
        volatile char dummy = shm[data_size - 1];
        (void)dummy;

        shmdt(shm);
        exit(0);
    } else {
        // 父進程：寫入
        char *shm = (char*)shmat(shmid, NULL, 0);
        if (shm == (char*)-1) {
            perror("shmat");
            waitpid(pid, NULL, 0);
            shmctl(shmid, IPC_RMID, NULL);
            return result;
        }

        long long start = get_time_us();

        // 寫入數據
        memset(shm, 'C', data_size);

        long long end = get_time_us();

        shmdt(shm);
        waitpid(pid, NULL, 0);
        shmctl(shmid, IPC_RMID, NULL);

        double time_sec = (end - start) / 1000000.0;
        result.time_sec = time_sec;
        result.throughput_mbps = (data_size / 1024.0 / 1024.0) / time_sec;
        result.latency_us = (end - start) / (double)(data_size / block_size);
    }

    return result;
}

/*
 * 4. System V 消息隊列測試
 */
typedef struct {
    long msg_type;
    char msg_text[8192];
} msg_buf_t;

benchmark_result_t test_msgq(size_t data_size, size_t block_size)
{
    benchmark_result_t result = {0};
    strcpy(result.name, "Message Queue");

    if (block_size > 8192) block_size = 8192;  // 消息隊列限制

    key_t key = ftok("/tmp", 'M');
    int msgid = msgget(key, IPC_CREAT | 0666);
    if (msgid < 0) {
        perror("msgget");
        return result;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        msgctl(msgid, IPC_RMID, NULL);
        return result;
    }

    if (pid == 0) {
        // 子進程：接收
        msg_buf_t msg;
        size_t total = 0;
        size_t msg_count = (data_size + block_size - 1) / block_size;

        for (size_t i = 0; i < msg_count; i++) {
            if (msgrcv(msgid, &msg, block_size, 1, 0) < 0) {
                perror("msgrcv");
                break;
            }
            total += block_size;
        }

        exit(0);
    } else {
        // 父進程：發送
        msg_buf_t msg;
        msg.msg_type = 1;
        memset(msg.msg_text, 'D', block_size);

        long long start = get_time_us();
        size_t total = 0;
        size_t msg_count = (data_size + block_size - 1) / block_size;

        for (size_t i = 0; i < msg_count; i++) {
            if (msgsnd(msgid, &msg, block_size, 0) < 0) {
                perror("msgsnd");
                break;
            }
            total += block_size;
        }

        long long end = get_time_us();

        waitpid(pid, NULL, 0);
        msgctl(msgid, IPC_RMID, NULL);

        double time_sec = (end - start) / 1000000.0;
        result.time_sec = time_sec;
        result.throughput_mbps = (total / 1024.0 / 1024.0) / time_sec;
        result.latency_us = (end - start) / (double)msg_count;
    }

    return result;
}

/*
 * 5. Unix Domain Socket 測試
 */
benchmark_result_t test_unix_socket(size_t data_size, size_t block_size)
{
    benchmark_result_t result = {0};
    strcpy(result.name, "Unix Domain Socket");

    unlink(SOCKET_PATH);

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return result;
    }

    if (pid == 0) {
        // 子進程：服務器
        sleep(1);  // 等待父進程創建 socket

        int sock = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock < 0) {
            perror("socket");
            exit(1);
        }

        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strcpy(addr.sun_path, SOCKET_PATH);

        if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("connect");
            close(sock);
            exit(1);
        }

        char *buffer = malloc(block_size);
        size_t total = 0;

        while (total < data_size) {
            ssize_t n = recv(sock, buffer, block_size, 0);
            if (n <= 0) break;
            total += n;
        }

        free(buffer);
        close(sock);
        exit(0);
    } else {
        // 父進程：客戶端
        int listen_sock = socket(AF_UNIX, SOCK_STREAM, 0);
        if (listen_sock < 0) {
            perror("socket");
            waitpid(pid, NULL, 0);
            return result;
        }

        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strcpy(addr.sun_path, SOCKET_PATH);

        if (bind(listen_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("bind");
            close(listen_sock);
            waitpid(pid, NULL, 0);
            return result;
        }

        listen(listen_sock, 1);

        int sock = accept(listen_sock, NULL, NULL);
        if (sock < 0) {
            perror("accept");
            close(listen_sock);
            waitpid(pid, NULL, 0);
            unlink(SOCKET_PATH);
            return result;
        }

        char *buffer = malloc(block_size);
        memset(buffer, 'E', block_size);

        long long start = get_time_us();
        size_t total = 0;

        while (total < data_size) {
            size_t to_send = (data_size - total < block_size) ?
                            (data_size - total) : block_size;
            ssize_t n = send(sock, buffer, to_send, 0);
            if (n <= 0) break;
            total += n;
        }

        long long end = get_time_us();

        close(sock);
        close(listen_sock);
        free(buffer);

        waitpid(pid, NULL, 0);
        unlink(SOCKET_PATH);

        double time_sec = (end - start) / 1000000.0;
        result.time_sec = time_sec;
        result.throughput_mbps = (data_size / 1024.0 / 1024.0) / time_sec;
        result.latency_us = (end - start) / (double)(data_size / block_size);
    }

    return result;
}

/*
 * 打印使用說明
 */
void print_usage(const char *prog)
{
    printf("使用方式: %s [選項]\n\n", prog);
    printf("選項:\n");
    printf("  --size SIZE     測試數據大小（默認: 1MB）\n");
    printf("                  支持: 1K, 1M, 10M 等\n");
    printf("  --block SIZE    塊大小（默認: 4KB）\n");
    printf("  pipe            只測試管道\n");
    printf("  fifo            只測試 FIFO\n");
    printf("  shm             只測試共享內存\n");
    printf("  msgq            只測試消息隊列\n");
    printf("  socket          只測試 Unix Socket\n");
    printf("  --help          顯示幫助\n");
    printf("\n");
    printf("示例:\n");
    printf("  %s                      # 運行所有測試\n", prog);
    printf("  %s --size 10M           # 測試 10MB 數據\n", prog);
    printf("  %s pipe shm             # 只測試管道和共享內存\n", prog);
    printf("\n");
}

/*
 * 解析大小參數
 */
size_t parse_size(const char *str)
{
    char *endptr;
    size_t size = strtoul(str, &endptr, 10);

    if (*endptr == 'K' || *endptr == 'k') {
        size *= 1024;
    } else if (*endptr == 'M' || *endptr == 'm') {
        size *= 1024 * 1024;
    }

    return size;
}

/*
 * 主函數
 */
int main(int argc, char *argv[])
{
    size_t data_size = DEFAULT_DATA_SIZE;
    size_t block_size = DEFAULT_BLOCK_SIZE;
    int test_all = 1;
    int test_pipe_flag = 0;
    int test_fifo_flag = 0;
    int test_shm_flag = 0;
    int test_msgq_flag = 0;
    int test_socket_flag = 0;

    // 解析參數
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--size") == 0 && i + 1 < argc) {
            data_size = parse_size(argv[++i]);
        } else if (strcmp(argv[i], "--block") == 0 && i + 1 < argc) {
            block_size = parse_size(argv[++i]);
        } else if (strcmp(argv[i], "pipe") == 0) {
            test_all = 0;
            test_pipe_flag = 1;
        } else if (strcmp(argv[i], "fifo") == 0) {
            test_all = 0;
            test_fifo_flag = 1;
        } else if (strcmp(argv[i], "shm") == 0) {
            test_all = 0;
            test_shm_flag = 1;
        } else if (strcmp(argv[i], "msgq") == 0) {
            test_all = 0;
            test_msgq_flag = 1;
        } else if (strcmp(argv[i], "socket") == 0) {
            test_all = 0;
            test_socket_flag = 1;
        }
    }

    if (test_all) {
        test_pipe_flag = test_fifo_flag = test_shm_flag =
        test_msgq_flag = test_socket_flag = 1;
    }

    // 打印配置
    char size_str[64], block_str[64];
    format_size(size_str, sizeof(size_str), data_size);
    format_size(block_str, sizeof(block_str), block_size);

    printf("\n========================================\n");
    printf("       IPC 性能對比測試\n");
    printf("========================================\n");
    printf("測試數據大小: %s\n", size_str);
    printf("塊大小: %s\n", block_str);
    printf("========================================\n\n");

    printf("%-20s | %-13s | %-10s | %-10s\n",
           "IPC 機制", "吞吐量", "延遲", "總時間");
    printf("---------------------|---------------|------------|------------\n");

    // 運行測試
    benchmark_result_t results[5];
    int result_count = 0;

    if (test_pipe_flag) {
        printf("測試管道...\n");
        results[result_count++] = test_pipe(data_size, block_size);
    }

    if (test_fifo_flag) {
        printf("測試 FIFO...\n");
        results[result_count++] = test_fifo(data_size, block_size);
    }

    if (test_shm_flag) {
        printf("測試共享內存...\n");
        results[result_count++] = test_shm(data_size, block_size);
    }

    if (test_msgq_flag) {
        printf("測試消息隊列...\n");
        results[result_count++] = test_msgq(data_size, block_size);
    }

    if (test_socket_flag) {
        printf("測試 Unix Socket...\n");
        results[result_count++] = test_unix_socket(data_size, block_size);
    }

    printf("\n========================================\n");
    printf("       測試結果\n");
    printf("========================================\n");
    printf("%-20s | %-13s | %-10s | %-10s\n",
           "IPC 機制", "吞吐量", "延遲", "總時間");
    printf("---------------------|---------------|------------|------------\n");

    for (int i = 0; i < result_count; i++) {
        print_result(&results[i]);
    }

    printf("========================================\n\n");

    // 打印建議
    printf("📝 建議:\n");
    printf("  • 共享內存: 最快，適合大量數據傳輸\n");
    printf("  • Unix Socket: 靈活，支持雙向通信\n");
    printf("  • 管道/FIFO: 簡單，適合小量數據\n");
    printf("  • 消息隊列: 結構化，支持優先級\n\n");

    return 0;
}

/*
 * 學習要點:
 * - 不同 IPC 機制的性能差異
 * - 共享內存通常最快（直接內存訪問）
 * - 消息隊列較慢（需要複製和系統調用）
 * - 管道和 FIFO 性能相近
 * - Unix Socket 靈活但有開銷
 *
 * 選擇建議:
 * - 大量數據 + 高性能 → 共享內存
 * - 網絡風格通信 → Unix Socket
 * - 簡單父子進程 → 匿名管道
 * - 無關進程通信 → FIFO
 * - 結構化消息 → 消息隊列
 */
