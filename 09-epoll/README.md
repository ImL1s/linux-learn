# epoll - 高性能 I/O 多路復用

## 📖 概念介紹

**epoll** 是 Linux 特有的高性能 I/O 多路復用機制，能夠高效地處理大量並發連接。它是 select 和 poll 的改進版本，特別適合高並發場景。

### 為什麼需要 epoll？

**傳統問題**（select/poll）：
- ❌ 每次調用都需要從用戶空間複製 fd 集合到內核
- ❌ 返回後需要線性掃描所有 fd 找出就緒的（O(n)）
- ❌ select 有 1024 個 fd 的限制

**epoll 解決方案**：
- ✅ 只需一次設置，內核維護 fd 集合
- ✅ 返回就緒的 fd 列表（O(1)）
- ✅ 無 fd 數量限制
- ✅ 支持邊緣觸發模式（ET）

### select vs poll vs epoll

| 特性 | select | poll | epoll |
|------|--------|------|-------|
| **fd 限制** | 1024 | 無限制 | 無限制 |
| **性能** | O(n) | O(n) | O(1) |
| **複製開銷** | 每次 | 每次 | 只一次 |
| **觸發模式** | LT | LT | LT/ET |
| **跨平台** | ✅ | ✅ | ❌ Linux |
| **適用場景** | <10 fd | <1000 fd | 高並發 |

## 🔧 epoll API

### 三個核心函數

```c
#include <sys/epoll.h>

int epoll_create1(int flags);  // 創建 epoll 實例
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);  // 控制
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);  // 等待
```

### epoll_create1() - 創建實例

```c
int epoll_create1(int flags);
```

**參數**：
- `flags`: 
  - `0`: 默認行為
  - `EPOLL_CLOEXEC`: close-on-exec

**返回值**：epoll 文件描述符

**示例**：
```c
int epoll_fd = epoll_create1(0);
if (epoll_fd == -1) {
    perror("epoll_create1");
    exit(1);
}
```

**舊API**：`epoll_create(int size)` - size 參數被忽略，建議用 epoll_create1

### epoll_ctl() - 控制 epoll

```c
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
```

**參數**：
- `epfd`: epoll 實例
- `op`: 操作類型
  - `EPOLL_CTL_ADD`: 添加 fd
  - `EPOLL_CTL_MOD`: 修改 fd 的事件
  - `EPOLL_CTL_DEL`: 刪除 fd
- `fd`: 要操作的文件描述符
- `event`: 事件結構

**事件結構**：
```c
struct epoll_event {
    uint32_t     events;    // 事件類型
    epoll_data_t data;      // 用戶數據
};

typedef union epoll_data {
    void        *ptr;
    int          fd;
    uint32_t     u32;
    uint64_t     u64;
} epoll_data_t;
```

**事件類型**：
- `EPOLLIN`: 可讀
- `EPOLLOUT`: 可寫
- `EPOLLERR`: 錯誤
- `EPOLLHUP`: 掛起
- `EPOLLET`: 邊緣觸發（Edge Triggered）
- `EPOLLONESHOT`: 一次性事件

**示例**：
```c
struct epoll_event ev;
ev.events = EPOLLIN | EPOLLET;  // 可讀 + 邊緣觸發
ev.data.fd = sockfd;

// 添加
epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &ev);

// 修改
ev.events = EPOLLIN | EPOLLOUT;
epoll_ctl(epoll_fd, EPOLL_CTL_MOD, sockfd, &ev);

// 刪除
epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sockfd, NULL);
```

### epoll_wait() - 等待事件

```c
int epoll_wait(int epfd, struct epoll_event *events, 
               int maxevents, int timeout);
```

**參數**：
- `epfd`: epoll 實例
- `events`: 用於接收就緒事件的數組
- `maxevents`: events 數組大小
- `timeout`: 超時時間（毫秒）
  - `-1`: 永久阻塞
  - `0`: 立即返回
  - `>0`: 超時時間

**返回值**：就緒的 fd 數量

**示例**：
```c
struct epoll_event events[MAX_EVENTS];

int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
if (nfds == -1) {
    perror("epoll_wait");
    exit(1);
}

for (int i = 0; i < nfds; i++) {
    int fd = events[i].data.fd;
    
    if (events[i].events & EPOLLIN) {
        // 可讀事件
        handle_read(fd);
    }
    if (events[i].events & EPOLLOUT) {
        // 可寫事件
        handle_write(fd);
    }
}
```

## 💡 核心概念

### 水平觸發 (LT) vs 邊緣觸發 (ET)

#### 水平觸發（Level Triggered）- 默認

**特點**：
- 只要 fd 處於就緒狀態，epoll_wait 就會通知
- 類似 select/poll 的行為
- 不會丟失事件

**示例**：
```c
// buffer 中有 100 字節數據
epoll_wait();  // 通知可讀
read(fd, buf, 50);  // 只讀 50 字節

epoll_wait();  // 再次通知可讀（還有 50 字節）
```

**優點**：
- ✅ 簡單，不易出錯
- ✅ 不會丟失事件
- ✅ 可以分多次讀取

**缺點**：
- ❌ 可能產生大量通知

#### 邊緣觸發（Edge Triggered）

**特點**：
- 只在 fd 狀態變化時通知一次
- 高性能，但需要正確處理
- 必須一次性讀完所有數據

**示例**：
```c
// buffer 中有 100 字節數據
epoll_wait();  // 通知可讀
read(fd, buf, 50);  // 只讀 50 字節

epoll_wait();  // 不會通知！（狀態未變化）
// 數據丟失！
```

**正確用法**：
```c
// 設置非阻塞 I/O
int flags = fcntl(fd, F_GETFL, 0);
fcntl(fd, F_SETFL, flags | O_NONBLOCK);

// 邊緣觸發
ev.events = EPOLLIN | EPOLLET;
epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);

// 循環讀取直到 EAGAIN
while (1) {
    int n = read(fd, buffer, sizeof(buffer));
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            break;  // 數據讀完
        }
        perror("read");
        break;
    }
    if (n == 0) {
        // 連接關閉
        break;
    }
    // 處理數據
}
```

**對比總結**：

| 特性 | LT | ET |
|------|----|----|
| **複雜度** | 簡單 | 複雜 |
| **性能** | 一般 | 高 |
| **通知次數** | 多 | 少 |
| **是否需要非阻塞** | 可選 | 必須 |
| **是否需要循環讀取** | 否 | 是 |
| **是否會丟事件** | 否 | 不正確使用會 |

### 非阻塞 I/O

**為什麼 ET 模式需要非阻塞 I/O？**

```c
// ❌ 錯誤：阻塞 I/O + ET
// 如果沒有更多數據，read() 會阻塞整個程序
while (1) {
    read(fd, buf, size);  // 可能阻塞！
}

// ✅ 正確：非阻塞 I/O + ET
while (1) {
    int n = read(fd, buf, size);
    if (n < 0 && errno == EAGAIN) {
        break;  // 數據讀完，不會阻塞
    }
}
```

**設置非阻塞**：
```c
int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
```

## 📁 範例程式

### epoll_server.c - 高性能 epoll 服務器

**特點**：
- 邊緣觸發（ET）模式
- 非阻塞 I/O
- 單線程處理數萬並發
- 完整的錯誤處理

**核心邏輯**：
```c
// 1. 創建 epoll
int epoll_fd = epoll_create1(0);

// 2. 添加監聽 socket（ET模式）
struct epoll_event ev;
ev.events = EPOLLIN | EPOLLET;
ev.data.fd = listen_fd;
epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev);

// 3. 主循環
while (1) {
    int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    
    for (int i = 0; i < nfds; i++) {
        int fd = events[i].data.fd;
        
        if (fd == listen_fd) {
            // 接受新連接
            while (1) {
                int conn_fd = accept(listen_fd, NULL, NULL);
                if (conn_fd == -1) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        break;  // 所有連接已接受
                    }
                    break;
                }
                set_nonblocking(conn_fd);
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = conn_fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_fd, &ev);
            }
        } else {
            // 處理客戶端數據（ET模式，必須循環讀取）
            while (1) {
                int n = read(fd, buffer, sizeof(buffer));
                if (n < 0) {
                    if (errno == EAGAIN) break;
                    close(fd);
                    break;
                }
                if (n == 0) {
                    close(fd);
                    break;
                }
                // 處理數據
                write(fd, buffer, n);
            }
        }
    }
}
```

**編譯運行**：
```bash
make epoll
./epoll_server 9999

# 測試
telnet localhost 9999
# 或
nc localhost 9999
```

## 🚀 性能測試

### 對比測試（處理 10000 個並發連接）

| 機制 | CPU 使用率 | 內存 | 吞吐量 |
|------|----------|------|-------|
| **select** | 95% | 高 | 低 |
| **poll** | 80% | 中 | 中 |
| **epoll (LT)** | 40% | 低 | 高 |
| **epoll (ET)** | 25% | 低 | 很高 |

### 為什麼 epoll 快？

1. **內核數據結構優化**
   - select/poll: 每次傳遞整個 fd 集合
   - epoll: 內核維護紅黑樹，只返回就緒的 fd

2. **就緒列表**
   - select/poll: 遍歷所有 fd（O(n)）
   - epoll: 直接獲取就緒列表（O(1)）

3. **回調機制**
   - epoll 使用回調，fd 就緒時主動加入就緒列表
   - 無需遍歷

## ⚠️ 常見陷阱

### 陷阱1：ET 模式忘記循環讀取

```c
// ❌ 錯誤
if (events[i].events & EPOLLIN) {
    read(fd, buf, sizeof(buf));  // 可能還有數據未讀！
}

// ✅ 正確
if (events[i].events & EPOLLIN) {
    while (1) {
        int n = read(fd, buf, sizeof(buf));
        if (n < 0 && errno == EAGAIN) break;
        // 處理數據
    }
}
```

### 陷阱2：忘記設置非阻塞

```c
// ❌ 錯誤：ET + 阻塞 I/O
ev.events = EPOLLIN | EPOLLET;
epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
read(fd, buf, size);  // 可能阻塞整個程序！

// ✅ 正確：ET + 非阻塞 I/O
set_nonblocking(fd);
ev.events = EPOLLIN | EPOLLET;
epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
```

### 陷阱3：修改事件時未用 EPOLL_CTL_MOD

```c
// ❌ 錯誤：直接用 ADD
ev.events = EPOLLIN | EPOLLOUT;
epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);  // 失敗！fd 已存在

// ✅ 正確：使用 MOD
ev.events = EPOLLIN | EPOLLOUT;
epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
```

### 陷阱4：關閉 fd 後未從 epoll 刪除

```c
// ❌ 錯誤
close(fd);  // fd 自動從 epoll 移除，但可能出問題

// ✅ 更好：顯式刪除
epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
close(fd);
```

## 🔍 高級用法

### EPOLLONESHOT - 一次性事件

**用途**：防止多線程中同一 fd 被多次處理

```c
ev.events = EPOLLIN | EPOLLONESHOT;
epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);

// 處理後需要重新添加
ev.events = EPOLLIN | EPOLLONESHOT;
epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
```

### EPOLLEXCLUSIVE - 獨占喚醒

**用途**：多進程/多線程場景，避免驚群

```c
ev.events = EPOLLIN | EPOLLEXCLUSIVE;
```

### epoll + 線程池

```c
// 主線程處理 accept
while (1) {
    int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    for (int i = 0; i < nfds; i++) {
        if (events[i].data.fd == listen_fd) {
            // accept 新連接
            int conn_fd = accept(listen_fd, NULL, NULL);
            // 交給線程池處理
            thread_pool_submit(handle_connection, (void*)conn_fd);
        }
    }
}
```

## 📚 實際應用場景

### 1. 高性能 Web 服務器
- Nginx 使用 epoll
- 單進程處理數萬連接

### 2. 實時通訊服務器
- 聊天服務器
- 遊戲服務器

### 3. 代理服務器
- 反向代理
- 負載均衡器

### 4. 數據庫連接池
- Redis 使用 epoll
- 高並發查詢處理

## 📚 參考資料

- **Linux Manual**:
  - `man epoll`
  - `man epoll_create1`
  - `man epoll_ctl`
  - `man epoll_wait`
  - `man 7 epoll`

- **書籍**:
  - "The Linux Programming Interface" - Chapter 63
  - "Linux System Programming" by Robert Love

- **源碼**:
  - Nginx epoll 實現
  - Redis ae_epoll.c

## 🎯 學習路徑

1. **先理解 select/poll**（見 17-select-poll）
2. **掌握 epoll LT 模式**（與 select 相似）
3. **理解非阻塞 I/O**
4. **學習 ET 模式**（高性能關鍵）
5. **實戰項目**：高並發服務器

## 💻 擴展實驗

1. 實現 LT 模式的 epoll 服務器
2. 對比 LT vs ET 性能差異
3. 測試不同並發連接數下的表現
4. 實現 epoll + 線程池
5. 添加定時器功能（timerfd）
6. 實現簡單的 HTTP 服務器
7. 使用 wrk 進行壓力測試
