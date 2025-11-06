# I/O 多路復用 (select/poll)

## 📖 概念介紹

**I/O 多路復用**允許單個線程同時監視多個文件描述符，當某個fd就緒時進行處理。

### 為什麼需要？

**問題**：如何同時處理多個客戶端連接？

**方案對比**：
1. **多進程**：為每個連接fork一個進程 → 資源消耗大
2. **多線程**：為每個連接創建線程 → 開銷較大  
3. **I/O多路復用**：單線程監視多個fd → 高效✓

## 🔧 API 說明

### select()

```c
int select(int nfds, fd_set *readfds, fd_set *writefds,
           fd_set *exceptfds, struct timeval *timeout);

// 操作fd_set的宏
FD_ZERO(&set);       // 清空集合
FD_SET(fd, &set);    // 添加fd
FD_CLR(fd, &set);    // 移除fd
FD_ISSET(fd, &set);  // 檢查fd是否在集合中
```

**限制**：
- ❌ 最多監視1024個fd（FD_SETSIZE）
- ❌ 每次調用需要重新設置fd_set
- ❌ 需要線性掃描所有fd

### poll()

```c
int poll(struct pollfd *fds, nfds_t nfds, int timeout);

struct pollfd {
    int   fd;         // 文件描述符
    short events;     // 關注的事件
    short revents;    // 實際發生的事件
};
```

**改進**：
- ✅ 無fd數量限制
- ✅ 不需要重新設置
- ❌ 仍需線性掃描

## 📁 範例程式

1. **select_server.c** - select實現的echo服務器
2. **poll_server.c** - poll實現的echo服務器

編譯運行：
```bash
make select-poll
# 終端1
./select_server
# 終端2
telnet localhost 9000
```

## 💡 三者對比

| 特性 | select | poll | epoll |
|------|--------|------|-------|
| **fd限制** | 1024 | 無限制 | 無限制 |
| **性能** | O(n) | O(n) | O(1) |
| **重新設置** | 每次 | 無需 | 無需 |
| **事件通知** | LT | LT | LT/ET |
| **跨平台** | ✅ | ✅ | ❌ (Linux) |

**結論**：
- 少量fd（<10）：任選
- 中等fd（<1000）：poll
- 大量fd（>1000）：epoll

## ⚠️ 水平觸發 vs 邊緣觸發

**水平觸發 (LT)**：
- 只要fd就緒就通知
- select/poll都是LT
- 不會丟失事件

**邊緣觸發 (ET)**：
- 只在fd狀態變化時通知
- 僅epoll支持
- 必須一次讀完所有數據

## 📚 參考資料

- `man select`
- `man poll`
- "The Linux Programming Interface" - Chapter 63
