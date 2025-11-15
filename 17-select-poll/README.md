# select/poll - I/O 多路復用

## 📖 概念介紹

**I/O 多路復用**允許單個進程/線程同時監控多個文件描述符（socket、文件、管道等）的I/O事件，是實現高並發網絡服務器的核心技術。

### 為什麼需要 I/O 多路復用？

**傳統阻塞 I/O 問題**:
```c
// 每個客戶端需要一個線程
while (1) {
    int client = accept(server_fd, ...);
    pthread_create(&thread, NULL, handle_client, (void*)client);
}
// 10,000 個客戶端 = 10,000 個線程（不可行！）
```

**I/O 多路復用解決方案**:
```c
// 單線程處理所有客戶端
while (1) {
    select(fds);  // 同時監控所有 socket
    for (ready_fd in fds) {
        handle(ready_fd);  // 處理就緒的
    }
}
```

### Linux I/O 多路復用演進

```
性能: 低 → 中 → 高
複雜度: 低 → 中 → 高
連接數: 1024 → 無限

select() (1983)
  ↓
poll() (1997)
  ↓
epoll() (2002) ← Linux專用，最高效
```

## 🔧 select() - 最經典的多路復用

```c
#include <sys/select.h>

int select(int nfds, fd_set *readfds, fd_set *writefds,
           fd_set *exceptfds, struct timeval *timeout);
```

### fd_set 操作宏

```c
FD_ZERO(&set);      // 清空集合
FD_SET(fd, &set);   // 添加 fd
FD_CLR(fd, &set);   // 移除 fd
FD_ISSET(fd, &set); // 檢查 fd 是否在集合中
```

### 參數說明

- `nfds`: 最大 fd + 1
- `readfds`: 監控可讀事件
- `writefds`: 監控可寫事件
- `exceptfds`: 監控異常事件
- `timeout`: 超時時間
  - `NULL`: 永久阻塞
  - `{0, 0}`: 立即返回（輪詢）
  - `{sec, usec}`: 超時時間

### 基本用法

```c
fd_set readfds;
FD_ZERO(&readfds);
FD_SET(server_fd, &readfds);
FD_SET(client1_fd, &readfds);
FD_SET(client2_fd, &readfds);

struct timeval timeout = {5, 0};  // 5秒超時

int ready = select(max_fd + 1, &readfds, NULL, NULL, &timeout);

if (ready > 0) {
    if (FD_ISSET(server_fd, &readfds)) {
        // 有新連接
        int client = accept(server_fd, ...);
    }
    if (FD_ISSET(client1_fd, &readfds)) {
        // client1 有數據可讀
        read(client1_fd, buf, sizeof(buf));
    }
}
```

### select() 的限制

❌ **最大監控數**: 默認 1024 個（FD_SETSIZE）
❌ **性能**: O(n) 掃描所有 fd
❌ **修改**: 每次調用後 fd_set 被修改，需重建
❌ **可移植性**: Windows 不支持非 socket

## 🔧 poll() - select() 的改進版

```c
#include <poll.h>

int poll(struct pollfd *fds, nfds_t nfds, int timeout);
```

### pollfd 結構

```c
struct pollfd {
    int   fd;        // 文件描述符
    short events;    // 監控的事件
    short revents;   // 實際發生的事件（由內核填充）
};
```

### 事件類型

```c
POLLIN     // 有數據可讀
POLLOUT    // 可以寫入數據
POLLERR    // 錯誤條件
POLLHUP    // 掛起（連接關閉）
POLLNVAL   // 無效的 fd
```

### 基本用法

```c
struct pollfd fds[MAX_CLIENTS];

// 初始化
fds[0].fd = server_fd;
fds[0].events = POLLIN;
fds[1].fd = client1_fd;
fds[1].events = POLLIN | POLLOUT;

int ready = poll(fds, 2, 5000);  // 5秒超時

if (ready > 0) {
    for (int i = 0; i < 2; i++) {
        if (fds[i].revents & POLLIN) {
            // 可讀
            read(fds[i].fd, buf, sizeof(buf));
        }
        if (fds[i].revents & POLLOUT) {
            // 可寫
            write(fds[i].fd, data, len);
        }
        if (fds[i].revents & POLLHUP) {
            // 連接關閉
            close(fds[i].fd);
        }
    }
}
```

### poll() 優勢

✅ **無 1024 限制**: 可監控任意數量
✅ **不修改輸入**: events 和 revents 分離
✅ **更清晰**: 每個 fd 獨立配置

❌ **性能仍是 O(n)**

## 📊 select vs poll vs epoll

| 特性 | select | poll | epoll |
|------|--------|------|-------|
| **最大連接數** | 1024 | 無限制 | 無限制 |
| **性能** | O(n) | O(n) | O(1) |
| **數據結構** | fd_set | pollfd[] | 紅黑樹+雙鏈表 |
| **事件通知** | 水平觸發 | 水平觸發 | 水平/邊緣 |
| **修改輸入** | ✅ | ❌ | ❌ |
| **可移植性** | 最好 | 好 | Linux專用 |
| **適用場景** | 少量連接 | 中等連接 | 大量連接 |

### 性能測試

```
監控 10,000 個連接，其中 10 個活躍:

select: ~10ms (掃描所有10,000個)
poll:   ~10ms (掃描所有10,000個)
epoll:  ~0.1ms (僅處理10個活躍的) ✅ 快100倍
```

## 🎯 實戰應用

### 應用 1: Echo 服務器（select）

```c
int server_fd = create_server(PORT);
int client_fds[MAX_CLIENTS];
int max_fd = server_fd;

while (1) {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(server_fd, &readfds);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_fds[i] > 0) {
            FD_SET(client_fds[i], &readfds);
            max_fd = MAX(max_fd, client_fds[i]);
        }
    }

    int ready = select(max_fd + 1, &readfds, NULL, NULL, NULL);

    // 新連接
    if (FD_ISSET(server_fd, &readfds)) {
        int client = accept(server_fd, NULL, NULL);
        add_client(client_fds, client);
    }

    // 處理現有連接
    for (int i = 0; i < MAX_CLIENTS; i++) {
        int fd = client_fds[i];
        if (fd > 0 && FD_ISSET(fd, &readfds)) {
            char buf[1024];
            int n = read(fd, buf, sizeof(buf));
            if (n > 0) {
                write(fd, buf, n);  // echo
            } else {
                close(fd);
                client_fds[i] = 0;
            }
        }
    }
}
```

### 應用 2: 超時檢測（poll）

```c
struct pollfd fds[1];
fds[0].fd = client_fd;
fds[0].events = POLLIN;

// 10秒超時
int ret = poll(fds, 1, 10000);

if (ret == 0) {
    printf("超時！\n");
} else if (ret > 0 && (fds[0].revents & POLLIN)) {
    read(client_fd, buf, sizeof(buf));
}
```

### 應用 3: 多協議服務器

```c
// 同時處理 TCP、UDP、管道
struct pollfd fds[3];
fds[0].fd = tcp_fd;
fds[0].events = POLLIN;
fds[1].fd = udp_fd;
fds[1].events = POLLIN;
fds[2].fd = pipe_fd;
fds[2].events = POLLIN;

while (1) {
    poll(fds, 3, -1);

    if (fds[0].revents & POLLIN) handle_tcp();
    if (fds[1].revents & POLLIN) handle_udp();
    if (fds[2].revents & POLLIN) handle_pipe();
}
```

## 📁 範例程式

### 1. select_server.c

**功能**: 使用 select 實現多客戶端 echo 服務器

**核心代碼**: 見上方應用1

### 2. poll_server.c

**功能**: 使用 poll 實現多客戶端 echo 服務器

### 編譯與運行

```bash
make select-poll

# 終端1
./select_server
# 監聽 9000 端口

# 終端2-4
telnet localhost 9000
# 輸入文本，服務器會echo回來
```

## 💡 重要知識點

### 1. 水平觸發 vs 邊緣觸發

**水平觸發 (Level-Triggered)**:
- select 和 poll 只支持水平觸發
- 只要有數據可讀，每次 select/poll 都會返回
- **優點**: 不會遺漏事件
- **缺點**: 如果不讀完，會重複觸發

**邊緣觸發 (Edge-Triggered)**:
- 僅 epoll 支持
- 只在狀態變化時觸發一次
- 必須一次性讀完數據

### 2. 驚群效應

```c
// 多個進程同時 select 同一個監聽 socket
// 當新連接到來時，所有進程都被喚醒
// 但只有一個能 accept 成功
// 其他進程白白浪費 CPU

// 解決：僅一個進程/線程監聽
```

### 3. select 的 nfds 參數

```c
// ❌ 錯誤
select(FD_SETSIZE, &readfds, ...);

// ✅ 正確
int max_fd = get_max_fd();
select(max_fd + 1, &readfds, ...);
```

## 🐛 常見陷阱

### 陷阱 1: 忘記重建 fd_set

```c
// ❌ 錯誤
fd_set readfds;
FD_ZERO(&readfds);
FD_SET(fd1, &readfds);

while (1) {
    select(max_fd + 1, &readfds, ...);  // readfds 被修改！
    // 第二次循環時 fd1 不在 readfds 中了
}

// ✅ 正確
while (1) {
    fd_set tmp = readfds;  // 每次複製
    select(max_fd + 1, &tmp, ...);
}
```

### 陷阱 2: 未檢查返回值

```c
// ❌ 危險
select(max_fd + 1, &readfds, ...);
// 可能返回 -1 (被信號中斷)

// ✅ 正確
int ret = select(...);
if (ret == -1 && errno == EINTR) {
    continue;  // 被信號中斷，重試
}
```

### 陷阱 3: poll 返回值理解錯誤

```c
int ret = poll(fds, nfds, timeout);

// ret > 0:  有 ret 個 fd 就緒
// ret == 0: 超時
// ret == -1: 錯誤

// ⚠️ 注意：仍需檢查每個 fd 的 revents
```

## 📚 參考資料

- `man select` - select 系統調用
- `man poll` - poll 系統調用
- `man 7 epoll` - epoll 機制
- [The C10K Problem](http://www.kegel.com/c10k.html)

---

## 📝 總結

### 何時使用？

✅ **select**: 連接數 < 100，需要跨平台
✅ **poll**: 連接數 < 1000，需要更清晰的 API
✅ **epoll**: 連接數 > 1000，Linux 專用，追求性能

### 關鍵要點

1. **select 的 1024 限制**: 使用 poll 或 epoll
2. **重建 fd_set**: select 會修改輸入
3. **O(n) 性能**: 大量連接時使用 epoll
4. **超時處理**: timeout 參數很有用
5. **信號中斷**: 處理 EINTR 錯誤

select/poll 是理解 I/O 多路復用的基礎，但在生產環境中，Linux 服務器應優先使用 epoll！
