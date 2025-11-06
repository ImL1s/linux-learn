# 信號量 (Semaphore)

## 📖 簡介

信號量（Semaphore）是用於進程/線程同步的經典機制，由荷蘭計算機科學家 Edsger Dijkstra 在 1965 年提出。信號量是一個計數器，用於控制對共享資源的訪問，可以解決生產者-消費者、讀者-寫者等經典同步問題。

**核心概念：**
- **P 操作** (wait/down)：申請資源，計數器減 1，若結果 < 0 則阻塞
- **V 操作** (signal/up)：釋放資源，計數器加 1，若有等待者則喚醒

**命名來源：**
- P 來自荷蘭語 "Proberen"（測試）
- V 來自荷蘭語 "Verhogen"（增加）

## 📁 範例文件

- `semaphore_demo.c` - 生產者-消費者問題完整實現（454 行，含詳細註解）

## 🎯 信號量的類型

### 1. 二元信號量 (Binary Semaphore)

**特點：**
- 值只能是 0 或 1
- 類似於互斥鎖（Mutex）
- 用於互斥訪問臨界區

**示例：**
```c
sem_t mutex;
sem_init(&mutex, 0, 1);  // 初始值為 1

// 線程 A
sem_wait(&mutex);   // P 操作：獲取鎖
// 臨界區
sem_post(&mutex);   // V 操作：釋放鎖

// 線程 B
sem_wait(&mutex);   // 會阻塞，直到線程 A 釋放
// 臨界區
sem_post(&mutex);
```

### 2. 計數信號量 (Counting Semaphore)

**特點：**
- 值可以是任意非負整數
- 用於資源計數
- 用於同步控制

**示例：**
```c
#define POOL_SIZE 5
sem_t pool;
sem_init(&pool, 0, POOL_SIZE);  // 初始值為 5

// 申請資源
sem_wait(&pool);   // 可用資源減 1
use_resource();

// 釋放資源
sem_post(&pool);   // 可用資源加 1
```

## 🔧 POSIX 信號量 API

### 基本操作

```c
#include <semaphore.h>

// 1. 初始化信號量
int sem_init(sem_t *sem, int pshared, unsigned int value);
/*
 * sem: 信號量指針
 * pshared:
 *   0 - 線程間共享（本地信號量）
 *   非0 - 進程間共享（需要共享內存）
 * value: 初始值
 * 返回: 成功返回 0，失敗返回 -1
 */

// 2. P 操作 - 阻塞等待
int sem_wait(sem_t *sem);
/*
 * 將信號量減 1
 * 如果結果 < 0，線程阻塞
 * 返回: 成功返回 0，失敗返回 -1
 */

// 3. P 操作 - 非阻塞嘗試
int sem_trywait(sem_t *sem);
/*
 * 嘗試將信號量減 1
 * 如果不能立即完成，返回 EAGAIN
 * 不阻塞
 */

// 4. P 操作 - 超時等待
int sem_timedwait(sem_t *sem, const struct timespec *abs_timeout);
/*
 * 在指定時間內等待
 * 超時返回 ETIMEDOUT
 */

// 5. V 操作 - 釋放資源
int sem_post(sem_t *sem);
/*
 * 將信號量加 1
 * 如果有等待的線程，喚醒一個
 */

// 6. 獲取當前值
int sem_getvalue(sem_t *sem, int *sval);
/*
 * 獲取信號量的當前值
 * 注意：值可能立即改變（多線程環境）
 */

// 7. 銷毀信號量
int sem_destroy(sem_t *sem);
/*
 * 釋放信號量資源
 * 必須在所有使用者完成後調用
 */
```

### 命名信號量（進程間）

```c
// 創建/打開命名信號量
sem_t *sem_open(const char *name, int oflag, mode_t mode, unsigned int value);

// 關閉信號量
int sem_close(sem_t *sem);

// 刪除命名信號量
int sem_unlink(const char *name);

// 示例：
sem_t *sem = sem_open("/my_semaphore", O_CREAT, 0644, 1);
if (sem == SEM_FAILED) {
    perror("sem_open failed");
}

sem_wait(sem);
// 臨界區
sem_post(sem);

sem_close(sem);
sem_unlink("/my_semaphore");
```

## 🧵 生產者-消費者問題

### 問題描述

**經典同步問題：**
- **生產者**：生成數據並放入有限大小的緩衝區
- **消費者**：從緩衝區取出數據並處理
- **約束條件**：
  1. 緩衝區滿時，生產者必須等待
  2. 緩衝區空時，消費者必須等待
  3. 同時只能有一個線程訪問緩衝區（互斥）

### 信號量解決方案

**需要的信號量：**
```c
sem_t empty;   // 空閒槽位數（初始 = 緩衝區大小）
sem_t full;    // 已用槽位數（初始 = 0）
sem_t mutex;   // 互斥鎖（初始 = 1）

sem_init(&empty, 0, BUFFER_SIZE);  // 例如: 10
sem_init(&full, 0, 0);
sem_init(&mutex, 0, 1);
```

**生產者邏輯：**
```c
void* producer(void* arg)
{
    while (1) {
        int item = produce_item();  // 生產數據

        sem_wait(&empty);   // 1. 等待空槽位（P操作）
        sem_wait(&mutex);   // 2. 獲取互斥鎖

        // === 臨界區開始 ===
        buffer[in] = item;
        in = (in + 1) % BUFFER_SIZE;
        // === 臨界區結束 ===

        sem_post(&mutex);   // 3. 釋放互斥鎖
        sem_post(&full);    // 4. 增加滿槽位（V操作）
    }
}
```

**消費者邏輯：**
```c
void* consumer(void* arg)
{
    while (1) {
        sem_wait(&full);    // 1. 等待滿槽位（P操作）
        sem_wait(&mutex);   // 2. 獲取互斥鎖

        // === 臨界區開始 ===
        int item = buffer[out];
        out = (out + 1) % BUFFER_SIZE;
        // === 臨界區結束 ===

        sem_post(&mutex);   // 3. 釋放互斥鎖
        sem_post(&empty);   // 4. 增加空槽位（V操作）

        consume_item(item);  // 消費數據
    }
}
```

### 操作順序的重要性

**❌ 錯誤的順序（會導致死鎖）：**
```c
// 生產者 - 錯誤！
sem_wait(&mutex);   // 先獲取鎖
sem_wait(&empty);   // 如果沒有空槽位，死鎖！
```

**原因：**
1. 生產者持有 `mutex`，等待 `empty`
2. 消費者想要獲取 `mutex` 來釋放 `empty`，但被阻塞
3. 形成循環等待，死鎖！

**✅ 正確的順序：**
```c
// 生產者 - 正確
sem_wait(&empty);   // 先等待資源
sem_wait(&mutex);   // 再獲取鎖
```

**規則：** 先獲取資源信號量（empty/full），再獲取互斥鎖（mutex）

## ⚖️ 信號量 vs 互斥鎖 vs 條件變量

### 功能對比

| 特性 | 互斥鎖 (Mutex) | 信號量 (Semaphore) | 條件變量 (Condition Variable) |
|------|---------------|-------------------|------------------------------|
| **主要用途** | 互斥訪問 | 互斥 + 同步 + 資源計數 | 複雜條件等待 |
| **所有權** | 有（誰鎖誰解） | 無 | 無（必須配合 mutex） |
| **計數能力** | 否（0/1） | 是（0~N） | 否 |
| **跨線程釋放** | 不可以 | 可以 | 可以 |
| **性能** | 最快 | 較快 | 較慢（需配合 mutex） |
| **適用場景** | 簡單互斥 | 資源池、生產者消費者 | 複雜等待條件 |

### 互斥鎖 (Mutex)

**特點：**
- 有所有權概念（誰加鎖誰解鎖）
- 只用於互斥，不能用於同步
- 最簡單直接

```c
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_lock(&mutex);
// 臨界區
pthread_mutex_unlock(&mutex);
```

### 信號量 (Semaphore)

**特點：**
- 沒有所有權概念（任何線程都可以 post）
- 可以用於互斥、同步、資源計數
- 更靈活但更容易出錯

```c
sem_t sem;
sem_init(&sem, 0, 1);

sem_wait(&sem);
// 臨界區
sem_post(&sem);
```

### 條件變量 (Condition Variable)

**特點：**
- 必須配合 mutex 使用
- 用於等待複雜的條件
- 支持 broadcast 喚醒多個線程

```c
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

// 等待者
pthread_mutex_lock(&mutex);
while (!condition) {
    pthread_cond_wait(&cond, &mutex);
}
pthread_mutex_unlock(&mutex);

// 喚醒者
pthread_mutex_lock(&mutex);
condition = true;
pthread_cond_signal(&cond);
pthread_mutex_unlock(&mutex);
```

### 何時使用什麼？

**使用互斥鎖（Mutex）：**
- 簡單的互斥訪問
- 保護臨界區
- 性能要求高
- 例子：保護共享計數器

**使用信號量（Semaphore）：**
- 資源池管理（如連接池、線程池）
- 生產者-消費者問題
- 限制並發訪問數量
- 例子：最多 5 個並發連接

**使用條件變量（Condition Variable）：**
- 複雜的等待條件（如：count > 10 && flag == true）
- 需要 broadcast 多個等待者
- 已經使用 mutex 保護共享數據
- 例子：線程池的工作隊列

## 🚨 常見陷阱與錯誤

### 1. 死鎖

**原因：P 操作順序錯誤**

```c
// ❌ 錯誤：先鎖後等資源
sem_wait(&mutex);   // 1. 獲取鎖
sem_wait(&empty);   // 2. 等待資源 → 可能死鎖！
put_item();
sem_post(&mutex);
sem_post(&full);

// ✅ 正確：先等資源後鎖
sem_wait(&empty);   // 1. 等待資源
sem_wait(&mutex);   // 2. 獲取鎖
put_item();
sem_post(&mutex);
sem_post(&full);
```

### 2. 忘記 V 操作

**問題：資源永久丟失**

```c
// ❌ 錯誤：忘記 post
sem_wait(&sem);
// 臨界區
if (error) {
    return;  // 忘記 sem_post(&sem)！
}
sem_post(&sem);

// ✅ 正確：使用清理處理
sem_wait(&sem);
pthread_cleanup_push(cleanup_sem, &sem);
// 臨界區
pthread_cleanup_pop(1);

void cleanup_sem(void *arg) {
    sem_post((sem_t*)arg);
}
```

### 3. P/V 不配對

**問題：信號量值錯誤**

```c
// ❌ 錯誤：多個 wait 但只有一個 post
sem_wait(&sem);
sem_wait(&sem);  // wait 了兩次
// 臨界區
sem_post(&sem);  // 只 post 一次，信號量永久減少！

// ✅ 正確：wait 和 post 必須配對
sem_wait(&sem);
// 臨界區
sem_post(&sem);
```

### 4. 初始值錯誤

**問題：邏輯錯誤**

```c
// ❌ 錯誤：空槽位初始化為 0
sem_init(&empty, 0, 0);  // 錯誤！應該是 BUFFER_SIZE
sem_init(&full, 0, BUFFER_SIZE);

// ✅ 正確
sem_init(&empty, 0, BUFFER_SIZE);  // 空槽位 = 緩衝區大小
sem_init(&full, 0, 0);              // 滿槽位 = 0
```

## 🔨 編譯與運行

### 編譯

```bash
# 使用 Makefile
make semaphore

# 或手動編譯（需要 pthread 庫）
gcc -o semaphore_demo semaphore_demo.c -pthread

# 或使用 -lpthread（舊版系統）
gcc -o semaphore_demo semaphore_demo.c -lpthread
```

### 運行

```bash
./semaphore_demo

# 輸出示例：
# ====== 生產者-消費者問題演示 ======
#
# 配置：
#   緩衝區大小: 10
#   生產者數量: 2
#   消費者數量: 3
#   每個生產者生產: 20 個項目
#   總項目數: 40
#
# [生產者 1] 啟動
# [生產者 2] 啟動
# [消費者 1] 啟動
# [消費者 2] 啟動
# [消費者 3] 啟動
#
# [生產者 1] 生產 item=342, 位置=0
# [消費者 1] 消費 item=342, 位置=0
# ...
```

## 🚀 實際應用場景

### 1. 資源池管理

**連接池：**
```c
#define MAX_CONNECTIONS 5
sem_t connection_pool;
sem_init(&connection_pool, 0, MAX_CONNECTIONS);

// 獲取連接
sem_wait(&connection_pool);  // 如果池滿，阻塞
Connection *conn = get_connection();
use_connection(conn);

// 釋放連接
release_connection(conn);
sem_post(&connection_pool);  // 可用連接+1
```

**線程池：**
```c
sem_t thread_pool;
sem_init(&thread_pool, 0, NUM_THREADS);

void submit_task(Task *task) {
    sem_wait(&thread_pool);  // 等待空閒線程
    assign_task_to_thread(task);
}

void thread_finished() {
    sem_post(&thread_pool);  // 釋放線程
}
```

### 2. 限流控制

**限制並發請求數：**
```c
#define MAX_CONCURRENT 10
sem_t rate_limiter;
sem_init(&rate_limiter, 0, MAX_CONCURRENT);

void handle_request(Request *req) {
    sem_wait(&rate_limiter);  // 最多 10 個並發
    process_request(req);
    sem_post(&rate_limiter);
}
```

### 3. 任務隊列

**工作隊列：**
```c
sem_t tasks_available;
sem_init(&tasks_available, 0, 0);  // 初始沒有任務

// 生產者：添加任務
void add_task(Task *task) {
    queue_push(task);
    sem_post(&tasks_available);  // 通知有新任務
}

// 消費者：處理任務
void* worker(void* arg) {
    while (1) {
        sem_wait(&tasks_available);  // 等待任務
        Task *task = queue_pop();
        process_task(task);
    }
}
```

### 4. 同步點（Barrier）

**等待所有線程到達同步點：**
```c
sem_t barrier;
int count = 0;
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;

void wait_at_barrier(int num_threads) {
    pthread_mutex_lock(&count_mutex);
    count++;
    if (count == num_threads) {
        for (int i = 0; i < num_threads - 1; i++) {
            sem_post(&barrier);  // 喚醒所有線程
        }
    }
    pthread_mutex_unlock(&count_mutex);

    if (count < num_threads) {
        sem_wait(&barrier);  // 等待所有線程到達
    }
}
```

## 🔍 System V 信號量（進程間）

POSIX 信號量主要用於線程間同步。如果需要進程間同步，可以使用 System V 信號量。

### System V vs POSIX

| 特性 | POSIX 信號量 | System V 信號量 |
|------|-------------|----------------|
| **頭文件** | `<semaphore.h>` | `<sys/sem.h>` |
| **初始化** | `sem_init()` | `semget()`, `semctl()` |
| **P 操作** | `sem_wait()` | `semop()` |
| **V 操作** | `sem_post()` | `semop()` |
| **銷毀** | `sem_destroy()` | `semctl(IPC_RMID)` |
| **簡單性** | 簡單直觀 | 複雜強大 |
| **功能** | 基本功能 | 原子操作集 |
| **用途** | 線程間 | 進程間 |

### System V 信號量示例

```c
#include <sys/sem.h>
#include <sys/ipc.h>

// 1. 創建或獲取信號量集
key_t key = ftok("/tmp", 'S');
int semid = semget(key, 1, IPC_CREAT | 0666);

// 2. 初始化信號量值
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

union semun arg;
arg.val = 1;
semctl(semid, 0, SETVAL, arg);

// 3. P 操作（減 1）
struct sembuf sb;
sb.sem_num = 0;      // 信號量編號
sb.sem_op = -1;      // 減 1
sb.sem_flg = 0;      // 阻塞等待
semop(semid, &sb, 1);

// 臨界區

// 4. V 操作（加 1）
sb.sem_op = 1;       // 加 1
semop(semid, &sb, 1);

// 5. 刪除信號量
semctl(semid, 0, IPC_RMID);
```

## 🐛 調試技巧

### 1. 查看信號量值

```c
int value;
sem_getvalue(&sem, &value);
printf("信號量當前值: %d\n", value);

// 注意：在多線程環境中，值可能立即改變
```

### 2. 使用調試工具

```bash
# Helgrind - 檢測同步錯誤
valgrind --tool=helgrind ./semaphore_demo

# DRD - 數據競爭檢測器
valgrind --tool=drd ./semaphore_demo

# GDB 調試
gdb ./semaphore_demo
(gdb) break sem_wait
(gdb) run
```

### 3. 添加日誌

```c
#ifdef DEBUG
    #define LOG(fmt, ...) \
        printf("[%s:%d] " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#else
    #define LOG(fmt, ...)
#endif

LOG("Before sem_wait, value=%d", value);
sem_wait(&sem);
LOG("After sem_wait");
```

### 4. 超時檢測死鎖

```c
struct timespec ts;
clock_gettime(CLOCK_REALTIME, &ts);
ts.tv_sec += 5;  // 5 秒超時

if (sem_timedwait(&sem, &ts) == -1) {
    if (errno == ETIMEDOUT) {
        printf("警告：等待超時，可能死鎖！\n");
    }
}
```

## ⚡ 性能考慮

### 1. 減小臨界區

```c
// ❌ 錯誤：臨界區太大
sem_wait(&mutex);
expensive_computation();  // 耗時操作在臨界區內
shared_data++;
sem_post(&mutex);

// ✅ 正確：只保護必要的操作
expensive_computation();  // 在臨界區外完成
sem_wait(&mutex);
shared_data++;
sem_post(&mutex);
```

### 2. 避免頻繁 P/V

```c
// ❌ 錯誤：每次操作都加鎖
for (int i = 0; i < 1000; i++) {
    sem_wait(&mutex);
    array[i]++;
    sem_post(&mutex);
}

// ✅ 正確：批量處理
sem_wait(&mutex);
for (int i = 0; i < 1000; i++) {
    array[i]++;
}
sem_post(&mutex);
```

### 3. 使用 trywait 避免阻塞

```c
// 非阻塞嘗試
if (sem_trywait(&sem) == 0) {
    // 成功獲取信號量
    do_work();
    sem_post(&sem);
} else {
    // 無法立即獲取，做其他工作
    do_other_work();
}
```

## 📚 總結

### 信號量的核心要點

1. **P 操作（sem_wait）** - 申請資源，可能阻塞
2. **V 操作（sem_post）** - 釋放資源，喚醒等待者
3. **二元信號量** - 用於互斥（類似 mutex）
4. **計數信號量** - 用於資源計數和同步
5. **操作順序很重要** - 先資源後互斥，避免死鎖
6. **P/V 必須配對** - 每個 wait 對應一個 post

### 與其他同步機制的關系

- **Mutex** - 最簡單，只用於互斥
- **Semaphore** - 最靈活，可互斥可同步可計數
- **Condition Variable** - 最強大，用於複雜條件等待

### 最佳實踐

1. **優先使用 mutex** - 如果只需要簡單互斥
2. **使用信號量計數** - 資源池、限流、生產者消費者
3. **避免死鎖** - 正確的 P 操作順序
4. **配對 P/V** - 確保每個 wait 都有對應的 post
5. **使用超時** - 避免永久阻塞
6. **減小臨界區** - 提高並發性能

### 學習建議

1. **理解 P/V 操作** - 這是信號量的核心
2. **掌握生產者消費者** - 這是經典應用
3. **理解死鎖** - 知道如何避免
4. **對比三種機制** - mutex、semaphore、condition variable
5. **實踐調試** - 使用 valgrind 等工具

---

**下一步：** 學習 `12-condition-var` (條件變量) 了解更高級的同步機制
