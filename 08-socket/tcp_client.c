/*
 * 檔案名稱: tcp_client.c
 * 功能說明: TCP 客戶端演示
 *
 * 知識點:
 *   1. socket() 創建套接字
 *   2. connect() 連接服務器
 *   3. send()/recv() 數據收發
 *   4. 交互式通訊
 *
 * 編譯方式: gcc -o tcp_client tcp_client.c
 * 執行方式: ./tcp_client [host] [port]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT 8888
#define BUFFER_SIZE 1024

int main(int argc, char *argv[])
{
    int sock_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    const char *host = DEFAULT_HOST;
    int port = DEFAULT_PORT;

    // 解析命令行參數
    if (argc > 1) {
        host = argv[1];
    }
    if (argc > 2) {
        port = atoi(argv[2]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "無效的端口號: %s\n", argv[2]);
            exit(EXIT_FAILURE);
        }
    }

    printf("====== TCP 客戶端演示 ======\n\n");

    /*
     * 步驟 1: 創建套接字
     */
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    printf("[1] 套接字創建成功\n");

    /*
     * 步驟 2: 設置服務器地址
     */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // 將 IP 地址字符串轉換為網絡地址
    if (inet_pton(AF_INET, host, &server_addr.sin_addr) <= 0) {
        // 如果不是 IP 地址，嘗試域名解析
        struct hostent *he = gethostbyname(host);
        if (he == NULL) {
            fprintf(stderr, "無法解析主機名: %s\n", host);
            close(sock_fd);
            exit(EXIT_FAILURE);
        }
        memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);
    }

    printf("[2] 服務器地址: %s:%d\n", host, port);

    /*
     * 步驟 3: 連接服務器
     *
     * int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
     *
     * 阻塞直到連接建立或失敗
     */
    printf("[3] 正在連接服務器...\n");
    if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect failed");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    printf("[成功] 已連接到服務器！\n\n");

    // 接收服務器的歡迎消息
    memset(buffer, 0, BUFFER_SIZE);
    ssize_t bytes = recv(sock_fd, buffer, BUFFER_SIZE - 1, 0);
    if (bytes > 0) {
        printf("服務器消息:\n%s\n", buffer);
    }

    /*
     * 步驟 4: 交互式通訊
     */
    printf("====== 開始通訊 ======\n");
    printf("輸入消息發送給服務器（'quit' 退出）\n\n");

    while (1) {
        // 讀取用戶輸入
        printf("您 > ");
        fflush(stdout);

        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            break;
        }

        // 移除換行符
        buffer[strcspn(buffer, "\n")] = 0;

        // 發送到服務器
        if (send(sock_fd, buffer, strlen(buffer), 0) == -1) {
            perror("send failed");
            break;
        }

        // 檢查是否退出
        if (strcmp(buffer, "quit") == 0) {
            printf("正在斷開連接...\n");
            break;
        }

        // 接收服務器響應
        memset(buffer, 0, BUFFER_SIZE);
        bytes = recv(sock_fd, buffer, BUFFER_SIZE - 1, 0);

        if (bytes <= 0) {
            if (bytes == 0) {
                printf("\n服務器已關閉連接\n");
            } else {
                perror("recv failed");
            }
            break;
        }

        printf("服務器 > %s\n", buffer);
    }

    // 清理
    close(sock_fd);
    printf("\n已斷開連接\n");

    return 0;
}

/*
 * 知識點補充：
 *
 * 1. 域名解析：
 *    struct hostent *gethostbyname(const char *name);
 *    - h_name: 正式主機名
 *    - h_addr_list: IP 地址列表
 *
 * 2. IP 地址轉換：
 *    inet_pton() - 字符串 → 網絡地址 (presentation to network)
 *    inet_ntop() - 網絡地址 → 字符串 (network to presentation)
 *
 * 3. 連接狀態：
 *    connect() 成功後，套接字狀態變為 ESTABLISHED
 *
 * 4. 數據發送：
 *    send() 可能不會一次發送全部數據
 *    生產環境需要檢查返回值並循環發送
 *
 * 5. 超時設置：
 *    struct timeval tv = {.tv_sec = 5, .tv_usec = 0};
 *    setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
 *    setsockopt(sock_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
 */
