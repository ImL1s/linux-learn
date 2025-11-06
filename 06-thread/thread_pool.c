/*
 * 檔案名稱: thread_pool.c
 * 功能說明: 線程池實現 - 實用的並發編程組件
 *
 * 功能特性:
 *   1. 固定大小的線程池
 *   2. 任務隊列管理
 *   3. 優雅關閉
 *   4. 統計信息（任務數、活躍線程等）
 *   5. 可配置的隊列大小
 *
 * 應用場景:
 *   - Web 服務器的請求處理
 *   - 數據庫連接池
 *   - 並行計算任務
 *   - 異步 I/O 處理
 *
 * 編譯方式: gcc -o thread_pool thread_pool.c -pthread
 * 執行方式: ./thread_pool
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>

/*
 * 任務結構
 */
typedef struct task_s {
    void (*function)(void *arg);  // 任務函數指針
    void *arg;                     // 任務參數
    struct task_s *next;           // 下一個任務（鏈表）
} task_t;

/*
 * 任務隊列
 */
typedef struct {
    task_t *head;                  // 隊列頭
    task_t *tail;                  // 隊列尾
    int count;                     // 當前任務數
    int max_size;                  // 最大隊列大小
} task_queue_t;

/*
 * 線程池結構
 */
typedef struct {
    pthread_t *threads;            // 線程數組
    int thread_count;              // 線程數量

    task_queue_t queue;            // 任務隊列

    pthread_mutex_t lock;          // 保護隊列的互斥鎖
    pthread_cond_t notify;         // 通知工作線程的條件變量

    int shutdown;                  // 關閉標誌
    int started;                   // 已啟動的線程數

    // 統計信息
    int tasks_completed;           // 已完成任務數
    int tasks_submitted;           // 已提交任務數
    int active_threads;            // 活躍線程數
} thread_pool_t;

/*
 * 錯誤碼
 */
typedef enum {
    POOL_SUCCESS = 0,
    POOL_INVALID = -1,
    POOL_LOCK_FAILURE = -2,
    POOL_QUEUE_FULL = -3,
    POOL_SHUTDOWN = -4,
    POOL_THREAD_FAILURE = -5
} pool_error_t;

/*
 * 創建線程池
 *
 * 參數:
 *   thread_count: 線程數量
 *   queue_size: 任務隊列最大大小（0 表示無限制）
 *
 * 返回: 線程池指針，失敗返回 NULL
 */
thread_pool_t* thread_pool_create(int thread_count, int queue_size)
{
    if (thread_count <= 0 || thread_count > 1024) {
        fprintf(stderr, "錯誤: 線程數量必須在 1-1024 之間\n");
        return NULL;
    }

    thread_pool_t *pool = (thread_pool_t*)malloc(sizeof(thread_pool_t));
    if (pool == NULL) {
        perror("malloc");
        return NULL;
    }

    // 初始化
    memset(pool, 0, sizeof(thread_pool_t));
    pool->thread_count = thread_count;
    pool->queue.max_size = queue_size;

    // 分配線程數組
    pool->threads = (pthread_t*)malloc(sizeof(pthread_t) * thread_count);
    if (pool->threads == NULL) {
        perror("malloc");
        free(pool);
        return NULL;
    }

    // 初始化互斥鎖和條件變量
    if (pthread_mutex_init(&pool->lock, NULL) != 0) {
        free(pool->threads);
        free(pool);
        return NULL;
    }

    if (pthread_cond_init(&pool->notify, NULL) != 0) {
        pthread_mutex_destroy(&pool->lock);
        free(pool->threads);
        free(pool);
        return NULL;
    }

    return pool;
}

/*
 * 工作線程函數
 */
static void* thread_worker(void *arg)
{
    thread_pool_t *pool = (thread_pool_t*)arg;

    printf("[線程池] 工作線程 %lu 啟動\n", pthread_self());

    while (1) {
        pthread_mutex_lock(&pool->lock);

        // 等待任務或關閉信號
        while (pool->queue.count == 0 && !pool->shutdown) {
            pthread_cond_wait(&pool->notify, &pool->lock);
        }

        if (pool->shutdown) {
            pthread_mutex_unlock(&pool->lock);
            break;
        }

        // 取出任務
        task_t *task = pool->queue.head;
        if (task == NULL) {
            pthread_mutex_unlock(&pool->lock);
            continue;
        }

        pool->queue.head = task->next;
        if (pool->queue.head == NULL) {
            pool->queue.tail = NULL;
        }
        pool->queue.count--;
        pool->active_threads++;

        pthread_mutex_unlock(&pool->lock);

        // 執行任務
        (*(task->function))(task->arg);
        free(task);

        // 更新統計
        pthread_mutex_lock(&pool->lock);
        pool->active_threads--;
        pool->tasks_completed++;
        pthread_mutex_unlock(&pool->lock);
    }

    pthread_mutex_lock(&pool->lock);
    pool->started--;
    pthread_mutex_unlock(&pool->lock);

    printf("[線程池] 工作線程 %lu 退出\n", pthread_self());

    return NULL;
}

/*
 * 啟動線程池
 */
int thread_pool_start(thread_pool_t *pool)
{
    if (pool == NULL) {
        return POOL_INVALID;
    }

    for (int i = 0; i < pool->thread_count; i++) {
        if (pthread_create(&pool->threads[i], NULL, thread_worker, pool) != 0) {
            fprintf(stderr, "錯誤: 無法創建線程 %d\n", i);
            return POOL_THREAD_FAILURE;
        }
        pool->started++;
    }

    printf("[線程池] 成功啟動 %d 個工作線程\n", pool->thread_count);

    return POOL_SUCCESS;
}

/*
 * 提交任務到線程池
 *
 * 參數:
 *   pool: 線程池指針
 *   function: 任務函數
 *   arg: 任務參數
 *
 * 返回: 0 成功，負數表示錯誤
 */
int thread_pool_submit(thread_pool_t *pool, void (*function)(void *), void *arg)
{
    if (pool == NULL || function == NULL) {
        return POOL_INVALID;
    }

    pthread_mutex_lock(&pool->lock);

    if (pool->shutdown) {
        pthread_mutex_unlock(&pool->lock);
        return POOL_SHUTDOWN;
    }

    // 檢查隊列是否已滿
    if (pool->queue.max_size > 0 && pool->queue.count >= pool->queue.max_size) {
        pthread_mutex_unlock(&pool->lock);
        return POOL_QUEUE_FULL;
    }

    // 創建任務
    task_t *task = (task_t*)malloc(sizeof(task_t));
    if (task == NULL) {
        pthread_mutex_unlock(&pool->lock);
        return POOL_INVALID;
    }

    task->function = function;
    task->arg = arg;
    task->next = NULL;

    // 加入隊列
    if (pool->queue.tail == NULL) {
        pool->queue.head = task;
        pool->queue.tail = task;
    } else {
        pool->queue.tail->next = task;
        pool->queue.tail = task;
    }
    pool->queue.count++;
    pool->tasks_submitted++;

    // 通知工作線程
    pthread_cond_signal(&pool->notify);
    pthread_mutex_unlock(&pool->lock);

    return POOL_SUCCESS;
}

/*
 * 等待所有任務完成
 */
int thread_pool_wait(thread_pool_t *pool)
{
    if (pool == NULL) {
        return POOL_INVALID;
    }

    while (1) {
        pthread_mutex_lock(&pool->lock);
        int count = pool->queue.count;
        int active = pool->active_threads;
        pthread_mutex_unlock(&pool->lock);

        if (count == 0 && active == 0) {
            break;
        }

        usleep(10000);  // 10ms
    }

    return POOL_SUCCESS;
}

/*
 * 獲取統計信息
 */
void thread_pool_get_stats(thread_pool_t *pool, int *submitted, int *completed,
                           int *queued, int *active)
{
    if (pool == NULL) return;

    pthread_mutex_lock(&pool->lock);
    if (submitted) *submitted = pool->tasks_submitted;
    if (completed) *completed = pool->tasks_completed;
    if (queued) *queued = pool->queue.count;
    if (active) *active = pool->active_threads;
    pthread_mutex_unlock(&pool->lock);
}

/*
 * 打印統計信息
 */
void thread_pool_print_stats(thread_pool_t *pool)
{
    if (pool == NULL) return;

    int submitted, completed, queued, active;
    thread_pool_get_stats(pool, &submitted, &completed, &queued, &active);

    printf("\n===== 線程池統計 =====\n");
    printf("線程數量: %d\n", pool->thread_count);
    printf("已提交任務: %d\n", submitted);
    printf("已完成任務: %d\n", completed);
    printf("隊列中任務: %d\n", queued);
    printf("活躍線程: %d\n", active);
    printf("=====================\n\n");
}

/*
 * 銷毀線程池
 */
int thread_pool_destroy(thread_pool_t *pool)
{
    if (pool == NULL) {
        return POOL_INVALID;
    }

    pthread_mutex_lock(&pool->lock);

    if (pool->shutdown) {
        pthread_mutex_unlock(&pool->lock);
        return POOL_SHUTDOWN;
    }

    pool->shutdown = 1;

    // 通知所有線程
    pthread_cond_broadcast(&pool->notify);
    pthread_mutex_unlock(&pool->lock);

    // 等待所有線程退出
    for (int i = 0; i < pool->thread_count; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    // 清理任務隊列
    pthread_mutex_lock(&pool->lock);
    task_t *task = pool->queue.head;
    while (task != NULL) {
        task_t *next = task->next;
        free(task);
        task = next;
    }
    pthread_mutex_unlock(&pool->lock);

    // 銷毀互斥鎖和條件變量
    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->notify);

    // 釋放內存
    free(pool->threads);
    free(pool);

    printf("[線程池] 已銷毀\n");

    return POOL_SUCCESS;
}

/* ============================================================
 * 測試和演示代碼
 * ============================================================ */

/*
 * 示例任務 1: 簡單計算
 */
void task_compute(void *arg)
{
    int *num = (int*)arg;
    printf("[任務] 計算 %d 的平方: %d\n", *num, (*num) * (*num));
    usleep(100000);  // 模擬工作 100ms
    free(num);
}

/*
 * 示例任務 2: 模擬 I/O 操作
 */
void task_io(void *arg)
{
    char *filename = (char*)arg;
    printf("[任務] 處理文件: %s\n", filename);
    usleep(200000);  // 模擬 I/O 200ms
    free(filename);
}

/*
 * 示例任務 3: 批量數據處理
 */
typedef struct {
    int start;
    int end;
    int *sum;
    pthread_mutex_t *lock;
} batch_task_arg_t;

void task_batch_sum(void *arg)
{
    batch_task_arg_t *data = (batch_task_arg_t*)arg;
    int local_sum = 0;

    for (int i = data->start; i <= data->end; i++) {
        local_sum += i;
    }

    pthread_mutex_lock(data->lock);
    *data->sum += local_sum;
    pthread_mutex_unlock(data->lock);

    printf("[任務] 計算 %d 到 %d 的和: %d\n", data->start, data->end, local_sum);

    free(data);
}

/*
 * 演示 1: 基本使用
 */
void demo_basic(void)
{
    printf("\n========== 演示 1: 基本使用 ==========\n\n");

    // 創建線程池（4 個線程，隊列大小 10）
    thread_pool_t *pool = thread_pool_create(4, 10);
    if (pool == NULL) {
        fprintf(stderr, "無法創建線程池\n");
        return;
    }

    // 啟動線程池
    thread_pool_start(pool);

    // 提交 10 個計算任務
    for (int i = 1; i <= 10; i++) {
        int *num = malloc(sizeof(int));
        *num = i;

        if (thread_pool_submit(pool, task_compute, num) != POOL_SUCCESS) {
            fprintf(stderr, "無法提交任務\n");
            free(num);
        }
    }

    // 等待所有任務完成
    printf("\n等待任務完成...\n");
    thread_pool_wait(pool);

    // 打印統計
    thread_pool_print_stats(pool);

    // 銷毀線程池
    thread_pool_destroy(pool);
}

/*
 * 演示 2: 混合任務
 */
void demo_mixed(void)
{
    printf("\n========== 演示 2: 混合任務 ==========\n\n");

    thread_pool_t *pool = thread_pool_create(3, 20);
    if (pool == NULL) return;

    thread_pool_start(pool);

    // 提交計算任務
    for (int i = 1; i <= 5; i++) {
        int *num = malloc(sizeof(int));
        *num = i * 10;
        thread_pool_submit(pool, task_compute, num);
    }

    // 提交 I/O 任務
    for (int i = 1; i <= 5; i++) {
        char *filename = malloc(64);
        snprintf(filename, 64, "data_%d.txt", i);
        thread_pool_submit(pool, task_io, filename);
    }

    thread_pool_wait(pool);
    thread_pool_print_stats(pool);
    thread_pool_destroy(pool);
}

/*
 * 演示 3: 並行計算（Map-Reduce 風格）
 */
void demo_parallel_computation(void)
{
    printf("\n========== 演示 3: 並行計算 ==========\n\n");

    const int N = 1000000;
    const int NUM_BATCHES = 10;
    const int BATCH_SIZE = N / NUM_BATCHES;

    thread_pool_t *pool = thread_pool_create(4, NUM_BATCHES);
    if (pool == NULL) return;

    thread_pool_start(pool);

    int total_sum = 0;
    pthread_mutex_t sum_lock = PTHREAD_MUTEX_INITIALIZER;

    // 將計算分成多個批次
    clock_t start = clock();

    for (int i = 0; i < NUM_BATCHES; i++) {
        batch_task_arg_t *arg = malloc(sizeof(batch_task_arg_t));
        arg->start = i * BATCH_SIZE + 1;
        arg->end = (i + 1) * BATCH_SIZE;
        arg->sum = &total_sum;
        arg->lock = &sum_lock;

        thread_pool_submit(pool, task_batch_sum, arg);
    }

    thread_pool_wait(pool);

    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;

    printf("\n並行計算結果:\n");
    printf("計算範圍: 1 到 %d\n", N);
    printf("總和: %d\n", total_sum);
    printf("耗時: %.4f 秒\n", time_spent);

    // 驗證結果
    long long expected = (long long)N * (N + 1) / 2;
    printf("預期值: %lld\n", expected);
    printf("結果%s\n", total_sum == expected ? "正確！" : "錯誤！");

    thread_pool_print_stats(pool);
    thread_pool_destroy(pool);

    pthread_mutex_destroy(&sum_lock);
}

/*
 * 主函數
 */
int main(void)
{
    printf("========================================\n");
    printf("       線程池實現與演示\n");
    printf("========================================\n");

    // 運行演示
    demo_basic();
    demo_mixed();
    demo_parallel_computation();

    printf("\n所有演示完成！\n");

    return 0;
}

/*
 * 使用要點:
 *
 * 1. 創建線程池:
 *    thread_pool_t *pool = thread_pool_create(4, 100);
 *
 * 2. 啟動線程池:
 *    thread_pool_start(pool);
 *
 * 3. 提交任務:
 *    thread_pool_submit(pool, task_function, task_arg);
 *
 * 4. 等待完成:
 *    thread_pool_wait(pool);
 *
 * 5. 銷毀線程池:
 *    thread_pool_destroy(pool);
 *
 * 學習要點:
 * - 線程池的設計模式
 * - 任務隊列管理（FIFO）
 * - 條件變量的使用（生產者-消費者模式）
 * - 互斥鎖保護共享數據
 * - 優雅關閉的實現
 * - 統計信息收集
 * - Map-Reduce 並行計算模式
 *
 * 實際應用:
 * - Web 服務器的請求處理
 * - 數據庫連接池
 * - 圖像/視頻處理
 * - 大數據分析
 * - 異步 I/O 處理
 */
