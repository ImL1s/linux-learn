# 進程管理 (Process Management)

## 📖 概念介紹

進程 (Process) 是操作系統中運行的程序實例。Linux 進程管理是系統程式設計的核心基礎，包括進程的創建、執行、同步和終止。

### 核心概念

- **進程 (Process)**: 程序的一次執行實例，包含代碼、數據、堆棧等
- **進程 ID (PID)**: 每個進程的唯一標識符
- **父進程 (Parent Process)**: 創建子進程的進程
- **子進程 (Child Process)**: 由父進程創建的新進程

## 🔧 系統調用

### fork() - 創建進程

```c
#include <unistd.h>
pid_t fork(void);
```

- **返回值**:
  - `> 0`: 在父進程中，返回子進程的 PID
  - `= 0`: 在子進程中
  - `< 0`: 創建失敗

### exec 家族 - 執行新程序

```c
int execl(const char *path, const char *arg, ..., NULL);
int execlp(const char *file, const char *arg, ..., NULL);
int execle(const char *path, const char *arg, ..., char *envp[]);
int execv(const char *path, char *const argv[]);
int execvp(const char *file, char *const argv[]);
int execve(const char *path, char *const argv[], char *const envp[]);
```

### wait/waitpid - 等待子進程

```c
#include <sys/wait.h>
pid_t wait(int *status);
pid_t waitpid(pid_t pid, int *status, int options);
```

## 📁 範例程式

### 1. fork_basic.c
**功能**: fork() 系統調用基礎演示
- 進程創建
- 父子進程區分
- 進程 ID 獲取
- 變量獨立性演示
- wait() 回收子進程

**編譯與運行**:
```bash
gcc -o fork_basic fork_basic.c
./fork_basic
```

### 2. exec_demo.c
**功能**: exec 家族函數演示
- execl() - 參數列表 + 絕對路徑
- execlp() - 參數列表 + PATH 搜索
- execv() - 參數數組 + 絕對路徑
- fork + exec 組合模式

**編譯與運行**:
```bash
gcc -o exec_demo exec_demo.c
./exec_demo
```

### 3. zombie_orphan.c
**功能**: 殭屍進程和孤兒進程演示
- 殭屍進程的產生
- 孤兒進程的產生
- 正確的進程回收方法
- WNOHANG 非阻塞等待

**編譯與運行**:
```bash
gcc -o zombie_orphan zombie_orphan.c
./zombie_orphan
```

## 📊 進程狀態

Linux 進程可能處於以下狀態：

```
R (Running)    - 運行或可運行
S (Sleeping)   - 可中斷睡眠
D (Disk sleep) - 不可中斷睡眠
T (Stopped)    - 停止（收到 SIGSTOP）
Z (Zombie)     - 殭屍進程
```

## 💡 重要概念

### 1. 寫時複製 (Copy-On-Write, COW)

fork() 創建子進程時，不會立即複製整個內存空間：

```
父進程內存        fork()        父子進程共享
┌─────────┐      ───>      ┌─────────┐
│ 代碼段  │                │ 代碼段  │ (共享)
│ 數據段  │                │ 數據段  │ (共享)
│ 堆棧    │                │ 堆棧    │ (共享)
└─────────┘                └─────────┘
                                │
                           寫入時才複製
                                ↓
                    ┌──────────────────┐
                    │ 父進程   │ 子進程 │
                    │ 獨立內存 │ 獨立內存│
                    └──────────────────┘
```

### 2. 殭屍進程 (Zombie Process)

**產生原因**: 子進程退出但父進程未調用 wait() 回收

**特徵**:
- 狀態顯示為 `Z`
- 已經終止但進程表項還保留
- 無法被 kill 命令殺死
- 佔用系統資源

**預防方法**:
```c
// 方法1: 及時調用 wait
wait(&status);

// 方法2: 忽略 SIGCHLD 信號
signal(SIGCHLD, SIG_IGN);

// 方法3: 在信號處理器中回收
void handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}
signal(SIGCHLD, handler);
```

### 3. 孤兒進程 (Orphan Process)

**產生原因**: 父進程先於子進程退出

**特徵**:
- PPID 變為 1 (init/systemd)
- 由 init 進程收養
- 不會造成資源浪費
- init 會自動回收

### 4. fork + exec 模式

這是 Unix/Linux 創建新進程並運行程序的標準模式：

```c
pid_t pid = fork();
if (pid == 0) {
    // 子進程
    execl("/bin/ls", "ls", "-l", NULL);
    // 如果 exec 失敗才會執行到這裡
    perror("exec failed");
    exit(1);
} else {
    // 父進程
    wait(NULL);  // 等待子進程結束
}
```

## 🎯 實驗建議

### 實驗 1: 觀察進程樹
```bash
# 運行程式
./fork_basic &

# 在另一個終端查看進程樹
pstree -p | grep fork_basic
ps -ef | grep fork_basic
```

### 實驗 2: 觀察殭屍進程
```bash
# 運行殭屍進程演示
./zombie_orphan
# 選擇選項 1

# 在另一個終端查看
ps aux | grep Z
ps -eo pid,ppid,stat,cmd | grep Z
```

### 實驗 3: 多次 fork
修改程式，連續調用兩次 fork()，觀察進程數量：
```c
fork();  // 創建 2 個進程
fork();  // 變成 4 個進程
fork();  // 變成 8 個進程
// n 次 fork 會創建 2^n 個進程
```

## ❓ 常見問題

**Q1: fork() 之後，父子進程共享什麼？**
- 共享：代碼段、打開的文件描述符
- 不共享：數據段、堆棧（採用寫時複製）

**Q2: 為什麼需要 fork + exec？**
- fork: 創建新進程
- exec: 替換進程的程序代碼
- 分離創建和執行，設計更靈活

**Q3: 如何統計殭屍進程數量？**
```bash
ps aux | awk '$8=="Z" {count++} END {print count}'
```

**Q4: vfork() 和 fork() 有什麼區別？**
- vfork() 不複製內存，父子進程共享地址空間
- vfork() 保證子進程先執行
- 子進程必須調用 exec() 或 _exit()
- 現代系統不推薦使用 vfork()

## 📚 延伸學習

### 相關系統調用
- `getpid()` - 獲取當前進程 PID
- `getppid()` - 獲取父進程 PID
- `exit()` - 終止進程
- `_exit()` - 立即終止進程（不調用清理函數）

### 進階主題
- 進程組和會話
- 守護進程 (Daemon)
- 進程調度
- 優先級調整 (nice, setpriority)

## 🔗 相關命令

```bash
# 查看進程信息
ps aux
ps -ef
ps -eo pid,ppid,stat,cmd

# 進程樹
pstree
pstree -p

# 實時監控
top
htop

# 查看特定進程
ps -p <pid> -o pid,ppid,stat,cmd

# 殺死進程
kill <pid>
kill -9 <pid>     # 強制殺死
killall <name>    # 根據名稱殺死
```

## 📖 推薦閱讀

- `man 2 fork`
- `man 2 exec`
- `man 2 wait`
- `man 2 waitpid`
- Advanced Programming in the UNIX Environment (APUE), Chapter 8
