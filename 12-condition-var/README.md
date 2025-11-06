# 條件變量 (Condition Variable)

## 📖 概念介紹

**條件變量 (Condition Variable)** 是 POSIX 線程庫提供的一種同步機制，允許線程等待某個條件成立。它是實現生產者-消費者問題、讀者-寫者問題等經典同步問題的重要工具。

### 什麼是條件變量？

條件變量是一個同步原語，允許線程：
- **等待**某個條件變為真
- **通知**其他線程條件已經改變
- 避免**忙等待** (busy waiting)，節省 CPU 資源

### 為什麼需要條件變量？

**問題場景**：消費者線程需要等待生產者線程生產數據

**錯誤方案1：忙等待**
```c
// ❌ 浪費 CPU
while (!data_ready) {
    // 不斷檢查，浪費 CPU 時間
}
```

**錯誤方案2：sleep 輪詢**
```c
// ❌ 響應延遲
while (!data_ready) {
    sleep(1); // 可能等待過久或檢查過頻
}
```

**正確方案：條件變量**
```c
// ✓ 高效、及時
pthread_mutex_lock(&mutex);
while (!data_ready) {
    pthread_cond_wait(&cond, &mutex);
}
// 使用數據
pthread_mutex_unlock(&mutex);
```

### 核心概念

1. **配合 mutex 使用**：條件變量總是與互斥鎖配合使用
2. **原子操作**：pthread_cond_wait() 原子性地解鎖 mutex 並進入等待
3. **喚醒機制**：signal 喚醒一個線程，broadcast 喚醒所有線程
4. **虛假喚醒**：必須用 while 循環檢查條件，而不是 if

## 🔧 API 說明

### pthread_cond_init() - 初始化條件變量

```c
#include <pthread.h>

int pthread_cond_init(pthread_cond_t *cond,
                      const pthread_condattr_t *attr);
```

**參數**:
- `cond`: 條件變量指針
- `attr`: 條件變量屬性（通常為 NULL，使用默認屬性）

**返回值**: 成功返回 0，失敗返回錯誤號

**示例**:
```c
pthread_cond_t cond;
pthread_cond_init(&cond, NULL);

// 或使用靜態初始化
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
```

### pthread_cond_wait() - 等待條件

```c
int pthread_cond_wait(pthread_cond_t *cond,
                      pthread_mutex_t *mutex);
```

**功能**: 等待條件變量，原子性地解鎖 mutex 並阻塞

**工作流程**:
1. 原子性地解鎖 mutex
2. 將線程放入等待隊列
3. 線程休眠
4. 被喚醒後重新競爭鎖定 mutex
5. 獲得 mutex 後返回

**參數**:
- `cond`: 條件變量
- `mutex`: 必須是已經鎖定的互斥鎖

**重要**：調用前必須持有 mutex！

**示例**:
```c
pthread_mutex_lock(&mutex);
while (!condition) {
    pthread_cond_wait(&cond, &mutex);
}
// 條件滿足，使用資源
pthread_mutex_unlock(&mutex);
```

### pthread_cond_signal() - 喚醒一個線程

```c
int pthread_cond_signal(pthread_cond_t *cond);
```

**功能**: 喚醒至少一個等待在條件變量上的線程

**特點**:
- 如果沒有線程等待，signal 會被忽略（不記憶）
- 如果有多個線程等待，由調度器決定喚醒哪一個
- 通常在修改條件後調用

**示例**:
```c
pthread_mutex_lock(&mutex);
// 修改條件
data_ready = true;
pthread_cond_signal(&cond);
pthread_mutex_unlock(&mutex);
```

### pthread_cond_broadcast() - 喚醒所有線程

```c
int pthread_cond_broadcast(pthread_cond_t *cond);
```

**功能**: 喚醒所有等待在條件變量上的線程

**使用場景**:
- 條件改變會影響所有等待線程
- 無法預知哪個線程應該被喚醒
- 讀者-寫者問題中寫者完成後喚醒所有讀者

**示例**:
```c
pthread_mutex_lock(&mutex);
// 修改影響所有線程的條件
shutdown = true;
pthread_cond_broadcast(&cond); // 喚醒所有等待線程
pthread_mutex_unlock(&mutex);
```

### pthread_cond_timedwait() - 帶超時的等待

```c
int pthread_cond_timedwait(pthread_cond_t *cond,
                           pthread_mutex_t *mutex,
                           const struct timespec *abstime);
```

**功能**: 等待條件變量，但有超時限制

**參數**:
- `abstime`: 絕對時間（不是相對時間！）

**返回值**:
- 0: 條件被滿足
- ETIMEDOUT: 超時
- 其他: 錯誤

**示例**:
```c
struct timespec ts;
clock_gettime(CLOCK_REALTIME, &ts);
ts.tv_sec += 5; // 5 秒後超時

pthread_mutex_lock(&mutex);
int ret = pthread_cond_timedwait(&cond, &mutex, &ts);
if (ret == ETIMEDOUT) {
    printf("等待超時\n");
}
pthread_mutex_unlock(&mutex);
```

### pthread_cond_destroy() - 銷毀條件變量

```c
int pthread_cond_destroy(pthread_cond_t *cond);
```

**注意**：銷毀前確保沒有線程在等待該條件變量

## 📁 範例程式

### 1. cond_basic.c
**功能**: 條件變量基礎演示

**知識點**:
- pthread_cond_wait() 的使用
- pthread_cond_signal() 喚醒單個線程
- pthread_cond_broadcast() 喚醒所有線程
- 為什麼用 while 而不是 if
- 虛假喚醒問題

**編譯運行**:
```bash
make condition-var
cd 12-condition-var
./cond_basic
```

**核心代碼片段**:
```c
// 等待方
pthread_mutex_lock(&mutex);
while (!ready) {
    pthread_cond_wait(&cond, &mutex);
}
// 使用資源
pthread_mutex_unlock(&mutex);

// 通知方
pthread_mutex_lock(&mutex);
ready = true;
pthread_cond_signal(&cond);
pthread_mutex_unlock(&mutex);
```

### 2. producer_consumer.c
**功能**: 使用條件變量解決生產者-消費者問題

**知識點**:
- 經典的生產者-消費者問題
- 有界緩衝區管理
- 兩個條件變量協同工作（not_full, not_empty）
- 多生產者多消費者場景

**編譯運行**:
```bash
gcc -o producer_consumer producer_consumer.c -pthread
./producer_consumer
```

**架構**:
```
生產者          緩衝區           消費者
  |              |                |
  |--生產-->  [buffer]  <--消費--|
  |              |                |
  等待 not_full  |    等待 not_empty
  |              |                |
  signal         |           signal
  not_empty      |           not_full
```

### 3. cond_vs_semaphore.c
**功能**: 條件變量 vs 信號量性能對比

**知識點**:
- 條件變量實現方案
- 信號量實現方案
- 性能測試方法
- 兩者的優缺點對比

**編譯運行**:
```bash
gcc -o cond_vs_semaphore cond_vs_semaphore.c -pthread
./cond_vs_semaphore
```

**對比結果**:
- 性能差異通常在 5% 以內
- 信號量略快（少一次 mutex 操作）
- 但條件變量更靈活、表達力更強

## 💡 重要知識點

### 為什麼必須配合 mutex 使用？

條件變量本身不提供互斥，它只負責等待和通知。

**錯誤示範**（沒有 mutex）：
```c
// ❌ 錯誤：競爭條件
if (!ready) {
    // 這裡可能被搶佔，ready 被其他線程修改
    pthread_cond_wait(&cond, &mutex); // 錯過信號！
}
```

**正確示範**（有 mutex）：
```c
// ✓ 正確
pthread_mutex_lock(&mutex);
while (!ready) {
    // mutex 保證原子性
    pthread_cond_wait(&cond, &mutex);
}
pthread_mutex_unlock(&mutex);
```

### 為什麼用 while 而不是 if？

**三個原因**：

1. **虛假喚醒 (Spurious Wakeup)**
   - 系統可能無故喚醒線程
   - POSIX 標準允許這種行為
   - 必須重新檢查條件

2. **條件被其他線程改變**
   - 多個線程同時被喚醒
   - 第一個線程消費了資源
   - 其他線程必須重新檢查

3. **signal vs broadcast**
   - broadcast 喚醒所有線程
   - 但可能只有一個資源
   - 其他線程必須重新等待

**示例**：
```c
// ❌ 錯誤：用 if
pthread_mutex_lock(&mutex);
if (count == 0) {
    pthread_cond_wait(&cond, &mutex);
}
// 可能 count 仍然是 0！（被其他線程消費了）
int item = buffer[--count];
pthread_mutex_unlock(&mutex);

// ✓ 正確：用 while
pthread_mutex_lock(&mutex);
while (count == 0) {
    pthread_cond_wait(&cond, &mutex);
}
// 確保 count > 0
int item = buffer[--count];
pthread_mutex_unlock(&mutex);
```

### signal 應該在 mutex 內還是外？

**推薦：在 mutex 內**
```c
pthread_mutex_lock(&mutex);
ready = true;
pthread_cond_signal(&cond); // 在鎖內
pthread_mutex_unlock(&mutex);
```

**也可以：在 mutex 外**
```c
pthread_mutex_lock(&mutex);
ready = true;
pthread_mutex_unlock(&mutex);
pthread_cond_signal(&cond); // 在鎖外
```

**區別**：
- 鎖內：等待線程被喚醒後立即阻塞（因為鎖還被持有）
- 鎖外：等待線程可能立即獲得鎖
- 性能差異很小，推薦鎖內（邏輯更清晰）

## 🔍 深入理解

### pthread_cond_wait() 的實現原理

```c
// 偽代碼展示 pthread_cond_wait() 的內部邏輯
int pthread_cond_wait(cond, mutex) {
    // 1. 將線程加入條件變量的等待隊列
    add_to_wait_queue(cond, current_thread);

    // 2. 原子性地解鎖 mutex 並阻塞
    atomic {
        unlock(mutex);
        block(current_thread);
    }

    // 3. 被喚醒後，重新鎖定 mutex
    lock(mutex);

    return 0;
}
```

**關鍵點**：解鎖和阻塞必須是原子操作，否則會丟失信號！

### 虛假喚醒的底層原因

1. **信號處理**：線程收到信號時可能被喚醒
2. **系統調用重啟**：某些系統調用被中斷後重新執行
3. **性能優化**：內核實現為了性能可能過度喚醒
4. **多核競爭**：多核 CPU 上的同步問題

### 條件變量 vs 信號量

| 特性 | 條件變量 | 信號量 |
|------|---------|--------|
| **獨立性** | 必須配合 mutex | 可獨立使用 |
| **計數** | 無計數值 | 有計數值 |
| **記憶性** | signal 不記憶 | post 會增加計數 |
| **表達力** | 可表達複雜條件 | 只能計數 |
| **廣播** | 支持 broadcast | 不支持 |
| **適用場景** | 等待條件成立 | 資源計數 |
| **代碼量** | 稍多 | 稍少 |
| **性能** | 略慢（需額外 mutex） | 略快 |

## ⚠️ 常見問題

### Q1: 為什麼 pthread_cond_wait() 需要傳入 mutex？

**答**: 為了實現原子性的"解鎖-等待"操作。

如果分開操作：
```c
// ❌ 錯誤：競爭窗口
pthread_mutex_unlock(&mutex);
// 這裡可能收到 signal，但錯過了！
pthread_cond_wait(&cond);
```

如果原子操作：
```c
// ✓ 正確：不會錯過信號
pthread_cond_wait(&cond, &mutex); // 原子性地解鎖並等待
```

### Q2: signal 沒有線程等待會怎樣？

**答**: 信號會被丟失，不會被記憶。

```c
// 線程 A
pthread_cond_signal(&cond); // 此時沒有線程等待

// 線程 B（稍後）
pthread_cond_wait(&cond, &mutex); // 會一直等待，不會收到之前的信號
```

**對比信號量**：sem_post() 會增加計數，即使沒有線程等待。

### Q3: 多個線程等待，signal 喚醒哪一個？

**答**: 由調度器決定，通常是等待時間最長的（FIFO），但不保證。

如果需要確定順序，有兩種方案：
1. 使用多個條件變量
2. 使用優先級隊列自己管理

### Q4: broadcast 會不會導致驚群效應？

**答**: 會的，但通常是必要的。

**驚群效應 (Thundering Herd)**：
- 所有線程被喚醒
- 但可能只有一個能獲得資源
- 其他線程重新睡眠
- 浪費 CPU

**緩解方法**：
- 確實需要喚醒所有線程時才用 broadcast
- 大多數情況用 signal 即可
- 使用多個條件變量細分條件

### Q5: 條件變量可以用於進程間同步嗎？

**答**: 可以，但需要設置屬性。

```c
pthread_condattr_t attr;
pthread_condattr_init(&attr);
pthread_condattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);

pthread_cond_t *cond = /* 共享內存中 */;
pthread_cond_init(cond, &attr);
```

但通常進程間同步更推薦使用：
- System V 信號量
- POSIX 有名信號量
- 共享內存 + 自旋鎖

## 🚀 實際應用

### 1. 線程池任務隊列

```c
typedef struct {
    task_t *tasks;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
} task_queue_t;

void* worker_thread(void* arg) {
    task_queue_t *queue = arg;

    while (1) {
        pthread_mutex_lock(&queue->mutex);

        while (queue->count == 0) {
            pthread_cond_wait(&queue->not_empty, &queue->mutex);
        }

        task_t task = dequeue(queue);
        pthread_mutex_unlock(&queue->mutex);

        execute(task);
    }
}
```

### 2. 讀者-寫者問題

```c
typedef struct {
    int readers;
    bool writer;
    pthread_mutex_t mutex;
    pthread_cond_t can_read;
    pthread_cond_t can_write;
} rwlock_t;

void read_lock(rwlock_t *rw) {
    pthread_mutex_lock(&rw->mutex);
    while (rw->writer) {
        pthread_cond_wait(&rw->can_read, &rw->mutex);
    }
    rw->readers++;
    pthread_mutex_unlock(&rw->mutex);
}

void write_lock(rwlock_t *rw) {
    pthread_mutex_lock(&rw->mutex);
    while (rw->writer || rw->readers > 0) {
        pthread_cond_wait(&rw->can_write, &rw->mutex);
    }
    rw->writer = true;
    pthread_mutex_unlock(&rw->mutex);
}
```

### 3. 事件通知系統

```c
typedef struct {
    bool event_occurred;
    pthread_mutex_t mutex;
    pthread_cond_t event;
} event_t;

// 等待事件
void wait_event(event_t *e) {
    pthread_mutex_lock(&e->mutex);
    while (!e->event_occurred) {
        pthread_cond_wait(&e->event, &e->mutex);
    }
    e->event_occurred = false; // 重置
    pthread_mutex_unlock(&e->mutex);
}

// 觸發事件
void trigger_event(event_t *e) {
    pthread_mutex_lock(&e->mutex);
    e->event_occurred = true;
    pthread_cond_broadcast(&e->event);
    pthread_mutex_unlock(&e->mutex);
}
```

## 📚 參考資料

- **POSIX 標準**: IEEE Std 1003.1-2017
- **Linux Manual**: `man pthread_cond_wait`
- **書籍**:
  - "Programming with POSIX Threads" by David R. Butenhof
  - "The Linux Programming Interface" by Michael Kerrisk
  - "UNIX Network Programming Vol.2" by W. Richard Stevens

## 🎯 學習建議

1. **先理解為什麼需要**：忙等待的問題
2. **掌握基本用法**：wait/signal 的配合
3. **理解 while 循環**：虛假喚醒問題
4. **實踐經典問題**：生產者-消費者
5. **對比其他機制**：信號量、互斥鎖
6. **注意常見陷阱**：忘記檢查條件、錯過信號

## 💻 實驗建議

1. 修改 producer_consumer.c，觀察不同緩衝區大小的影響
2. 嘗試用 if 代替 while，觀察會出現什麼問題
3. 比較 signal 和 broadcast 的性能差異
4. 實現一個簡單的線程池
5. 用條件變量實現讀者-寫者問題
