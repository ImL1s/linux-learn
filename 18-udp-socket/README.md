# UDP Socket - 用戶數據報協議

## 📖 概念介紹

**UDP (User Datagram Protocol)** 是一種無連接、不可靠的傳輸層協議。與 TCP 不同，UDP 不保證數據傳輸的可靠性、順序性和完整性，但提供了更低的延遲和更高的效率。

### TCP vs UDP

```
TCP (Transmission Control Protocol):
  [連接] → [數據傳輸] → [關閉連接]
  - 可靠傳輸（確認、重傳）
  - 有序到達
  - 面向連接
  - 流量控制
  - 適用於：HTTP、FTP、SSH

UDP (User Datagram Protocol):
  [直接發送數據報]
  - 不可靠（可能丟失、亂序、重複）
  - 無連接
  - 無流量控制
  - 低延遲
  - 適用於：DNS、視頻流、遊戲
```

## 📊 TCP vs UDP 對比

| 特性 | TCP | UDP |
|------|-----|-----|
| **連接** | 面向連接 | 無連接 |
| **可靠性** | ✅ 可靠 | ❌ 不可靠 |
| **順序** | ✅ 保證 | ❌ 不保證 |
| **速度** | 慢（3次握手） | 快 |
| **開銷** | 大（20字節頭部） | 小（8字節頭部） |
| **流量控制** | ✅ 有 | ❌ 無 |
| **擁塞控制** | ✅ 有 | ❌ 無 |
| **廣播/多播** | ❌ 不支持 | ✅ 支持 |
| **適用場景** | 文件傳輸、網頁 | 流媒體、遊戲、DNS |

### 何時使用 UDP？

✅ **適合 UDP**:
- 實時音視頻（容忍丟包）
- 在線遊戲（低延遲優先）
- DNS 查詢（請求-響應簡單）
- 流媒體直播
- IoT 設備心跳
- 局域網設備發現（廣播）

❌ **不適合 UDP**:
- 文件傳輸
- 郵件發送
- 重要數據傳輸
- 需要順序保證的場景

## 🔧 UDP Socket API

### 創建 UDP Socket

```c
#include <sys/socket.h>

int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
//                            ^^^^^^^^^^^
//                            DGRAM = UDP
```

### 綁定地址 (服務器端)

```c
struct sockaddr_in addr;
addr.sin_family = AF_INET;
addr.sin_port = htons(PORT);
addr.sin_addr.s_addr = INADDR_ANY;

bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));
```

### 發送數據

```c
ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen);
```

**示例**:
```c
struct sockaddr_in server_addr;
server_addr.sin_family = AF_INET;
server_addr.sin_port = htons(8888);
inet_pton(AF_INET, "192.168.1.100", &server_addr.sin_addr);

char *msg = "Hello UDP";
sendto(sockfd, msg, strlen(msg), 0,
       (struct sockaddr*)&server_addr, sizeof(server_addr));
```

### 接收數據

```c
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen);
```

**示例**:
```c
char buf[1024];
struct sockaddr_in client_addr;
socklen_t addr_len = sizeof(client_addr);

int n = recvfrom(sockfd, buf, sizeof(buf), 0,
                 (struct sockaddr*)&client_addr, &addr_len);

printf("收到 %d 字節，來自 %s:%d\n", n,
       inet_ntoa(client_addr.sin_addr),
       ntohs(client_addr.sin_port));
```

## 🎯 UDP 編程模式

### 模式 1: 簡單請求-響應

```c
// 客戶端
char request[] = "GET_DATA";
sendto(sockfd, request, strlen(request), 0, &server_addr, addr_len);

char response[1024];
recvfrom(sockfd, response, sizeof(response), 0, NULL, NULL);

// 服務器
char request[1024];
struct sockaddr_in client_addr;
socklen_t client_len = sizeof(client_addr);

recvfrom(sockfd, request, sizeof(request), 0,
         (struct sockaddr*)&client_addr, &client_len);

char response[] = "DATA_HERE";
sendto(sockfd, response, strlen(response), 0,
       (struct sockaddr*)&client_addr, client_len);
```

### 模式 2: 連接模式 (connect)

```c
// UDP 可以調用 connect（僅設置默認目標）
connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));

// 之後可以使用 send/recv 而不是 sendto/recvfrom
send(sockfd, buf, len, 0);
recv(sockfd, buf, sizeof(buf), 0);

// 優點：簡化 API，內核過濾非目標地址的數據
```

### 模式 3: 廣播

```c
// 服務器啟用廣播
int broadcast = 1;
setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST,
           &broadcast, sizeof(broadcast));

struct sockaddr_in broadcast_addr;
broadcast_addr.sin_family = AF_INET;
broadcast_addr.sin_port = htons(9999);
broadcast_addr.sin_addr.s_addr = inet_addr("255.255.255.255");

// 發送廣播
sendto(sockfd, msg, len, 0,
       (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr));

// 局域網內所有監聽 9999 端口的設備都能收到
```

### 模式 4: 多播

```c
// 加入多播組
struct ip_mreq mreq;
mreq.imr_multiaddr.s_addr = inet_addr("239.0.0.1");  // 多播地址
mreq.imr_interface.s_addr = INADDR_ANY;

setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
           &mreq, sizeof(mreq));

// 發送到多播組
struct sockaddr_in multicast_addr;
multicast_addr.sin_family = AF_INET;
multicast_addr.sin_addr.s_addr = inet_addr("239.0.0.1");
multicast_addr.sin_port = htons(8888);

sendto(sockfd, msg, len, 0,
       (struct sockaddr*)&multicast_addr, sizeof(multicast_addr));
```

## 📁 範例程式

### 1. udp_server.c

**功能**: UDP echo 服務器

**核心代碼**:
```c
int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

struct sockaddr_in server_addr;
server_addr.sin_family = AF_INET;
server_addr.sin_port = htons(8888);
server_addr.sin_addr.s_addr = INADDR_ANY;

bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));

while (1) {
    char buf[1024];
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int n = recvfrom(sockfd, buf, sizeof(buf), 0,
                     (struct sockaddr*)&client_addr, &client_len);

    printf("收到: %.*s\n", n, buf);

    // Echo 回去
    sendto(sockfd, buf, n, 0,
           (struct sockaddr*)&client_addr, client_len);
}
```

### 2. udp_client.c

**功能**: UDP 客戶端

**核心代碼**:
```c
int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

struct sockaddr_in server_addr;
server_addr.sin_family = AF_INET;
server_addr.sin_port = htons(8888);
inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

char *msg = "Hello UDP!";
sendto(sockfd, msg, strlen(msg), 0,
       (struct sockaddr*)&server_addr, sizeof(server_addr));

char buf[1024];
int n = recvfrom(sockfd, buf, sizeof(buf), 0, NULL, NULL);
printf("服務器響應: %.*s\n", n, buf);

close(sockfd);
```

### 編譯與運行

```bash
make udp-socket

# 終端1: 啟動服務器
./udp_server
# 監聽 8888 端口

# 終端2: 運行客戶端
./udp_client
# 發送 "Hello UDP!"
# 收到回顯: Hello UDP!
```

## 💡 重要知識點

### 1. UDP 數據報大小限制

```c
// 理論最大: 65507 字節 (65535 - 8字節UDP頭 - 20字節IP頭)
// 實際建議: <= 1472 字節（避免 IP 分片）

// 以太網 MTU = 1500
// - 20 字節 IP 頭
// - 8 字節 UDP 頭
// = 1472 字節有效載荷

// 超過會觸發 IP 分片，降低可靠性
```

### 2. UDP 不保證順序

```c
// 發送方
sendto(sockfd, "packet1", 7, ...);
sendto(sockfd, "packet2", 7, ...);
sendto(sockfd, "packet3", 7, ...);

// 接收方可能收到順序:
// packet2, packet1, packet3 ⚠️ 順序不同
// 或 packet1, packet3 ⚠️ packet2 丟失
```

**應對**:
```c
struct UDPPacket {
    uint32_t seq_num;  // 序列號
    char data[1024];
};

// 發送時添加序列號
// 接收時根據序列號重排序
```

### 3. UDP 丟包問題

```c
// UDP 不保證送達
// 網絡擁塞時可能丟包

// 應對策略:
// 1. 應用層重傳
// 2. 添加超時機制
// 3. 使用 ACK 確認
```

**簡單 ACK 機制**:
```c
// 客戶端
send_packet(data);
set_timeout(500ms);

if (recv_ack() == TIMEOUT) {
    retransmit(data);  // 重傳
}
```

### 4. recvfrom 的地址參數

```c
// 獲取發送方地址
struct sockaddr_in client_addr;
socklen_t len = sizeof(client_addr);
recvfrom(sockfd, buf, size, 0,
         (struct sockaddr*)&client_addr, &len);
// client_addr 現在包含發送方 IP 和端口

// 忽略發送方地址
recvfrom(sockfd, buf, size, 0, NULL, NULL);
```

## 🎓 實戰應用

### 應用 1: 簡單的 DNS 查詢

```c
// DNS 使用 UDP 端口 53
struct sockaddr_in dns_server;
dns_server.sin_family = AF_INET;
dns_server.sin_port = htons(53);
inet_pton(AF_INET, "8.8.8.8", &dns_server.sin_addr);

// 構造 DNS 查詢報文
char query[512];
build_dns_query(query, "example.com");

sendto(sockfd, query, query_len, 0,
       (struct sockaddr*)&dns_server, sizeof(dns_server));

char response[512];
recvfrom(sockfd, response, sizeof(response), 0, NULL, NULL);
parse_dns_response(response);
```

### 應用 2: 心跳檢測

```c
// 設備每秒發送心跳
while (1) {
    char heartbeat[] = "ALIVE";
    sendto(sockfd, heartbeat, 5, 0, &server_addr, addr_len);
    sleep(1);
}

// 服務器檢測超時
struct timeval timeout = {3, 0};  // 3秒超時
setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,
           &timeout, sizeof(timeout));

if (recvfrom(...) == -1 && errno == EAGAIN) {
    printf("設備離線！\n");
}
```

### 應用 3: 局域網設備發現

```c
// 廣播發現請求
int broadcast = 1;
setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));

struct sockaddr_in broadcast_addr;
broadcast_addr.sin_family = AF_INET;
broadcast_addr.sin_port = htons(9999);
broadcast_addr.sin_addr.s_addr = inet_addr("255.255.255.255");

char discovery[] = "WHO_IS_THERE";
sendto(sockfd, discovery, strlen(discovery), 0,
       (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr));

// 監聽響應
while (1) {
    struct sockaddr_in device_addr;
    socklen_t len = sizeof(device_addr);
    char response[1024];

    int n = recvfrom(sockfd, response, sizeof(response), 0,
                     (struct sockaddr*)&device_addr, &len);

    printf("發現設備: %s:%d - %.*s\n",
           inet_ntoa(device_addr.sin_addr),
           ntohs(device_addr.sin_port),
           n, response);
}
```

## 🐛 常見陷阱

### 陷阱 1: 緩衝區溢出

```c
// ❌ 危險
char buf[10];
recvfrom(sockfd, buf, 1024, ...);  // 緩衝區只有10字節！

// ✅ 正確
char buf[1024];
recvfrom(sockfd, buf, sizeof(buf), ...);
```

### 陷阱 2: 未檢查返回值

```c
// ❌ 危險
sendto(sockfd, buf, len, ...);
// sendto 可能返回 -1

// ✅ 正確
if (sendto(...) == -1) {
    perror("sendto");
}
```

### 陷阱 3: 忘記網絡字節序

```c
// ❌ 錯誤
addr.sin_port = 8888;  // 主機字節序

// ✅ 正確
addr.sin_port = htons(8888);  // 網絡字節序
```

### 陷阱 4: 多線程競爭

```c
// ⚠️ 多個線程同時 recvfrom 同一個 socket
// 可能導致數據包分配混亂

// ✅ 解決：
// 1. 僅一個線程接收
// 2. 或每個線程使用獨立的 socket
```

## 📚 參考資料

### 官方文檔
- `man 7 udp` - UDP 協議
- `man sendto` - 發送 UDP 數據
- `man recvfrom` - 接收 UDP 數據
- `man 7 socket` - Socket API

### 推薦書籍
- *UNIX Network Programming Vol. 1* - W. Richard Stevens
- *TCP/IP Illustrated Vol. 1* - W. Richard Stevens

---

## 📝 總結

### TCP vs UDP 選擇

✅ **使用 UDP**:
- 實時性要求高
- 可容忍丟包
- 廣播/多播需求
- 輕量級通訊

✅ **使用 TCP**:
- 可靠性要求高
- 數據完整性重要
- 大量數據傳輸
- 需要流量控制

### 關鍵要點

1. **無連接**: 無需 listen/accept/connect
2. **不可靠**: 自行處理丟包、重傳、順序
3. **數據報邊界**: 保持完整的消息邊界
4. **MTU 限制**: 建議 <= 1472 字節
5. **適用場景**: 實時、容錯、廣播

UDP 是網絡編程的重要組成部分，理解其特性和限制，才能在合適的場景中發揮其優勢！
