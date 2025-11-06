# UDP Socket 編程

## 📖 概念介紹

**UDP (User Datagram Protocol)** 是無連接的傳輸層協議，提供不可靠的數據報服務。

### UDP vs TCP

| 特性 | TCP | UDP |
|------|-----|-----|
| **連接** | 面向連接 | 無連接 |
| **可靠性** | 可靠（重傳、順序） | 不可靠 |
| **速度** | 較慢 | 快 |
| **開銷** | 大 | 小 |
| **數據邊界** | 無（流式） | 有（數據報） |
| **適用** | 文件傳輸、Web | 視頻、遊戲、DNS |

## 🔧 API 說明

### 創建UDP socket

```c
int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
```

**SOCK_DGRAM** 表示數據報socket（UDP）

### sendto() - 發送數據

```c
ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen);
```

**特點**：
- 每次都需要指定目標地址
- 無連接，直接發送

### recvfrom() - 接收數據

```c
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen);
```

**特點**：
- 可以獲取發送方地址
- 保留數據報邊界

## 📁 範例程式

1. **udp_server.c** - UDP echo服務器
2. **udp_client.c** - UDP客戶端

編譯運行：
```bash
make udp-socket
# 終端1
./udp_server
# 終端2
./udp_client
```

## 💡 UDP 特性

### 無連接

- 無需三次握手
- 無需維護連接狀態
- 發送即走，不關心是否到達

### 數據報邊界

```c
// 發送方
send("Hello", 5);
send("World", 5);

// 接收方：兩次recv，保持邊界
recv(buf, 100);  // "Hello"
recv(buf, 100);  // "World"
```

TCP會合併成 "HelloWorld"

### 不可靠性

- ❌ 不保證送達
- ❌ 不保證順序
- ❌ 不保證不重複
- ✅ 有校驗和（可選）

## ⚠️ 適用場景

**✅ 適合UDP**：
- 實時視頻/音頻
- 在線遊戲
- DNS查詢
- DHCP
- 對少量丟包容忍
- 需要低延遲

**❌ 不適合UDP**：
- 文件傳輸
- 郵件
- Web瀏覽
- 需要可靠傳輸

## 📚 參考資料

- `man sendto`
- `man recvfrom`
- "UNIX Network Programming Vol.1" - Chapter 8
