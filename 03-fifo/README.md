# 命名管道 (FIFO - First In First Out)

## 📖 概念介紹

**命名管道 (Named Pipe 或 FIFO)** 是一種特殊類型的文件，它提供了一種讓**無親緣關係的進程**進行通訊的機制。與匿名管道不同，FIFO 在文件系統中有一個實際的路徑名。

### FIFO vs 匿名管道

| 特性 | 匿名管道 (Pipe) | 命名管道 (FIFO) |
|------|----------------|----------------|
| 文件名 | 無 | 有（文件系統中） |
| 進程關係 | 必須有親緣關係 | 可以無親緣關係 |
| 創建方式 | `pipe()` | `mkfifo()` |
| 通訊方向 | 單向 | 單向 |
| 生命週期 | 進程結束即消失 | 需手動刪除 |
| 使用場景 | 父子進程通訊 | 任意進程通訊 |

## 🔧 系統調用

### mkfifo() - 創建 FIFO

```c
#include <sys/types.h>
#include <sys/stat.h>

int mkfifo(const char *pathname, mode_t mode);
```

- **參數**:
  - `pathname`: FIFO 文件的路徑
  - `mode`: 文件權限 (如 0666)
- **返回值**: 成功返回 0，失敗返回 -1

### open() - 打開 FIFO

```c
int fd = open(pathname, O_RDONLY);  // 只讀
int fd = open(pathname, O_WRONLY);  // 只寫
```

### 其他相關函數

- `read()` - 從 FIFO 讀取
- `write()` - 向 FIFO 寫入
- `close()` - 關閉 FIFO
- `unlink()` - 刪除 FIFO 文件

## 📁 範例程式

### fifo_writer.c
**功能**: FIFO 寫入端
- 創建 FIFO
- 交互式發送消息
- 清理 FIFO 文件

### fifo_reader.c
**功能**: FIFO 讀取端
- 檢查並打開 FIFO
- 循環接收消息
- 檢測寫入端關閉

## 🔨 使用方式

### 步驟 1: 編譯程式

```bash
gcc -o fifo_writer fifo_writer.c
gcc -o fifo_reader fifo_reader.c
```

### 步驟 2: 運行演示

**終端 1** - 啟動寫入端:
```bash
./fifo_writer
```

**終端 2** - 啟動讀取端:
```bash
./fifo_reader
```

### 步驟 3: 交互測試

在寫入端終端輸入消息，會在讀取端看到輸出。

## 📊 執行流程

```
終端 1 (Writer)              終端 2 (Reader)
     │                            │
     ├─> mkfifo()                 │
     │   創建 /tmp/my_fifo        │
     │                            │
     ├─> open(O_WRONLY)           │
     │   [阻塞等待讀者...]        │
     │                            ├─> open(O_RDONLY)
     │   [解除阻塞]               │   [雙方都準備好]
     │                            │
     ├─> write("Hello")           │
     │   ───────────────────────> ├─> read()
     │                            │   顯示 "Hello"
     │                            │
     ├─> write("World")           │
     │   ───────────────────────> ├─> read()
     │                            │   顯示 "World"
     │                            │
     ├─> close()                  │
     ├─> unlink()                 ├─> read() 返回 0 (EOF)
     │   刪除 FIFO                ├─> close()
     │                            │
     └─> 退出                     └─> 退出
```

## 💡 重要概念

### 1. FIFO 的阻塞行為

**寫入端 (O_WRONLY)**:
- 默認會阻塞，直到有讀取端打開
- 使用 `O_NONBLOCK` 可避免阻塞

**讀取端 (O_RDONLY)**:
- 默認會阻塞，直到有寫入端打開
- 使用 `O_NONBLOCK` 可避免阻塞

**示例**:
```c
// 非阻塞打開
int fd = open(FIFO_PATH, O_WRONLY | O_NONBLOCK);
if (fd == -1 && errno == ENXIO) {
    printf("沒有讀取端打開\n");
}
```

### 2. EOF 檢測

當所有寫入端關閉時，讀取端的 `read()` 返回 0：

```c
ssize_t n = read(fd, buffer, size);
if (n == 0) {
    printf("所有寫入端已關閉\n");
}
```

### 3. 文件系統中的 FIFO

查看 FIFO 文件：
```bash
ls -l /tmp/my_fifo
# 輸出: prw-r--r-- 1 user user 0 Jan 1 12:00 /tmp/my_fifo
#       ^ 'p' 表示這是管道 (pipe)
```

### 4. 原子寫入

單次寫入不超過 `PIPE_BUF` (通常 4096 字節) 可保證原子性：

```c
#include <limits.h>
printf("PIPE_BUF = %d\n", PIPE_BUF);

// 小於 PIPE_BUF 的寫入是原子的
if (msg_size <= PIPE_BUF) {
    write(fd, msg, msg_size);  // 原子操作
}
```

## 🎯 使用場景

### 1. 客戶端-服務器通訊

```
Client 1 ─┐
Client 2 ─┼─> FIFO ─> Server
Client 3 ─┘
```

### 2. 日誌收集系統

```c
// 多個應用寫入日誌
write(log_fifo, log_msg, len);

// 日誌服務器讀取並保存
while (read(log_fifo, buf, SIZE) > 0) {
    fprintf(log_file, "%s", buf);
}
```

### 3. Shell 腳本間通訊

```bash
#!/bin/bash
mkfifo /tmp/pipe

# 後台讀取
cat /tmp/pipe &

# 寫入數據
echo "Hello from script" > /tmp/pipe

# 清理
rm /tmp/pipe
```

## 🔍 進階技巧

### 1. 非阻塞模式

```c
int fd = open(FIFO_PATH, O_RDONLY | O_NONBLOCK);

while (1) {
    ssize_t n = read(fd, buffer, SIZE);
    if (n == -1 && errno == EAGAIN) {
        // 暫無數據，做其他工作
        usleep(100000);
        continue;
    }
    // 處理數據...
}
```

### 2. 使用 select() 多路複用

```c
fd_set readfds;
struct timeval timeout;

while (1) {
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    int ret = select(fd + 1, &readfds, NULL, NULL, &timeout);
    if (ret > 0 && FD_ISSET(fd, &readfds)) {
        read(fd, buffer, SIZE);
        // 處理數據...
    }
}
```

### 3. 雙向通訊

創建兩個 FIFO 實現雙向通訊：

```c
// 服務器端
mkfifo("/tmp/req", 0666);   // 請求管道
mkfifo("/tmp/resp", 0666);  // 響應管道

int req_fd = open("/tmp/req", O_RDONLY);
int resp_fd = open("/tmp/resp", O_WRONLY);

// 讀取請求，發送響應
read(req_fd, request, SIZE);
write(resp_fd, response, SIZE);
```

## ❓ 常見問題

**Q1: FIFO 和 Socket 如何選擇？**

- **FIFO**: 本地、簡單、輕量
- **Socket**: 網絡、雙向、更靈活

**Q2: 多個寫者會怎樣？**

- 數據可能交錯
- 寫入 < PIPE_BUF 可保證原子性

**Q3: FIFO 文件佔用磁盤空間嗎？**

- 不佔用，數據在內核緩衝區
- 只有一個目錄項

**Q4: 如何避免死鎖？**

- 使用 O_NONBLOCK
- 或確保讀寫端正確配對

## 🔗 Shell 命令

```bash
# 創建 FIFO
mkfifo /tmp/myfifo

# 查看 FIFO
ls -l /tmp/myfifo
file /tmp/myfifo

# 終端 1: 讀取
cat < /tmp/myfifo

# 終端 2: 寫入
echo "Hello" > /tmp/myfifo

# 刪除 FIFO
rm /tmp/myfifo
```

## 📚 延伸學習

- **Message Queue**: 更高級的 IPC，支持消息優先級
- **Shared Memory**: 最快的 IPC 方式 → 參見 `07-shared-memory/`
- **Unix Domain Socket**: 功能更強大的本地通訊
- **D-Bus**: 現代 Linux 的進程間消息系統

## 🔗 相關命令和工具

```bash
# 查看打開的 FIFO
lsof | grep FIFO

# 查看進程打開的文件
lsof -p <pid>

# 監控 FIFO 流量（調試用）
strace -e trace=read,write ./fifo_reader
```

## 📖 推薦閱讀

- `man 3 mkfifo`
- `man 7 fifo`
- Advanced Programming in the UNIX Environment, Chapter 15
- The Linux Programming Interface, Chapter 44
