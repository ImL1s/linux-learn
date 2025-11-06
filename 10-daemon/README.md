# 守護進程 (Daemon)

## 📖 簡介

守護進程（Daemon）是在後台運行的特殊進程，沒有控制終端，通常在系統啟動時啟動，並在系統運行期間持續提供服務。

**常見的守護進程：**
- `httpd` / `nginx` - Web 服務器
- `sshd` - SSH 服務
- `mysqld` - 數據庫服務
- `crond` - 定時任務調度
- `systemd` - 系統和服務管理器

## 📁 範例文件

- `daemon_demo.c` - 完整的守護進程實現（480 行，含詳細註解）

## 🎯 守護進程的特點

### 1. 基本特徵

- **後台運行**：沒有控制終端，不與用戶直接交互
- **長時間運行**：通常從系統啟動一直運行到系統關閉
- **父進程是 init/systemd**：PPID 通常是 1
- **獨立的會話**：是會話首進程，也是進程組組長
- **工作目錄是根目錄**：避免佔用可卸載的文件系統

### 2. 查看守護進程特徵

```bash
# 查看守護進程
ps -ef | grep daemon_demo

# 詳細查看進程信息
ps -o pid,ppid,sid,tty,cmd -p <pid>

# 輸出示例：
# PID  PPID   SID TTY  CMD
# 1234    1  1234  ?   ./daemon_demo
#
# 注意：TTY 顯示為 ? (沒有終端)
#      PPID 是 1 (父進程是 init/systemd)
#      SID = PID (會話首進程)
```

## 🔧 守護進程創建步驟

### 完整流程（8 個步驟）

守護進程的創建需要遵循嚴格的步驟，確保進程正確脫離終端並在後台運行。

#### 步驟 1: 清除文件模式創建掩碼

```c
umask(0);
```

**原因：**
- 守護進程從父進程繼承的 `umask` 可能不合適
- `umask(0)` 確保守護進程創建的文件有正確的權限
- 避免文件權限被意外限制

#### 步驟 2: 第一次 fork() - 父進程退出

```c
pid = fork();
if (pid < 0) return -1;
if (pid > 0) exit(0);  // 父進程退出
```

**目的：**
1. 讓父進程退出，守護進程在後台運行
2. 確保子進程不是進程組組長（為 `setsid()` 做準備）
3. 如果從 shell 啟動，shell 會認為命令已結束

#### 步驟 3: 創建新會話 - setsid()

```c
if (setsid() < 0) return -1;
```

**`setsid()` 的三個作用：**
1. **成為新會話的會話首進程** (Session Leader)
2. **成為新進程組的組長進程** (Process Group Leader)
3. **脫離控制終端**

**會話和進程組的概念：**

```
會話 (Session)
├── 進程組 1 (Foreground Process Group)
│   ├── 進程 1
│   └── 進程 2
└── 進程組 2 (Background Process Group)
    ├── 進程 3
    └── 進程 4
```

- **會話**：一組進程組的集合，最多有一個控制終端
- **進程組**：一組相關進程，可以一起接收信號
- **會話首進程**：創建會話的第一個進程

**為什麼需要新會話？**
- 原會話與終端綁定，會收到終端相關信號（如 SIGHUP）
- 新會話沒有控制終端，不受終端影響
- 終端關閉時，守護進程不會收到 SIGHUP 信號

#### 步驟 4: 第二次 fork() - 再次 fork

```c
signal(SIGHUP, SIG_IGN);  // 忽略 SIGHUP

pid = fork();
if (pid < 0) return -1;
if (pid > 0) exit(0);  // 第一個子進程退出
```

**為什麼要兩次 fork？**

這是守護進程編程中最重要的技巧之一：

**第一次 fork 的作用：**
- 讓父進程退出
- 確保子進程不是進程組組長
- 為調用 `setsid()` 做準備（只有非進程組組長才能調用）

**第二次 fork 的作用：**
- **防止進程重新獲得控制終端**
- 會話首進程有能力打開終端並將其作為控制終端
- 第二次 fork 後，進程不再是會話首進程
- 永遠無法重新獲得控制終端

**圖解兩次 fork：**

```
原始進程 (父進程)
    |
    | 第一次 fork
    v
子進程 1 (進程組組長)
    |
    | setsid() - 成為會話首進程，脫離終端
    |
    | 第二次 fork
    v
子進程 2 (不是會話首進程)
    |
    | 守護進程 - 永遠無法重新獲得控制終端
```

**注意：** 第二次 fork 是可選的，但強烈推薦！它提供了額外的安全保障。

#### 步驟 5: 改變當前工作目錄

```c
if (chdir("/") < 0) return -1;
```

**原因：**
- 守護進程可能長時間運行
- 如果工作目錄在可卸載的文件系統上（如 `/home`, `/mnt`）
- 會導致該文件系統無法卸載（busy）
- 通常改為根目錄 `/` 或 `/tmp`

#### 步驟 6: 關閉所有文件描述符

```c
struct rlimit rl;
getrlimit(RLIMIT_NOFILE, &rl);

if (rl.rlim_max == RLIM_INFINITY) {
    rl.rlim_max = 1024;
}

for (int i = 0; i < rl.rlim_max; i++) {
    close(i);
}
```

**原因：**
- 守護進程不需要從父進程繼承的文件描述符
- 包括標準輸入(0)、標準輸出(1)、標準錯誤(2)
- 關閉繼承的文件可以釋放資源
- 防止意外的文件訪問

#### 步驟 7: 重定向標準文件描述符到 /dev/null

```c
int fd0 = open("/dev/null", O_RDWR);  // stdin (fd 0)
int fd1 = dup(fd0);                    // stdout (fd 1)
int fd2 = dup(fd0);                    // stderr (fd 2)
```

**原因：**
- 某些庫函數假定 fd 0, 1, 2 是打開的
- 如果這些描述符關閉，可能導致錯誤
- `/dev/null` 是特殊文件：
  - 讀取：立即返回 EOF
  - 寫入：數據被丟棄

#### 步驟 8: 處理 umask（已在步驟 1 完成）

## 🔒 PID 文件鎖

### 為什麼需要 PID 文件？

**主要目的：**
1. **防止守護進程被多次啟動** - 通過文件鎖實現互斥
2. **記錄進程 ID** - 便於管理和停止守護進程
3. **提供狀態信息** - 其他程序可以檢查守護進程是否運行

### 實現方法

```c
int create_pid_file(void)
{
    int pid_fd;
    char pid_str[16];
    struct flock fl;

    // 1. 打開或創建 PID 文件
    pid_fd = open(PID_FILE, O_RDWR | O_CREAT, 0644);
    if (pid_fd == -1) {
        syslog(LOG_ERR, "無法創建 PID 文件: %s", strerror(errno));
        return -1;
    }

    // 2. 設置文件鎖
    fl.l_type = F_WRLCK;      // 寫鎖（排他鎖）
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;             // 鎖定整個文件

    // 3. 嘗試加鎖
    if (fcntl(pid_fd, F_SETLK, &fl) == -1) {
        if (errno == EACCES || errno == EAGAIN) {
            syslog(LOG_ERR, "守護進程已在運行");
        } else {
            syslog(LOG_ERR, "無法鎖定 PID 文件: %s", strerror(errno));
        }
        close(pid_fd);
        return -1;
    }

    // 4. 清空文件並寫入當前 PID
    ftruncate(pid_fd, 0);
    snprintf(pid_str, sizeof(pid_str), "%d\n", getpid());
    write(pid_fd, pid_str, strlen(pid_str));

    // 注意：不關閉 pid_fd，保持鎖直到進程退出
    return pid_fd;
}
```

**文件鎖的工作原理：**
- 第一個守護進程啟動時，成功獲得文件鎖
- 第二個守護進程嘗試啟動時，`fcntl()` 返回 `EAGAIN`
- 文件鎖會在進程退出時自動釋放
- 不需要手動解鎖

**PID 文件位置：**
- 系統守護進程：`/var/run/daemon_name.pid`
- 用戶守護進程：`/tmp/daemon_name.pid` 或 `~/.daemon_name.pid`

## 📝 系統日誌 (syslog)

### 為什麼需要 syslog？

守護進程沒有控制終端，無法使用 `printf()` 輸出信息。`syslog` 提供了統一的日誌管理機制。

### 使用方法

```c
// 1. 打開日誌連接
openlog(DAEMON_NAME,           // 日誌標識（前綴）
        LOG_PID | LOG_CONS,    // 選項
        LOG_DAEMON);           // 設施

// 2. 寫入日誌
syslog(LOG_INFO, "守護進程啟動，PID: %d", getpid());
syslog(LOG_ERR, "發生錯誤: %s", strerror(errno));

// 3. 關閉日誌連接
closelog();
```

### 日誌級別（優先級）

| 級別 | 宏定義 | 說明 |
|------|--------|------|
| 0 | LOG_EMERG | 系統不可用（最嚴重） |
| 1 | LOG_ALERT | 必須立即採取行動 |
| 2 | LOG_CRIT | 嚴重錯誤 |
| 3 | LOG_ERR | 錯誤 |
| 4 | LOG_WARNING | 警告 |
| 5 | LOG_NOTICE | 正常但重要的信息 |
| 6 | LOG_INFO | 一般信息 |
| 7 | LOG_DEBUG | 調試信息 |

### 日誌選項（openlog flags）

- `LOG_PID` - 在每條日誌中包含進程 ID
- `LOG_CONS` - 如果無法寫入日誌，輸出到控制台
- `LOG_NDELAY` - 立即打開日誌連接
- `LOG_PERROR` - 同時輸出到 stderr

### 查看日誌

```bash
# 實時查看系統日誌
tail -f /var/log/syslog | grep daemon_demo

# 或（取決於系統配置）
tail -f /var/log/messages | grep daemon_demo

# 使用 journalctl (systemd 系統)
journalctl -f -u daemon_demo
```

## 📡 信號處理

守護進程通過信號接收外部命令。

### 常用信號

| 信號 | 用途 | 處理方式 |
|------|------|----------|
| SIGTERM | 終止進程 | 優雅退出，清理資源 |
| SIGHUP | 掛起 | 重新加載配置文件 |
| SIGPIPE | 管道破裂 | 忽略（避免意外終止） |
| SIGUSR1/SIGUSR2 | 用戶自定義 | 自定義操作 |

### 信號處理器實現

```c
volatile sig_atomic_t running = 1;

void signal_handler(int sig)
{
    switch (sig) {
        case SIGTERM:
        case SIGINT:
            syslog(LOG_INFO, "收到終止信號 %d，準備退出", sig);
            running = 0;  // 設置退出標誌
            break;

        case SIGHUP:
            syslog(LOG_INFO, "收到 SIGHUP 信號，重新加載配置");
            // 重新讀取配置文件
            reload_config();
            break;
    }
}

int main(void)
{
    // 註冊信號處理器
    signal(SIGTERM, signal_handler);
    signal(SIGHUP, signal_handler);
    signal(SIGPIPE, SIG_IGN);

    // 主循環
    while (running) {
        // 執行守護進程的工作
        do_work();
    }

    // 清理資源
    cleanup();
    return 0;
}
```

**重要提示：**
- 使用 `volatile sig_atomic_t` 類型的標誌變量
- 信號處理器應該簡短，只設置標誌
- 避免在信號處理器中調用非信號安全的函數

## 🔨 編譯與運行

### 編譯

```bash
# 使用 Makefile
make daemon

# 或手動編譯
gcc -o daemon_demo daemon_demo.c
```

### 啟動守護進程

```bash
# 啟動
./daemon_demo

# 輸出示例：
# 守護進程已啟動，PID: 12345
# 日誌文件: /tmp/daemon.log
# PID 文件: /tmp/daemon_demo.pid
# 使用 'kill -TERM 12345' 停止守護進程
```

### 查看運行狀態

```bash
# 查看進程
ps -ef | grep daemon_demo

# 查看詳細信息（注意 TTY 為 ?）
ps -o pid,ppid,sid,tty,cmd -p $(cat /tmp/daemon_demo.pid)

# 查看打開的文件
lsof -p $(cat /tmp/daemon_demo.pid)
```

### 查看日誌

```bash
# 查看守護進程自己的日誌文件
tail -f /tmp/daemon.log

# 查看系統日誌
tail -f /var/log/syslog | grep daemon_demo
```

### 管理守護進程

```bash
# 停止守護進程（優雅退出）
kill -TERM $(cat /tmp/daemon_demo.pid)

# 或使用信號名
kill -SIGTERM $(cat /tmp/daemon_demo.pid)

# 重新加載配置
kill -HUP $(cat /tmp/daemon_demo.pid)

# 強制終止（不推薦，無法清理資源）
kill -KILL $(cat /tmp/daemon_demo.pid)
```

## 🐛 調試技巧

### 1. 查看進程信息

```bash
# 基本信息
ps -ef | grep daemon_demo

# 詳細信息
ps -o pid,ppid,pgid,sid,tty,tpgid,stat,cmd -p <pid>

# 進程樹
pstree -p | grep daemon_demo
```

### 2. 追蹤系統調用

```bash
# 啟動時追蹤
strace -o trace.log ./daemon_demo

# 附加到運行中的進程
strace -p <pid>
```

### 3. 查看打開的文件

```bash
# 查看所有打開的文件描述符
lsof -p <pid>

# 查看網絡連接
netstat -anp | grep <pid>
# 或
ss -anp | grep <pid>
```

### 4. 內存和 CPU 使用

```bash
# 實時監控
top -p <pid>

# 或
htop -p <pid>
```

## ❓ 常見問題

### Q1: 守護進程如何退出？

**A:** 發送 `SIGTERM` 信號，進程收到後優雅退出：

```bash
kill -TERM $(cat /tmp/daemon_demo.pid)
```

守護進程應該：
1. 在信號處理器中設置退出標誌
2. 在主循環中檢查標誌並退出
3. 清理資源（關閉文件、刪除 PID 文件等）

### Q2: 如何重新加載配置？

**A:** 發送 `SIGHUP` 信號：

```bash
kill -HUP $(cat /tmp/daemon_demo.pid)
```

守護進程應該在 `SIGHUP` 處理器中重新讀取配置文件。

### Q3: 守護進程崩潰怎麼辦？

**A:** 使用進程管理工具自動重啟：

**systemd:**
```ini
[Unit]
Description=My Daemon

[Service]
Type=forking
ExecStart=/path/to/daemon_demo
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
```

**supervisord:**
```ini
[program:daemon_demo]
command=/path/to/daemon_demo
autostart=true
autorestart=true
```

### Q4: 為什麼要兩次 fork？

**A:** 這是守護進程編程的關鍵技巧：

1. **第一次 fork**：讓父進程退出，子進程繼續運行
2. **setsid()**：創建新會話，脫離終端
3. **第二次 fork**：防止進程重新獲得控制終端

會話首進程有能力打開終端並作為控制終端。第二次 fork 後，進程不再是會話首進程，永遠無法重新獲得控制終端。

### Q5: 守護進程如何調試？

**A:** 調試守護進程比較困難，因為沒有終端輸出。建議：

1. **開發階段**：不執行守護進程化，直接運行
   ```c
   #ifdef DEBUG
       // 不調用 daemonize()
   #else
       daemonize();
   #endif
   ```

2. **使用日誌**：大量使用 `syslog()` 記錄關鍵步驟

3. **使用調試器**：
   ```bash
   gdb -p <pid>  # 附加到運行中的進程
   ```

## 🚀 實際應用

### 1. 系統服務

- **Web 服務器**: nginx, apache
- **數據庫**: MySQL, PostgreSQL, Redis
- **SSH 服務**: sshd
- **郵件服務**: postfix, sendmail

### 2. 監控和管理

- **系統監控**: collectd, zabbix-agent, prometheus node_exporter
- **日誌收集**: rsyslog, fluentd
- **配置管理**: puppet, chef

### 3. 計劃任務

- **cron**: 定時任務調度
- **atd**: 一次性任務調度

### 4. 網絡服務

- **消息隊列**: RabbitMQ, Kafka
- **緩存**: Memcached, Redis
- **代理**: HAProxy, Squid

## 🔄 與 systemd 集成

現代 Linux 系統使用 `systemd` 管理服務，不需要手動守護進程化。

### systemd 服務單元文件

創建 `/etc/systemd/system/daemon_demo.service`：

```ini
[Unit]
Description=Daemon Demo Service
After=network.target

[Service]
Type=forking
ExecStart=/usr/local/bin/daemon_demo
PIDFile=/var/run/daemon_demo.pid
Restart=on-failure
RestartSec=5s

# 安全選項
PrivateTmp=yes
NoNewPrivileges=true
ProtectSystem=strict
ProtectHome=yes

[Install]
WantedBy=multi-user.target
```

### systemd 管理命令

```bash
# 重新加載 systemd 配置
systemctl daemon-reload

# 啟動服務
systemctl start daemon_demo

# 停止服務
systemctl stop daemon_demo

# 重啟服務
systemctl restart daemon_demo

# 重新加載配置
systemctl reload daemon_demo

# 查看狀態
systemctl status daemon_demo

# 查看日誌
journalctl -u daemon_demo -f

# 開機自啟
systemctl enable daemon_demo

# 禁用自啟
systemctl disable daemon_demo
```

### systemd 服務類型

- `Type=simple`: 不 fork，直接在前台運行（推薦）
- `Type=forking`: 傳統守護進程（需要兩次 fork）
- `Type=oneshot`: 執行一次就退出
- `Type=notify`: 使用 `sd_notify()` 通知 systemd

**推薦使用 `Type=simple`**，讓 systemd 處理進程管理：

```c
int main(void)
{
    // 不調用 daemonize()，直接運行主循環
    // systemd 會處理所有守護進程化的細節

    openlog(DAEMON_NAME, LOG_PID, LOG_DAEMON);
    signal(SIGTERM, signal_handler);
    signal(SIGHUP, signal_handler);

    syslog(LOG_INFO, "服務啟動");
    daemon_work();
    syslog(LOG_INFO, "服務停止");

    closelog();
    return 0;
}
```

## 📚 總結

### 守護進程的核心要點

1. **兩次 fork** - 脫離終端，防止重新獲得控制終端
2. **setsid()** - 創建新會話，脫離控制終端
3. **chdir("/")** - 避免佔用可卸載文件系統
4. **關閉文件描述符** - 清理繼承的文件
5. **重定向 stdin/stdout/stderr** - 防止庫函數錯誤
6. **PID 文件鎖** - 防止多次啟動
7. **syslog** - 統一的日誌管理
8. **信號處理** - SIGTERM 優雅退出，SIGHUP 重新加載

### 現代最佳實踐

- **優先使用 systemd** - 讓系統管理器處理守護進程化
- **使用 Type=simple** - 不需要手動 fork
- **使用 journalctl** - systemd 的日誌系統
- **容器化** - 使用 Docker/Kubernetes 管理應用生命週期

### 學習建議

1. **理解兩次 fork** - 這是守護進程編程的精髓
2. **掌握信號處理** - 實現優雅退出和配置重載
3. **熟悉 systemd** - 現代系統的標準服務管理器
4. **實踐調試技巧** - 守護進程的調試與普通進程不同

---

**下一步：** 學習 `11-semaphore` (信號量) 或 `16-timer` (定時器)
