/*
 * 檔案名稱: epoll_server.c
 * 功能說明: epoll I/O 多路復用服務器
 *
 * 知識點:
 *   1. epoll_create1() 創建 epoll 實例
 *   2. epoll_ctl() 控制事件
 *   3. epoll_wait() 等待事件
 *   4. 非阻塞 I/O
 *   5. 邊緣觸發 (ET) vs 水平觸發 (LT)
 *
 * 優勢：
 *   - 單線程處理大量並發連接
 *   - O(1) 時間複雜度
 *   - 適合高並發服務器
 *
 * 編譯方式: gcc -o epoll_server epoll_server.c
 * 執行方式: ./epoll_server [port]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define DEFAULT_PORT 9999
#define MAX_EVENTS 64
#define BUFFER_SIZE 1024
#define BACKLOG 128

volatile sig_atomic_t server_running = 1;

void sigint_handler(int sig)
{
    (void)sig;
    printf("\n[服務器] 收到終止信號\n");
    server_running = 0;
}

/*
 * 設置文件描述符為非阻塞模式
 *
 * 為什麼需要非阻塞？
 * - epoll 配合非阻塞 I/O 使用效率最高
 * - 避免阻塞在單個連接上
 * - 可以及時處理其他就緒的連接
 */
int set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        return -1;
    }

    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) == -1) {
        perror("fcntl F_SETFL");
        return -1;
    }

    return 0;
}

/*
 * 創建並配置監聽套接字
 */
int create_and_bind(int port)
{
    int listen_fd;
    struct sockaddr_in addr;

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        perror("socket");
        return -1;
    }

    // 設置地址重用
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 綁定地址
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(listen_fd);
        return -1;
    }

    return listen_fd;
}

/*
 * 處理新連接
 */
void handle_accept(int listen_fd, int epoll_fd)
{
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_len);

        if (client_fd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 所有連接都已處理完畢
                break;
            } else {
                perror("accept");
                break;
            }
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        printf("[連接] 新客戶端: %s:%d (fd=%d)\n",
               client_ip, ntohs(client_addr.sin_port), client_fd);

        // 設置為非阻塞
        if (set_nonblocking(client_fd) == -1) {
            close(client_fd);
            continue;
        }

        /*
         * 將新連接加入 epoll
         *
         * EPOLLIN: 可讀事件
         * EPOLLET: 邊緣觸發模式
         *
         * 邊緣觸發 vs 水平觸發：
         *
         * 水平觸發 (LT - Level Triggered)：
         *   - 只要有數據就會通知
         *   - 簡單，不易出錯
         *   - 可能產生大量不必要的通知
         *
         * 邊緣觸發 (ET - Edge Triggered)：
         *   - 只在狀態變化時通知一次
         *   - 更高效，適合高並發
         *   - 必須一次性處理完所有數據
         *   - 必須使用非阻塞 I/O
         */
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLET;  // 邊緣觸發
        ev.data.fd = client_fd;

        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
            perror("epoll_ctl: add client");
            close(client_fd);
            continue;
        }

        // 發送歡迎消息
        const char *welcome = "歡迎使用 epoll 服務器！\n";
        send(client_fd, welcome, strlen(welcome), 0);
    }
}

/*
 * 處理客戶端數據
 */
void handle_read(int client_fd, int epoll_fd)
{
    char buffer[BUFFER_SIZE];

    /*
     * 邊緣觸發模式下，必須循環讀取直到 EAGAIN
     * 否則可能丟失數據
     */
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t count = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);

        if (count == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 數據讀取完畢
                break;
            } else {
                perror("recv");
                // 關閉連接
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                close(client_fd);
                printf("[斷開] 客戶端 fd=%d（錯誤）\n", client_fd);
                break;
            }
        }
        else if (count == 0) {
            // 客戶端關閉連接
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
            close(client_fd);
            printf("[斷開] 客戶端 fd=%d（正常關閉）\n", client_fd);
            break;
        }

        // 回顯數據
        printf("[數據] fd=%d 收到 %ld 字節: %s", client_fd, count, buffer);

        // Echo 回客戶端
        char response[BUFFER_SIZE + 20];
        snprintf(response, sizeof(response), "服務器回顯: %s", buffer);
        send(client_fd, response, strlen(response), 0);
    }
}

int main(int argc, char *argv[])
{
    int port = DEFAULT_PORT;
    int listen_fd, epoll_fd;
    struct epoll_event ev, events[MAX_EVENTS];

    // 解析端口
    if (argc > 1) {
        port = atoi(argv[1]);
    }

    printf("====== epoll 服務器演示 ======\n\n");

    // 註冊信號
    signal(SIGINT, sigint_handler);
    signal(SIGPIPE, SIG_IGN);  // 忽略 SIGPIPE

    /*
     * 步驟 1: 創建監聽套接字
     */
    listen_fd = create_and_bind(port);
    if (listen_fd == -1) {
        exit(EXIT_FAILURE);
    }

    if (set_nonblocking(listen_fd) == -1) {
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(listen_fd, BACKLOG) == -1) {
        perror("listen");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    printf("[1] 監聽端口: %d\n", port);

    /*
     * 步驟 2: 創建 epoll 實例
     *
     * int epoll_create1(int flags);
     *
     * flags:
     *   0: 默認行為
     *   EPOLL_CLOEXEC: exec 時關閉
     *
     * 返回值：epoll 文件描述符
     */
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    printf("[2] epoll 實例創建成功 (fd=%d)\n", epoll_fd);

    /*
     * 步驟 3: 將監聽套接字加入 epoll
     */
    ev.events = EPOLLIN | EPOLLET;  // 邊緣觸發
    ev.data.fd = listen_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev) == -1) {
        perror("epoll_ctl: listen_fd");
        close(listen_fd);
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }

    printf("[3] 監聽套接字已加入 epoll\n");
    printf("[4] 服務器啟動完成，等待連接...\n\n");

    /*
     * 步驟 4: 事件循環
     */
    while (server_running) {
        /*
         * epoll_wait() - 等待事件
         *
         * int epoll_wait(int epfd, struct epoll_event *events,
         *                int maxevents, int timeout);
         *
         * timeout:
         *   -1: 永久阻塞
         *   0: 立即返回（輪詢）
         *   >0: 超時時間（毫秒）
         *
         * 返回值：就緒的文件描述符數量
         */
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, 1000);  // 1秒超時

        if (n == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("epoll_wait");
            break;
        }

        // 處理所有就緒的事件
        for (int i = 0; i < n; i++) {
            if (events[i].data.fd == listen_fd) {
                // 監聽套接字就緒：有新連接
                handle_accept(listen_fd, epoll_fd);
            } else {
                // 客戶端套接字就緒：有數據可讀
                handle_read(events[i].data.fd, epoll_fd);
            }
        }
    }

    // 清理
    printf("\n[服務器] 正在關閉...\n");
    close(listen_fd);
    close(epoll_fd);
    printf("[服務器] 已退出\n");

    return 0;
}

/*
 * epoll 知識點總結：
 *
 * 1. epoll vs select/poll：
 *
 *    特性         select      poll        epoll
 *    -------------------------------------------
 *    監視上限     1024        無限        無限
 *    時間複雜度   O(n)        O(n)        O(1)
 *    事件通知     全量        全量        增量
 *    性能         低          中          高
 *
 * 2. epoll 的三個系統調用：
 *
 *    epoll_create1() - 創建 epoll 實例
 *    epoll_ctl()     - 添加/修改/刪除監視的文件描述符
 *    epoll_wait()    - 等待事件
 *
 * 3. epoll_ctl 操作：
 *
 *    EPOLL_CTL_ADD - 添加文件描述符
 *    EPOLL_CTL_MOD - 修改事件類型
 *    EPOLL_CTL_DEL - 刪除文件描述符
 *
 * 4. 事件類型：
 *
 *    EPOLLIN  - 可讀
 *    EPOLLOUT - 可寫
 *    EPOLLERR - 錯誤
 *    EPOLLHUP - 掛起
 *    EPOLLET  - 邊緣觸發
 *    EPOLLONESHOT - 一次性事件
 *
 * 5. 邊緣觸發 (ET) 注意事項：
 *
 *    - 必須使用非阻塞 I/O
 *    - 必須循環讀取直到 EAGAIN
 *    - 狀態變化才通知，不會重複通知
 *    - 更高效，但編程難度更高
 *
 * 6. 水平觸發 (LT) 特點：
 *
 *    - epoll 默認模式
 *    - 只要有數據就會通知
 *    - 編程簡單，不易出錯
 *    - 與 select/poll 行為類似
 *
 * 7. 使用場景：
 *
 *    - Web 服務器 (nginx, lighttpd)
 *    - 代理服務器
 *    - 實時通訊服務器
 *    - 任何高並發 I/O 場景
 *
 * 8. 性能優化：
 *
 *    - 使用邊緣觸發模式
 *    - 合理設置緩衝區大小
 *    - 避免頻繁的 epoll_ctl
 *    - 使用 EPOLLONESHOT 處理競態
 *
 * 9. 常見錯誤：
 *
 *    - ET 模式下沒有循環讀取到 EAGAIN
 *    - 忘記設置非阻塞
 *    - 沒有正確處理 EPOLLHUP
 *    - 忘記從 epoll 中刪除已關閉的 fd
 */
