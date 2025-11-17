/*
 * 檔案名稱: shm_writer.c
 * 功能說明: System V 共享內存 - 寫入端
 *
 * 知識點:
 *   1. shmget() 創建/獲取共享內存
 *   2. shmat() 映射共享內存到進程地址空間
 *   3. shmdt() 分離共享內存
 *   4. shmctl() 控制共享內存
 *   5. IPC key 的概念
 *
 * 編譯方式: gcc -o shm_writer shm_writer.c
 * 執行方式: ./shm_writer
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define SHM_KEY 0x1234      // IPC 鍵值
#define SHM_SIZE 1024       // 共享內存大小

// 共享內存的數據結構
struct shared_data {
    int counter;
    char message[256];
    int flag;  // 0: 寫入端在寫, 1: 數據可讀
};

int main(void)
{
    int shmid;
    struct shared_data *shared_mem;
    char input[256];

    printf("====== 共享內存寫入端 ======\n\n");

    /*
     * shmget() - 創建或獲取共享內存段
     *
     * int shmget(key_t key, size_t size, int shmflg);
     *
     * 參數：
     *   key: IPC 鍵值（用於標識共享內存）
     *   size: 共享內存大小（字節）
     *   shmflg: 標誌位
     *     IPC_CREAT: 如果不存在則創建
     *     IPC_EXCL: 與 IPC_CREAT 一起使用，如果已存在則報錯
     *     0666: 權限（類似文件權限）
     *
     * 返回值：
     *   成功：共享內存 ID
     *   失敗：-1
     */
    shmid = shmget(SHM_KEY, SHM_SIZE, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }

    printf("[創建] 共享內存 ID: %d\n", shmid);
    printf("[Key] IPC Key: 0x%x\n", SHM_KEY);
    printf("[大小] %d 字節\n\n", SHM_SIZE);

    /*
     * shmat() - 映射共享內存到進程地址空間
     *
     * void *shmat(int shmid, const void *shmaddr, int shmflg);
     *
     * 參數：
     *   shmid: 共享內存 ID
     *   shmaddr: 映射地址（NULL 表示由系統選擇）
     *   shmflg: 標誌位
     *     0: 可讀可寫
     *     SHM_RDONLY: 只讀
     *
     * 返回值：
     *   成功：映射地址
     *   失敗：(void*)-1
     */
    shared_mem = (struct shared_data*)shmat(shmid, NULL, 0);
    if (shared_mem == (void*)-1) {
        perror("shmat failed");
        // 清理已創建的共享內存段
        shmctl(shmid, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }

    printf("[映射] 共享內存地址: %p\n\n", (void*)shared_mem);

    // 初始化共享內存
    shared_mem->counter = 0;
    strcpy(shared_mem->message, "初始消息");
    shared_mem->flag = 0;

    printf("====== 使用說明 ======\n");
    printf("1. 啟動 shm_reader 讀取共享內存\n");
    printf("2. 在此輸入消息，會寫入共享內存\n");
    printf("3. 輸入 'quit' 退出\n");
    printf("====================\n\n");

    // 交互式寫入
    while (1) {
        printf("請輸入消息: ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        // 移除換行符
        size_t len = strlen(input);
        if (len > 0 && input[len-1] == '\n') {
            input[len-1] = '\0';
        }

        // 檢查退出命令
        if (strcmp(input, "quit") == 0) {
            printf("\n用戶請求退出\n");
            break;
        }

        /*
         * ⚠️ 教育警告：以下代碼存在競爭條件！
         *
         * 問題：
         *   - flag 的讀寫不是原子操作
         *   - reader 可能在寫入過程中讀取數據
         *   - 可能導致數據不一致
         *
         * 正確做法：
         *   - 使用 System V 信號量（sem_get/sem_op）
         *   - 使用 POSIX 信號量（sem_open/sem_wait/sem_post）
         *   - 或使用互斥鎖配合共享內存
         *
         * 本例僅用於演示基本共享內存操作，實際應用必須添加同步機制！
         */
        shared_mem->flag = 0;  // 標記為正在寫入（⚠️ 不安全！）
        shared_mem->counter++;
        strncpy(shared_mem->message, input, sizeof(shared_mem->message) - 1);
        shared_mem->message[sizeof(shared_mem->message) - 1] = '\0';  // 確保 null 終止
        shared_mem->flag = 1;  // 標記為可讀（⚠️ 不安全！）

        printf("  → 已寫入共享內存 (計數: %d)\n\n", shared_mem->counter);
    }

    /*
     * shmdt() - 分離共享內存
     *
     * int shmdt(const void *shmaddr);
     *
     * 注意：
     *   - 分離不會刪除共享內存
     *   - 只是取消映射
     */
    if (shmdt(shared_mem) == -1) {
        perror("shmdt failed");
    } else {
        printf("\n[分離] 已分離共享內存\n");
    }

    /*
     * shmctl() - 控制共享內存
     *
     * int shmctl(int shmid, int cmd, struct shmid_ds *buf);
     *
     * cmd 選項：
     *   IPC_STAT: 獲取共享內存狀態
     *   IPC_SET: 設置共享內存屬性
     *   IPC_RMID: 刪除共享內存
     */
    printf("[刪除] 正在刪除共享內存...\n");

    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl IPC_RMID failed");
    } else {
        printf("[刪除] 共享內存已標記刪除\n");
        printf("[說明] 當所有進程分離後，共享內存將被銷毀\n");
    }

    printf("\n程序結束\n");
    return 0;
}

/*
 * 知識點總結：
 *
 * 1. 共享內存的優勢：
 *    - 最快的 IPC 方式（直接訪問內存）
 *    - 不需要數據複製
 *    - 適合大量數據傳輸
 *
 * 2. IPC Key 的生成：
 *    - 手動指定：0x1234
 *    - ftok()：根據文件路徑生成
 *      key_t key = ftok("/tmp/myfile", 'A');
 *
 * 3. 生命週期：
 *    - 創建後持久存在
 *    - 不會隨進程結束而消失
 *    - 需要顯式調用 shmctl(IPC_RMID) 刪除
 *
 * 4. 同步問題：
 *    - 共享內存本身不提供同步機制
 *    - 需要結合信號量、互斥鎖等
 *    - 本例使用簡單的 flag 標誌（不安全）
 *
 * 5. 查看系統共享內存：
 *    ipcs -m          # 查看所有共享內存
 *    ipcs -m -i <id>  # 查看特定共享內存詳情
 *    ipcrm -m <id>    # 刪除共享內存
 *
 * 6. 權限：
 *    - 0666：所有用戶可讀可寫
 *    - 0644：所有者可讀寫，其他用戶只讀
 *    - 0600：僅所有者可讀寫
 *
 * 7. 限制：
 *    查看系統限制：
 *    cat /proc/sys/kernel/shmmax  # 最大共享內存段大小
 *    cat /proc/sys/kernel/shmall  # 所有共享內存總大小
 *    cat /proc/sys/kernel/shmmni  # 最大共享內存段數量
 */
