/*
 * 檔案名稱: file_operations.c
 * 功能說明: 文件 I/O 操作演示
 *
 * 知識點:
 *   1. open/read/write/close 系統調用
 *   2. 文件描述符的概念
 *   3. 文件權限設置
 *   4. lseek 文件定位
 *   5. 文件鎖 (flock/fcntl)
 *
 * 編譯方式: gcc -o file_operations file_operations.c
 * 執行方式: ./file_operations
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#define TEST_FILE "test_data.txt"
#define BUFFER_SIZE 1024

void demo_basic_io(void);
void demo_lseek(void);
void demo_file_lock(void);

int main(void)
{
    printf("====== 文件 I/O 操作演示 ======\n\n");

    // 演示 1: 基本讀寫操作
    demo_basic_io();

    printf("\n按 Enter 繼續下一個演示...");
    getchar();

    // 演示 2: 文件定位 (lseek)
    demo_lseek();

    printf("\n按 Enter 繼續下一個演示...");
    getchar();

    // 演示 3: 文件鎖
    demo_file_lock();

    return 0;
}

/*
 * 演示 1: 基本文件讀寫操作
 */
void demo_basic_io(void)
{
    int fd;
    ssize_t bytes_written, bytes_read;
    char write_buf[] = "Hello, Linux File I/O!\n這是測試數據。\n";
    char read_buf[BUFFER_SIZE];

    printf("【演示 1: 基本文件讀寫】\n\n");

    /*
     * open() - 打開或創建文件
     *
     * int open(const char *pathname, int flags, mode_t mode);
     *
     * flags 常用選項：
     *   O_RDONLY  - 只讀
     *   O_WRONLY  - 只寫
     *   O_RDWR    - 讀寫
     *   O_CREAT   - 文件不存在則創建
     *   O_TRUNC   - 清空文件內容
     *   O_APPEND  - 追加模式
     *
     * mode: 文件權限（八進制）
     *   0644 = rw-r--r--
     *   0755 = rwxr-xr-x
     */
    fd = open(TEST_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open for write failed");
        return;
    }

    printf("[寫入] 打開文件: %s (fd=%d)\n", TEST_FILE, fd);

    // 寫入數據
    bytes_written = write(fd, write_buf, strlen(write_buf));
    if (bytes_written == -1) {
        perror("write failed");
        close(fd);
        return;
    }

    printf("[寫入] 成功寫入 %ld 字節\n", bytes_written);
    printf("[內容] %s", write_buf);

    // 關閉文件
    close(fd);
    printf("[寫入] 關閉文件\n\n");

    // 重新打開文件進行讀取
    fd = open(TEST_FILE, O_RDONLY);
    if (fd == -1) {
        perror("open for read failed");
        return;
    }

    printf("[讀取] 打開文件: %s (fd=%d)\n", TEST_FILE, fd);

    // 讀取數據
    memset(read_buf, 0, BUFFER_SIZE);
    bytes_read = read(fd, read_buf, BUFFER_SIZE - 1);
    if (bytes_read == -1) {
        perror("read failed");
        close(fd);
        return;
    }

    printf("[讀取] 成功讀取 %ld 字節\n", bytes_read);
    printf("[內容] %s\n", read_buf);

    close(fd);
    printf("[讀取] 關閉文件\n");
}

/*
 * 演示 2: lseek 文件定位
 */
void demo_lseek(void)
{
    int fd;
    char buffer[20];
    off_t offset;

    printf("\n【演示 2: 文件定位 (lseek)】\n\n");

    // 打開文件
    fd = open(TEST_FILE, O_RDONLY);
    if (fd == -1) {
        perror("open failed");
        return;
    }

    /*
     * lseek() - 移動文件讀寫位置
     *
     * off_t lseek(int fd, off_t offset, int whence);
     *
     * whence 選項：
     *   SEEK_SET - 從文件開頭
     *   SEEK_CUR - 從當前位置
     *   SEEK_END - 從文件末尾
     */

    // 1. 讀取開頭 10 字節
    memset(buffer, 0, sizeof(buffer));
    ssize_t n = read(fd, buffer, 10);
    printf("[1] 從開頭讀取 10 字節 (%ld bytes): %s\n", n, buffer);

    // 2. 移動到開頭，重新讀取
    lseek(fd, 0, SEEK_SET);
    memset(buffer, 0, sizeof(buffer));
    n = read(fd, buffer, 10);
    printf("[2] 重新從開頭讀取 (%ld bytes): %s\n", n, buffer);

    // 3. 移動到文件末尾
    offset = lseek(fd, 0, SEEK_END);
    printf("[3] 文件大小: %ld 字節\n", offset);

    // 4. 從末尾往前 10 字節
    lseek(fd, -10, SEEK_END);
    memset(buffer, 0, sizeof(buffer));
    n = read(fd, buffer, 10);
    printf("[4] 從末尾往前 10 字節 (%ld bytes): %s\n", n, buffer);

    close(fd);
}

/*
 * 演示 3: 文件鎖 (Advisory Locking)
 */
void demo_file_lock(void)
{
    int fd;
    struct flock lock;

    printf("\n【演示 3: 文件鎖】\n\n");

    fd = open(TEST_FILE, O_RDWR);
    if (fd == -1) {
        perror("open failed");
        return;
    }

    /*
     * struct flock 結構：
     *   short l_type;    // F_RDLCK, F_WRLCK, F_UNLCK
     *   short l_whence;  // SEEK_SET, SEEK_CUR, SEEK_END
     *   off_t l_start;   // 鎖定起始位置
     *   off_t l_len;     // 鎖定長度（0 表示到文件末尾）
     *   pid_t l_pid;     // 持有鎖的進程 ID
     */

    // 設置寫鎖
    lock.l_type = F_WRLCK;      // 寫鎖（排他鎖）
    lock.l_whence = SEEK_SET;   // 從文件開頭
    lock.l_start = 0;           // 起始位置
    lock.l_len = 0;             // 鎖定整個文件

    printf("[鎖定] 嘗試獲取寫鎖...\n");

    if (fcntl(fd, F_SETLK, &lock) == -1) {
        if (errno == EACCES || errno == EAGAIN) {
            printf("[鎖定] 文件已被其他進程鎖定\n");
        } else {
            perror("fcntl F_SETLK failed");
        }
        close(fd);
        return;
    }

    printf("[鎖定] 成功獲取寫鎖\n");
    printf("[操作] 持有鎖 5 秒（期間其他進程無法寫入）\n");
    sleep(5);

    // 釋放鎖
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    printf("[解鎖] 已釋放鎖\n");

    close(fd);
}

/*
 * 知識點總結：
 *
 * 1. 文件描述符 (File Descriptor)：
 *    - 非負整數，用於標識打開的文件
 *    - 0: stdin, 1: stdout, 2: stderr
 *    - 新打開的文件從 3 開始
 *
 * 2. open() flags 組合：
 *    open("file", O_RDWR | O_CREAT | O_TRUNC, 0644);
 *    - 讀寫模式
 *    - 不存在則創建
 *    - 存在則清空
 *    - 權限 rw-r--r--
 *
 * 3. lseek() 的妙用：
 *    - 獲取文件大小：lseek(fd, 0, SEEK_END);
 *    - 創建空洞文件（sparse file）
 *    - 隨機訪問文件
 *
 * 4. 文件鎖類型：
 *    - F_RDLCK: 共享鎖（讀鎖），多個進程可同時持有
 *    - F_WRLCK: 排他鎖（寫鎖），只有一個進程可持有
 *    - F_UNLCK: 解鎖
 *
 * 5. fcntl() 鎖定命令：
 *    - F_SETLK: 非阻塞獲取鎖
 *    - F_SETLKW: 阻塞獲取鎖（等待直到成功）
 *    - F_GETLK: 測試鎖
 */
