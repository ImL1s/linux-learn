# 定時器 (Timer)

## 📖 概念介紹

**定時器**是Linux系統中用於定期執行任務或實現超時控制的重要機制。從簡單的 `alarm()` 到靈活的 POSIX 定時器，Linux 提供了多層次的定時解決方案。

### 為什麼需要定時器？

常見應用場景：
- **週期性任務**: 每秒刷新數據、每分鐘檢查狀態
- **超時控制**: 網絡請求超時、用戶輸入超時
- **性能採樣**: 定期收集CPU/內存統計信息
- **動畫/遊戲**: 幀率控制、遊戲循環

### Linux 定時器演進

```
簡單 → 複雜
低精度 → 高精度
單一 → 多實例

alarm()
  ↓
setitimer()
  ↓
timer_create() (POSIX)
  ↓
timerfd (可與epoll集成)
```

## 🔧 API 詳解

### 1. alarm() - 最簡單的定時器

```c
#include <unistd.h>

unsigned int alarm(unsigned int seconds);
```

**功能**: 在指定秒數後發送 `SIGALRM` 信號

**特點**:
- ✅ 極簡單
- ❌ 只能設置一次（再次調用會重置）
- ❌ 秒級精度
- ❌ 每個進程只有一個

**示例**:
```c
void alarm_handler(int sig) {
    printf("時間到！\n");
}

signal(SIGALRM, alarm_handler);
alarm(3);  // 3秒後觸發

// 取消定時器
alarm(0);
```

**返回值**: 上一個alarm的剩餘秒數（0表示無）

### 2. setitimer() - 週期性定時器

```c
#include <sys/time.h>

int setitimer(int which, const struct itimerval *new_value,
              struct itimerval *old_value);
```

**參數**:
- `which`: 定時器類型
  - `ITIMER_REAL`: 實際時間（wall clock），觸發 `SIGALRM`
  - `ITIMER_VIRTUAL`: 用戶態CPU時間，觸發 `SIGVTALRM`
  - `ITIMER_PROF`: 用戶態+內核態CPU時間，觸發 `SIGPROF`

```c
struct itimerval {
    struct timeval it_interval;  // 週期間隔
    struct timeval it_value;     // 首次觸發時間
};

struct timeval {
    time_t      tv_sec;    // 秒
    suseconds_t tv_usec;   // 微秒
};
```

**示例**:
```c
// 每 100ms 觸發一次
struct itimerval timer;
timer.it_value.tv_sec = 0;
timer.it_value.tv_usec = 100000;    // 首次100ms後
timer.it_interval.tv_sec = 0;
timer.it_interval.tv_usec = 100000;  // 之後每100ms

setitimer(ITIMER_REAL, &timer, NULL);

// 停止定時器
timer.it_value.tv_sec = 0;
timer.it_value.tv_usec = 0;
setitimer(ITIMER_REAL, &timer, NULL);
```

**優點**:
- ✅ 支持週期性觸發
- ✅ 微秒級精度
- ✅ 三種類型可選

**缺點**:
- ❌ 每種類型只能有一個實例
- ❌ 使用信號（不是線程安全的）

### 3. timer_create() - POSIX 定時器

```c
#include <time.h>
#include <signal.h>

int timer_create(clockid_t clockid, struct sigevent *sevp,
                 timer_t *timerid);
int timer_settime(timer_t timerid, int flags,
                  const struct itimerspec *new_value,
                  struct itimerspec *old_value);
int timer_delete(timer_t timerid);
```

**特點**:
- ✅ 可創建多個定時器
- ✅ 納秒級精度
- ✅ 更靈活的通知機制

**clockid 類型**:
```c
CLOCK_REALTIME      // 系統實時時間
CLOCK_MONOTONIC     // 單調遞增時間（不受NTP影響）
CLOCK_PROCESS_CPUTIME_ID   // 進程CPU時間
CLOCK_THREAD_CPUTIME_ID    // 線程CPU時間
```

**示例**:
```c
timer_t timerid;
struct sigevent sev;
struct itimerspec its;

// 設置通知方式（信號）
sev.sigev_notify = SIGEV_SIGNAL;
sev.sigev_signo = SIGRTMIN;
sev.sigev_value.sival_ptr = &timerid;

// 創建定時器
timer_create(CLOCK_REALTIME, &sev, &timerid);

// 設置定時：1秒後開始，每500ms觸發
its.it_value.tv_sec = 1;
its.it_value.tv_nsec = 0;
its.it_interval.tv_sec = 0;
its.it_interval.tv_nsec = 500000000;  // 500ms

timer_settime(timerid, 0, &its, NULL);

// 刪除定時器
timer_delete(timerid);
```

### 4. timerfd - 文件描述符定時器

```c
#include <sys/timerfd.h>

int timerfd_create(int clockid, int flags);
int timerfd_settime(int fd, int flags,
                    const struct itimerspec *new_value,
                    struct itimerspec *old_value);
```

**優勢**: 可與 `epoll`/`select` 集成

**示例**:
```c
int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);

struct itimerspec spec;
spec.it_value.tv_sec = 1;
spec.it_value.tv_nsec = 0;
spec.it_interval.tv_sec = 1;
spec.it_interval.tv_nsec = 0;

timerfd_settime(tfd, 0, &spec, NULL);

// 與 epoll 集成
epoll_ctl(epfd, EPOLL_CTL_ADD, tfd, &ev);

// 讀取定時器（消費事件）
uint64_t exp;
read(tfd, &exp, sizeof(exp));
```

## 📊 定時器對比

| 特性 | alarm | setitimer | timer_create | timerfd |
|------|-------|-----------|--------------|---------|
| **精度** | 秒 | 微秒 | 納秒 | 納秒 |
| **週期** | ❌ 單次 | ✅ 支持 | ✅ 支持 | ✅ 支持 |
| **多實例** | ❌ 1個 | ❌ 3個 | ✅ 無限 | ✅ 無限 |
| **通知** | SIGALRM | 信號 | 信號/線程 | 文件描述符 |
| **epoll集成** | ❌ | ❌ | ❌ | ✅ |
| **線程安全** | ❌ | ❌ | ⚠️ | ✅ |
| **複雜度** | 低 | 中 | 高 | 中 |

### 選擇建議

```
簡單超時（秒級）        → alarm()
週期性任務（毫秒級）    → setitimer()
多個定時器              → timer_create()
與epoll/select集成      → timerfd
需要線程安全            → timerfd
```

## 🎯 實戰應用場景

### 場景 1: 簡單超時檢測

```c
// 用戶輸入超時（5秒）
#include <signal.h>
#include <setjmp.h>

static jmp_buf env;

void timeout_handler(int sig) {
    longjmp(env, 1);
}

char *get_input_with_timeout() {
    signal(SIGALRM, timeout_handler);

    if (setjmp(env) == 0) {
        alarm(5);
        char *input = fgets(buffer, sizeof(buffer), stdin);
        alarm(0);  // 取消定時器
        return input;
    } else {
        printf("輸入超時！\n");
        return NULL;
    }
}
```

### 場景 2: 週期性狀態刷新

```c
// 每秒更新一次狀態
void update_handler(int sig) {
    static int count = 0;
    printf("Tick %d\n", ++count);

    // 刷新數據
    refresh_data();
}

void setup_periodic_update() {
    signal(SIGALRM, update_handler);

    struct itimerval timer;
    timer.it_value.tv_sec = 1;
    timer.it_value.tv_usec = 0;
    timer.it_interval = timer.it_value;  // 每1秒

    setitimer(ITIMER_REAL, &timer, NULL);
}
```

### 場景 3: 性能採樣

```c
// 每10ms採樣一次CPU使用率
void profiling_handler(int sig) {
    sample_cpu_usage();
    sample_memory();
}

void start_profiling() {
    signal(SIGALRM, profiling_handler);

    struct itimerval timer;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 10000;  // 10ms
    timer.it_interval = timer.it_value;

    setitimer(ITIMER_REAL, &timer, NULL);
}
```

### 場景 4: 與epoll集成的定時器

```c
// 事件驅動架構中的定時器
int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);

struct itimerspec spec = {
    .it_value = {1, 0},
    .it_interval = {1, 0}
};
timerfd_settime(tfd, 0, &spec, NULL);

// 添加到epoll
struct epoll_event ev;
ev.events = EPOLLIN;
ev.data.fd = tfd;
epoll_ctl(epfd, EPOLL_CTL_ADD, tfd, &ev);

// 事件循環
while (1) {
    int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
    for (int i = 0; i < n; i++) {
        if (events[i].data.fd == tfd) {
            uint64_t exp;
            read(tfd, &exp, sizeof(exp));
            printf("定時器觸發\n");
        }
    }
}
```

## 📁 範例程式

### 1. alarm_demo.c - 簡單定時器

**功能**: 演示 alarm() 基本用法

**核心代碼**:
```c
void handler(int sig) {
    printf("SIGALRM received!\n");
}

signal(SIGALRM, handler);
alarm(3);  // 3秒後觸發
pause();   // 等待信號
```

### 2. setitimer_demo.c - 週期定時器

**功能**: 演示 setitimer() 週期性觸發

**核心代碼**:
```c
void timer_handler(int sig) {
    static int count = 0;
    printf("Timer tick: %d\n", ++count);
}

signal(SIGALRM, timer_handler);

struct itimerval timer;
timer.it_value.tv_sec = 0;
timer.it_value.tv_usec = 500000;    // 首次500ms
timer.it_interval.tv_sec = 0;
timer.it_interval.tv_usec = 500000; // 每500ms

setitimer(ITIMER_REAL, &timer, NULL);
```

### 編譯與運行

```bash
make timer

./alarm_demo
# 3秒後輸出: SIGALRM received!

./setitimer_demo
# 每500ms輸出: Timer tick: 1, 2, 3...
```

## 💡 重要知識點

### 1. 信號處理注意事項

```c
// ❌ 不安全：在信號處理器中使用非異步信號安全函數
void bad_handler(int sig) {
    printf("Time's up!\n");  // printf 不是信號安全的
    malloc(100);             // malloc 不是信號安全的
}

// ✅ 安全：只使用異步信號安全函數
void safe_handler(int sig) {
    char msg[] = "Timer\n";
    write(STDOUT_FILENO, msg, sizeof(msg));
    // 或設置全局標誌，在主循環中處理
    global_flag = 1;
}
```

**異步信號安全函數**:
- `write()`, `read()`, `_exit()`
- **不安全**: `printf()`, `malloc()`, `free()`, `pthread_*`

### 2. 定時器精度問題

```c
// 系統負載影響精度
struct itimerval timer;
timer.it_interval.tv_usec = 1000;  // 設置1ms

// 實際觸發間隔可能是 1-10ms
// 取決於：系統負載、調度器、時鐘頻率
```

**提高精度**:
- 使用 `CLOCK_MONOTONIC` (不受NTP影響)
- 使用實時調度策略 (`SCHED_FIFO`)
- 避免在信號處理器中執行耗時操作

### 3. 定時器衝突

```c
// ❌ 錯誤：alarm 和 setitimer(ITIMER_REAL) 衝突
alarm(5);
setitimer(ITIMER_REAL, &timer, NULL);  // 取消了 alarm

// ✅ 正確：使用同一種機制
setitimer(ITIMER_REAL, &timer1, NULL);
// 或使用 timer_create 創建多個
```

### 4. 信號遺漏問題

```c
// 信號不會排隊
// 如果處理器執行時間過長，可能錯過後續信號

void slow_handler(int sig) {
    sleep(2);  // ⚠️ 如果定時器每1秒觸發一次，會錯過信號
}

// ✅ 解決：快速處理，或使用 timerfd
void fast_handler(int sig) {
    flag = 1;  // 僅設置標誌
}

// 主循環
while (1) {
    if (flag) {
        flag = 0;
        do_work();  // 在此處理耗時任務
    }
}
```

## 🐛 常見陷阱

### 陷阱 1: 忘記處理信號

```c
// ❌ 忘記設置信號處理器
alarm(5);
// 5秒後進程會終止（SIGALRM 默認行為）

// ✅ 正確
signal(SIGALRM, handler);
alarm(5);
```

### 陷阱 2: 多線程中使用 alarm/setitimer

```c
// ⚠️ 警告：信號會發送到隨機線程
pthread_create(&thread1, NULL, worker, NULL);
alarm(5);  // SIGALRM 可能發送到 thread1

// ✅ 使用 timer_create 指定線程
// 或使用 timerfd（線程安全）
```

### 陷阱 3: 定時器時間歸零

```c
struct itimerval timer;
timer.it_value.tv_sec = 5;
timer.it_value.tv_usec = 0;
timer.it_interval.tv_sec = 0;  // ⚠️ 週期為0，只觸發一次
timer.it_interval.tv_usec = 0;

// ✅ 週期性觸發
timer.it_interval = timer.it_value;
```

## 📚 參考資料

### 官方文檔
- `man alarm` - alarm 系統調用
- `man setitimer` - setitimer 系統調用
- `man timer_create` - POSIX 定時器
- `man timerfd_create` - 文件描述符定時器
- `man 7 signal` - 信號安全

### 推薦書籍
- *The Linux Programming Interface* - Chapter 23 (Timers and Sleeping)
- *UNIX Network Programming* - Chapter 20 (Broadcasting)

---

## 📝 總結

### 何時使用哪種定時器？

✅ **alarm()**: 簡單超時（秒級）
✅ **setitimer()**: 週期性任務（毫秒級）
✅ **timer_create()**: 多個定時器、高精度
✅ **timerfd**: 與 epoll 集成、線程安全

### 關鍵要點

1. **信號安全**: 信號處理器中只使用異步信號安全函數
2. **精度限制**: 受系統負載和調度影響
3. **衝突問題**: alarm 和 setitimer(ITIMER_REAL) 互斥
4. **線程安全**: alarm/setitimer 不適合多線程
5. **信號遺漏**: 信號不排隊，處理要快速

掌握定時器，讓你的程序能夠優雅地處理週期性任務和超時控制！
