/*
 * 檔案名稱: shm_reader.c
 * 功能說明: System V 共享內存 - 讀取端
 *
 * 知識點:
 *   1. 連接已存在的共享內存
 *   2. 讀取共享數據
 *   3. 簡單的同步機制
 *
 * 編譯方式: gcc -o shm_reader shm_reader.c
 * 執行方式: ./shm_reader
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define SHM_KEY 0x1234
#define SHM_SIZE 1024

// 與 writer 相同的數據結構
struct shared_data {
    int counter;
    char message[256];
    int flag;
};

int main(void)
{
    int shmid;
    struct shared_data *shared_mem;
    int last_counter = 0;

    printf("====== 共享內存讀取端 ======\n\n");

    // 獲取已存在的共享內存
    shmid = shmget(SHM_KEY, SHM_SIZE, 0666);
    if (shmid == -1) {
        perror("shmget failed");
        printf("\n[錯誤] 共享內存不存在\n");
        printf("[提示] 請先運行 shm_writer 創建共享內存\n");
        exit(EXIT_FAILURE);
    }

    printf("[連接] 共享內存 ID: %d\n", shmid);
    printf("[Key] IPC Key: 0x%x\n\n", SHM_KEY);

    // 映射共享內存
    shared_mem = (struct shared_data*)shmat(shmid, NULL, 0);
    if (shared_mem == (void*)-1) {
        perror("shmat failed");
        exit(EXIT_FAILURE);
    }

    printf("[映射] 共享內存地址: %p\n", (void*)shared_mem);
    printf("\n開始監聽共享內存...\n");
    printf("（按 Ctrl+C 退出）\n\n");

    // 循環讀取共享內存
    while (1) {
        // 檢查是否有新數據
        if (shared_mem->flag == 1 && shared_mem->counter > last_counter) {
            printf("┌─ 新消息 #%d ─────────────\n", shared_mem->counter);
            printf("│ 內容: %s\n", shared_mem->message);
            printf("└──────────────────────────\n\n");

            last_counter = shared_mem->counter;

            // 重置標誌（簡單的同步）
            shared_mem->flag = 0;
        }

        // 休眠一小段時間，避免 CPU 空轉
        usleep(100000);  // 100ms
    }

    // 分離共享內存
    if (shmdt(shared_mem) == -1) {
        perror("shmdt failed");
    }

    return 0;
}

/*
 * 使用步驟：
 *
 * 終端 1:
 * $ ./shm_writer
 * 請輸入消息: Hello
 * 請輸入消息: World
 *
 * 終端 2:
 * $ ./shm_reader
 * ┌─ 新消息 #1 ─────────────
 * │ 內容: Hello
 * └──────────────────────────
 *
 * ┌─ 新消息 #2 ─────────────
 * │ 內容: World
 * └──────────────────────────
 *
 * 進階同步機制：
 *
 * 1. 使用信號量：
 *    sem_t *sem = sem_open("/my_sem", O_CREAT, 0666, 1);
 *    sem_wait(sem);  // 加鎖
 *    // 訪問共享內存
 *    sem_post(sem);  // 解鎖
 *
 * 2. 使用 POSIX 共享內存 (更現代)：
 *    int fd = shm_open("/myshm", O_CREAT | O_RDWR, 0666);
 *    ftruncate(fd, size);
 *    void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE,
 *                     MAP_SHARED, fd, 0);
 *
 * 3. 結合文件鎖：
 *    使用 fcntl() 或 flock() 對共享內存進行鎖定
 */
