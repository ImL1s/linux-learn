/*
 * 檔案名稱: exec_demo.c
 * 功能說明: 演示 exec 家族函數的用法
 *
 * 知識點:
 *   1. exec 家族函數的作用
 *   2. execl, execlp, execle 的區別
 *   3. fork + exec 的經典組合
 *   4. exec 後的代碼不會執行（除非 exec 失敗）
 *
 * 編譯方式: gcc -o exec_demo exec_demo.c
 * 執行方式: ./exec_demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

/*
 * exec 家族函數總覽：
 *
 * int execl(const char *path, const char *arg, ...);
 * int execlp(const char *file, const char *arg, ...);
 * int execle(const char *path, const char *arg, ..., char *const envp[]);
 * int execv(const char *path, char *const argv[]);
 * int execvp(const char *file, char *const argv[]);
 * int execve(const char *path, char *const argv[], char *const envp[]);
 *
 * 命名規則：
 * - l (list): 參數以列表形式傳遞
 * - v (vector): 參數以數組形式傳遞
 * - p (path): 在 PATH 環境變量中搜索可執行文件
 * - e (environment): 可以指定環境變量
 */

void demo_execl(void);
void demo_execlp(void);
void demo_execv(void);

int main(void)
{
    int choice;

    printf("====== Exec 家族函數演示 ======\n\n");
    printf("選擇要演示的 exec 函數：\n");
    printf("1. execl()  - 列表形式，絕對路徑\n");
    printf("2. execlp() - 列表形式，PATH 搜索\n");
    printf("3. execv()  - 數組形式，絕對路徑\n");
    printf("0. 全部演示\n");
    printf("\n請輸入選項 (0-3): ");

    if (scanf("%d", &choice) != 1) {
        printf("無效輸入\n");
        return 1;
    }

    switch (choice) {
        case 1:
            demo_execl();
            break;
        case 2:
            demo_execlp();
            break;
        case 3:
            demo_execv();
            break;
        case 0:
            demo_execl();
            printf("\n按 Enter 繼續下一個演示...");
            getchar(); getchar();
            demo_execlp();
            printf("\n按 Enter 繼續下一個演示...");
            getchar();
            demo_execv();
            break;
        default:
            printf("無效選項\n");
            return 1;
    }

    return 0;
}

/*
 * 演示 1: execl() - 使用參數列表，需要完整路徑
 *
 * int execl(const char *path, const char *arg, ...);
 *
 * 參數說明：
 * - path: 可執行文件的完整路徑
 * - arg: 參數列表，第一個通常是程序名，最後必須是 NULL
 */
void demo_execl(void)
{
    pid_t pid;

    printf("\n【演示 execl()】\n");
    printf("功能：使用 ls 命令列出當前目錄\n\n");

    pid = fork();

    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) {
        // 子進程：執行 /bin/ls -l -h
        printf("[子進程 %d] 準備調用 execl()...\n", getpid());
        printf("[子進程 %d] 即將執行: /bin/ls -l -h\n\n", getpid());

        /*
         * execl 調用說明：
         * - 第一個參數：可執行文件的絕對路徑
         * - 後續參數：命令行參數（argv[0], argv[1], ...）
         * - 最後必須是 NULL
         *
         * 注意：第一個參數（"ls"）是 argv[0]，慣例上應該是程序名
         */
        execl("/bin/ls", "ls", "-l", "-h", NULL);

        /*
         * 如果 execl 成功，下面的代碼永遠不會執行！
         * 因為進程的代碼段已經被替換了。
         *
         * 只有 execl 失敗時，才會執行到這裡。
         */
        perror("[子進程] execl failed");
        exit(EXIT_FAILURE);
    }
    else {
        // 父進程：等待子進程完成
        int status;
        waitpid(pid, &status, 0);
        printf("\n[父進程] 子進程執行完畢\n");
    }
}

/*
 * 演示 2: execlp() - 使用參數列表，在 PATH 中搜索
 *
 * int execlp(const char *file, const char *arg, ...);
 *
 * 與 execl 的區別：
 * - 不需要提供完整路徑
 * - 會在 PATH 環境變量指定的目錄中搜索可執行文件
 */
void demo_execlp(void)
{
    pid_t pid;

    printf("\n【演示 execlp()】\n");
    printf("功能：執行 echo 命令輸出文本\n\n");

    pid = fork();

    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) {
        // 子進程：執行 echo
        printf("[子進程 %d] 準備調用 execlp()...\n", getpid());
        printf("[子進程 %d] 即將執行: echo\n\n", getpid());

        /*
         * execlp 會自動在 PATH 環境變量中搜索 "echo"
         * 不需要指定 /bin/echo
         */
        execlp("echo", "echo", "Hello from execlp!", "這是第二個參數", NULL);

        // 如果執行到這裡，說明 execlp 失敗了
        perror("[子進程] execlp failed");
        exit(EXIT_FAILURE);
    }
    else {
        // 父進程：等待子進程
        int status;
        waitpid(pid, &status, 0);
        printf("\n[父進程] 子進程執行完畢\n");
    }
}

/*
 * 演示 3: execv() - 使用參數數組
 *
 * int execv(const char *path, char *const argv[]);
 *
 * 與 execl 的區別：
 * - 參數以數組形式傳遞，而不是可變參數列表
 * - 適合參數數量動態變化的情況
 */
void demo_execv(void)
{
    pid_t pid;

    printf("\n【演示 execv()】\n");
    printf("功能：使用 ps 命令查看進程\n\n");

    pid = fork();

    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) {
        // 子進程：執行 ps aux
        printf("[子進程 %d] 準備調用 execv()...\n", getpid());
        printf("[子進程 %d] 即將執行: ps aux\n\n", getpid());

        /*
         * 構建參數數組
         * 注意：
         * 1. 第一個元素通常是程序名
         * 2. 最後一個元素必須是 NULL
         */
        char *args[] = {
            "ps",
            "aux",
            NULL  // 必須以 NULL 結尾
        };

        // 使用參數數組調用 execv
        execv("/bin/ps", args);

        // 如果執行到這裡，說明 execv 失敗了
        perror("[子進程] execv failed");
        exit(EXIT_FAILURE);
    }
    else {
        // 父進程：等待子進程
        int status;
        waitpid(pid, &status, 0);
        printf("\n[父進程] 子進程執行完畢\n");
    }
}

/*
 * 知識點總結：
 *
 * 1. exec 家族的作用：
 *    - 用新程序替換當前進程的代碼段
 *    - PID 不變，但代碼、數據、堆棧都被替換
 *    - 保留：PID, PPID, 打開的文件描述符（除非設置了 close-on-exec）
 *
 * 2. fork + exec 組合：
 *    - fork(): 創建新進程
 *    - exec(): 在新進程中運行其他程序
 *    - 這是 Unix/Linux 中創建新進程並運行程序的標準方式
 *    - Shell 執行命令就是這樣實現的
 *
 * 3. 選擇合適的 exec 函數：
 *
 *    使用 execl/execv:
 *    - 知道可執行文件的絕對路徑
 *    - 例如：系統程序 /bin/ls, /usr/bin/gcc
 *
 *    使用 execlp/execvp:
 *    - 依賴 PATH 環境變量
 *    - 例如：常用命令 ls, echo, grep
 *
 *    使用 execle/execve:
 *    - 需要自定義環境變量
 *    - 例如：設置特殊的 PATH, LANG 等
 *
 * 4. 參數傳遞方式：
 *
 *    使用 l (list):
 *    - 參數數量固定且已知
 *    - execl("/bin/ls", "ls", "-l", NULL);
 *
 *    使用 v (vector):
 *    - 參數數量動態變化
 *    - 參數來自數組或動態構建
 *
 * 5. 常見錯誤：
 *    - 忘記在參數列表末尾添加 NULL
 *    - 混淆 PATH 參數和 FILE 參數
 *    - 沒有檢查 exec 的返回值
 *    - 在不需要的地方調用 fork()
 *
 * 6. 實際應用：
 *    - Shell 命令執行
 *    - CGI 程序執行
 *    - 服務器進程管理
 *    - 守護進程 (daemon) 的啟動
 *
 * 7. 與 system() 的區別：
 *    - system(): 創建子進程並執行 shell 命令，會調用 /bin/sh
 *    - fork + exec: 直接執行程序，不經過 shell，更高效、更安全
 */
