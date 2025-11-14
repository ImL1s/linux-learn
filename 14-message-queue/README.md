# 消息隊列 (Message Queue)

## 📖 概念介紹

**System V 消息隊列**是一種進程間通訊 (IPC) 機制，允許進程通過發送和接收消息進行異步通訊。與管道不同，消息隊列提供結構化的消息傳遞，支持消息類型、優先級和異步處理。

### 什麼是消息隊列？

消息隊列是由內核維護的消息鏈表，存儲在內核空間中。多個進程可以向同一個隊列發送消息，也可以從隊列中接收消息，無需進程之間有親緣關係。

```
發送進程A ──┐
            ├──→ [消息1][消息2][消息3] ──┐
發送進程B ──┘       內核消息隊列         ├──→ 接收進程C
                                      └──→ 接收進程D
```

### 核心特性

1. **異步通訊**
   - 發送者和接收者不需要同時運行
   - 發送者發送消息後立即返回，無需等待
   - 接收者可以隨時讀取累積的消息

2. **消息類型機制**
   - 每條消息都有類型標識（long型，> 0）
   - 接收者可以選擇性接收特定類型的消息
   - 實現優先級隊列和消息分類

3. **持久性**
   - 消息隊列在進程退出後仍然存在
   - 除非顯式刪除或系統重啟
   - 可能導致資源洩漏（需手動清理）

4. **有序性**
   - 同類型消息保持 FIFO 順序
   - 可按類型、優先級接收
   - 支持非阻塞和阻塞模式

### 與管道的對比

```
管道 (Pipe):
  進程A ──寫→ [字節流] ──讀→ 進程B
  - 字節流，無結構
  - 必須同時運行
  - 單向通訊

消息隊列 (Message Queue):
  進程A ──發→ [消息1][消息2] ──收→ 進程B
           [消息3][消息4]
  - 結構化消息
  - 異步通訊
  - 雙向（需兩個隊列）
```

## 🔧 System V 消息隊列 API

### 創建/獲取消息隊列

```c
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

int msgget(key_t key, int msgflg);
```

**參數**:
- `key`: 隊列的唯一標識（通常使用 `ftok()` 生成）
- `msgflg`: 標誌位
  - `IPC_CREAT`: 不存在則創建
  - `IPC_EXCL`: 與 `IPC_CREAT` 一起使用，已存在則失敗
  - `0666`: 權限設置（類似文件權限）

**返回值**:
- 成功：消息隊列標識符（msgid）
- 失敗：-1

**示例**:
```c
// 方法1：使用固定 key
int msgid = msgget(1234, 0666 | IPC_CREAT);

// 方法2：使用 ftok() 生成 key
key_t key = ftok("/tmp/msgq", 'A');
int msgid = msgget(key, 0666 | IPC_CREAT);

// 方法3：使用私有隊列
int msgid = msgget(IPC_PRIVATE, 0666);
```

### 發送消息

```c
int msgsnd(int msqid, const void *msgp, size_t msgsz, int msgflg);
```

**參數**:
- `msqid`: 消息隊列標識符
- `msgp`: 指向消息結構的指針
- `msgsz`: 消息數據的大小（**不包括** msg_type）
- `msgflg`:
  - `0`: 阻塞（隊列滿時等待）
  - `IPC_NOWAIT`: 非阻塞（隊列滿時立即返回 EAGAIN）

**消息結構**:
```c
struct msgbuf {
    long mtype;       /* 消息類型，必須 > 0 */
    char mtext[512];  /* 消息數據 */
};
```

**返回值**:
- 成功：0
- 失敗：-1

### 接收消息

```c
ssize_t msgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp, int msgflg);
```

**參數**:
- `msqid`: 消息隊列標識符
- `msgp`: 接收消息的緩衝區
- `msgsz`: 消息數據的最大大小
- `msgtyp`: 消息類型選擇
  - `= 0`: 接收隊列中第一條消息（FIFO）
  - `> 0`: 接收指定類型的第一條消息
  - `< 0`: 接收類型 ≤ |msgtyp| 的最小類型消息（優先級）
- `msgflg`:
  - `0`: 阻塞
  - `IPC_NOWAIT`: 非阻塞
  - `MSG_NOERROR`: 消息過大時截斷（而不是返回錯誤）

**返回值**:
- 成功：接收到的字節數
- 失敗：-1

**msgtyp 使用示例**:
```c
// 接收任意消息（FIFO）
msgrcv(msgid, &msg, sizeof(msg.mtext), 0, 0);

// 只接收類型為 5 的消息
msgrcv(msgid, &msg, sizeof(msg.mtext), 5, 0);

// 接收類型 ≤ 3 的消息（類型最小的優先）
msgrcv(msgid, &msg, sizeof(msg.mtext), -3, 0);
```

### 控制/刪除消息隊列

```c
int msgctl(int msqid, int cmd, struct msqid_ds *buf);
```

**常用命令**:
- `IPC_STAT`: 獲取消息隊列狀態
- `IPC_SET`: 設置消息隊列屬性
- `IPC_RMID`: 刪除消息隊列

**示例**:
```c
// 獲取狀態
struct msqid_ds ds;
msgctl(msgid, IPC_STAT, &ds);
printf("隊列中消息數: %ld\n", ds.msg_qnum);

// 刪除隊列
msgctl(msgid, IPC_RMID, NULL);
```

## 📋 消息結構設計

### 基本結構

```c
struct msg_buffer {
    long msg_type;        // 必須是第一個字段
    char msg_text[512];   // 消息內容（可自定義）
};
```

### 複雜結構示例

```c
// 多字段消息
struct complex_msg {
    long mtype;
    int priority;
    pid_t sender_pid;
    time_t timestamp;
    char payload[256];
};

// 變長消息
struct var_msg {
    long mtype;
    int data_len;
    char data[];  // 柔性數組成員 (C99)
};

// 分類消息
#define MSG_TYPE_REQUEST  1
#define MSG_TYPE_RESPONSE 2
#define MSG_TYPE_EVENT    3

struct typed_msg {
    long mtype;  // 使用宏定義的類型
    int command;
    char args[128];
};
```

## 🎯 實戰應用場景

### 場景 1: 簡單的進程間消息傳遞

**用途**: 進程A通知進程B執行任務

```c
// 進程 A (發送者)
struct msg_buffer msg;
msg.msg_type = 1;
strcpy(msg.msg_text, "執行備份任務");
msgsnd(msgid, &msg, strlen(msg.msg_text) + 1, 0);

// 進程 B (接收者)
struct msg_buffer msg;
msgrcv(msgid, &msg, sizeof(msg.msg_text), 1, 0);
printf("收到任務: %s\n", msg.msg_text);
execute_task(msg.msg_text);
```

### 場景 2: 請求-響應模式

```c
// 客戶端
struct request_msg {
    long mtype;
    pid_t client_pid;
    int request_id;
    char query[256];
};

// 1. 發送請求（類型 = 1，表示請求）
req.mtype = 1;
req.client_pid = getpid();
req.request_id = 12345;
strcpy(req.query, "SELECT * FROM users");
msgsnd(msgid, &req, sizeof(req) - sizeof(long), 0);

// 2. 等待響應（類型 = 客戶端PID）
struct response_msg {
    long mtype;
    int request_id;
    char result[512];
};
response_msg resp;
msgrcv(msgid, &resp, sizeof(resp) - sizeof(long), getpid(), 0);
```

```c
// 服務端
// 1. 接收請求（類型 = 1）
request_msg req;
msgrcv(msgid, &req, sizeof(req) - sizeof(long), 1, 0);

// 2. 處理請求
process_query(req.query, result);

// 3. 發送響應（類型 = 客戶端PID）
response_msg resp;
resp.mtype = req.client_pid;
resp.request_id = req.request_id;
strcpy(resp.result, result);
msgsnd(msgid, &resp, sizeof(resp) - sizeof(long), 0);
```

### 場景 3: 優先級隊列

```c
#define PRIORITY_URGENT  1
#define PRIORITY_HIGH    2
#define PRIORITY_NORMAL  3
#define PRIORITY_LOW     4

// 發送不同優先級的消息
msg.msg_type = PRIORITY_URGENT;
strcpy(msg.msg_text, "緊急任務");
msgsnd(msgid, &msg, ...);

msg.msg_type = PRIORITY_LOW;
strcpy(msg.msg_text, "低優先級任務");
msgsnd(msgid, &msg, ...);

// 接收時優先處理緊急消息
// -4 表示接收類型 ≤ 4 的消息，類型最小的優先
msgrcv(msgid, &msg, ..., -4, 0);
// 會先收到 PRIORITY_URGENT (1)，再收到 PRIORITY_HIGH (2)
```

### 場景 4: 多訂閱者模式

```c
#define TOPIC_NEWS    100
#define TOPIC_WEATHER 200
#define TOPIC_SPORTS  300

// 發布者：廣播消息
msg.msg_type = TOPIC_NEWS;
strcpy(msg.msg_text, "突發新聞");
msgsnd(msgid, &msg, ...);

// 訂閱者A：只關注新聞
msgrcv(msgid, &msg, ..., TOPIC_NEWS, 0);

// 訂閱者B：只關注天氣
msgrcv(msgid, &msg, ..., TOPIC_WEATHER, 0);

// 訂閱者C：關注所有（類型=0）
msgrcv(msgid, &msg, ..., 0, 0);
```

## 📊 IPC 機制對比

| 特性 | Pipe | FIFO | 消息隊列 | 共享內存 | Socket |
|------|------|------|----------|----------|--------|
| **親緣關係** | 必須 | 不必 | 不必 | 不必 | 不必 |
| **數據結構** | 字節流 | 字節流 | **結構化消息** | 自定義 | 字節流 |
| **同步性** | 同步 | 同步 | **異步** | 需同步 | 同步/異步 |
| **持久性** | ❌ | ❌ | **✅** | **✅** | ❌ |
| **網絡支持** | ❌ | ❌ | ❌ | ❌ | **✅** |
| **性能** | 中 | 中 | 中 | **最快** | 低 |
| **實現複雜度** | 低 | 低 | 中 | 高 | 中 |
| **適用場景** | 父子進程 | 簡單通訊 | **多進程協作** | 大數據共享 | 網絡/本地 |

### 何時使用消息隊列？

✅ **適合使用**:
1. 需要結構化消息（而不是字節流）
2. 發送者和接收者不同時運行
3. 需要消息優先級/分類
4. 一對多或多對一通訊
5. 消息量不是特別大（< MB/s）

❌ **不適合使用**:
1. 需要極高性能（使用共享內存）
2. 消息非常大（> 8KB，考慮管道或共享內存）
3. 需要網絡通訊（使用 Socket）
4. 需要點對點全雙工通訊（使用 Socket pair）
5. 簡單的父子進程通訊（使用 Pipe）

## 📁 範例程式

### 1. msg_sender.c - 消息發送端

**功能**: 交互式發送消息

**核心代碼**:
```c
struct msg_buffer {
    long msg_type;
    char msg_text[512];
};

int main() {
    int msgid = msgget(MSG_KEY, 0666 | IPC_CREAT);

    struct msg_buffer msg;
    while (1) {
        fgets(msg.msg_text, 512, stdin);
        if (strcmp(msg.msg_text, "quit") == 0) break;

        msg.msg_type = 1;
        msgsnd(msgid, &msg, strlen(msg.msg_text) + 1, 0);
    }
}
```

### 2. msg_receiver.c - 消息接收端

**功能**: 持續接收並顯示消息

**核心代碼**:
```c
int main() {
    int msgid = msgget(MSG_KEY, 0666 | IPC_CREAT);

    struct msg_buffer msg;
    while (1) {
        msgrcv(msgid, &msg, sizeof(msg.msg_text), 1, 0);
        printf("收到消息: %s\n", msg.msg_text);
    }
}
```

### 編譯與運行

```bash
# 編譯
make message-queue

# 終端 1: 啟動接收端
cd 14-message-queue
./msg_receiver

# 終端 2: 發送消息
./msg_sender
> Hello World
✓ 已發送
> 測試消息
✓ 已發送
> quit

# 終端 1 輸出:
# 收到消息: Hello World
# 收到消息: 測試消息
```

## 💡 高級主題

### 1. 消息隊列限制

Linux 系統對消息隊列有多項限制：

```c
// 查看系統限制
#include <sys/msg.h>

struct msqid_ds ds;
msgctl(msgid, IPC_STAT, &ds);

// /proc/sys/kernel/msgmax - 單條消息最大大小 (默認 8192 bytes)
// /proc/sys/kernel/msgmnb - 隊列最大總字節數 (默認 16384 bytes)
// /proc/sys/kernel/msgmni - 系統最大消息隊列數 (默認 32000)
```

**查看和修改限制**:
```bash
# 查看當前限制
cat /proc/sys/kernel/msgmax
cat /proc/sys/kernel/msgmnb
cat /proc/sys/kernel/msgmni

# 臨時修改（重啟失效）
echo 65536 > /proc/sys/kernel/msgmax

# 永久修改
sudo vi /etc/sysctl.conf
# 添加:
kernel.msgmax = 65536
kernel.msgmnb = 65536
sudo sysctl -p
```

### 2. 非阻塞模式

```c
// 非阻塞發送
if (msgsnd(msgid, &msg, msgsz, IPC_NOWAIT) == -1) {
    if (errno == EAGAIN) {
        printf("隊列已滿，稍後重試\n");
    }
}

// 非阻塞接收
if (msgrcv(msgid, &msg, msgsz, 0, IPC_NOWAIT) == -1) {
    if (errno == ENOMSG) {
        printf("隊列為空\n");
    }
}
```

### 3. 帶超時的接收

POSIX 消息隊列提供了超時機制，但 System V 沒有。可以使用信號實現：

```c
#include <signal.h>
#include <setjmp.h>

static jmp_buf jmpbuf;

void alarm_handler(int sig) {
    longjmp(jmpbuf, 1);
}

// 3秒超時接收
signal(SIGALRM, alarm_handler);
if (setjmp(jmpbuf) == 0) {
    alarm(3);
    msgrcv(msgid, &msg, msgsz, 0, 0);
    alarm(0);  // 取消鬧鐘
    printf("收到消息\n");
} else {
    printf("接收超時\n");
}
```

### 4. 查看和管理消息隊列

```bash
# 查看所有消息隊列
ipcs -q
# 輸出:
# ------ Message Queues --------
# key        msqid      owner      perms      used-bytes   messages
# 0x000004d2 0          root       666        0            0

# 查看詳細信息
ipcs -q -i <msqid>

# 刪除特定隊列
ipcrm -q <msqid>

# 按 key 刪除
ipcrm -Q <key>

# 刪除所有消息隊列（危險！）
ipcs -q | awk 'NR>3 {print $2}' | xargs -I {} ipcrm -q {}
```

### 5. 自動清理機制

消息隊列不會自動刪除，需要顯式清理：

```c
#include <signal.h>
#include <stdlib.h>

int msgid;

void cleanup(int sig) {
    msgctl(msgid, IPC_RMID, NULL);
    printf("消息隊列已清理\n");
    exit(0);
}

int main() {
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);
    atexit(cleanup_at_exit);

    msgid = msgget(1234, 0666 | IPC_CREAT);
    // ... 使用消息隊列
}

void cleanup_at_exit() {
    msgctl(msgid, IPC_RMID, NULL);
}
```

## 🐛 常見問題與調試

### 問題 1: 消息隊列已滿

**症狀**: `msgsnd()` 返回 EAGAIN（非阻塞）或長時間阻塞

**原因**:
- 隊列中消息總大小超過 `msgmnb` 限制
- 接收端處理速度太慢

**解決方案**:
```c
// 1. 增加隊列大小限制
sudo sysctl -w kernel.msgmnb=65536

// 2. 使用非阻塞發送+重試
while (msgsnd(msgid, &msg, msgsz, IPC_NOWAIT) == -1) {
    if (errno == EAGAIN) {
        usleep(10000);  // 等待 10ms
        continue;
    }
    perror("msgsnd");
    break;
}

// 3. 檢查隊列狀態
struct msqid_ds ds;
msgctl(msgid, IPC_STAT, &ds);
if (ds.msg_qbytes - ds.msg_cbytes < msgsz) {
    printf("隊列空間不足\n");
}
```

### 問題 2: 消息隊列洩漏

**症狀**: `ipcs -q` 顯示大量未刪除的隊列

**預防**:
1. 始終在程序退出時刪除隊列
2. 使用信號處理器清理
3. 定期檢查和清理

```c
// 創建時使用 IPC_EXCL 避免衝突
int msgid = msgget(key, 0666 | IPC_CREAT | IPC_EXCL);
if (msgid == -1 && errno == EEXIST) {
    // 隊列已存在，刪除舊的
    msgid = msgget(key, 0666);
    msgctl(msgid, IPC_RMID, NULL);
    msgid = msgget(key, 0666 | IPC_CREAT);
}
```

### 問題 3: 消息過大

**症狀**: `msgsnd()` 返回 EINVAL

**原因**: 消息大小超過 `msgmax`（默認 8KB）

**解決方案**:
```c
// 1. 檢查消息大小
if (data_size > 8000) {  // 保守估計
    fprintf(stderr, "消息太大: %zu bytes\n", data_size);
}

// 2. 分片發送
#define CHUNK_SIZE 4096
for (size_t offset = 0; offset < total_size; offset += CHUNK_SIZE) {
    size_t chunk_size = MIN(CHUNK_SIZE, total_size - offset);
    memcpy(msg.data, buffer + offset, chunk_size);
    msg.mtype = chunk_id++;
    msgsnd(msgid, &msg, chunk_size, 0);
}

// 3. 使用共享內存傳遞大數據
struct msg_with_shm {
    long mtype;
    int shmid;
    size_t data_size;
};
```

### 問題 4: 死鎖

**場景**: 兩個進程相互等待對方的消息

```c
// 進程 A
msgsnd(msgid, &msg, ..., 0);    // 發送給 B
msgrcv(msgid, &resp, ..., 0);   // 等待 B 的響應

// 進程 B
msgsnd(msgid, &msg, ..., 0);    // 發送給 A（但隊列滿！）
msgrcv(msgid, &resp, ..., 0);   // 等待 A（但 A 也在等待）
```

**解決方案**:
```c
// 使用非阻塞模式
msgsnd(msgid, &msg, msgsz, IPC_NOWAIT);

// 或使用超時機制
// 或使用兩個隊列（請求隊列 + 響應隊列）
```

### 調試技巧

```c
// 1. 打印隊列狀態
void print_msgq_stats(int msgid) {
    struct msqid_ds ds;
    msgctl(msgid, IPC_STAT, &ds);

    printf("消息隊列狀態:\n");
    printf("  當前消息數: %lu\n", ds.msg_qnum);
    printf("  總字節數: %lu\n", ds.msg_cbytes);
    printf("  最大字節數: %lu\n", ds.msg_qbytes);
    printf("  最後發送時間: %ld\n", ds.msg_stime);
    printf("  最後接收時間: %ld\n", ds.msg_rtime);
}

// 2. 啟用調試輸出
#define DEBUG 1
#if DEBUG
#define debug_print(fmt, ...) \
    fprintf(stderr, "[%s:%d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define debug_print(fmt, ...)
#endif

// 3. 使用 strace 追蹤系統調用
// strace -e trace=msgget,msgsnd,msgrcv,msgctl ./msg_sender
```

## 🎓 POSIX 消息隊列 vs System V 消息隊列

| 特性 | System V | POSIX |
|------|----------|-------|
| **API** | msgget/msgsnd/msgrcv | mq_open/mq_send/mq_receive |
| **命名** | 整數 key | 文件系統路徑 |
| **消息優先級** | 類型字段 | 專門的優先級參數 |
| **超時** | 不支持 | 支持 |
| **異步通知** | 不支持 | 支持（信號/線程） |
| **可移植性** | 較老，但廣泛支持 | 較新，POSIX 標準 |
| **清理** | 手動 ipcrm | 自動（mq_unlink） |

**POSIX 消息隊列示例**:
```c
#include <mqueue.h>

// 打開/創建
mqd_t mq = mq_open("/myqueue", O_CREAT | O_RDWR, 0644, NULL);

// 發送（帶優先級）
mq_send(mq, msg, strlen(msg), priority);

// 接收（帶超時）
struct timespec timeout = {.tv_sec = 5};
mq_timedreceive(mq, buf, MAX_SIZE, &priority, &timeout);

// 關閉和刪除
mq_close(mq);
mq_unlink("/myqueue");
```

## 📝 最佳實踐

### 1. 設計原則

```c
// ✅ 好的設計
struct well_designed_msg {
    long mtype;           // 消息類型
    int version;          // 協議版本
    pid_t sender_pid;     // 發送者PID
    time_t timestamp;     // 時間戳
    uint32_t checksum;    // 校驗和
    char payload[256];    // 實際數據
};

// ❌ 差的設計
struct poor_msg {
    long mtype;
    char data[4096];  // 太大，可能超過限制
    // 沒有版本控制
    // 沒有錯誤檢測
};
```

### 2. 錯誤處理

```c
// ✅ 完善的錯誤處理
if (msgsnd(msgid, &msg, msgsz, 0) == -1) {
    switch (errno) {
        case EAGAIN:
            fprintf(stderr, "隊列已滿\n");
            break;
        case EIDRM:
            fprintf(stderr, "隊列已被刪除\n");
            break;
        case EINTR:
            fprintf(stderr, "被信號中斷\n");
            continue;  // 重試
        case EINVAL:
            fprintf(stderr, "無效參數或消息過大\n");
            break;
        default:
            perror("msgsnd");
    }
}
```

### 3. 資源清理

```c
// ✅ 確保清理
int main() {
    int msgid = msgget(key, 0666 | IPC_CREAT);

    // 註冊清理函數
    atexit(cleanup);
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // ... 使用消息隊列 ...

    // 正常退出時清理
    cleanup();
    return 0;
}

void cleanup() {
    if (msgid != -1) {
        msgctl(msgid, IPC_RMID, NULL);
        msgid = -1;
    }
}
```

### 4. 性能優化

```c
// 批量處理
#define BATCH_SIZE 10
struct msg_buffer msgs[BATCH_SIZE];

// 發送端：批量發送
for (int i = 0; i < BATCH_SIZE; i++) {
    prepare_message(&msgs[i]);
    msgsnd(msgid, &msgs[i], ..., IPC_NOWAIT);
}

// 接收端：批量接收
while (msgrcv(msgid, &msg, ..., 0, IPC_NOWAIT) != -1) {
    process_message(&msg);
}
```

## 📚 參考資料

### 官方文檔
- `man msgget` - 創建/獲取消息隊列
- `man msgsnd` - 發送消息
- `man msgrcv` - 接收消息
- `man msgctl` - 控制消息隊列
- `man 7 svipc` - System V IPC 概述

### 推薦書籍
- *UNIX Network Programming, Volume 2: Interprocess Communications* - W. Richard Stevens
- *The Linux Programming Interface* - Michael Kerrisk, Chapter 46-47
- *Advanced Programming in the UNIX Environment* - W. Richard Stevens, Chapter 15

### 在線資源
- [Linux IPC](https://www.kernel.org/doc/html/latest/ipc/index.html)
- [POSIX Message Queues](https://man7.org/linux/man-pages/man7/mq_overview.7.html)

---

## 📝 總結

消息隊列是一種強大的 IPC 機制，特別適合需要結構化消息傳遞的場景。

### 何時使用消息隊列

✅ **使用場景**:
- 需要異步通訊
- 需要消息類型/優先級
- 多個生產者/消費者
- 進程生命週期不同步

❌ **不使用場景**:
- 需要極高性能（→ 共享內存）
- 消息非常大（→ 共享內存 + 通知）
- 需要網絡通訊（→ Socket）
- 簡單父子進程（→ Pipe）

### 關鍵要點

1. **消息結構**: 第一個字段必須是 `long mtype`
2. **大小參數**: `msgsnd`/`msgrcv` 的 size 不包括 mtype
3. **手動清理**: 使用 `msgctl(IPC_RMID)` 或 `ipcrm`
4. **系統限制**: 注意 msgmax/msgmnb/msgmni 限制
5. **錯誤處理**: 處理 EAGAIN, EIDRM, EINTR 等錯誤

掌握消息隊列，讓你的多進程程序能夠優雅地異步協作！
