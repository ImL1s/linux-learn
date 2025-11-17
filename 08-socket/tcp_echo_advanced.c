/*
 * 檔案名稱: tcp_echo_advanced.c
 * 功能說明: 增強版 TCP Echo 服務器 - 展示更靈活和實用的設計
 *
 * 新增功能:
 *   1. 命令行參數支持（端口、模式、最大連接數等）
 *   2. 多種工作模式（fork/thread）
 *   3. 實時統計（連接數、數據量、運行時間）
 *   4. 優雅退出
 *   5. 日誌級別控制
 *
 * 編譯方式: gcc -o tcp_echo_advanced tcp_echo_advanced.c -pthread
 * 執行方式:
 *   ./tcp_echo_advanced                                    # 默認配置
 *   ./tcp_echo_advanced -p 9000 -m thread -c 50          # 自定義配置
 *   ./tcp_echo_advanced --help                            # 查看幫助
 *
 * 測試方式:
 *   telnet localhost 8888
 *   nc localhost 8888
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <getopt.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/*
 * 默認配置
 */
#define DEFAULT_PORT 8888
#define DEFAULT_MODE "fork"
#define DEFAULT_MAX_CONN 100
#define DEFAULT_BUFFER_SIZE 4096
#define DEFAULT_BACKLOG 128

/*
 * 服務器配置結構
 */
typedef struct {
    int port;
    char mode[16];           // fork 或 thread
    int max_connections;
    int buffer_size;
    int backlog;
    int verbose;             // 0=quiet, 1=normal, 2=verbose
} server_config_t;

/*
 * 統計信息結構（使用原子操作更好，但為了兼容性使用互斥鎖）
 */
typedef struct {
    pthread_mutex_t lock;
    int total_connections;
    int active_connections;
    long bytes_received;
    long bytes_sent;
    time_t start_time;
} server_stats_t;

/*
 * 客戶端處理參數
 */
typedef struct {
    int client_fd;
    struct sockaddr_in client_addr;
    server_config_t *config;
    server_stats_t *stats;
} client_handler_args_t;

/*
 * 全局變量
 */
volatile sig_atomic_t server_running = 1;
server_stats_t g_stats;
pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * 打印使用說明
 */
void print_usage(const char *prog_name)
{
    printf("使用方式: %s [選項]\n\n", prog_name);
    printf("選項:\n");
    printf("  -p, --port PORT        監聽端口（默認: %d）\n", DEFAULT_PORT);
    printf("  -m, --mode MODE        工作模式: fork|thread（默認: %s）\n", DEFAULT_MODE);
    printf("  -c, --max-conn NUM     最大連接數（默認: %d）\n", DEFAULT_MAX_CONN);
    printf("  -b, --buffer SIZE      緩衝區大小（默認: %d）\n", DEFAULT_BUFFER_SIZE);
    printf("  -v, --verbose          詳細輸出\n");
    printf("  -q, --quiet            安靜模式\n");
    printf("  -h, --help             顯示幫助信息\n");
    printf("\n");
    printf("示例:\n");
    printf("  %s -p 9000 -m thread -c 50 -v\n", prog_name);
    printf("  %s --port 8080 --mode fork --max-conn 100\n", prog_name);
    printf("\n");
}

/*
 * 初始化統計信息
 */
void stats_init(server_stats_t *stats)
{
    pthread_mutex_init(&stats->lock, NULL);
    stats->total_connections = 0;
    stats->active_connections = 0;
    stats->bytes_received = 0;
    stats->bytes_sent = 0;
    stats->start_time = time(NULL);
}

/*
 * 更新統計信息
 */
void stats_update_connection(server_stats_t *stats, int delta)
{
    pthread_mutex_lock(&stats->lock);
    if (delta > 0) {
        stats->total_connections++;
        stats->active_connections++;
    } else {
        stats->active_connections--;
    }
    pthread_mutex_unlock(&stats->lock);
}

void stats_update_bytes(server_stats_t *stats, long recv_bytes, long send_bytes)
{
    pthread_mutex_lock(&stats->lock);
    stats->bytes_received += recv_bytes;
    stats->bytes_sent += send_bytes;
    pthread_mutex_unlock(&stats->lock);
}

/*
 * 打印統計信息
 */
void stats_print(server_stats_t *stats)
{
    pthread_mutex_lock(&stats->lock);

    time_t now = time(NULL);
    int uptime = (int)difftime(now, stats->start_time);
    int hours = uptime / 3600;
    int minutes = (uptime % 3600) / 60;
    int seconds = uptime % 60;

    printf("\n========== 服務器統計 ==========\n");
    printf("運行時間: %02d:%02d:%02d\n", hours, minutes, seconds);
    printf("總連接數: %d\n", stats->total_connections);
    printf("活躍連接: %d\n", stats->active_connections);
    printf("接收數據: %.2f MB (%ld bytes)\n",
           stats->bytes_received / 1024.0 / 1024.0, stats->bytes_received);
    printf("發送數據: %.2f MB (%ld bytes)\n",
           stats->bytes_sent / 1024.0 / 1024.0, stats->bytes_sent);

    if (uptime > 0) {
        printf("平均 QPS: %.2f 連接/秒\n",
               stats->total_connections / (double)uptime);
    }
    printf("================================\n\n");

    pthread_mutex_unlock(&stats->lock);
}

/*
 * 信號處理器
 */
void sigint_handler(int sig)
{
    (void)sig;
    printf("\n\n[服務器] 收到終止信號，準備退出...\n");
    server_running = 0;
}

void sigchld_handler(int sig)
{
    (void)sig;
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

void sigusr1_handler(int sig)
{
    (void)sig;
    // SIGUSR1 用於打印統計信息（kill -USR1 <pid>）
    stats_print(&g_stats);
}

/*
 * 客戶端處理邏輯（核心業務）
 */
int handle_client_connection(int client_fd, struct sockaddr_in *client_addr,
                              server_config_t *config, server_stats_t *stats)
{
    char buffer[config->buffer_size];
    char client_ip[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, &client_addr->sin_addr, client_ip, sizeof(client_ip));
    int client_port = ntohs(client_addr->sin_port);

    // 記錄連接
    stats_update_connection(stats, 1);

    if (config->verbose >= 1) {
        printf("[連接] %s:%d 已連接 (PID/TID=%d)\n",
               client_ip, client_port, (int)getpid());
    }

    // 發送歡迎消息
    const char *welcome =
        "========================================\n"
        "  歡迎使用增強版 TCP Echo 服務器！\n"
        "========================================\n"
        "命令:\n"
        "  help  - 顯示幫助\n"
        "  stats - 顯示統計信息\n"
        "  time  - 顯示服務器時間\n"
        "  quit  - 斷開連接\n"
        "其他輸入將被回顯\n"
        "----------------------------------------\n";

    ssize_t sent = send(client_fd, welcome, strlen(welcome), 0);
    if (sent == -1) {
        perror("[錯誤] send welcome");
    }

    // 主循環
    long conn_recv = 0, conn_sent = 0;

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

        if (bytes <= 0) {
            if (bytes == 0) {
                if (config->verbose >= 1) {
                    printf("[斷開] %s:%d 已斷開\n", client_ip, client_port);
                }
            } else {
                if (config->verbose >= 1) {
                    perror("[錯誤] recv");
                }
            }
            break;
        }

        conn_recv += bytes;

        // 移除換行符
        buffer[strcspn(buffer, "\r\n")] = 0;

        if (config->verbose >= 2) {
            printf("[數據] %s:%d > %s\n", client_ip, client_port, buffer);
        }

        // 處理命令
        if (strcmp(buffer, "quit") == 0) {
            const char *msg = "再見！\n";
            sent = send(client_fd, msg, strlen(msg), 0);
            if (sent == -1) {
                perror("[錯誤] send quit");
            }
            break;
        } else if (strcmp(buffer, "help") == 0) {
            sent = send(client_fd, welcome, strlen(welcome), 0);
            if (sent == -1) {
                perror("[錯誤] send help");
            }
        } else if (strcmp(buffer, "stats") == 0) {
            char stats_msg[512];
            pthread_mutex_lock(&stats->lock);
            snprintf(stats_msg, sizeof(stats_msg),
                    "總連接: %d | 活躍: %d | 接收: %ld bytes | 發送: %ld bytes\n",
                    stats->total_connections,
                    stats->active_connections,
                    stats->bytes_received,
                    stats->bytes_sent);
            pthread_mutex_unlock(&stats->lock);
            sent = send(client_fd, stats_msg, strlen(stats_msg), 0);
            if (sent == -1) {
                perror("[錯誤] send stats");
            }
        } else if (strcmp(buffer, "time") == 0) {
            time_t now = time(NULL);
            char *time_str = ctime(&now);
            char time_msg[128];
            snprintf(time_msg, sizeof(time_msg), "服務器時間: %s", time_str);
            sent = send(client_fd, time_msg, strlen(time_msg), 0);
            if (sent == -1) {
                perror("[錯誤] send time");
            }
        } else {
            // Echo 回去
            char echo_msg[config->buffer_size + 64];
            int len = snprintf(echo_msg, sizeof(echo_msg), "Echo: %s\n", buffer);
            sent = send(client_fd, echo_msg, len, 0);
            if (sent == -1) {
                perror("[錯誤] send echo");
            } else {
                conn_sent += len;
            }
        }
    }

    // 更新統計
    stats_update_bytes(stats, conn_recv, conn_sent);
    stats_update_connection(stats, -1);

    close(client_fd);
    return 0;
}

/*
 * Fork 模式的客戶端處理器
 */
void handle_client_fork(int client_fd, struct sockaddr_in *client_addr,
                        server_config_t *config, server_stats_t *stats)
{
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        close(client_fd);
        return;
    }

    if (pid == 0) {
        // 子進程
        handle_client_connection(client_fd, client_addr, config, stats);
        exit(0);
    } else {
        // 父進程
        close(client_fd);  // 父進程不需要這個 fd
    }
}

/*
 * Thread 模式的客戶端處理器
 */
void* handle_client_thread(void* arg)
{
    client_handler_args_t *args = (client_handler_args_t*)arg;

    // 設置為 detached，自動回收資源
    pthread_detach(pthread_self());

    handle_client_connection(args->client_fd, &args->client_addr,
                            args->config, args->stats);

    free(args);
    return NULL;
}

/*
 * 主函數
 */
int main(int argc, char *argv[])
{
    server_config_t config = {
        .port = DEFAULT_PORT,
        .max_connections = DEFAULT_MAX_CONN,
        .buffer_size = DEFAULT_BUFFER_SIZE,
        .backlog = DEFAULT_BACKLOG,
        .verbose = 1  // normal
    };
    strcpy(config.mode, DEFAULT_MODE);

    // 解析命令行參數
    static struct option long_options[] = {
        {"port",      required_argument, 0, 'p'},
        {"mode",      required_argument, 0, 'm'},
        {"max-conn",  required_argument, 0, 'c'},
        {"buffer",    required_argument, 0, 'b'},
        {"verbose",   no_argument,       0, 'v'},
        {"quiet",     no_argument,       0, 'q'},
        {"help",      no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "p:m:c:b:vqh", long_options, NULL)) != -1) {
        switch (opt) {
            case 'p':
                config.port = atoi(optarg);
                break;
            case 'm':
                strncpy(config.mode, optarg, sizeof(config.mode) - 1);
                break;
            case 'c':
                config.max_connections = atoi(optarg);
                break;
            case 'b':
                config.buffer_size = atoi(optarg);
                break;
            case 'v':
                config.verbose = 2;
                break;
            case 'q':
                config.verbose = 0;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    // 驗證參數
    if (strcmp(config.mode, "fork") != 0 && strcmp(config.mode, "thread") != 0) {
        fprintf(stderr, "錯誤: 模式必須是 'fork' 或 'thread'\n");
        return 1;
    }

    // 初始化統計
    stats_init(&g_stats);

    // 設置信號處理
    signal(SIGINT, sigint_handler);
    signal(SIGTERM, sigint_handler);
    signal(SIGUSR1, sigusr1_handler);

    if (strcmp(config.mode, "fork") == 0) {
        signal(SIGCHLD, sigchld_handler);
    }

    // 創建 socket
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket failed");
        return 1;
    }

    // 設置 SO_REUSEADDR
    int opt_reuse = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt_reuse, sizeof(opt_reuse));

    // 綁定地址
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(config.port);

    if (bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(listen_fd);
        return 1;
    }

    // 監聽
    if (listen(listen_fd, config.backlog) < 0) {
        perror("listen failed");
        close(listen_fd);
        return 1;
    }

    // 打印配置信息
    printf("\n========================================\n");
    printf("  增強版 TCP Echo 服務器\n");
    printf("========================================\n");
    printf("監聽端口: %d\n", config.port);
    printf("工作模式: %s\n", config.mode);
    printf("最大連接: %d\n", config.max_connections);
    printf("緩衝大小: %d bytes\n", config.buffer_size);
    printf("日誌級別: %s\n",
           config.verbose == 0 ? "quiet" : (config.verbose == 1 ? "normal" : "verbose"));
    printf("========================================\n");
    printf("服務器已啟動，按 Ctrl+C 退出\n");
    printf("發送 SIGUSR1 查看統計: kill -USR1 %d\n", getpid());
    printf("========================================\n\n");

    // 主循環
    while (server_running) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        int client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &addr_len);

        if (client_fd < 0) {
            if (errno == EINTR) continue;  // 被信號中斷
            perror("accept failed");
            continue;
        }

        // 根據模式處理客戶端
        if (strcmp(config.mode, "fork") == 0) {
            handle_client_fork(client_fd, &client_addr, &config, &g_stats);
        } else {
            // Thread 模式
            client_handler_args_t *args = malloc(sizeof(client_handler_args_t));
            args->client_fd = client_fd;
            args->client_addr = client_addr;
            args->config = &config;
            args->stats = &g_stats;

            pthread_t thread;
            if (pthread_create(&thread, NULL, handle_client_thread, args) != 0) {
                perror("pthread_create failed");
                close(client_fd);
                free(args);
            }
        }
    }

    // 清理
    close(listen_fd);

    // 打印最終統計
    printf("\n服務器正在關閉...\n");
    stats_print(&g_stats);

    pthread_mutex_destroy(&g_stats.lock);

    printf("服務器已關閉。\n");
    return 0;
}

/*
 * 使用示例和測試：
 *
 * 1. 默認配置運行：
 *    ./tcp_echo_advanced
 *
 * 2. 自定義端口和模式：
 *    ./tcp_echo_advanced -p 9000 -m thread
 *
 * 3. 詳細輸出：
 *    ./tcp_echo_advanced -v
 *
 * 4. 測試連接（另一個終端）：
 *    telnet localhost 8888
 *    或
 *    nc localhost 8888
 *
 * 5. 查看統計信息（另一個終端）：
 *    kill -USR1 $(pgrep tcp_echo_advanced)
 *
 * 6. 性能測試（使用 ab 或自定義腳本）：
 *    for i in {1..100}; do (echo "test message" | nc localhost 8888 &); done
 *
 * 學習要點：
 * - 命令行參數解析（getopt_long）
 * - 多種工作模式（fork vs thread）
 * - 統計信息收集
 * - 信號處理（SIGINT, SIGUSR1）
 * - 互斥鎖保護共享數據
 * - 優雅退出
 */
