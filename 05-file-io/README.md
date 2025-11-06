# 文件 I/O (File Input/Output)

## 📖 概念介紹

Linux 文件 I/O 是系統程式設計的基礎。在 Linux 中，"一切皆文件"，包括普通文件、設備、管道、socket 等，都可以通過文件描述符進行統一的讀寫操作。

### 文件描述符 (File Descriptor)

- 非負整數，用於標識打開的文件
- 每個進程都有自己的文件描述符表
- 標準文件描述符：
  - `0` - 標準輸入 (stdin)
  - `1` - 標準輸出 (stdout)
  - `2` - 標準錯誤 (stderr)

## 🔧 系統調用

### open() - 打開文件

```c
#include <fcntl.h>
int open(const char *pathname, int flags, mode_t mode);
```

**flags 選項**:
- `O_RDONLY` - 只讀
- `O_WRONLY` - 只寫
- `O_RDWR` - 讀寫
- `O_CREAT` - 不存在則創建
- `O_TRUNC` - 清空文件
- `O_APPEND` - 追加模式
- `O_NONBLOCK` - 非阻塞模式

**mode 權限** (八進制):
- `0644` = `rw-r--r--`
- `0755` = `rwxr-xr-x`
- `0600` = `rw-------`

### read() - 讀取數據

```c
#include <unistd.h>
ssize_t read(int fd, void *buf, size_t count);
```

- **返回值**: 讀取的字節數，0 表示 EOF，-1 表示錯誤

### write() - 寫入數據

```c
ssize_t write(int fd, const void *buf, size_t count);
```

- **返回值**: 寫入的字節數，-1 表示錯誤

### lseek() - 文件定位

```c
off_t lseek(int fd, off_t offset, int whence);
```

**whence 選項**:
- `SEEK_SET` - 從文件開頭
- `SEEK_CUR` - 從當前位置
- `SEEK_END` - 從文件末尾

### close() - 關閉文件

```c
int close(int fd);
```

### fcntl() - 文件控制

```c
int fcntl(int fd, int cmd, ...);
```

**常用命令**:
- `F_GETFL` - 獲取文件狀態標誌
- `F_SETFL` - 設置文件狀態標誌
- `F_SETLK` - 設置文件鎖（非阻塞）
- `F_SETLKW` - 設置文件鎖（阻塞）
- `F_GETLK` - 測試文件鎖

## 📁 範例程式

### file_operations.c
**功能**: 文件 I/O 綜合演示
- 基本讀寫操作
- lseek 文件定位
- 文件鎖 (fcntl)

**編譯與運行**:
```bash
gcc -o file_operations file_operations.c
./file_operations
```

## 💡 重要概念

### 1. 文件描述符與文件指針

```c
// 低級 I/O (無緩衝)
int fd = open("file.txt", O_RDONLY);
read(fd, buf, size);
close(fd);

// 高級 I/O (有緩衝)
FILE *fp = fopen("file.txt", "r");
fread(buf, 1, size, fp);
fclose(fp);
```

**區別**:
| 特性 | 低級 I/O | 高級 I/O |
|------|---------|---------|
| 標識 | 文件描述符 (int) | 文件指針 (FILE*) |
| 緩衝 | 無 | 有 |
| 效率 | 較低 | 較高 |
| 控制 | 精細 | 簡單 |
| 跨平台 | Unix/Linux | ANSI C |

### 2. lseek 的妙用

#### 獲取文件大小
```c
off_t size = lseek(fd, 0, SEEK_END);
```

#### 創建空洞文件 (Sparse File)
```c
int fd = open("sparse.txt", O_WRONLY | O_CREAT, 0644);
lseek(fd, 1024 * 1024, SEEK_SET);  // 跳過 1MB
write(fd, "X", 1);
close(fd);
// 文件大小 1MB，但只佔用 1 個數據塊
```

#### 當前位置
```c
off_t pos = lseek(fd, 0, SEEK_CUR);
```

### 3. 文件鎖 (File Locking)

#### 記錄鎖結構
```c
struct flock {
    short l_type;    // F_RDLCK, F_WRLCK, F_UNLCK
    short l_whence;  // SEEK_SET, SEEK_CUR, SEEK_END
    off_t l_start;   // 鎖定起始位置
    off_t l_len;     // 鎖定長度 (0 = 到 EOF)
    pid_t l_pid;     // 持有鎖的進程 ID
};
```

#### 鎖的類型
- **共享鎖 (F_RDLCK)**: 讀鎖，多個進程可同時持有
- **排他鎖 (F_WRLCK)**: 寫鎖，獨佔訪問
- **解鎖 (F_UNLCK)**: 釋放鎖

#### 鎖定示例
```c
struct flock lock;
lock.l_type = F_WRLCK;     // 寫鎖
lock.l_whence = SEEK_SET;
lock.l_start = 0;
lock.l_len = 0;            // 鎖定整個文件

// 非阻塞獲取鎖
if (fcntl(fd, F_SETLK, &lock) == -1) {
    if (errno == EACCES || errno == EAGAIN) {
        printf("文件已被鎖定\n");
    }
}

// 阻塞獲取鎖
fcntl(fd, F_SETLKW, &lock);  // 等待直到獲得鎖
```

### 4. 原子操作

某些操作需要保證原子性：

#### O_APPEND 標誌
```c
// 多個進程同時追加，不會出現數據交錯
int fd = open("log.txt", O_WRONLY | O_APPEND);
write(fd, data, len);
```

#### O_CREAT | O_EXCL
```c
// 原子創建，如果存在則失敗
int fd = open("lockfile", O_CREAT | O_EXCL, 0644);
if (fd == -1) {
    // 文件已存在
}
```

### 5. 錯誤處理

```c
#include <errno.h>

int fd = open("file.txt", O_RDONLY);
if (fd == -1) {
    switch (errno) {
        case ENOENT:
            printf("文件不存在\n");
            break;
        case EACCES:
            printf("權限不足\n");
            break;
        case EMFILE:
            printf("打開文件過多\n");
            break;
        default:
            perror("open");
    }
}
```

## 🎯 實用技巧

### 1. 複製文件

```c
int copy_file(const char *src, const char *dst) {
    int src_fd = open(src, O_RDONLY);
    int dst_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    char buf[4096];
    ssize_t n;
    while ((n = read(src_fd, buf, sizeof(buf))) > 0) {
        write(dst_fd, buf, n);
    }

    close(src_fd);
    close(dst_fd);
    return 0;
}
```

### 2. 追加日誌

```c
void log_message(const char *msg) {
    int fd = open("app.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
    write(fd, msg, strlen(msg));
    write(fd, "\n", 1);
    close(fd);
}
```

### 3. 文件存在性檢查

```c
#include <unistd.h>

if (access("file.txt", F_OK) == 0) {
    printf("文件存在\n");
}
if (access("file.txt", R_OK) == 0) {
    printf("可讀\n");
}
```

## ❓ 常見問題

**Q1: open() 和 fopen() 如何選擇？**

- 需要精細控制 → `open()`
- 文本處理、格式化 → `fopen()`
- 與其他系統調用配合 → `open()`
- 跨平台應用 → `fopen()`

**Q2: 為什麼 write() 返回值可能小於請求的字節數？**

可能原因：
- 磁盤空間不足
- 寫入到管道或 socket
- 被信號中斷

正確處理：
```c
ssize_t write_all(int fd, const void *buf, size_t count) {
    size_t total = 0;
    while (total < count) {
        ssize_t n = write(fd, buf + total, count - total);
        if (n == -1) {
            if (errno == EINTR) continue;
            return -1;
        }
        total += n;
    }
    return total;
}
```

**Q3: 文件鎖的類型？**

- **建議性鎖 (Advisory Lock)**: `fcntl()` 實現，進程需要主動檢查
- **強制性鎖 (Mandatory Lock)**: 內核強制，較少使用

Linux 默認使用建議性鎖。

**Q4: 如何實現文件映射？**

使用 `mmap()`:
```c
void *addr = mmap(NULL, size, PROT_READ | PROT_WRITE,
                  MAP_SHARED, fd, 0);
// 直接操作內存即可修改文件
munmap(addr, size);
```

## 🔍 性能優化

### 1. 使用更大的緩衝區

```c
#define BUF_SIZE 65536  // 64KB
char buf[BUF_SIZE];
```

### 2. 批量操作

避免頻繁的小量讀寫。

### 3. 使用 mmap

對於大文件，`mmap()` 通常比 `read()/write()` 更快。

### 4. O_DIRECT 標誌

跳過內核緩衝區，直接 I/O（需要對齊）。

## 🔗 相關命令

```bash
# 查看打開的文件
lsof -p <pid>

# 查看文件描述符
ls -l /proc/<pid>/fd/

# 查看文件鎖
lslocks

# 測試文件
dd if=/dev/zero of=testfile bs=1M count=100
```

## 📚 延伸學習

- **異步 I/O**: `aio_read()`, `aio_write()`
- **內存映射**: `mmap()`, `munmap()`
- **向量 I/O**: `readv()`, `writev()`
- **零拷貝**: `sendfile()`, `splice()`

## 📖 推薦閱讀

- `man 2 open`
- `man 2 read`
- `man 2 write`
- `man 2 lseek`
- `man 2 fcntl`
- Advanced Programming in the UNIX Environment, Chapter 3
