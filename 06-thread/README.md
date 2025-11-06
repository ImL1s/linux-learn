# 多線程編程 (Multithreading)

## 📖 概念介紹

**線程 (Thread)** 是進程內的執行單元。一個進程可以包含多個線程，它們共享進程的資源，但各自有獨立的執行流程。

### 線程 vs 進程

| 特性 | 進程 (Process) | 線程 (Thread) |
|------|---------------|--------------|
| 地址空間 | 獨立 | 共享 |
| 資源 | 獨立 | 共享（除了棧） |
| 通訊 | IPC (複雜) | 共享變量（簡單） |
| 創建開銷 | 大 | 小 |
| 切換開銷 | 大 | 小 |
| 安全性 | 高（隔離） | 低（需同步） |

### 線程的優勢

- ✅ 輕量級，創建和銷毀快
- ✅ 共享內存，通訊方便
- ✅ 充分利用多核 CPU
- ✅ 適合 I/O 密集型任務

### 線程的挑戰

- ⚠️ 需要同步機制
- ⚠️ 容易出現競爭條件
- ⚠️ 調試困難
- ⚠️ 可能死鎖

## 🔧 POSIX 線程 API

### pthread_create() - 創建線程

```c
#include <pthread.h>
int pthread_create(pthread_t *thread,
                   const pthread_attr_t *attr,
                   void *(*start_routine)(void *),
                   void *arg);
```

- **thread**: 線程 ID（輸出）
- **attr**: 線程屬性（NULL 使用默認）
- **start_routine**: 線程函數
- **arg**: 傳遞給線程的參數

### pthread_join() - 等待線程

```c
int pthread_join(pthread_t thread, void **retval);
```

- 阻塞等待指定線程結束
- 回收線程資源
- 獲取線程返回值

### pthread_detach() - 分離線程

```c
int pthread_detach(pthread_t thread);
```

- 設置為分離態
- 結束後自動回收資源
- 不能再 `join`

### pthread_exit() - 退出線程

```c
void pthread_exit(void *retval);
```

### pthread_self() - 獲取當前線程 ID

```c
pthread_t pthread_self(void);
```

## 🔒 同步機制

### 1. 互斥鎖 (Mutex)

#### 初始化
```c
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// 或動態初始化
pthread_mutex_t mutex;
pthread_mutex_init(&mutex, NULL);
```

#### 操作
```c
pthread_mutex_lock(&mutex);      // 加鎖（阻塞）
pthread_mutex_trylock(&mutex);   // 嘗試加鎖（非阻塞）
pthread_mutex_unlock(&mutex);    // 解鎖
pthread_mutex_destroy(&mutex);   // 銷毀
```

#### 使用示例
```c
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int counter = 0;

void* thread_func(void* arg) {
    pthread_mutex_lock(&mutex);
    counter++;  // 臨界區
    pthread_mutex_unlock(&mutex);
    return NULL;
}
```

### 2. 條件變量 (Condition Variable)

用於線程間的等待/通知：

```c
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// 等待
pthread_mutex_lock(&mutex);
while (!condition) {
    pthread_cond_wait(&cond, &mutex);
}
pthread_mutex_unlock(&mutex);

// 通知
pthread_cond_signal(&cond);      // 喚醒一個
pthread_cond_broadcast(&cond);   // 喚醒所有
```

### 3. 讀寫鎖 (RW Lock)

```c
pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;

pthread_rwlock_rdlock(&rwlock);  // 讀鎖
pthread_rwlock_wrlock(&rwlock);  // 寫鎖
pthread_rwlock_unlock(&rwlock);  // 解鎖
```

### 4. 自旋鎖 (Spinlock)

```c
pthread_spinlock_t spinlock;
pthread_spin_init(&spinlock, 0);

pthread_spin_lock(&spinlock);
pthread_spin_unlock(&spinlock);
pthread_spin_destroy(&spinlock);
```

## 📁 範例程式

### thread_demo.c
**功能**: 多線程綜合演示
- 基本線程創建和等待
- 競爭條件 (Race Condition) 演示
- 使用互斥鎖解決競爭條件
- 線程返回值處理

**編譯與運行**:
```bash
gcc -o thread_demo thread_demo.c -pthread
./thread_demo
```

**重要**: 必須使用 `-pthread` 選項編譯！

## 💡 重要概念

### 1. 競爭條件 (Race Condition)

當多個線程訪問共享數據且至少一個線程修改數據時，如果沒有適當同步，就會出現競爭條件。

#### 問題示例
```c
int counter = 0;  // 共享變量

void* increment(void* arg) {
    for (int i = 0; i < 100000; i++) {
        counter++;  // 不是原子操作！
        // 實際上是：
        // 1. 讀取 counter
        // 2. 加 1
        // 3. 寫回 counter
    }
    return NULL;
}
```

如果兩個線程同時執行，可能導致：
- 線程 A 讀取 counter = 0
- 線程 B 讀取 counter = 0
- 線程 A 寫入 counter = 1
- 線程 B 寫入 counter = 1
- 結果：兩次增加，counter 只增加了 1

#### 解決方案
```c
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int counter = 0;

void* safe_increment(void* arg) {
    for (int i = 0; i < 100000; i++) {
        pthread_mutex_lock(&mutex);
        counter++;
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}
```

### 2. 死鎖 (Deadlock)

#### 產生條件
1. 互斥：資源不能共享
2. 持有並等待：持有資源並等待其他資源
3. 不可搶占：資源不能被強制奪走
4. 循環等待：存在循環等待鏈

#### 死鎖示例
```c
// 線程 1
pthread_mutex_lock(&mutex_A);
pthread_mutex_lock(&mutex_B);

// 線程 2
pthread_mutex_lock(&mutex_B);  // 死鎖！
pthread_mutex_lock(&mutex_A);
```

#### 避免死鎖
```c
// 方法 1: 統一加鎖順序
// 所有線程都先鎖 A，再鎖 B

// 方法 2: 使用 trylock
if (pthread_mutex_trylock(&mutex_B) != 0) {
    pthread_mutex_unlock(&mutex_A);
    // 稍後重試
}

// 方法 3: 使用超時機制
pthread_mutex_timedlock(&mutex, &timeout);
```

### 3. 線程安全

**線程安全函數**：多個線程同時調用不會出問題
**不安全函數**：需要外部同步

```c
// 線程安全版本（_r 後綴）
strtok_r()   // vs strtok()
localtime_r() // vs localtime()
rand_r()     // vs rand()
```

### 4. 線程局部存儲 (TLS)

每個線程有自己的副本：

```c
__thread int thread_local_var = 0;

// 或使用 pthread API
pthread_key_t key;
pthread_key_create(&key, NULL);
pthread_setspecific(key, value);
void* value = pthread_getspecific(key);
```

## 🎯 常見模式

### 1. 生產者-消費者

```c
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int buffer[SIZE];
int count = 0;

void* producer(void* arg) {
    while (1) {
        pthread_mutex_lock(&mutex);
        while (count == SIZE) {
            pthread_cond_wait(&cond, &mutex);
        }
        // 生產數據
        buffer[count++] = produce();
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }
}

void* consumer(void* arg) {
    while (1) {
        pthread_mutex_lock(&mutex);
        while (count == 0) {
            pthread_cond_wait(&cond, &mutex);
        }
        // 消費數據
        consume(buffer[--count]);
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }
}
```

### 2. 線程池

```c
typedef struct {
    void (*function)(void*);
    void* argument;
} task_t;

typedef struct {
    pthread_t* threads;
    task_queue_t* queue;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int count;
} threadpool_t;
```

### 3. 讀者-寫者

```c
pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;

// 讀者
pthread_rwlock_rdlock(&rwlock);
// 讀取數據
pthread_rwlock_unlock(&rwlock);

// 寫者
pthread_rwlock_wrlock(&rwlock);
// 修改數據
pthread_rwlock_unlock(&rwlock);
```

## ❓ 常見問題

**Q1: 什麼時候使用多線程？**

適合多線程：
- I/O 密集型任務
- 需要並發響應
- 多核 CPU 計算密集型

不適合：
- CPU 綁定的 Python (GIL)
- 簡單任務（開銷大於收益）

**Q2: 互斥鎖 vs 自旋鎖？**

- **互斥鎖**: 睡眠等待，適合長時間持鎖
- **自旋鎖**: 忙等待，適合短時間持鎖

**Q3: 如何調試多線程程序？**

```bash
# 使用 gdb
gcc -g thread_demo.c -pthread -o thread_demo
gdb ./thread_demo

(gdb) info threads      # 查看所有線程
(gdb) thread 2          # 切換到線程 2
(gdb) thread apply all bt  # 所有線程的棧回溯

# 使用 helgrind (Valgrind 工具)
valgrind --tool=helgrind ./thread_demo

# 使用 ThreadSanitizer
gcc -fsanitize=thread -g -o thread_demo thread_demo.c -pthread
```

**Q4: 如何提高性能？**

1. 減少鎖的持有時間
2. 細粒度鎖 vs 粗粒度鎖
3. 無鎖數據結構
4. 減少線程創建/銷毀（線程池）

## 🔍 性能分析

### 查看線程

```bash
# 查看進程的線程
ps -eLf | grep program_name
top -H -p <pid>

# 查看線程狀態
cat /proc/<pid>/status | grep Threads
ls -l /proc/<pid>/task/
```

### 檢測問題

```bash
# 死鎖檢測
valgrind --tool=helgrind ./program

# 競爭條件檢測
gcc -fsanitize=thread program.c -pthread
```

## 📚 延伸學習

- **C++11 threads**: `std::thread`, `std::mutex`
- **無鎖編程**: 原子操作、CAS
- **協程 (Coroutine)**: 更輕量的並發
- **異步 I/O**: `epoll`, `io_uring`

## 🔗 相關工具

```bash
# 性能分析
perf record -g ./program
perf report

# 線程分析
strace -f ./program
```

## 📖 推薦閱讀

- `man pthread`
- `man pthread_create`
- `man pthread_mutex_lock`
- Programming with POSIX Threads (David R. Butenhof)
- The Art of Multiprocessor Programming
