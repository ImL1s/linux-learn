# TCP Socket 網路編程

## 📖 概念介紹

**Socket（套接字）** 是網路編程的基礎，提供進程間通過網路進行通訊的能力。它是應用層與傳輸層之間的接口，允許應用程式通過標準化的API進行網路通訊。

### 什麼是 Socket？

Socket 是通訊端點的抽象，可以理解為網路通訊的「插座」：
- 應用程式通過 Socket 發送和接收數據
- Socket 隱藏了底層網路協議的複雜性
- 提供統一的 API 介面

### Socket 的類型

1. **流式 Socket (SOCK_STREAM)**
   - 使用 TCP 協議
   - 面向連接
   - 可靠、有序、無重複
   - 本主題重點

2. **數據報 Socket (SOCK_DGRAM)**
   - 使用 UDP 協議
   - 無連接
   - 不可靠但快速
   - 見 18-udp-socket

3. **原始 Socket (SOCK_RAW)**
   - 直接訪問底層協議
   - 需要 root 權限
   - 用於特殊應用（如 ping）

## 🔧 TCP Socket API

### 服務器端流程

```c
socket()    // 1. 創建 socket
  ↓
bind()      // 2. 綁定地址和端口
  ↓
listen()    // 3. 監聽連接
  ↓
accept()    // 4. 接受客戶端連接（阻塞）
  ↓
send/recv   // 5. 數據收發
  ↓
close()     // 6. 關閉連接
```

### 客戶端流程

```c
socket()    // 1. 創建 socket
  ↓
connect()   // 2. 連接到服務器（阻塞）
  ↓
send/recv   // 3. 數據收發
  ↓
close()     // 4. 關閉連接
```

### socket() - 創建套接字

```c
#include <sys/socket.h>

int socket(int domain, int type, int protocol);
```

**參數**：
- `domain`: 協議族
  - `AF_INET`: IPv4
  - `AF_INET6`: IPv6
  - `AF_UNIX`: 本地通訊
- `type`: Socket 類型
  - `SOCK_STREAM`: TCP
  - `SOCK_DGRAM`: UDP
- `protocol`: 通常為 0（自動選擇）

**返回值**：成功返回文件描述符，失敗返回 -1

**示例**：
```c
int sockfd = socket(AF_INET, SOCK_STREAM, 0);
if (sockfd < 0) {
    perror("socket");
    exit(1);
}
```

### bind() - 綁定地址

```c
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
```

**作用**：將 socket 綁定到特定的 IP 地址和端口

**地址結構**：
```c
struct sockaddr_in {
    sa_family_t    sin_family;  // AF_INET
    in_port_t      sin_port;    // 端口號（網路字節序）
    struct in_addr sin_addr;    // IP 地址
};
```

**示例**：
```c
struct sockaddr_in server_addr;
memset(&server_addr, 0, sizeof(server_addr));
server_addr.sin_family = AF_INET;
server_addr.sin_addr.s_addr = INADDR_ANY;  // 監聽所有接口
server_addr.sin_port = htons(8888);         // 端口 8888

bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
```

### listen() - 監聽連接

```c
int listen(int sockfd, int backlog);
```

**參數**：
- `sockfd`: socket 文件描述符
- `backlog`: 等待連接隊列的最大長度

**作用**：將 socket 設置為被動監聽模式

**示例**：
```c
listen(sockfd, 5);  // 最多 5 個等待連接
```

### accept() - 接受連接

```c
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
```

**作用**：從已完成連接隊列中取出一個連接

**特點**：
- 阻塞調用（除非設置非阻塞）
- 返回新的 socket 用於與客戶端通訊
- 原 socket 繼續監聽新連接

**示例**：
```c
struct sockaddr_in client_addr;
socklen_t addr_len = sizeof(client_addr);

int client_fd = accept(sockfd, (struct sockaddr*)&client_addr, &addr_len);
if (client_fd < 0) {
    perror("accept");
    exit(1);
}

// 獲取客戶端信息
char client_ip[INET_ADDRSTRLEN];
inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
int client_port = ntohs(client_addr.sin_port);
printf("客戶端連接: %s:%d\n", client_ip, client_port);
```

### connect() - 連接到服務器

```c
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
```

**作用**：客戶端連接到服務器

**示例**：
```c
struct sockaddr_in server_addr;
server_addr.sin_family = AF_INET;
server_addr.sin_port = htons(8888);
inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
    perror("connect");
    exit(1);
}
```

### send/recv - 數據收發

```c
ssize_t send(int sockfd, const void *buf, size_t len, int flags);
ssize_t recv(int sockfd, void *buf, size_t len, int flags);
```

**或使用**：
```c
ssize_t write(int sockfd, const void *buf, size_t count);
ssize_t read(int sockfd, void *buf, size_t count);
```

**示例**：
```c
char buffer[1024];

// 發送
const char *msg = "Hello, Server!";
send(sockfd, msg, strlen(msg), 0);

// 接收
ssize_t n = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
if (n > 0) {
    buffer[n] = '\0';
    printf("收到: %s\n", buffer);
}
```

## 📁 範例程式

### 1. tcp_server.c - TCP 服務器

**功能**：多進程並發 TCP echo 服務器

**特點**：
- 使用 fork() 為每個客戶端創建子進程
- SIGCHLD 處理避免殭屍進程
- SO_REUSEADDR 避免 "Address already in use" 錯誤
- 完整的錯誤處理

**核心代碼**：
```c
// 設置 SO_REUSEADDR
int opt = 1;
setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

// 主循環
while (server_running) {
    client_fd = accept(server_fd, &client_addr, &addr_len);
    
    pid_t pid = fork();
    if (pid == 0) {
        // 子進程處理客戶端
        close(server_fd);
        handle_client(client_fd);
        exit(0);
    }
    close(client_fd);  // 父進程關閉客戶端 fd
}
```

**編譯運行**：
```bash
make socket
./tcp_server 8888
```

### 2. tcp_client.c - TCP 客戶端

**功能**：簡單的 TCP 客戶端

**特點**：
- 域名解析（gethostbyname）
- 互動式命令行界面
- 優雅退出處理

**編譯運行**：
```bash
./tcp_client localhost 8888
# 或
./tcp_client 127.0.0.1 8888
```

**測試**：
```bash
# 終端1：啟動服務器
./tcp_server 8888

# 終端2：連接客戶端
./tcp_client localhost 8888

# 或使用 telnet 測試
telnet localhost 8888

# 或使用 nc（netcat）
nc localhost 8888
```

## 💡 重要概念

### 網路字節序

**問題**：不同CPU架構字節序不同
- 大端序（Big-Endian）：高位字節在前
- 小端序（Little-Endian）：低位字節在前

**解決**：統一使用網路字節序（大端序）

**轉換函數**：
```c
// Host to Network
uint16_t htons(uint16_t hostshort);    // 短整型
uint32_t htonl(uint32_t hostlong);     // 長整型

// Network to Host
uint16_t ntohs(uint16_t netshort);
uint32_t ntohl(uint32_t netlong);
```

**使用**：
```c
// 設置端口
addr.sin_port = htons(8888);  // 主機序 → 網路序

// 讀取端口
int port = ntohs(addr.sin_port);  // 網路序 → 主機序
```

### IP 地址轉換

**字符串 ↔ 二進制**：

```c
// 舊API（僅IPv4）
inet_aton()  // 字符串 → 二進制
inet_ntoa()  // 二進制 → 字符串

// 新API（IPv4/IPv6）
inet_pton()  // 字符串 → 二進制（推薦）
inet_ntop()  // 二進制 → 字符串（推薦）
```

**示例**：
```c
// 字符串 → 二進制
struct in_addr addr;
inet_pton(AF_INET, "192.168.1.1", &addr);

// 二進制 → 字符串
char ip_str[INET_ADDRSTRLEN];
inet_ntop(AF_INET, &addr, ip_str, sizeof(ip_str));
```

### SO_REUSEADDR 選項

**問題**：服務器重啟後可能遇到 "Address already in use" 錯誤

**原因**：TIME_WAIT 狀態（TCP 四次揮手後的等待期）

**解決**：設置 SO_REUSEADDR

```c
int opt = 1;
setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
```

### TCP 三次握手

```
客戶端                    服務器
  |                         |
  |-------- SYN ----------->|  1. 客戶端發送 SYN
  |                         |
  |<----- SYN + ACK --------|  2. 服務器回復 SYN+ACK
  |                         |
  |-------- ACK ----------->|  3. 客戶端發送 ACK
  |                         |
  |   連接建立（ESTABLISHED）  |
```

### TCP 四次揮手

```
客戶端                    服務器
  |                         |
  |-------- FIN ----------->|  1. 客戶端關閉
  |                         |
  |<------- ACK ------------|  2. 服務器確認
  |                         |
  |<------- FIN ------------|  3. 服務器關閉
  |                         |
  |-------- ACK ----------->|  4. 客戶端確認
  |                         |
  |     TIME_WAIT (2MSL)    |
```

## 🔍 並發模型

### 1. 多進程模型（本示例）

```c
while (1) {
    client_fd = accept(server_fd, ...);
    
    pid_t pid = fork();
    if (pid == 0) {
        // 子進程處理
        handle_client(client_fd);
        exit(0);
    }
    close(client_fd);
}
```

**優點**：
- ✅ 簡單易實現
- ✅ 進程隔離，穩定性好
- ✅ 一個進程崩潰不影響其他

**缺點**：
- ❌ 資源消耗大
- ❌ 進程創建開銷大
- ❌ 不適合高並發

### 2. 多線程模型

```c
while (1) {
    client_fd = accept(server_fd, ...);
    
    pthread_t thread;
    pthread_create(&thread, NULL, handle_client, (void*)client_fd);
    pthread_detach(thread);
}
```

**優點**：
- ✅ 比多進程輕量
- ✅ 共享內存，通訊方便

**缺點**：
- ❌ 仍有創建開銷
- ❌ 需要線程安全

### 3. I/O 多路復用（select/poll/epoll）

**見 17-select-poll 和 09-epoll**

**優點**：
- ✅ 單線程處理多連接
- ✅ 資源消耗小
- ✅ 適合高並發

## ⚠️ 常見問題

### Q1: 為什麼 bind() 失敗，提示 "Address already in use"？

**原因**：端口處於 TIME_WAIT 狀態

**解決方案1**：設置 SO_REUSEADDR
```c
int opt = 1;
setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
```

**解決方案2**：等待 2MSL（約 1-4 分鐘）

**解決方案3**：更換端口

### Q2: 為什麼 recv() 返回 0？

**答**：對端關閉了連接

```c
int n = recv(sockfd, buffer, sizeof(buffer), 0);
if (n == 0) {
    printf("對端關閉連接\n");
    close(sockfd);
}
```

### Q3: 如何處理半關閉狀態？

**答**：使用 shutdown() 而不是 close()

```c
shutdown(sockfd, SHUT_WR);  // 關閉寫，但仍可讀
```

### Q4: 如何設置超時？

**方法1**：setsockopt 設置 SO_RCVTIMEO/SO_SNDTIMEO
```c
struct timeval tv;
tv.tv_sec = 5;
tv.tv_usec = 0;
setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
```

**方法2**：使用 select() 配合超時
```c
fd_set readfds;
FD_ZERO(&readfds);
FD_SET(sockfd, &readfds);

struct timeval tv = {5, 0};  // 5 秒
int ret = select(sockfd + 1, &readfds, NULL, NULL, &tv);
```

### Q5: 粘包問題如何解決？

**問題**：TCP 是流式協議，無消息邊界

**解決方案**：
1. **固定長度**：每條消息固定長度
2. **特殊字符分隔**：如 `\n`, `\r\n`
3. **長度前綴**：先發消息長度，再發內容
4. **應用層協議**：如 HTTP, Protocol Buffers

**示例（長度前綴）**：
```c
// 發送
uint32_t len = htonl(strlen(msg));
send(sockfd, &len, sizeof(len), 0);
send(sockfd, msg, strlen(msg), 0);

// 接收
uint32_t len;
recv(sockfd, &len, sizeof(len), 0);
len = ntohl(len);

char *msg = malloc(len + 1);
recv(sockfd, msg, len, 0);
msg[len] = '\0';
```

## 🚀 實際應用

### Web 服務器

```c
// 簡化的 HTTP 服務器
void handle_http_request(int client_fd) {
    char buffer[4096];
    recv(client_fd, buffer, sizeof(buffer), 0);
    
    const char *response = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "\r\n"
        "<h1>Hello, World!</h1>";
    
    send(client_fd, response, strlen(response), 0);
    close(client_fd);
}
```

### 聊天室服務器

```c
// 廣播消息給所有客戶端
void broadcast(const char *msg, int exclude_fd) {
    for (int i = 0; i < client_count; i++) {
        if (clients[i] != exclude_fd) {
            send(clients[i], msg, strlen(msg), 0);
        }
    }
}
```

### 文件傳輸

```c
// 發送文件
FILE *fp = fopen("file.txt", "rb");
char buffer[4096];
size_t n;

while ((n = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
    send(sockfd, buffer, n, 0);
}
fclose(fp);
```

## 📚 參考資料

- **Linux Manual**:
  - `man 2 socket`
  - `man 2 bind`
  - `man 2 listen`
  - `man 2 accept`
  - `man 2 connect`
  - `man 7 tcp`
  - `man 7 ip`

- **書籍**:
  - "UNIX Network Programming Vol.1" by W. Richard Stevens
  - "TCP/IP Illustrated Vol.1" by W. Richard Stevens
  - "The Linux Programming Interface" - Chapter 56-61

- **RFC**:
  - RFC 793: TCP
  - RFC 791: IP

## 🎯 學習建議

1. **理解三次握手和四次揮手**：這是 TCP 的核心
2. **掌握字節序轉換**：網路編程必須處理
3. **熟悉並發模型**：多進程、多線程、I/O多路復用
4. **練習錯誤處理**：網路編程錯誤多，要充分處理
5. **閱讀 Wireshark 抓包**：可視化理解網路協議
6. **實現完整應用**：如聊天室、文件傳輸服務器

## 💻 擴展實驗

1. 修改為多線程服務器
2. 實現連接池機制
3. 添加心跳檢測
4. 實現斷線重連
5. 使用 select/poll/epoll 改進
6. 實現簡單的HTTP服務器
7. 實現聊天室功能
8. 添加 SSL/TLS 加密（OpenSSL）
