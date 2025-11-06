# 守護進程 (Daemon)

## 📖 簡介

守護進程是在後台運行的特殊進程，沒有控制終端，通常在系統啟動時啟動。例如：httpd, sshd, mysqld。

## 📁 範例文件

- `daemon_demo.c` - 完整的守護進程實現

## 🔨 編譯運行

```bash
# 編譯
make daemon

# 或單獨編譯
gcc -o daemon_demo daemon_demo.c

# 運行
./daemon_demo

# 查看進程
ps -ef | grep daemon_demo
ps -o pid,ppid,sid,tty,cmd -p <pid>

# 查看日誌
tail -f /tmp/daemon.log
tail -f /var/log/syslog | grep daemon_demo

# 停止守護進程
kill -TERM <pid>

# 重新加載配置
kill -HUP <pid>
```

## 💡 核心知識點

詳細的知識點和註解請查看源代碼：

- 守護進程標準創建步驟
- 兩次 fork() 的原因
- setsid() 創建新會話
- 會話和進程組
- umask/chdir/close 操作
- PID 文件鎖防止重複啟動
- syslog 日誌系統
- 信號處理（SIGTERM/SIGHUP）

**📝 所有詳細說明都在源代碼的註解中！**
