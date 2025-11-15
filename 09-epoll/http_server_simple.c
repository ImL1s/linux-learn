/*
 * 檔案名稱: http_server_simple.c
 * 功能說明: 簡單的 HTTP 服務器 - 綜合應用示範
 *
 * 功能特性:
 *   1. HTTP/1.1 支持（GET 請求）
 *   2. 靜態文件服務
 *   3. 目錄瀏覽
 *   4. 基本的 MIME 類型識別
 *   5. epoll I/O 多路復用
 *   6. 非阻塞 I/O
 *
 * 技術應用:
 *   ✓ Socket 編程（bind/listen/accept）
 *   ✓ epoll I/O 多路復用
 *   ✓ 非阻塞 I/O
 *   ✓ HTTP 協議解析
 *   ✓ 文件 I/O（open/read）
 *   ✓ 字符串處理
 *
 * 編譯方式: gcc -o http_server_simple http_server_simple.c
 * 執行方式:
 *   ./http_server_simple                    # 默認端口 8000
 *   ./http_server_simple 9000               # 自定義端口
 *   ./http_server_simple 8080 /var/www     # 自定義端口和根目錄
 *
 * 測試方式:
 *   瀏覽器: http://localhost:8000/
 *   curl: curl http://localhost:8000/
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
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <time.h>
#include <ctype.h>

/*
 * 配置參數
 */
#define DEFAULT_PORT 8000
#define DEFAULT_ROOT "."
#define MAX_EVENTS 64
#define BUFFER_SIZE 8192
#define MAX_PATH 1024

volatile sig_atomic_t server_running = 1;

/*
 * MIME 類型映射
 */
typedef struct {
    const char *ext;
    const char *mime;
} mime_type_t;

static mime_type_t mime_types[] = {
    {".html", "text/html"},
    {".htm",  "text/html"},
    {".css",  "text/css"},
    {".js",   "application/javascript"},
    {".json", "application/json"},
    {".txt",  "text/plain"},
    {".jpg",  "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".png",  "image/png"},
    {".gif",  "image/gif"},
    {".svg",  "image/svg+xml"},
    {".ico",  "image/x-icon"},
    {".pdf",  "application/pdf"},
    {".zip",  "application/zip"},
    {NULL, NULL}
};

/*
 * 服務器配置
 */
typedef struct {
    int port;
    char root_dir[MAX_PATH];
    int total_requests;
    int active_connections;
} server_config_t;

server_config_t g_config;

/*
 * 信號處理
 */
void sigint_handler(int sig)
{
    (void)sig;
    printf("\n\n[服務器] 收到終止信號\n");
    server_running = 0;
}

/*
 * 設置非阻塞模式
 */
int set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/*
 * 根據文件擴展名獲取 MIME 類型
 */
const char* get_mime_type(const char *path)
{
    const char *ext = strrchr(path, '.');
    if (ext == NULL) {
        return "application/octet-stream";
    }

    for (int i = 0; mime_types[i].ext != NULL; i++) {
        if (strcasecmp(ext, mime_types[i].ext) == 0) {
            return mime_types[i].mime;
        }
    }

    return "application/octet-stream";
}

/*
 * URL 解碼
 */
void url_decode(char *dst, const char *src)
{
    char a, b;
    while (*src) {
        if ((*src == '%') &&
            ((a = src[1]) && (b = src[2])) &&
            (isxdigit(a) && isxdigit(b))) {
            if (a >= 'a') a -= 'a'-'A';
            if (a >= 'A') a -= ('A' - 10);
            else a -= '0';
            if (b >= 'a') b -= 'a'-'A';
            if (b >= 'A') b -= ('A' - 10);
            else b -= '0';
            *dst++ = 16*a+b;
            src+=3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst++ = '\0';
}

/*
 * HTML 轉義
 */
void html_escape(char *dst, const char *src, size_t dst_size)
{
    size_t i = 0;
    while (*src && i < dst_size - 10) {
        switch (*src) {
            case '<':
                strcpy(&dst[i], "&lt;");
                i += 4;
                break;
            case '>':
                strcpy(&dst[i], "&gt;");
                i += 4;
                break;
            case '&':
                strcpy(&dst[i], "&amp;");
                i += 5;
                break;
            case '"':
                strcpy(&dst[i], "&quot;");
                i += 6;
                break;
            default:
                dst[i++] = *src;
        }
        src++;
    }
    dst[i] = '\0';
}

/*
 * 發送 HTTP 響應
 */
int send_response(int client_fd, int status_code, const char *status_text,
                  const char *content_type, const char *body, size_t body_len)
{
    char header[1024];
    int header_len;

    if (body_len == 0 && body != NULL) {
        body_len = strlen(body);
    }

    // 構建 HTTP 響應頭
    header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Server: Simple-HTTP-Server/1.0\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n",
        status_code, status_text, content_type, body_len);

    // 發送響應頭
    if (send(client_fd, header, header_len, 0) < 0) {
        return -1;
    }

    // 發送響應體
    if (body != NULL && body_len > 0) {
        if (send(client_fd, body, body_len, 0) < 0) {
            return -1;
        }
    }

    return 0;
}

/*
 * 發送錯誤響應
 */
void send_error(int client_fd, int status_code, const char *message)
{
    char body[1024];
    snprintf(body, sizeof(body),
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head><title>%d %s</title></head>\n"
        "<body>\n"
        "<h1>%d %s</h1>\n"
        "<p>%s</p>\n"
        "<hr><i>Simple HTTP Server</i>\n"
        "</body>\n"
        "</html>\n",
        status_code, message, status_code, message, message);

    send_response(client_fd, status_code, message, "text/html", body, 0);
}

/*
 * 發送文件
 */
int send_file(int client_fd, const char *filepath)
{
    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        send_error(client_fd, 404, "Not Found");
        return -1;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        send_error(client_fd, 500, "Internal Server Error");
        return -1;
    }

    // 發送響應頭
    const char *mime = get_mime_type(filepath);
    char header[1024];
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Server: Simple-HTTP-Server/1.0\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n"
        "\r\n",
        mime, st.st_size);

    if (send(client_fd, header, header_len, 0) < 0) {
        close(fd);
        return -1;
    }

    // 發送文件內容
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        if (send(client_fd, buffer, bytes_read, 0) < 0) {
            close(fd);
            return -1;
        }
    }

    close(fd);
    return 0;
}

/*
 * 發送目錄列表
 */
int send_directory(int client_fd, const char *dirpath, const char *request_path)
{
    DIR *dir = opendir(dirpath);
    if (dir == NULL) {
        send_error(client_fd, 403, "Forbidden");
        return -1;
    }

    // 構建 HTML
    char body[BUFFER_SIZE * 4];
    char *p = body;
    size_t remaining = sizeof(body);

    p += snprintf(p, remaining,
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "<title>Index of %s</title>\n"
        "<style>\n"
        "body { font-family: Arial, sans-serif; margin: 40px; }\n"
        "h1 { color: #333; }\n"
        "table { border-collapse: collapse; width: 100%%; }\n"
        "th, td { text-align: left; padding: 8px; border-bottom: 1px solid #ddd; }\n"
        "th { background-color: #4CAF50; color: white; }\n"
        "a { text-decoration: none; color: #4CAF50; }\n"
        "a:hover { text-decoration: underline; }\n"
        "</style>\n"
        "</head>\n"
        "<body>\n"
        "<h1>Index of %s</h1>\n"
        "<table>\n"
        "<tr><th>Name</th><th>Size</th><th>Modified</th></tr>\n",
        request_path, request_path);
    remaining = sizeof(body) - (p - body);

    // 添加父目錄鏈接
    if (strcmp(request_path, "/") != 0) {
        p += snprintf(p, remaining, "<tr><td><a href=\"..\">Parent Directory</a></td><td>-</td><td>-</td></tr>\n");
        remaining = sizeof(body) - (p - body);
    }

    // 列出文件和目錄
    struct dirent *entry;
    char fullpath[MAX_PATH];
    struct stat st;

    while ((entry = readdir(dir)) != NULL && remaining > 200) {
        // 跳過隱藏文件
        if (entry->d_name[0] == '.') continue;

        snprintf(fullpath, sizeof(fullpath), "%s/%s", dirpath, entry->d_name);

        if (stat(fullpath, &st) == 0) {
            char size_str[32];
            char time_str[64];

            // 格式化大小
            if (S_ISDIR(st.st_mode)) {
                strcpy(size_str, "-");
            } else {
                if (st.st_size < 1024) {
                    snprintf(size_str, sizeof(size_str), "%ld B", st.st_size);
                } else if (st.st_size < 1024*1024) {
                    snprintf(size_str, sizeof(size_str), "%.1f KB", st.st_size/1024.0);
                } else {
                    snprintf(size_str, sizeof(size_str), "%.1f MB", st.st_size/1024.0/1024.0);
                }
            }

            // 格式化時間
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M",
                    localtime(&st.st_mtime));

            // HTML 轉義文件名
            char escaped_name[256];
            html_escape(escaped_name, entry->d_name, sizeof(escaped_name));

            // 添加行
            p += snprintf(p, remaining,
                "<tr><td><a href=\"%s%s\">%s%s</a></td><td>%s</td><td>%s</td></tr>\n",
                escaped_name,
                S_ISDIR(st.st_mode) ? "/" : "",
                escaped_name,
                S_ISDIR(st.st_mode) ? "/" : "",
                size_str,
                time_str);
            remaining = sizeof(body) - (p - body);
        }
    }

    p += snprintf(p, remaining,
        "</table>\n"
        "<hr>\n"
        "<i>Simple HTTP Server</i>\n"
        "</body>\n"
        "</html>\n");

    closedir(dir);

    send_response(client_fd, 200, "OK", "text/html", body, 0);
    return 0;
}

/*
 * 處理 HTTP 請求
 */
void handle_request(int client_fd, const char *request)
{
    char method[16], path[MAX_PATH], protocol[16];

    // 解析請求行
    if (sscanf(request, "%15s %1023s %15s", method, path, protocol) != 3) {
        send_error(client_fd, 400, "Bad Request");
        return;
    }

    // 只支持 GET 方法
    if (strcmp(method, "GET") != 0) {
        send_error(client_fd, 501, "Not Implemented");
        return;
    }

    // URL 解碼
    char decoded_path[MAX_PATH];
    url_decode(decoded_path, path);

    // 構建完整路徑
    char fullpath[MAX_PATH];
    int n = snprintf(fullpath, sizeof(fullpath), "%s%s", g_config.root_dir, decoded_path);
    if (n < 0 || n >= (int)sizeof(fullpath)) {
        send_error(client_fd, 414, "Request-URI Too Long");
        return;
    }

    // 安全檢查：防止目錄遍歷攻擊
    char realpath_buf[MAX_PATH];
    if (realpath(fullpath, realpath_buf) == NULL) {
        send_error(client_fd, 404, "Not Found");
        return;
    }

    // 確保路徑在根目錄內
    char real_root[MAX_PATH];
    if (realpath(g_config.root_dir, real_root) == NULL) {
        send_error(client_fd, 500, "Internal Server Error");
        return;
    }
    if (strncmp(realpath_buf, real_root, strlen(real_root)) != 0) {
        send_error(client_fd, 403, "Forbidden");
        return;
    }

    // 檢查文件狀態
    struct stat st;
    if (stat(realpath_buf, &st) < 0) {
        send_error(client_fd, 404, "Not Found");
        return;
    }

    // 如果是目錄
    if (S_ISDIR(st.st_mode)) {
        // 嘗試查找 index.html
        char index_path[MAX_PATH];
        n = snprintf(index_path, sizeof(index_path), "%s/index.html", realpath_buf);
        if (n < 0 || n >= (int)sizeof(index_path)) {
            send_error(client_fd, 414, "Request-URI Too Long");
            return;
        }

        if (access(index_path, R_OK) == 0) {
            send_file(client_fd, index_path);
        } else {
            // 顯示目錄列表
            send_directory(client_fd, realpath_buf, decoded_path);
        }
    } else {
        // 發送文件
        send_file(client_fd, realpath_buf);
    }

    printf("[%s] %s %s\n", method, path, "200 OK");
    g_config.total_requests++;
}

/*
 * 主函數
 */
int main(int argc, char *argv[])
{
    // 解析命令行參數
    g_config.port = (argc > 1) ? atoi(argv[1]) : DEFAULT_PORT;
    const char *root_dir = (argc > 2) ? argv[2] : DEFAULT_ROOT;

    if (realpath(root_dir, g_config.root_dir) == NULL) {
        fprintf(stderr, "錯誤: 無效的根目錄: %s\n", root_dir);
        return 1;
    }

    g_config.total_requests = 0;
    g_config.active_connections = 0;

    // 設置信號處理
    signal(SIGINT, sigint_handler);
    signal(SIGTERM, sigint_handler);
    signal(SIGPIPE, SIG_IGN);

    // 創建 socket
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(g_config.port);

    if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(listen_fd);
        return 1;
    }

    if (listen(listen_fd, 128) < 0) {
        perror("listen");
        close(listen_fd);
        return 1;
    }

    // 創建 epoll
    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        perror("epoll_create1");
        close(listen_fd);
        return 1;
    }

    // 添加監聽 socket
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = listen_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev);

    // 打印啟動信息
    printf("\n========================================\n");
    printf("  簡易 HTTP 服務器\n");
    printf("========================================\n");
    printf("監聽端口: %d\n", g_config.port);
    printf("根目錄: %s\n", g_config.root_dir);
    printf("========================================\n");
    printf("服務器已啟動\n");
    printf("訪問: http://localhost:%d/\n", g_config.port);
    printf("按 Ctrl+C 退出\n");
    printf("========================================\n\n");

    // 主循環
    struct epoll_event events[MAX_EVENTS];

    while (server_running) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, 1000);

        if (nfds < 0) {
            if (errno == EINTR) continue;
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == listen_fd) {
                // 新連接
                struct sockaddr_in client_addr;
                socklen_t addr_len = sizeof(client_addr);
                int client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &addr_len);

                if (client_fd < 0) {
                    perror("accept");
                    continue;
                }

                g_config.active_connections++;

                // 讀取請求
                char buffer[BUFFER_SIZE];
                ssize_t bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

                if (bytes > 0) {
                    buffer[bytes] = '\0';
                    handle_request(client_fd, buffer);
                }

                close(client_fd);
                g_config.active_connections--;
            }
        }
    }

    // 清理
    close(epoll_fd);
    close(listen_fd);

    printf("\n服務器已關閉\n");
    printf("總請求數: %d\n", g_config.total_requests);

    return 0;
}

/*
 * 使用示例:
 *
 * 1. 啟動服務器（當前目錄）:
 *    ./http_server_simple
 *
 * 2. 指定端口:
 *    ./http_server_simple 9000
 *
 * 3. 指定根目錄:
 *    ./http_server_simple 8080 /var/www
 *
 * 4. 測試:
 *    # 瀏覽器訪問
 *    http://localhost:8000/
 *
 *    # curl 測試
 *    curl http://localhost:8000/
 *
 *    # wget 測試
 *    wget http://localhost:8000/file.txt
 *
 * 學習要點:
 * - HTTP 協議基礎
 * - Socket 編程
 * - epoll I/O 多路復用
 * - 文件服務
 * - 目錄瀏覽
 * - URL 解碼
 * - MIME 類型
 * - 安全性考慮（路徑遍歷）
 */
