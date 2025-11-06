# 信號處理 (Signal Handling)

## 📖 概念介紹

**信號 (Signal)** 是 Unix/Linux 系統中用於進程間通訊和異步事件處理的機制。信號是一種軟件中斷，用於通知進程發生了某個事件。

### 信號的特點

- **異步性**: 信號可以在任何時候到達
- **優先級**: 某些信號會中斷當前執行
- **處理方式**: 可以捕獲、忽略或使用默認處理
- **有限性**: 信號編號有限，不適合大量數據傳輸

## 🔧 系統調用

### signal() - 註冊信號處理器

```c
#include <signal.h>
typedef void (*sighandler_t)(int);
sighandler_t signal(int signum, sighandler_t handler);
```

- **參數**:
  - `signum`: 信號編號 (SIGINT, SIGUSR1, etc.)
  - `handler`: 處理函數，或 `SIG_DFL` (默認) / `SIG_IGN` (忽略)
- **返回值**: 之前的處理器，或 `SIG_ERR`

### kill() - 發送信號

```c
#include <signal.h>
int kill(pid_t pid, int sig);
```

- **參數**:
  - `pid`: 目標進程 PID
  - `sig`: 信號編號
- **返回值**: 成功返回 0，失敗返回 -1

### raise() - 發送信號給自己

```c
int raise(int sig);
```

### alarm() - 設置鬧鐘

```c
unsigned int alarm(unsigned int seconds);
```

## 📊 常用信號

| 信號 | 編號 | 默認動作 | 說明 |
|------|------|---------|------|
| SIGHUP | 1 | 終止 | 終端掛起 |
| SIGINT | 2 | 終止 | 中斷 (Ctrl+C) |
| SIGQUIT | 3 | Core dump | 退出 (Ctrl+\\) |
| SIGILL | 4 | Core dump | 非法指令 |
| SIGABRT | 6 | Core dump | abort() 調用 |
| SIGFPE | 8 | Core dump | 浮點異常 |
| SIGKILL | 9 | 終止 | 強制終止（不可捕獲）|
| SIGSEGV | 11 | Core dump | 段錯誤 |
| SIGPIPE | 13 | 終止 | 管道破裂 |
| SIGALRM | 14 | 終止 | alarm() 鬧鐘 |
| SIGTERM | 15 | 終止 | 終止信號 |
| SIGUSR1 | 10 | 終止 | 用戶自定義信號 1 |
| SIGUSR2 | 12 | 終止 | 用戶自定義信號 2 |
| SIGCHLD | 17 | 忽略 | 子進程狀態改變 |
| SIGSTOP | 19 | 停止 | 停止進程（不可捕獲）|
| SIGCONT | 18 | 繼續 | 繼續執行 |

## 📁 範例程式

### signal_demo.c
**功能**: 綜合信號處理演示
- SIGINT (Ctrl+C) 優雅退出
- SIGUSR1 自定義信號處理
- SIGTERM 終止信號處理
- SIGALRM 定時器信號
- 信號發送與接收

**編譯與運行**:
```bash
gcc -o signal_demo signal_demo.c
./signal_demo
```

## 💡 重要概念

### 1. 信號處理器安全

**異步信號安全函數** (可在信號處理器中使用):
```c
// 安全的函數
write()
_exit()
signal()
kill()
getpid()
```

**不安全的函數** (不應在信號處理器中使用):
```c
// 不安全的函數
printf(), fprintf()  // 不是異步信號安全
malloc(), free()     // 可能導致死鎖
exit()              // 會調用清理函數
```

### 2. volatile sig_atomic_t

信號處理器和主程序間共享的變量應使用此類型：

```c
volatile sig_atomic_t flag = 0;

void handler(int sig) {
    flag = 1;  // 原子操作，安全
}

int main(void) {
    signal(SIGINT, handler);
    while (!flag) {
        // 等待信號
    }
}
```

### 3. 信號屏蔽

臨時阻止信號的傳遞：

```c
sigset_t set, oldset;
sigemptyset(&set);
sigaddset(&set, SIGINT);

// 阻塞 SIGINT
sigprocmask(SIG_BLOCK, &set, &oldset);
// 執行關鍵代碼...
// 恢復
sigprocmask(SIG_SETMASK, &oldset, NULL);
```

### 4. sigaction() - 更可靠的方式

`sigaction()` 比 `signal()` 更可靠：

```c
struct sigaction sa;
sa.sa_handler = handler;
sigemptyset(&sa.sa_mask);
sa.sa_flags = 0;
sigaction(SIGINT, &sa, NULL);
```

## 🎯 使用場景

### 1. 優雅退出

```c
volatile sig_atomic_t running = 1;

void sigint_handler(int sig) {
    running = 0;
}

int main(void) {
    signal(SIGINT, sigint_handler);

    while (running) {
        // 主循環
    }

    // 清理資源...
    return 0;
}
```

### 2. 定時任務

```c
void sigalrm_handler(int sig) {
    printf("定時任務執行\n");
    alarm(60);  // 60 秒後再次觸發
}

int main(void) {
    signal(SIGALRM, sigalrm_handler);
    alarm(60);  // 首次 60 秒後觸發

    while (1) {
        pause();  // 等待信號
    }
}
```

### 3. 回收子進程

```c
void sigchld_handler(int sig) {
    // 回收所有結束的子進程
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main(void) {
    signal(SIGCHLD, sigchld_handler);

    // 創建子進程...
}
```

## ❓ 常見問題

**Q1: SIGKILL 和 SIGSTOP 為什麼不能被捕獲？**

這是系統設計，確保管理員總能終止或停止失控的進程。

**Q2: 信號會丟失嗎？**

標準信號不排隊，同一信號多次發送可能只處理一次。實時信號 (SIGRTMIN-SIGRTMAX) 會排隊。

**Q3: 信號處理器能調用 printf 嗎？**

技術上可以，但不安全。應該使用 `write()` 替代：
```c
const char msg[] = "Signal received\n";
write(STDOUT_FILENO, msg, sizeof(msg)-1);
```

**Q4: 如何發送信號給其他進程？**

```bash
# 命令行
kill -SIGUSR1 <pid>
kill -10 <pid>       # 10 是 SIGUSR1 的編號

# 代碼
kill(pid, SIGUSR1);
```

## 🔍 調試技巧

### 查看信號

```bash
# 列出所有信號
kill -l

# 查看進程收到的信號
cat /proc/<pid>/status | grep Sig
```

### 發送信號

```bash
# 發送 SIGINT
kill -INT <pid>

# 發送 SIGUSR1
kill -USR1 <pid>

# 強制殺死
kill -9 <pid>
```

## 📚 延伸學習

### sigaction vs signal

- `sigaction()` 更可靠，行為一致
- `signal()` 行為在不同系統可能不同
- 生產環境推薦使用 `sigaction()`

### 實時信號

```c
// SIGRTMIN 到 SIGRTMAX
// 會排隊，不會丟失
// 可以傳遞額外數據
```

### 信號集操作

```c
sigset_t set;
sigemptyset(&set);     // 清空
sigfillset(&set);      // 填滿
sigaddset(&set, SIG);  // 添加
sigdelset(&set, SIG);  // 刪除
sigismember(&set, SIG); // 測試
```

## 🔗 相關命令

```bash
# 查看所有信號
kill -l

# 發送信號
kill -<signal> <pid>
killall -<signal> <name>

# 查看進程信號狀態
cat /proc/<pid>/status | grep Sig
```

## 📖 推薦閱讀

- `man 7 signal` - 信號概覽
- `man 2 signal` - signal 函數
- `man 2 sigaction` - sigaction 函數
- `man 2 kill` - kill 函數
- Advanced Programming in the UNIX Environment, Chapter 10
