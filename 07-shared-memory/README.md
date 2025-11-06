# 共享內存 (Shared Memory)

## 📖 概念介紹

**共享內存 (Shared Memory)** 是最快的進程間通訊 (IPC) 方式。它允許多個進程訪問同一塊物理內存區域，避免了數據複製，效率極高。

### 共享內存的特點

- ✅ **最快的 IPC**: 沒有數據複製
- ✅ **大容量**: 可以共享大量數據
- ✅ **靈活**: 可以存儲任意數據結構
- ⚠️ **需要同步**: 必須配合信號量或互斥鎖
- ⚠️ **生命週期**: 需要顯式刪除

## 🔧 System V 共享內存 API

### shmget() - 創建/獲取共享內存

```c
#include <sys/ipc.h>
#include <sys/shm.h>

int shmget(key_t key, size_t size, int shmflg);
```

- **key**: IPC 鍵值（唯一標識）
- **size**: 共享內存大小（字節）
- **shmflg**: 標誌位
  - `IPC_CREAT`: 不存在則創建
  - `IPC_EXCL`: 已存在則報錯
  - `0666`: 權限
- **返回值**: 共享內存 ID，失敗返回 -1

### shmat() - 映射共享內存

```c
void *shmat(int shmid, const void *shmaddr, int shmflg);
```

- **shmid**: 共享內存 ID
- **shmaddr**: 映射地址（NULL 由系統選擇）
- **shmflg**: 標誌位
  - `0`: 可讀可寫
  - `SHM_RDONLY`: 只讀
- **返回值**: 映射地址，失敗返回 `(void*)-1`

### shmdt() - 分離共享內存

```c
int shmdt(const void *shmaddr);
```

- 取消映射，不會刪除共享內存
- **返回值**: 成功返回 0，失敗返回 -1

### shmctl() - 控制共享內存

```c
int shmctl(int shmid, int cmd, struct shmid_ds *buf);
```

- **cmd** 選項:
  - `IPC_STAT`: 獲取狀態信息
  - `IPC_SET`: 設置屬性
  - `IPC_RMID`: 刪除共享內存
  - `IPC_INFO`: 獲取系統限制信息

## 🔑 IPC Key 的生成

### 方法 1: 手動指定

```c
#define SHM_KEY 0x1234
int shmid = shmget(SHM_KEY, size, IPC_CREAT | 0666);
```

### 方法 2: ftok() 生成

```c
key_t key = ftok("/tmp/myfile", 'A');
if (key == -1) {
    perror("ftok");
}
int shmid = shmget(key, size, IPC_CREAT | 0666);
```

- 基於文件路徑和項目 ID 生成唯一 key
- 文件必須存在
- 同一路徑和 ID 總是生成相同的 key

### 方法 3: IPC_PRIVATE

```c
int shmid = shmget(IPC_PRIVATE, size, IPC_CREAT | 0666);
```

- 創建私有共享內存
- 只能由親緣進程訪問（通過 fork 繼承 shmid）

## 📁 範例程式

### shm_writer.c
**功能**: 共享內存寫入端
- 創建共享內存
- 映射到進程地址空間
- 交互式寫入數據
- 清理和刪除

### shm_reader.c
**功能**: 共享內存讀取端
- 連接已存在的共享內存
- 映射並讀取數據
- 簡單的同步機制

**編譯與運行**:
```bash
# 編譯
gcc -o shm_writer shm_writer.c
gcc -o shm_reader shm_reader.c

# 運行（兩個終端）
# 終端 1
./shm_writer

# 終端 2
./shm_reader
```

## 💡 重要概念

### 1. 生命週期

共享內存不會隨進程結束而消失：

```
創建 (shmget)
    ↓
映射 (shmat) ← 可多次映射
    ↓
使用
    ↓
分離 (shmdt) ← 只是取消映射
    ↓
刪除 (shmctl IPC_RMID) ← 才真正刪除
```

**重要**: 必須顯式刪除，否則會一直佔用系統資源！

### 2. 共享內存的狀態

```c
struct shmid_ds {
    struct ipc_perm shm_perm;    // 權限
    size_t          shm_segsz;   // 大小
    time_t          shm_atime;   // 最後 attach 時間
    time_t          shm_dtime;   // 最後 detach 時間
    time_t          shm_ctime;   // 最後修改時間
    pid_t           shm_cpid;    // 創建者 PID
    pid_t           shm_lpid;    // 最後操作者 PID
    shmatt_t        shm_nattch;  // 當前附加數
};
```

查看狀態：
```c
struct shmid_ds buf;
shmctl(shmid, IPC_STAT, &buf);
printf("Size: %zu\n", buf.shm_segsz);
printf("Attached: %d\n", buf.shm_nattch);
```

### 3. 同步問題

**問題**: 共享內存本身不提供同步機制

**解決方案**:

#### 方案 1: 信號量

```c
#include <sys/sem.h>

// 創建信號量
int semid = semget(key, 1, IPC_CREAT | 0666);
semctl(semid, 0, SETVAL, 1);

// P 操作（加鎖）
struct sembuf sb = {0, -1, 0};
semop(semid, &sb, 1);

// 訪問共享內存...

// V 操作（解鎖）
sb.sem_op = 1;
semop(semid, &sb, 1);
```

#### 方案 2: 文件鎖

```c
int lock_fd = open("/tmp/shm.lock", O_CREAT | O_RDWR, 0666);

struct flock fl = {
    .l_type = F_WRLCK,
    .l_whence = SEEK_SET,
    .l_start = 0,
    .l_len = 0
};

fcntl(lock_fd, F_SETLKW, &fl);  // 加鎖
// 訪問共享內存...
fl.l_type = F_UNLCK;
fcntl(lock_fd, F_SETLK, &fl);   // 解鎖
```

#### 方案 3: 簡單標誌（本範例使用）

```c
struct shared_data {
    int flag;      // 0: 寫入中, 1: 可讀
    int counter;
    char data[256];
};

// 寫入端
shared->flag = 0;
shared->counter++;
strcpy(shared->data, "message");
shared->flag = 1;

// 讀取端
while (shared->flag != 1) {
    usleep(1000);
}
printf("Data: %s\n", shared->data);
shared->flag = 0;
```

**注意**: 簡單標誌不適合高並發場景！

### 4. 權限

```c
// 0666: 所有用戶可讀可寫
shmget(key, size, IPC_CREAT | 0666);

// 0644: 所有者可讀寫，其他只讀
shmget(key, size, IPC_CREAT | 0644);

// 0600: 僅所有者可讀寫
shmget(key, size, IPC_CREAT | 0600);
```

查看權限：
```bash
ipcs -m
```

## 🎯 使用場景

### 1. 高性能數據共享

```c
// 大數據傳輸
struct large_data {
    int count;
    double values[1000000];
};

int shmid = shmget(key, sizeof(struct large_data),
                   IPC_CREAT | 0666);
struct large_data *data = shmat(shmid, NULL, 0);
```

### 2. 多進程協作

```c
// 任務隊列
struct task_queue {
    int head, tail;
    task_t tasks[QUEUE_SIZE];
};
```

### 3. 共享配置

```c
// 配置信息
struct config {
    int max_connections;
    char server_addr[64];
    int port;
};
```

## 🔍 系統管理

### 查看共享內存

```bash
# 列出所有共享內存
ipcs -m

# 詳細信息
ipcs -m -i <shmid>

# 查看限制
ipcs -m -l
```

輸出示例：
```
------ Shared Memory Segments --------
key        shmid      owner      perms      bytes      nattch     status
0x00001234 0          user       666        1024       2
```

### 刪除共享內存

```bash
# 刪除指定共享內存
ipcrm -m <shmid>

# 刪除所有（小心！）
ipcrm -a
```

### 系統限制

查看系統限制：
```bash
# 最大共享內存段大小
cat /proc/sys/kernel/shmmax

# 所有共享內存總大小
cat /proc/sys/kernel/shmall

# 最大共享內存段數量
cat /proc/sys/kernel/shmmni
```

設置限制（需要 root）：
```bash
sysctl -w kernel.shmmax=68719476736    # 64GB
sysctl -w kernel.shmall=4294967296     # 16GB (頁數)
```

## ❓ 常見問題

**Q1: 共享內存 vs mmap？**

| 特性 | System V shm | POSIX shm (shm_open) | mmap |
|------|-------------|---------------------|------|
| 標準 | System V IPC | POSIX | POSIX |
| 接口 | shmget/shmat | shm_open/mmap | mmap |
| 名稱 | key (整數) | 路徑名 | 文件路徑 |
| 持久性 | 需手動刪除 | 需手動 shm_unlink | 文件系統 |

**Q2: 共享內存會被交換到磁盤嗎？**

會的，除非使用 `shmctl(SHM_LOCK)` 鎖定在內存中（需要權限）。

**Q3: 如何避免內存洩漏？**

```c
// 設置自動刪除標記
shmctl(shmid, IPC_RMID, NULL);
// 共享內存會在最後一個進程 detach 後自動刪除
```

**Q4: 多個進程同時寫入會怎樣？**

會出現競爭條件！必須使用同步機制（信號量、互斥鎖等）。

## 🚀 POSIX 共享內存

更現代的替代方案：

```c
#include <sys/mman.h>
#include <fcntl.h>

// 創建
int fd = shm_open("/myshm", O_CREAT | O_RDWR, 0666);
ftruncate(fd, size);

// 映射
void *addr = mmap(NULL, size, PROT_READ | PROT_WRITE,
                  MAP_SHARED, fd, 0);

// 使用...

// 清理
munmap(addr, size);
close(fd);
shm_unlink("/myshm");
```

**優點**:
- 使用文件描述符（更統一）
- 命名更直觀（路徑名）
- 更符合 POSIX 標準

## 📚 延伸學習

- **POSIX 共享內存**: `shm_open()`, `mmap()`
- **內存映射文件**: `mmap()` 映射普通文件
- **大頁內存**: 提高 TLB 命中率
- **NUMA**: 多處理器系統的內存親和性

## 📖 推薦閱讀

- `man shmget`
- `man shmat`
- `man shmctl`
- `man shm_overview`
- Unix Network Programming Volume 2 (IPC)
- The Linux Programming Interface, Chapter 48
