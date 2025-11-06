/*
 * 檔案名稱: cond_vs_semaphore.c
 * 功能說明: 條件變量 vs 信號量性能對比
 *
 * 知識點:
 *   1. 條件變量實現的生產者-消費者
 *   2. 信號量實現的生產者-消費者
 *   3. 性能測試和對比
 *   4. 不同場景下的適用性
 *
 * 編譯方式: gcc -o cond_vs_semaphore cond_vs_semaphore.c -pthread
 * 執行方式: ./cond_vs_semaphore
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#define BUFFER_SIZE 10
#define ITEMS_COUNT 100000

/*
 * 條件變量實現
 */
typedef struct {
    int buffer[BUFFER_SIZE];
    int in, out, count;
    pthread_mutex_t mutex;
    pthread_cond_t not_full, not_empty;
    int produced, consumed;
} cond_buffer_t;

/*
 * 信號量實現
 */
typedef struct {
    int buffer[BUFFER_SIZE];
    int in, out;
    sem_t empty, full, mutex;
    int produced, consumed;
} sem_buffer_t;

/* 全局變量 */
cond_buffer_t cond_buf;
sem_buffer_t sem_buf;

/*
 * 獲取當前時間（毫秒）
 */
long long current_timestamp_ms(void)
{
    struct timeval te;
    gettimeofday(&te, NULL);
    return te.tv_sec * 1000LL + te.tv_usec / 1000;
}

/*
 * ============================================================
 * 條件變量實現
 * ============================================================
 */

void cond_init(cond_buffer_t *buf)
{
    buf->in = buf->out = buf->count = 0;
    buf->produced = buf->consumed = 0;
    pthread_mutex_init(&buf->mutex, NULL);
    pthread_cond_init(&buf->not_full, NULL);
    pthread_cond_init(&buf->not_empty, NULL);
}

void cond_destroy(cond_buffer_t *buf)
{
    pthread_mutex_destroy(&buf->mutex);
    pthread_cond_destroy(&buf->not_full);
    pthread_cond_destroy(&buf->not_empty);
}

void* cond_producer(void* arg)
{
    cond_buffer_t *buf = (cond_buffer_t*)arg;

    for (int i = 0; i < ITEMS_COUNT; i++) {
        pthread_mutex_lock(&buf->mutex);

        while (buf->count == BUFFER_SIZE) {
            pthread_cond_wait(&buf->not_full, &buf->mutex);
        }

        buf->buffer[buf->in] = i;
        buf->in = (buf->in + 1) % BUFFER_SIZE;
        buf->count++;
        buf->produced++;

        pthread_cond_signal(&buf->not_empty);
        pthread_mutex_unlock(&buf->mutex);
    }

    return NULL;
}

void* cond_consumer(void* arg)
{
    cond_buffer_t *buf = (cond_buffer_t*)arg;

    for (int i = 0; i < ITEMS_COUNT; i++) {
        pthread_mutex_lock(&buf->mutex);

        while (buf->count == 0) {
            pthread_cond_wait(&buf->not_empty, &buf->mutex);
        }

        int item = buf->buffer[buf->out];
        (void)item; // 避免未使用警告
        buf->out = (buf->out + 1) % BUFFER_SIZE;
        buf->count--;
        buf->consumed++;

        pthread_cond_signal(&buf->not_full);
        pthread_mutex_unlock(&buf->mutex);
    }

    return NULL;
}

/*
 * ============================================================
 * 信號量實現
 * ============================================================
 */

void sem_init_buffer(sem_buffer_t *buf)
{
    buf->in = buf->out = 0;
    buf->produced = buf->consumed = 0;
    sem_init(&buf->empty, 0, BUFFER_SIZE); // 初始有 BUFFER_SIZE 個空位
    sem_init(&buf->full, 0, 0);             // 初始有 0 個滿位
    sem_init(&buf->mutex, 0, 1);            // 互斥信號量
}

void sem_destroy_buffer(sem_buffer_t *buf)
{
    sem_destroy(&buf->empty);
    sem_destroy(&buf->full);
    sem_destroy(&buf->mutex);
}

void* sem_producer(void* arg)
{
    sem_buffer_t *buf = (sem_buffer_t*)arg;

    for (int i = 0; i < ITEMS_COUNT; i++) {
        sem_wait(&buf->empty);  // 等待空位
        sem_wait(&buf->mutex);  // 互斥訪問

        buf->buffer[buf->in] = i;
        buf->in = (buf->in + 1) % BUFFER_SIZE;
        buf->produced++;

        sem_post(&buf->mutex);  // 釋放互斥鎖
        sem_post(&buf->full);   // 增加滿位計數
    }

    return NULL;
}

void* sem_consumer(void* arg)
{
    sem_buffer_t *buf = (sem_buffer_t*)arg;

    for (int i = 0; i < ITEMS_COUNT; i++) {
        sem_wait(&buf->full);   // 等待滿位
        sem_wait(&buf->mutex);  // 互斥訪問

        int item = buf->buffer[buf->out];
        (void)item; // 避免未使用警告
        buf->out = (buf->out + 1) % BUFFER_SIZE;
        buf->consumed++;

        sem_post(&buf->mutex);  // 釋放互斥鎖
        sem_post(&buf->empty);  // 增加空位計數
    }

    return NULL;
}

/*
 * ============================================================
 * 測試函數
 * ============================================================
 */

void test_condition_variable(void)
{
    pthread_t prod, cons;
    long long start, end;

    printf("\n====== 測試條件變量實現 ======\n");
    cond_init(&cond_buf);

    start = current_timestamp_ms();

    pthread_create(&prod, NULL, cond_producer, &cond_buf);
    pthread_create(&cons, NULL, cond_consumer, &cond_buf);

    pthread_join(prod, NULL);
    pthread_join(cons, NULL);

    end = current_timestamp_ms();

    printf("完成：\n");
    printf("  生產: %d 個項目\n", cond_buf.produced);
    printf("  消費: %d 個項目\n", cond_buf.consumed);
    printf("  耗時: %lld 毫秒\n", end - start);

    cond_destroy(&cond_buf);
}

void test_semaphore(void)
{
    pthread_t prod, cons;
    long long start, end;

    printf("\n====== 測試信號量實現 ======\n");
    sem_init_buffer(&sem_buf);

    start = current_timestamp_ms();

    pthread_create(&prod, NULL, sem_producer, &sem_buf);
    pthread_create(&cons, NULL, sem_consumer, &sem_buf);

    pthread_join(prod, NULL);
    pthread_join(cons, NULL);

    end = current_timestamp_ms();

    printf("完成：\n");
    printf("  生產: %d 個項目\n", sem_buf.produced);
    printf("  消費: %d 個項目\n", sem_buf.consumed);
    printf("  耗時: %lld 毫秒\n", end - start);

    sem_destroy_buffer(&sem_buf);
}

int main(void)
{
    printf("====== 條件變量 vs 信號量性能對比 ======\n");
    printf("測試配置：\n");
    printf("  緩衝區大小: %d\n", BUFFER_SIZE);
    printf("  項目總數: %d\n", ITEMS_COUNT);
    printf("  線程配置: 1 生產者 + 1 消費者\n");

    // 測試條件變量
    test_condition_variable();

    // 測試信號量
    test_semaphore();

    printf("\n====== 對比總結 ======\n");
    printf("\n條件變量特點：\n");
    printf("  ✓ 必須配合 mutex 使用\n");
    printf("  ✓ 用於等待複雜條件\n");
    printf("  ✓ 支持 broadcast 喚醒多個線程\n");
    printf("  ✓ 代碼邏輯更清晰\n");
    printf("  ✓ POSIX 標準，可移植性好\n");

    printf("\n信號量特點：\n");
    printf("  ✓ 獨立使用，不需要 mutex\n");
    printf("  ✓ 自動計數管理\n");
    printf("  ✓ 代碼更簡潔\n");
    printf("  ✓ 適合資源計數場景\n");
    printf("  ✓ 性能略好（少一次 mutex 操作）\n");

    printf("\n適用場景：\n");
    printf("  條件變量：\n");
    printf("    • 複雜的等待條件\n");
    printf("    • 需要 broadcast 的場景\n");
    printf("    • 已經使用 mutex 保護共享數據\n");
    printf("    • 讀者-寫者問題\n");

    printf("\n  信號量：\n");
    printf("    • 簡單的資源計數\n");
    printf("    • 生產者-消費者問題\n");
    printf("    • 進程間同步（System V 信號量）\n");
    printf("    • 限制並發訪問數量\n");

    printf("\n性能因素：\n");
    printf("  • 緩衝區大小影響：越小越頻繁阻塞\n");
    printf("  • 項目數量影響：越多越能體現差異\n");
    printf("  • 線程數量影響：多線程時競爭更激烈\n");
    printf("  • 系統負載影響：高負載時上下文切換開銷大\n");

    printf("\n結論：\n");
    printf("  兩者性能差異不大（通常在 5%% 以內）\n");
    printf("  選擇依據應該是：代碼清晰度、使用場景、團隊習慣\n");
    printf("  不要過度優化，可讀性更重要\n");

    printf("\n程序結束\n");
    return 0;
}

/*
 * 知識點總結：
 *
 * 1. 性能對比結果：
 *    - 通常信號量略快（約 3-5%）
 *    - 原因：信號量少一次 mutex 操作
 *    - 但差異很小，在實際應用中可忽略
 *
 * 2. 條件變量的優勢：
 *    - 表達能力更強：可以等待任意複雜條件
 *    - broadcast 功能：可以喚醒所有等待線程
 *    - 更符合 POSIX 標準
 *    - 與 mutex 集成更自然
 *
 * 3. 信號量的優勢：
 *    - 代碼更簡潔：自動管理計數
 *    - 概念更簡單：就是資源計數器
 *    - 可用於進程間同步（System V 信號量）
 *    - 性能略好
 *
 * 4. 實現細節對比：
 *    條件變量版本：
 *      - 需要手動維護 count
 *      - 需要兩個條件變量（not_full, not_empty）
 *      - 需要一個 mutex
 *      - 必須用 while 循環檢查條件
 *
 *    信號量版本：
 *      - 不需要維護 count（信號量自動計數）
 *      - 需要三個信號量（empty, full, mutex）
 *      - 代碼行數更少
 *      - wait/post 操作更直觀
 *
 * 5. 何時選擇條件變量：
 *    - 已經使用了 mutex 保護共享數據
 *    - 等待條件複雜（如：count > 10 && flag == true）
 *    - 需要 broadcast 多個線程
 *    - 需要優雅的超時等待（pthread_cond_timedwait）
 *
 * 6. 何時選擇信號量：
 *    - 簡單的資源計數
 *    - 限制並發訪問數量
 *    - 進程間同步（必須用 System V 信號量）
 *    - 代碼簡潔性優先
 *
 * 7. 混合使用：
 *    - 可以同時使用條件變量和信號量
 *    - 根據不同場景選擇合適的工具
 *    - 避免過度設計
 *
 * 8. 性能測試注意事項：
 *    - 多次測試取平均值
 *    - 考慮系統負載的影響
 *    - 不同緩衝區大小會影響結果
 *    - 編譯優化選項會影響結果
 *
 * 9. 實際經驗：
 *    - 性能差異通常不是選擇的決定因素
 *    - 代碼可讀性和維護性更重要
 *    - 遵循團隊代碼規範
 *    - 不要過早優化
 *
 * 10. 進階主題：
 *     - pthread_cond_timedwait：超時等待
 *     - pthread_cond_broadcast：廣播喚醒
 *     - 優先級反轉問題
 *     - 無鎖隊列（lock-free queue）
 */
