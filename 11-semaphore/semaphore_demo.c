/*
 * 檔案名稱: semaphore_demo.c
 * 功能說明: 信號量 (Semaphore) 完整演示 - 生產者消費者問題
 *
 * 知識點:
 *   1. POSIX 信號量 (sem_t)
 *   2. System V 信號量
 *   3. 生產者-消費者問題
 *   4. 互斥與同步
 *   5. 信號量的 P/V 操作
 *
 * 應用場景:
 *   - 進程/線程間同步
 *   - 資源計數和管理
 *   - 生產者-消費者模型
 *   - 讀者-寫者問題
 *   - 哲學家就餐問題
 *
 * 編譯方式: gcc -o semaphore_demo semaphore_demo.c -pthread
 * 執行方式: ./semaphore_demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>

#define BUFFER_SIZE 10
#define NUM_PRODUCERS 2
#define NUM_CONSUMERS 3
#define ITEMS_PER_PRODUCER 20

/*
 * 共享緩衝區結構
 */
typedef struct {
    int buffer[BUFFER_SIZE];
    int in;   // 生產者放置位置
    int out;  // 消費者取出位置
} shared_buffer_t;

shared_buffer_t shared_buffer = {.in = 0, .out = 0};

/*
 * 信號量
 *
 * empty: 空閒槽位數量（初始為 BUFFER_SIZE）
 * full:  已用槽位數量（初始為 0）
 * mutex: 互斥鎖（初始為 1）
 */
sem_t empty;   // 計數信號量：表示空槽位
sem_t full;    // 計數信號量：表示滿槽位
sem_t mutex;   // 二元信號量：保護臨界區

int total_produced = 0;
int total_consumed = 0;

/*
 * 生產者線程
 */
void* producer(void* arg)
{
    int id = *(int*)arg;
    int item;

    printf("[生產者 %d] 啟動\n", id);

    for (int i = 0; i < ITEMS_PER_PRODUCER; i++) {
        // 生產一個項目
        item = rand() % 1000 + 1;

        /*
         * P 操作 (wait): sem_wait()
         * V 操作 (signal): sem_post()
         *
         * 生產者的同步邏輯：
         * 1. 等待空閒槽位（P(empty)）
         * 2. 獲取互斥鎖（P(mutex)）
         * 3. 放入緩衝區
         * 4. 釋放互斥鎖（V(mutex)）
         * 5. 增加滿槽位（V(full)）
         */

        // 等待空閒槽位
        sem_wait(&empty);

        // 進入臨界區（獲取互斥鎖）
        sem_wait(&mutex);

        // === 臨界區開始 ===
        shared_buffer.buffer[shared_buffer.in] = item;
        printf("[生產者 %d] 生產 item=%d, 位置=%d\n",
               id, item, shared_buffer.in);
        shared_buffer.in = (shared_buffer.in + 1) % BUFFER_SIZE;
        total_produced++;
        // === 臨界區結束 ===

        // 釋放互斥鎖
        sem_post(&mutex);

        // 增加滿槽位計數
        sem_post(&full);

        // 模擬生產時間
        usleep(rand() % 100000);
    }

    printf("[生產者 %d] 完成，共生產 %d 個項目\n", id, ITEMS_PER_PRODUCER);
    return NULL;
}

/*
 * 消費者線程
 */
void* consumer(void* arg)
{
    int id = *(int*)arg;
    int item;
    int count = 0;

    printf("[消費者 %d] 啟動\n", id);

    while (1) {
        /*
         * 消費者的同步邏輯：
         * 1. 等待滿槽位（P(full)）
         * 2. 獲取互斥鎖（P(mutex)）
         * 3. 從緩衝區取出
         * 4. 釋放互斥鎖（V(mutex)）
         * 5. 增加空槽位（V(empty)）
         */

        // 等待滿槽位
        if (sem_wait(&full) != 0) {
            break;
        }

        // 進入臨界區
        sem_wait(&mutex);

        // 檢查是否完成
        if (total_consumed >= NUM_PRODUCERS * ITEMS_PER_PRODUCER) {
            sem_post(&mutex);
            sem_post(&full);  // 放回信號量，讓其他消費者退出
            break;
        }

        // === 臨界區開始 ===
        item = shared_buffer.buffer[shared_buffer.out];
        printf("[消費者 %d] 消費 item=%d, 位置=%d\n",
               id, item, shared_buffer.out);
        shared_buffer.out = (shared_buffer.out + 1) % BUFFER_SIZE;
        total_consumed++;
        count++;
        // === 臨界區結束 ===

        // 釋放互斥鎖
        sem_post(&mutex);

        // 增加空槽位計數
        sem_post(&empty);

        // 模擬消費時間
        usleep(rand() % 150000);
    }

    printf("[消費者 %d] 完成，共消費 %d 個項目\n", id, count);
    return NULL;
}

int main(void)
{
    pthread_t producers[NUM_PRODUCERS];
    pthread_t consumers[NUM_CONSUMERS];
    int producer_ids[NUM_PRODUCERS];
    int consumer_ids[NUM_CONSUMERS];

    printf("====== 生產者-消費者問題演示 ======\n\n");
    printf("配置：\n");
    printf("  緩衝區大小: %d\n", BUFFER_SIZE);
    printf("  生產者數量: %d\n", NUM_PRODUCERS);
    printf("  消費者數量: %d\n", NUM_CONSUMERS);
    printf("  每個生產者生產: %d 個項目\n", ITEMS_PER_PRODUCER);
    printf("  總項目數: %d\n\n", NUM_PRODUCERS * ITEMS_PER_PRODUCER);

    srand(time(NULL));

    /*
     * 初始化信號量
     *
     * int sem_init(sem_t *sem, int pshared, unsigned int value);
     *
     * pshared:
     *   0 - 線程間共享（本例）
     *   非0 - 進程間共享（需要共享內存）
     *
     * value: 初始值
     */

    // 空閒槽位：初始為緩衝區大小
    if (sem_init(&empty, 0, BUFFER_SIZE) != 0) {
        perror("sem_init empty failed");
        return EXIT_FAILURE;
    }

    // 滿槽位：初始為 0
    if (sem_init(&full, 0, 0) != 0) {
        perror("sem_init full failed");
        sem_destroy(&empty);
        return EXIT_FAILURE;
    }

    // 互斥鎖：初始為 1（二元信號量）
    if (sem_init(&mutex, 0, 1) != 0) {
        perror("sem_init mutex failed");
        sem_destroy(&empty);
        sem_destroy(&full);
        return EXIT_FAILURE;
    }

    printf("信號量初始化完成\n\n");

    /*
     * 創建生產者線程
     */
    printf("創建生產者線程...\n");
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        producer_ids[i] = i + 1;
        if (pthread_create(&producers[i], NULL, producer, &producer_ids[i]) != 0) {
            perror("pthread_create producer failed");
            return EXIT_FAILURE;
        }
    }

    /*
     * 創建消費者線程
     */
    printf("創建消費者線程...\n\n");
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        consumer_ids[i] = i + 1;
        if (pthread_create(&consumers[i], NULL, consumer, &consumer_ids[i]) != 0) {
            perror("pthread_create consumer failed");
            return EXIT_FAILURE;
        }
    }

    printf("====== 開始生產和消費 ======\n\n");

    /*
     * 等待所有生產者完成
     */
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        pthread_join(producers[i], NULL);
    }

    printf("\n所有生產者已完成\n");

    /*
     * 等待所有消費者完成
     */
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        pthread_join(consumers[i], NULL);
    }

    printf("所有消費者已完成\n\n");

    /*
     * 統計結果
     */
    printf("====== 統計結果 ======\n");
    printf("總共生產: %d 個項目\n", total_produced);
    printf("總共消費: %d 個項目\n", total_consumed);

    if (total_produced == total_consumed) {
        printf("✅ 生產和消費數量匹配，測試通過！\n");
    } else {
        printf("❌ 數量不匹配，存在錯誤！\n");
    }

    /*
     * 清理信號量
     */
    sem_destroy(&empty);
    sem_destroy(&full);
    sem_destroy(&mutex);

    printf("\n程序結束\n");
    return EXIT_SUCCESS;
}

/*
 * 信號量知識點總結：
 *
 * 1. 信號量的概念：
 *    - 計數器，用於控制對共享資源的訪問
 *    - P 操作 (wait/down)：計數器減 1，若結果 < 0 則阻塞
 *    - V 操作 (signal/up)：計數器加 1，若有等待進程則喚醒
 *
 * 2. 信號量的類型：
 *    二元信號量 (Binary Semaphore):
 *    - 值只能是 0 或 1
 *    - 類似於互斥鎖 (Mutex)
 *    - 用於互斥訪問
 *
 *    計數信號量 (Counting Semaphore):
 *    - 值可以是任意非負整數
 *    - 用於資源計數
 *    - 用於同步
 *
 * 3. POSIX 信號量 vs System V 信號量：
 *
 *    特性          POSIX             System V
 *    ------------------------------------------------
 *    頭文件        semaphore.h       sys/sem.h
 *    初始化        sem_init()        semget()
 *    P 操作        sem_wait()        semop()
 *    V 操作        sem_post()        semop()
 *    銷毀          sem_destroy()     semctl()
 *    簡單性        簡單              複雜
 *    功能          基本              強大（原子操作集）
 *
 * 4. POSIX 信號量 API：
 *
 *    sem_init()    - 初始化信號量
 *    sem_wait()    - P 操作（阻塞）
 *    sem_trywait() - 非阻塞 P 操作
 *    sem_timedwait() - 超時 P 操作
 *    sem_post()    - V 操作
 *    sem_getvalue() - 獲取當前值
 *    sem_destroy() - 銷毀信號量
 *
 * 5. 生產者-消費者問題：
 *
 *    問題描述：
 *    - 生產者生成數據放入緩衝區
 *    - 消費者從緩衝區取出數據
 *    - 緩衝區大小有限
 *    - 需要同步和互斥
 *
 *    使用的信號量：
 *    - empty: 空閒槽位數（初始 = 緩衝區大小）
 *    - full: 已用槽位數（初始 = 0）
 *    - mutex: 互斥鎖（初始 = 1）
 *
 *    生產者邏輯：
 *    P(empty)   // 等待空槽位
 *    P(mutex)   // 獲取鎖
 *    [放入緩衝區]
 *    V(mutex)   // 釋放鎖
 *    V(full)    // 增加滿槽位
 *
 *    消費者邏輯：
 *    P(full)    // 等待滿槽位
 *    P(mutex)   // 獲取鎖
 *    [取出緩衝區]
 *    V(mutex)   // 釋放鎖
 *    V(empty)   // 增加空槽位
 *
 * 6. 信號量 vs 互斥鎖：
 *
 *    互斥鎖 (Mutex):
 *    - 所有權概念（誰鎖誰解鎖）
 *    - 只能用於互斥
 *    - 不能用於同步
 *
 *    信號量 (Semaphore):
 *    - 沒有所有權概念
 *    - 既可互斥也可同步
 *    - 更靈活但更容易出錯
 *
 * 7. 常見問題和陷阱：
 *
 *    死鎖：
 *    - P 操作順序錯誤
 *    - 例如：生產者先 P(mutex) 再 P(empty) → 死鎖
 *    - 正確：先 P(empty) 再 P(mutex)
 *
 *    忘記 V 操作：
 *    - 導致資源永久丟失
 *    - 其他進程/線程永久阻塞
 *
 *    P/V 不配對：
 *    - 每個 P 必須對應一個 V
 *    - 順序可能不同，但數量必須相等
 *
 * 8. 實際應用場景：
 *
 *    資源池管理：
 *    - 連接池（數據庫連接、HTTP 連接）
 *    - 線程池
 *    - 內存池
 *
 *    任務隊列：
 *    - 工作隊列
 *    - 消息隊列
 *    - 事件處理
 *
 *    同步控制：
 *    - 多階段任務協調
 *    - Barrier 同步
 *    - 階段同步
 *
 * 9. 高級用法：
 *
 *    多個資源類型：
 *    - 每種資源一個信號量
 *    - 按固定順序獲取，避免死鎖
 *
 *    優先級：
 *    - 高優先級先獲取資源
 *    - 需要多個隊列
 *
 *    超時機制：
 *    - sem_timedwait() 避免永久阻塞
 *    - 提高系統魯棒性
 *
 * 10. 性能考慮：
 *
 *     - 信號量操作涉及系統調用，有開銷
 *     - 減少臨界區大小
 *     - 避免頻繁 P/V 操作
 *     - 批量處理提高效率
 *
 * 11. 調試技巧：
 *
 *     sem_getvalue(&sem, &val) - 查看當前值
 *     printf 調試（但會影響時序）
 *     使用 helgrind（Valgrind 工具）
 *     DRD（Data Race Detector）
 *
 * 12. System V 信號量（進程間）：
 *
 *     // 創建
 *     int semid = semget(key, nsems, IPC_CREAT | 0666);
 *
 *     // 初始化
 *     semctl(semid, 0, SETVAL, 1);
 *
 *     // P 操作
 *     struct sembuf sb = {0, -1, 0};
 *     semop(semid, &sb, 1);
 *
 *     // V 操作
 *     sb.sem_op = 1;
 *     semop(semid, &sb, 1);
 *
 *     // 刪除
 *     semctl(semid, 0, IPC_RMID);
 */
