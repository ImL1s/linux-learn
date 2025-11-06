/*
 * 檔案名稱: producer_consumer.c
 * 功能說明: 使用條件變量解決生產者-消費者問題
 *
 * 知識點:
 *   1. 經典的生產者-消費者問題
 *   2. 使用條件變量實現同步
 *   3. 有界緩衝區管理
 *   4. 多生產者多消費者場景
 *   5. 兩個條件變量協同工作
 *
 * 編譯方式: gcc -o producer_consumer producer_consumer.c -pthread
 * 執行方式: ./producer_consumer
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#define BUFFER_SIZE 5
#define PRODUCER_COUNT 2
#define CONSUMER_COUNT 2
#define ITEMS_PER_PRODUCER 10

/*
 * 循環緩衝區結構
 */
typedef struct {
    int buffer[BUFFER_SIZE];
    int in;    // 生產者放入位置
    int out;   // 消費者取出位置
    int count; // 當前緩衝區中的項目數

    pthread_mutex_t mutex;      // 保護緩衝區
    pthread_cond_t not_full;    // 緩衝區未滿條件
    pthread_cond_t not_empty;   // 緩衝區非空條件
} bounded_buffer_t;

bounded_buffer_t buffer = {
    .in = 0,
    .out = 0,
    .count = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .not_full = PTHREAD_COND_INITIALIZER,
    .not_empty = PTHREAD_COND_INITIALIZER
};

int total_produced = 0;
int total_consumed = 0;

/*
 * 生產者線程
 */
void* producer(void* arg)
{
    int producer_id = *(int*)arg;
    int produced_count = 0;

    printf("[生產者 %d] 啟動\n", producer_id);

    for (int i = 0; i < ITEMS_PER_PRODUCER; i++) {
        // 準備要生產的項目
        int item = producer_id * 100 + i;

        /*
         * 生產者邏輯：
         * 1. 鎖定 mutex
         * 2. 檢查緩衝區是否已滿
         * 3. 如果滿了，等待 not_full 條件變量
         * 4. 將項目放入緩衝區
         * 5. 發送 not_empty 信號
         * 6. 解鎖 mutex
         */
        pthread_mutex_lock(&buffer.mutex);

        // 等待緩衝區有空位
        while (buffer.count == BUFFER_SIZE) {
            printf("[生產者 %d] 緩衝區已滿 (%d/%d)，等待...\n",
                   producer_id, buffer.count, BUFFER_SIZE);
            pthread_cond_wait(&buffer.not_full, &buffer.mutex);
        }

        // 放入緩衝區
        buffer.buffer[buffer.in] = item;
        buffer.in = (buffer.in + 1) % BUFFER_SIZE;
        buffer.count++;
        produced_count++;
        total_produced++;

        printf("[生產者 %d] 生產項目 %d，緩衝區 %d/%d（總共生產 %d）\n",
               producer_id, item, buffer.count, BUFFER_SIZE, produced_count);

        // 通知消費者：緩衝區非空
        pthread_cond_signal(&buffer.not_empty);

        pthread_mutex_unlock(&buffer.mutex);

        // 模擬生產耗時
        usleep(rand() % 500000);
    }

    printf("[生產者 %d] 完成，共生產 %d 個項目\n", producer_id, produced_count);
    return NULL;
}

/*
 * 消費者線程
 */
void* consumer(void* arg)
{
    int consumer_id = *(int*)arg;
    int consumed_count = 0;
    int expected_total = PRODUCER_COUNT * ITEMS_PER_PRODUCER;

    printf("[消費者 %d] 啟動\n", consumer_id);

    /*
     * 消費者邏輯：
     * 1. 鎖定 mutex
     * 2. 檢查緩衝區是否為空 且 是否還有項目要消費
     * 3. 如果為空，等待 not_empty 條件變量
     * 4. 從緩衝區取出項目
     * 5. 發送 not_full 信號
     * 6. 解鎖 mutex
     */
    while (1) {
        pthread_mutex_lock(&buffer.mutex);

        /*
         * 退出條件：
         * - 緩衝區為空 AND
         * - 所有項目都已被消費
         */
        while (buffer.count == 0 && total_consumed < expected_total) {
            printf("[消費者 %d] 緩衝區為空，等待...\n", consumer_id);
            pthread_cond_wait(&buffer.not_empty, &buffer.mutex);
        }

        // 檢查是否應該退出
        if (buffer.count == 0 && total_consumed >= expected_total) {
            pthread_mutex_unlock(&buffer.mutex);
            break;
        }

        // 從緩衝區取出
        int item = buffer.buffer[buffer.out];
        buffer.out = (buffer.out + 1) % BUFFER_SIZE;
        buffer.count--;
        consumed_count++;
        total_consumed++;

        printf("[消費者 %d] 消費項目 %d，緩衝區 %d/%d（總共消費 %d/%d）\n",
               consumer_id, item, buffer.count, BUFFER_SIZE,
               total_consumed, expected_total);

        // 通知生產者：緩衝區未滿
        pthread_cond_signal(&buffer.not_full);

        pthread_mutex_unlock(&buffer.mutex);

        // 模擬消費耗時
        usleep(rand() % 700000);
    }

    printf("[消費者 %d] 完成，共消費 %d 個項目\n", consumer_id, consumed_count);
    return NULL;
}

/*
 * 打印緩衝區狀態
 */
void print_buffer_status(void)
{
    printf("\n====== 最終緩衝區狀態 ======\n");
    printf("緩衝區大小: %d\n", BUFFER_SIZE);
    printf("當前項目數: %d\n", buffer.count);
    printf("生產位置 (in): %d\n", buffer.in);
    printf("消費位置 (out): %d\n", buffer.out);
    printf("總共生產: %d\n", total_produced);
    printf("總共消費: %d\n", total_consumed);

    if (total_produced == total_consumed) {
        printf("✓ 生產和消費數量一致！\n");
    } else {
        printf("✗ 數量不一致，可能有錯誤\n");
    }
}

int main(void)
{
    pthread_t producers[PRODUCER_COUNT];
    pthread_t consumers[CONSUMER_COUNT];
    int producer_ids[PRODUCER_COUNT];
    int consumer_ids[CONSUMER_COUNT];

    printf("====== 生產者-消費者問題演示（條件變量實現）======\n\n");
    printf("配置：\n");
    printf("  緩衝區大小: %d\n", BUFFER_SIZE);
    printf("  生產者數量: %d\n", PRODUCER_COUNT);
    printf("  消費者數量: %d\n", CONSUMER_COUNT);
    printf("  每個生產者生產: %d 個項目\n", ITEMS_PER_PRODUCER);
    printf("  預期總項目數: %d\n\n", PRODUCER_COUNT * ITEMS_PER_PRODUCER);

    // 設置隨機種子
    srand(time(NULL));

    // 創建生產者線程
    printf("創建生產者線程...\n");
    for (int i = 0; i < PRODUCER_COUNT; i++) {
        producer_ids[i] = i + 1;
        pthread_create(&producers[i], NULL, producer, &producer_ids[i]);
    }

    // 創建消費者線程
    printf("創建消費者線程...\n\n");
    for (int i = 0; i < CONSUMER_COUNT; i++) {
        consumer_ids[i] = i + 1;
        pthread_create(&consumers[i], NULL, consumer, &consumer_ids[i]);
    }

    // 等待所有生產者完成
    for (int i = 0; i < PRODUCER_COUNT; i++) {
        pthread_join(producers[i], NULL);
    }
    printf("\n所有生產者已完成\n\n");

    // 等待所有消費者完成
    for (int i = 0; i < CONSUMER_COUNT; i++) {
        pthread_join(consumers[i], NULL);
    }
    printf("\n所有消費者已完成\n");

    // 打印最終狀態
    print_buffer_status();

    // 清理資源
    pthread_mutex_destroy(&buffer.mutex);
    pthread_cond_destroy(&buffer.not_full);
    pthread_cond_destroy(&buffer.not_empty);

    printf("\n程序結束\n");
    return 0;
}

/*
 * 知識點詳解：
 *
 * 1. 生產者-消費者問題：
 *    - 經典的同步問題
 *    - 生產者生產數據放入緩衝區
 *    - 消費者從緩衝區取出數據
 *    - 緩衝區有界（固定大小）
 *
 * 2. 需要解決的問題：
 *    - 緩衝區滿時，生產者必須等待
 *    - 緩衝區空時，消費者必須等待
 *    - 多個生產者/消費者併發訪問，需要互斥
 *    - 避免忙等待，提高效率
 *
 * 3. 條件變量方案：
 *    需要：
 *    - 1 個 mutex：保護緩衝區
 *    - 2 個條件變量：
 *      • not_full：緩衝區未滿（生產者等待）
 *      • not_empty：緩衝區非空（消費者等待）
 *
 * 4. 為什麼需要兩個條件變量？
 *    - 生產者和消費者等待的條件不同
 *    - 生產者：等待 "非滿" 條件
 *    - 消費者：等待 "非空" 條件
 *    - 分開可以精確喚醒，避免虛假喚醒
 *
 * 5. 循環緩衝區：
 *    - 使用數組實現
 *    - in 指針：下一個生產位置
 *    - out 指針：下一個消費位置
 *    - 取模運算實現循環：(index + 1) % SIZE
 *
 * 6. 生產者流程：
 *    pthread_mutex_lock(&mutex);
 *    while (count == BUFFER_SIZE) {        // 緩衝區滿
 *        pthread_cond_wait(&not_full, &mutex);
 *    }
 *    // 放入項目
 *    buffer[in] = item;
 *    in = (in + 1) % SIZE;
 *    count++;
 *    pthread_cond_signal(&not_empty);      // 通知消費者
 *    pthread_mutex_unlock(&mutex);
 *
 * 7. 消費者流程：
 *    pthread_mutex_lock(&mutex);
 *    while (count == 0) {                  // 緩衝區空
 *        pthread_cond_wait(&not_empty, &mutex);
 *    }
 *    // 取出項目
 *    item = buffer[out];
 *    out = (out + 1) % SIZE;
 *    count--;
 *    pthread_cond_signal(&not_full);       // 通知生產者
 *    pthread_mutex_unlock(&mutex);
 *
 * 8. 與信號量方案的對比：
 *    信號量方案：
 *      sem_t empty;  // 空槽位數量
 *      sem_t full;   // 滿槽位數量
 *      sem_t mutex;  // 互斥鎖
 *
 *    條件變量方案：
 *      pthread_mutex_t mutex;
 *      pthread_cond_t not_full;
 *      pthread_cond_t not_empty;
 *
 *    條件變量優勢：
 *      ✓ 更靈活，可以表達複雜條件
 *      ✓ POSIX 標準，可移植性好
 *      ✓ 代碼邏輯更清晰
 *
 *    信號量優勢：
 *      ✓ 更簡潔，代碼更短
 *      ✓ 自動計數，不需要手動維護 count
 *
 * 9. 多生產者多消費者：
 *    - 使用 while 循環檢查條件（必須）
 *    - 避免虛假喚醒
 *    - 多個生產者可能同時被喚醒
 *    - 第一個生產者消費完後，其他的必須重新檢查
 *
 * 10. 常見錯誤：
 *     - 用 if 代替 while（虛假喚醒問題）
 *     - 忘記發送信號（導致死鎖）
 *     - 在臨界區外發送信號（雖然可行，但不推薦）
 *     - count 計數錯誤
 *     - 忘記取模運算（緩衝區溢出）
 *
 * 11. 性能優化：
 *     - 減小臨界區範圍
 *     - 批量生產/消費
 *     - 使用 signal 而非 broadcast
 *     - 調整緩衝區大小
 *
 * 12. 實際應用：
 *     - 線程池任務隊列
 *     - 消息隊列
 *     - 日誌系統
 *     - 網絡數據包處理
 *     - 多媒體流處理
 */
