/*
 * 檔案名稱: daemon_demo.c
 * 功能說明: Linux 守護進程 (Daemon) 完整演示
 *
 * 知識點:
 *   1. 守護進程的概念和特點
 *   2. 守護進程創建的標準步驟
 *   3. 會話(session)和進程組
 *   4. 文件描述符重定向
 *   5. 工作目錄和文件掩碼
 *   6. 進程鎖文件防止重複啟動
 *   7. 信號處理和優雅退出
 *   8. 日誌記錄 (syslog)
 *
 * 編譯方式: gcc -o daemon_demo daemon_demo.c
 * 執行方式: ./daemon_demo
 * 查看方式: ps -ef | grep daemon_demo
 *          tail -f /tmp/daemon.log
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <syslog.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>

#define DAEMON_NAME "daemon_demo"
#define PID_FILE "/tmp/daemon_demo.pid"
#define LOG_FILE "/tmp/daemon.log"
#define WORK_DIR "/"

volatile sig_atomic_t running = 1;

/*
 * 信號處理器
 */
void signal_handler(int sig)
{
    switch (sig) {
        case SIGTERM:
        case SIGINT:
            syslog(LOG_INFO, "收到終止信號 %d，準備退出", sig);
            running = 0;
            break;
        case SIGHUP:
            syslog(LOG_INFO, "收到 SIGHUP 信號，重新加載配置");
            // 這裡可以重新讀取配置文件
            break;
    }
}

/*
 * 創建 PID 文件並加鎖
 *
 * 作用：防止守護進程被多次啟動
 * 原理：使用文件鎖實現互斥
 */
int create_pid_file(void)
{
    int pid_fd;
    char pid_str[16];
    struct flock fl;

    // 打開或創建 PID 文件
    pid_fd = open(PID_FILE, O_RDWR | O_CREAT, 0644);
    if (pid_fd == -1) {
        syslog(LOG_ERR, "無法創建 PID 文件: %s", strerror(errno));
        return -1;
    }

    /*
     * 設置文件鎖
     *
     * 如果已有其他進程持有鎖，說明守護進程已在運行
     */
    fl.l_type = F_WRLCK;    // 寫鎖（排他鎖）
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;           // 鎖定整個文件

    if (fcntl(pid_fd, F_SETLK, &fl) == -1) {
        if (errno == EACCES || errno == EAGAIN) {
            syslog(LOG_ERR, "守護進程已在運行");
        } else {
            syslog(LOG_ERR, "無法鎖定 PID 文件: %s", strerror(errno));
        }
        close(pid_fd);
        return -1;
    }

    // 清空文件並寫入當前 PID
    if (ftruncate(pid_fd, 0) == -1) {
        syslog(LOG_ERR, "無法清空 PID 文件: %s", strerror(errno));
        close(pid_fd);
        return -1;
    }

    snprintf(pid_str, sizeof(pid_str), "%d\n", getpid());
    if (write(pid_fd, pid_str, strlen(pid_str)) == -1) {
        syslog(LOG_ERR, "無法寫入 PID: %s", strerror(errno));
        close(pid_fd);
        return -1;
    }

    // 注意：不關閉 pid_fd，保持鎖的持有直到進程退出
    return pid_fd;
}

/*
 * 守護進程化
 *
 * 標準的守護進程創建流程（遵循 UNIX 編程慣例）
 */
int daemonize(void)
{
    pid_t pid;
    int i, fd0, fd1, fd2;
    struct rlimit rl;
    struct sigaction sa;

    /*
     * 步驟 1: 清除文件模式創建掩碼
     *
     * 原因：守護進程從父進程繼承的 umask 可能不合適
     * umask(0) 確保守護進程創建的文件有正確的權限
     */
    umask(0);

    /*
     * 步驟 2: 獲取最大文件描述符數
     *
     * 後續需要關閉所有繼承的文件描述符
     */
    if (getrlimit(RLIMIT_NOFILE, &rl) < 0) {
        perror("getrlimit failed");
        return -1;
    }

    /*
     * 步驟 3: 創建子進程，父進程退出
     *
     * 目的：
     * 1. 確保守護進程不是進程組組長
     * 2. 使守護進程在後台運行
     */
    pid = fork();
    if (pid < 0) {
        perror("fork #1 failed");
        return -1;
    }
    if (pid > 0) {
        exit(0);  // 父進程退出
    }

    /*
     * 步驟 4: 創建新會話
     *
     * setsid() 的三個作用：
     * 1. 成為新會話的會話首進程
     * 2. 成為新進程組的組長進程
     * 3. 脫離控制終端
     *
     * 此時進程沒有控制終端，不會收到來自終端的信號
     */
    if (setsid() < 0) {
        perror("setsid failed");
        return -1;
    }

    /*
     * 設置信號處理
     *
     * 忽略 SIGHUP：當會話首進程退出時，
     * 內核會向會話中所有進程發送 SIGHUP
     */
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) < 0) {
        perror("sigaction failed");
        return -1;
    }

    /*
     * 步驟 5: 再次 fork
     *
     * 目的：確保守護進程不是會話首進程
     * 原因：會話首進程可以重新獲得控制終端
     * 第二次 fork 後，進程不再是會話首進程，
     * 永遠無法重新獲得控制終端
     */
    pid = fork();
    if (pid < 0) {
        perror("fork #2 failed");
        return -1;
    }
    if (pid > 0) {
        exit(0);  // 第一個子進程退出
    }

    /*
     * 步驟 6: 改變當前工作目錄
     *
     * 原因：守護進程可能長時間運行，
     * 如果工作目錄在可卸載的文件系統上，
     * 會導致該文件系統無法卸載
     *
     * 通常改為根目錄 "/"
     */
    if (chdir(WORK_DIR) < 0) {
        perror("chdir failed");
        return -1;
    }

    /*
     * 步驟 7: 關閉所有打開的文件描述符
     *
     * 守護進程不需要從父進程繼承的文件描述符
     * 包括標準輸入、標準輸出、標準錯誤
     */
    if (rl.rlim_max == RLIM_INFINITY) {
        rl.rlim_max = 1024;
    }
    for (i = 0; i < (int)rl.rlim_max; i++) {
        close(i);
    }

    /*
     * 步驟 8: 重定向標準文件描述符到 /dev/null
     *
     * 原因：某些庫函數假定這些文件描述符是打開的
     * 如果不重定向，可能導致錯誤
     *
     * /dev/null 是特殊文件：
     * - 讀取返回 EOF
     * - 寫入的數據被丟棄
     */
    fd0 = open("/dev/null", O_RDWR);  // stdin
    fd1 = dup(fd0);                    // stdout
    fd2 = dup(fd0);                    // stderr

    if (fd0 != 0 || fd1 != 1 || fd2 != 2) {
        syslog(LOG_ERR, "意外的文件描述符: %d %d %d", fd0, fd1, fd2);
        return -1;
    }

    return 0;
}

/*
 * 守護進程的主要工作邏輯
 */
void daemon_work(void)
{
    FILE *log_fp;
    int count = 0;

    // 打開日誌文件
    log_fp = fopen(LOG_FILE, "a");
    if (log_fp == NULL) {
        syslog(LOG_ERR, "無法打開日誌文件: %s", strerror(errno));
        return;
    }

    syslog(LOG_INFO, "守護進程工作循環開始");

    /*
     * 主工作循環
     *
     * 實際的守護進程會在這裡執行各種任務：
     * - Web 服務器：處理 HTTP 請求
     * - 數據庫：處理查詢
     * - 監控程序：收集系統信息
     * - 定時任務：執行計劃任務
     */
    while (running) {
        time_t now = time(NULL);
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S",
                 localtime(&now));

        // 寫入日誌
        fprintf(log_fp, "[%s] 守護進程運行中 (計數: %d)\n", time_str, ++count);
        fflush(log_fp);

        syslog(LOG_INFO, "心跳: %d", count);

        // 模擬工作
        sleep(5);
    }

    fclose(log_fp);
    syslog(LOG_INFO, "守護進程工作循環結束");
}

int main(void)
{
    int pid_fd;

    /*
     * 步驟 1: 初始化系統日誌
     *
     * openlog() 參數：
     * - ident: 日誌標識（每條日誌的前綴）
     * - option:
     *   LOG_PID: 在日誌中包含進程 ID
     *   LOG_CONS: 如果無法寫入日誌，輸出到控制台
     * - facility: 日誌類別（LOG_DAEMON 表示守護進程）
     */
    openlog(DAEMON_NAME, LOG_PID | LOG_CONS, LOG_DAEMON);
    syslog(LOG_INFO, "守護進程啟動中...");

    /*
     * 步驟 2: 守護進程化
     */
    if (daemonize() == -1) {
        syslog(LOG_ERR, "守護進程化失敗");
        closelog();
        return EXIT_FAILURE;
    }

    syslog(LOG_INFO, "守護進程化成功，PID: %d", getpid());

    /*
     * 步驟 3: 創建 PID 文件
     *
     * 防止守護進程被多次啟動
     */
    pid_fd = create_pid_file();
    if (pid_fd == -1) {
        closelog();
        return EXIT_FAILURE;
    }

    syslog(LOG_INFO, "PID 文件創建成功: %s", PID_FILE);

    /*
     * 步驟 4: 設置信號處理
     */
    signal(SIGTERM, signal_handler);  // 終止信號
    signal(SIGINT, signal_handler);   // 中斷信號（雖然通常不會收到）
    signal(SIGHUP, signal_handler);   // 掛起信號（重新加載配置）
    signal(SIGPIPE, SIG_IGN);         // 忽略管道破裂信號

    syslog(LOG_INFO, "信號處理器已註冊");

    /*
     * 步驟 5: 執行守護進程的實際工作
     */
    syslog(LOG_INFO, "守護進程啟動完成，開始工作");
    syslog(LOG_INFO, "日誌文件: %s", LOG_FILE);
    printf("守護進程已啟動，PID: %d\n", getpid());
    printf("日誌文件: %s\n", LOG_FILE);
    printf("PID 文件: %s\n", PID_FILE);
    printf("使用 'kill -TERM %d' 停止守護進程\n", getpid());

    daemon_work();

    /*
     * 步驟 6: 清理資源
     */
    close(pid_fd);
    unlink(PID_FILE);

    syslog(LOG_INFO, "守護進程正常退出");
    closelog();

    return EXIT_SUCCESS;
}

/*
 * 守護進程知識點總結：
 *
 * 1. 什麼是守護進程？
 *    - 在後台運行的進程
 *    - 沒有控制終端
 *    - 通常在系統啟動時啟動
 *    - 以 root 或特定用戶身份運行
 *    - 例如：httpd, sshd, crond
 *
 * 2. 守護進程的特點：
 *    - 長時間運行（可能是系統整個生命週期）
 *    - 父進程通常是 init (PID=1) 或 systemd
 *    - 會話首進程和進程組組長
 *    - 沒有控制終端（tty 顯示為 ?）
 *    - 工作目錄通常是根目錄 /
 *
 * 3. 守護進程創建步驟：
 *    ① umask(0) - 清除文件掩碼
 *    ② fork() - 父進程退出
 *    ③ setsid() - 創建新會話
 *    ④ fork() - 再次 fork（可選但推薦）
 *    ⑤ chdir() - 改變工作目錄
 *    ⑥ close() - 關閉文件描述符
 *    ⑦ 重定向 stdin/stdout/stderr 到 /dev/null
 *
 * 4. 會話 (Session) 和進程組：
 *    - 會話：一組進程組的集合
 *    - 進程組：一組相關進程
 *    - 會話首進程：會話中的第一個進程
 *    - setsid() 創建新會話並脫離終端
 *
 * 5. 為什麼要兩次 fork？
 *    第一次 fork：
 *    - 讓父進程退出
 *    - 確保子進程不是進程組組長
 *    - 為 setsid() 做準備
 *
 *    第二次 fork：
 *    - 確保進程不是會話首進程
 *    - 防止進程重新獲得控制終端
 *    - 更安全可靠
 *
 * 6. PID 文件的作用：
 *    - 防止多次啟動
 *    - 記錄進程 ID（便於管理）
 *    - 使用文件鎖實現互斥
 *    - 位置通常在 /var/run/ 或 /tmp/
 *
 * 7. 系統日誌 (syslog)：
 *    - 守護進程沒有終端，無法使用 printf
 *    - syslog 統一管理日誌
 *    - 日誌通常在 /var/log/messages 或 /var/log/syslog
 *    - 日誌級別：DEBUG, INFO, NOTICE, WARNING, ERR, CRIT, ALERT, EMERG
 *
 * 8. 信號處理：
 *    SIGTERM - 終止守護進程（優雅退出）
 *    SIGHUP - 重新加載配置文件
 *    SIGPIPE - 忽略（避免意外終止）
 *
 * 9. 啟動和管理：
 *    傳統方式：
 *    ./daemon_demo          # 啟動
 *    kill -TERM <pid>       # 停止
 *    kill -HUP <pid>        # 重新加載
 *
 *    systemd 方式：
 *    systemctl start daemon_demo
 *    systemctl stop daemon_demo
 *    systemctl reload daemon_demo
 *    systemctl status daemon_demo
 *
 * 10. 實際應用：
 *     - Web 服務器 (nginx, apache)
 *     - 數據庫 (mysql, postgresql)
 *     - SSH 服務 (sshd)
 *     - 計劃任務 (crond)
 *     - 系統監控 (collectd, zabbix-agent)
 *     - 消息隊列 (rabbitmq, kafka)
 *
 * 11. 調試技巧：
 *     ps -ef | grep daemon_demo    # 查看進程
 *     ps -o pid,ppid,sid,tty,cmd   # 查看詳細信息
 *     lsof -p <pid>                # 查看打開的文件
 *     tail -f /var/log/syslog      # 實時查看日誌
 *     strace -p <pid>              # 追蹤系統調用
 *
 * 12. 常見問題：
 *     Q: 守護進程如何退出？
 *     A: 發送 SIGTERM 信號，進程收到後優雅退出
 *
 *     Q: 如何重新加載配置？
 *     A: 發送 SIGHUP 信號，進程收到後重新讀取配置
 *
 *     Q: 守護進程崩潰怎麼辦？
 *     A: 使用 systemd 或 supervisord 自動重啟
 *
 * 13. 現代替代方案：
 *     - systemd: 推薦使用 Type=simple 或 Type=forking
 *     - supervisor: Python 編寫的進程管理工具
 *     - 容器化: Docker/Kubernetes 管理應用生命週期
 */
