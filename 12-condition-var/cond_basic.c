/*
 * 檔案名稱: cond_basic.c
 * 功能說明: pthread 條件變量 (Condition Variable) 基礎演示
 *
 * 知識點:
 *   1. pthread_cond_t 條件變量
 *   2. pthread_cond_wait() 等待條件
 *   3. pthread_cond_signal() 喚醒單個線程
 *   4. pthread_cond_broadcast() 喚醒所有線程
 *   5. 條件變量必須配合 mutex 使用
 *   6. 虛假喚醒 (spurious wakeup) 問題
 *
 * 編譯方式: gcc -o cond_basic cond_basic.c -pthread
 * 執行方式: ./cond_basic
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

/*
 * 共享數據結構
 */
typedef struct {
    int value;
    bool ready;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} shared_data_t;

shared_data_t shared_data = {
    .value = 0,
    .ready = false,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .cond = PTHREAD_COND_INITIALIZER
};

/*
 * 等待線程：等待條件滿足
 */
void* waiter_thread(void* arg)
{
    int thread_id = *(int*)arg;

    printf("[等待線程 %d] 啟動\n", thread_id);

    /*
     * 條件變量使用的標準模式：
     *
     * pthread_mutex_lock(&mutex);
     * while (!condition) {
     *     pthread_cond_wait(&cond, &mutex);
     * }
     * // 使用共享資源
     * pthread_mutex_unlock(&mutex);
     */

    pthread_mutex_lock(&shared_data.mutex);

    printf("[等待線程 %d] 獲得鎖，檢查條件...\n", thread_id);

    /*
     * 重要：為什麼要用 while 而不是 if？
     *
     * 原因：虛假喚醒 (Spurious Wakeup)
     * - pthread_cond_wait() 可能在沒有 signal/broadcast 的情況下被喚醒
     * - 多個線程競爭時，條件可能被其他線程改變
     * - 因此必須用 while 循環再次檢查條件
     */
    while (!shared_data.ready) {
        printf("[等待線程 %d] 條件不滿足，進入等待...\n", thread_id);

        /*
         * pthread_cond_wait() 做了三件事：
         * 1. 原子性地解鎖 mutex
         * 2. 將線程放入條件變量的等待隊列，阻塞
         * 3. 被喚醒後，重新鎖定 mutex
         *
         * 注意：被喚醒時，mutex 已經被重新鎖定
         */
        pthread_cond_wait(&shared_data.cond, &shared_data.mutex);

        printf("[等待線程 %d] 被喚醒，重新檢查條件...\n", thread_id);
    }

    // 條件滿足，使用共享資源
    printf("[等待線程 %d] 條件滿足！讀取到值: %d\n", thread_id, shared_data.value);

    pthread_mutex_unlock(&shared_data.mutex);

    printf("[等待線程 %d] 完成\n", thread_id);

    return NULL;
}

/*
 * 信號線程：改變條件並通知
 */
void* signaler_thread(void* arg)
{
    (void)arg;

    printf("[信號線程] 啟動\n");
    printf("[信號線程] 準備數據中...\n");

    // 模擬數據準備
    sleep(2);

    /*
     * 修改共享數據的標準模式：
     */
    pthread_mutex_lock(&shared_data.mutex);

    shared_data.value = 42;
    shared_data.ready = true;

    printf("[信號線程] 數據已準備好：value = %d\n", shared_data.value);

    /*
     * pthread_cond_signal() vs pthread_cond_broadcast()
     *
     * signal(): 喚醒一個等待線程（如果有多個，由調度器決定喚醒哪個）
     * broadcast(): 喚醒所有等待線程
     *
     * 通常使用 signal() 即可，除非確實需要喚醒所有線程
     */
    printf("[信號線程] 發送信號喚醒等待線程...\n");
    pthread_cond_signal(&shared_data.cond);

    pthread_mutex_unlock(&shared_data.mutex);

    printf("[信號線程] 完成\n");

    return NULL;
}

/*
 * 演示 broadcast 的使用
 */
void demo_broadcast(void)
{
    pthread_t waiters[3];
    pthread_t signaler;
    int ids[3] = {1, 2, 3};

    printf("\n====== 演示 pthread_cond_broadcast() ======\n\n");

    // 重置共享數據
    shared_data.value = 0;
    shared_data.ready = false;

    // 創建3個等待線程
    for (int i = 0; i < 3; i++) {
        pthread_create(&waiters[i], NULL, waiter_thread, &ids[i]);
        usleep(100000); // 稍微延遲，確保線程依次啟動
    }

    // 等待所有等待線程進入等待狀態
    sleep(1);

    printf("\n[主線程] 所有等待線程已就緒，準備發送廣播信號...\n\n");

    // 修改條件並廣播
    pthread_mutex_lock(&shared_data.mutex);
    shared_data.value = 100;
    shared_data.ready = true;
    printf("[主線程] 數據已準備：value = %d\n", shared_data.value);
    printf("[主線程] 發送 broadcast 喚醒所有等待線程...\n");
    pthread_cond_broadcast(&shared_data.cond);
    pthread_mutex_unlock(&shared_data.mutex);

    // 等待所有線程完成
    for (int i = 0; i < 3; i++) {
        pthread_join(waiters[i], NULL);
    }

    printf("\n[主線程] 所有線程已完成\n");
}

/*
 * 演示 signal 的使用
 */
void demo_signal(void)
{
    pthread_t waiter;
    pthread_t signaler;
    int id = 1;

    printf("\n====== 演示 pthread_cond_signal() ======\n\n");

    // 重置共享數據
    shared_data.value = 0;
    shared_data.ready = false;

    // 創建等待線程
    pthread_create(&waiter, NULL, waiter_thread, &id);

    sleep(1); // 確保等待線程先進入等待狀態

    // 創建信號線程
    pthread_create(&signaler, NULL, signaler_thread, NULL);

    // 等待線程完成
    pthread_join(waiter, NULL);
    pthread_join(signaler, NULL);

    printf("\n[主線程] 演示完成\n");
}

int main(void)
{
    printf("====== pthread 條件變量基礎演示 ======\n");

    // 演示1：signal 單個線程
    demo_signal();

    printf("\n按 Enter 繼續...");
    getchar();

    // 演示2：broadcast 多個線程
    demo_broadcast();

    // 清理資源
    pthread_mutex_destroy(&shared_data.mutex);
    pthread_cond_destroy(&shared_data.cond);

    printf("\n程序結束\n");
    return 0;
}

/*
 * 知識點總結：
 *
 * 1. 條件變量的作用：
 *    - 線程間的同步機制
 *    - 允許線程等待某個條件成立
 *    - 比忙等待（busy waiting）更高效
 *
 * 2. 為什麼需要配合 mutex？
 *    - 保護共享數據（條件本身）
 *    - 避免競爭條件
 *    - pthread_cond_wait() 會原子性地解鎖和等待
 *
 * 3. pthread_cond_wait() 的工作流程：
 *    ① 解鎖 mutex（原子操作）
 *    ② 將線程放入等待隊列
 *    ③ 線程休眠
 *    ④ 被喚醒後重新競爭 mutex
 *    ⑤ 獲得 mutex 後返回
 *
 * 4. 為什麼用 while 而不是 if？
 *    - 虛假喚醒：系統可能無故喚醒線程
 *    - 條件變化：其他線程可能改變條件
 *    - 多個等待者：被喚醒時條件可能已被其他線程使用
 *
 * 5. signal vs broadcast：
 *    - signal(): 喚醒一個線程，適合單個消費者
 *    - broadcast(): 喚醒所有線程，適合多個消費者
 *    - signal() 更高效，broadcast() 更安全
 *
 * 6. 使用模式：
 *    等待方：
 *      pthread_mutex_lock(&mutex);
 *      while (!condition) {
 *          pthread_cond_wait(&cond, &mutex);
 *      }
 *      // 使用資源
 *      pthread_mutex_unlock(&mutex);
 *
 *    通知方：
 *      pthread_mutex_lock(&mutex);
 *      // 修改條件
 *      pthread_cond_signal(&cond);  // 或 broadcast
 *      pthread_mutex_unlock(&mutex);
 *
 * 7. 常見錯誤：
 *    - 忘記用 while 循環檢查條件
 *    - 沒有持有 mutex 就調用 wait
 *    - signal/broadcast 時沒有持有 mutex（雖然不是必須，但建議持有）
 *    - 忘記初始化或銷毀條件變量
 *
 * 8. 條件變量 vs 信號量：
 *    條件變量：
 *      ✓ 必須配合 mutex 使用
 *      ✓ 用於等待某個條件成立
 *      ✓ 沒有計數值
 *      ✓ 喚醒操作不記憶（沒有線程等待時，signal 會丟失）
 *
 *    信號量：
 *      ✓ 獨立使用
 *      ✓ 用於資源計數
 *      ✓ 有計數值
 *      ✓ post 操作會增加計數（即使沒有線程等待）
 *
 * 9. 適用場景：
 *    - 生產者-消費者問題
 *    - 讀者-寫者問題
 *    - 任務隊列
 *    - 線程池
 *    - 事件通知
 *
 * 10. 性能考慮：
 *     - 條件變量比忙等待省 CPU
 *     - signal 比 broadcast 高效
 *     - 避免頻繁的 signal/broadcast
 *     - 批量處理可減少喚醒次數
 */
