/*
 * 檔案名稱: tcp_server.c
 * 功能說明: TCP 服務器演示
 *
 * 知識點:
 *   1. socket() 創建套接字
 *   2. bind() 綁定地址和端口
 *   3. listen() 監聽連接
 *   4. accept() 接受客戶端連接
 *   5. send()/recv() 數據收發
 *   6. 多客戶端處理（fork 模式）
 *
 * 編譯方式: gcc -o tcp_server tcp_server.c
 * 執行方式: ./tcp_server [port]
 * 測試方式: telnet localhost 8888
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define DEFAULT_PORT 8888
#define BACKLOG 10
#define BUFFER_SIZE 1024

volatile sig_atomic_t server_running = 1;

/*
 * SIGINT 信號處理器（Ctrl+C 優雅退出）
 */
void sigint_handler(int sig)
{
    (void)sig;
    printf("\n[服務器] 收到終止信號，準備退出...\n");
    server_running = 0;
}

/*
 * SIGCHLD 信號處理器（回收子進程，避免殭屍進程）
 */
void sigchld_handler(int sig)
{
    (void)sig;
    // 回收所有已終止的子進程
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

/*
 * 處理客戶端連接（在子進程中執行）
 */
void handle_client(int client_fd, struct sockaddr_in *client_addr)
{
    char buffer[BUFFER_SIZE];
    char client_ip[INET_ADDRSTRLEN];

    // 獲取客戶端 IP 地址
    inet_ntop(AF_INET, &client_addr->sin_addr, client_ip, sizeof(client_ip));
    int client_port = ntohs(client_addr->sin_port);

    printf("[子進程 %d] 連接來自 %s:%d\n", getpid(), client_ip, client_port);

    // 發送歡迎消息
    const char *welcome = "歡迎連接到 TCP 服務器！\n輸入 'quit' 退出\n";
    ssize_t sent = send(client_fd, welcome, strlen(welcome), 0);
    if (sent == -1) {
        perror("[子進程] send welcome failed");
    }

    // 主循環：接收並回顯客戶端消息
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);

        ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);

        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                printf("[子進程 %d] 客戶端 %s:%d 已斷開連接\n",
                       getpid(), client_ip, client_port);
            } else {
                perror("[子進程] recv error");
            }
            break;
        }

        // 移除換行符
        buffer[strcspn(buffer, "\r\n")] = 0;

        printf("[子進程 %d] 收到: %s\n", getpid(), buffer);

        // 檢查退出命令
        if (strcmp(buffer, "quit") == 0) {
            const char *goodbye = "再見！\n";
            sent = send(client_fd, goodbye, strlen(goodbye), 0);
            if (sent == -1) {
                perror("[子進程] send goodbye failed");
            }
            printf("[子進程 %d] 客戶端請求斷開\n", getpid());
            break;
        }

        // 回顯消息（Echo）
        char response[BUFFER_SIZE + 20];
        snprintf(response, sizeof(response), "服務器回顯: %s\n", buffer);
        sent = send(client_fd, response, strlen(response), 0);
        if (sent == -1) {
            perror("[子進程] send response failed");
            break;  // 發送失敗，斷開連接
        }
    }

    close(client_fd);
    printf("[子進程 %d] 連接處理完成\n", getpid());
}

int main(int argc, char *argv[])
{
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    int port = DEFAULT_PORT;

    // 解析端口參數
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "無效的端口號: %s\n", argv[1]);
            exit(EXIT_FAILURE);
        }
    }

    printf("====== TCP 服務器演示 ======\n\n");

    // 註冊信號處理器
    signal(SIGINT, sigint_handler);
    signal(SIGCHLD, sigchld_handler);

    /*
     * 步驟 1: 創建套接字
     *
     * int socket(int domain, int type, int protocol);
     *
     * domain: AF_INET (IPv4), AF_INET6 (IPv6)
     * type: SOCK_STREAM (TCP), SOCK_DGRAM (UDP)
     * protocol: 0 (自動選擇協議)
     */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    printf("[1] 套接字創建成功 (fd=%d)\n", server_fd);

    /*
     * 設置套接字選項：允許地址重用
     *
     * 作用：避免 "Address already in use" 錯誤
     * 場景：服務器重啟時，端口可能還處於 TIME_WAIT 狀態
     */
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("[2] 設置 SO_REUSEADDR 選項\n");

    /*
     * 步驟 2: 綁定地址和端口
     *
     * struct sockaddr_in {
     *     sa_family_t    sin_family;  // AF_INET
     *     in_port_t      sin_port;    // 端口號 (網絡字節序)
     *     struct in_addr sin_addr;    // IP 地址
     * };
     */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // 監聽所有網卡
    server_addr.sin_port = htons(port);        // 主機字節序 → 網絡字節序

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("[3] 綁定到 0.0.0.0:%d\n", port);

    /*
     * 步驟 3: 開始監聽
     *
     * int listen(int sockfd, int backlog);
     *
     * backlog: 等待連接隊列的最大長度
     */
    if (listen(server_fd, BACKLOG) == -1) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("[4] 開始監聽，等待客戶端連接...\n");
    printf("[提示] 可使用 telnet localhost %d 測試\n", port);
    printf("[提示] 按 Ctrl+C 停止服務器\n\n");

    /*
     * 步驟 4: 主循環 - 接受並處理客戶端連接
     */
    while (server_running) {
        /*
         * accept() 會阻塞，直到有客戶端連接
         *
         * 返回值：新的套接字文件描述符（用於與客戶端通訊）
         * client_addr: 客戶端地址信息（輸出參數）
         */
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);

        if (client_fd == -1) {
            if (errno == EINTR) {
                // 被信號中斷，繼續
                continue;
            }
            perror("accept failed");
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        int client_port = ntohs(client_addr.sin_port);

        printf("[主進程] 接受連接: %s:%d\n", client_ip, client_port);

        /*
         * 創建子進程處理客戶端
         *
         * 優點：
         * - 支持多客戶端並發
         * - 客戶端之間相互隔離
         *
         * 缺點：
         * - 進程創建開銷較大
         * - 不適合大量短連接
         *
         * 替代方案：
         * - 多線程（pthread_create）
         * - I/O 多路復用（epoll）
         */
        pid_t pid = fork();

        if (pid == -1) {
            perror("fork failed");
            close(client_fd);
            continue;
        }
        else if (pid == 0) {
            // 子進程
            close(server_fd);  // 子進程不需要監聽套接字
            handle_client(client_fd, &client_addr);
            exit(EXIT_SUCCESS);
        }
        else {
            // 父進程
            close(client_fd);  // 父進程不需要客戶端套接字
            printf("[主進程] 創建子進程 %d 處理客戶端\n\n", pid);
        }
    }

    // 清理
    printf("\n[服務器] 正在關閉...\n");
    close(server_fd);
    printf("[服務器] 已退出\n");

    return 0;
}

/*
 * 知識點總結：
 *
 * 1. TCP 服務器基本流程：
 *    socket() → bind() → listen() → accept() → recv()/send() → close()
 *
 * 2. 網絡字節序轉換：
 *    htons() - host to network short (主機 → 網絡，16位)
 *    htonl() - host to network long (主機 → 網絡，32位)
 *    ntohs() - network to host short (網絡 → 主機，16位)
 *    ntohl() - network to host long (網絡 → 主機，32位)
 *
 * 3. 地址結構：
 *    INADDR_ANY (0.0.0.0) - 監聽所有網卡
 *    INADDR_LOOPBACK (127.0.0.1) - 本地回環
 *
 * 4. SO_REUSEADDR 選項：
 *    - 允許綁定到處於 TIME_WAIT 狀態的地址
 *    - 避免 "Address already in use" 錯誤
 *    - 服務器重啟時非常有用
 *
 * 5. 並發模型：
 *    - 多進程（本例）：每個客戶端一個進程
 *    - 多線程：每個客戶端一個線程
 *    - I/O 多路復用：單進程處理多客戶端（epoll/select）
 *    - 異步 I/O：非阻塞 I/O + 事件驅動
 *
 * 6. 常見問題：
 *    Q: 為什麼需要 fork 後關閉不用的文件描述符？
 *    A: 文件描述符是引用計數的，必須在所有進程中關閉才能真正釋放
 *
 *    Q: backlog 參數如何選擇？
 *    A: 通常 5-10，取決於預期的並發連接數
 *
 *    Q: 如何處理客戶端異常斷開？
 *    A: recv() 返回 0 表示正常關閉，-1 表示錯誤
 *
 * 7. 安全考慮：
 *    - 限制並發連接數
 *    - 超時機制
 *    - 輸入驗證
 *    - 緩衝區溢出防護
 *
 * 8. 性能優化：
 *    - TCP_NODELAY：禁用 Nagle 算法
 *    - TCP_CORK：優化小數據包發送
 *    - SO_SNDBUF/SO_RCVBUF：調整緩衝區大小
 *    - 非阻塞 I/O + epoll
 */
