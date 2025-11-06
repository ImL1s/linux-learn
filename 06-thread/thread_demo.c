/*
 * 檔案名稱: thread_demo.c
 * 功能說明: POSIX 線程 (pthread) 演示
 *
 * 知識點:
 *   1. pthread_create() 創建線程
 *   2. pthread_join() 等待線程結束
 *   3. pthread_mutex_t 互斥鎖
 *   4. 線程同步與競爭條件
 *   5. 線程安全
 *
 * 編譯方式: gcc -o thread_demo thread_demo.c -pthread
 * 執行方式: ./thread_demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

// 全局共享變量
int global_counter = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * 線程函數 1: 簡單的計數器
 */
void* thread_function(void* arg)
{
    int thread_id = *(int*)arg;

    printf("[線程 %d] 開始執行 (TID=%lu)\n", thread_id, pthread_self());

    for (int i = 0; i < 5; i++) {
        printf("[線程 %d] 循環 %d\n", thread_id, i + 1);
        sleep(1);
    }

    printf("[線程 %d] 執行完畢\n", thread_id);

    // 返回值（可選）
    int* result = malloc(sizeof(int));
    *result = thread_id * 100;
    return result;
}

/*
 * 線程函數 2: 無互斥鎖的計數（演示競爭條件）
 */
void* unsafe_counter(void* arg)
{
    int thread_id = *(int*)arg;

    for (int i = 0; i < 100000; i++) {
        global_counter++;  // 競爭條件：不是原子操作！
    }

    printf("[線程 %d] 完成計數\n", thread_id);
    return NULL;
}

/*
 * 線程函數 3: 使用互斥鎖的安全計數
 */
void* safe_counter(void* arg)
{
    int thread_id = *(int*)arg;

    for (int i = 0; i < 100000; i++) {
        pthread_mutex_lock(&mutex);    // 加鎖
        global_counter++;
        pthread_mutex_unlock(&mutex);  // 解鎖
    }

    printf("[線程 %d] 完成安全計數\n", thread_id);
    return NULL;
}

/*
 * 演示 1: 基本線程創建和等待
 */
void demo_basic_thread(void)
{
    pthread_t thread1, thread2;
    int id1 = 1, id2 = 2;
    int* result1;
    int* result2;

    printf("====== 演示 1: 基本線程操作 ======\n\n");

    /*
     * pthread_create() - 創建線程
     *
     * int pthread_create(pthread_t *thread,
     *                    const pthread_attr_t *attr,
     *                    void *(*start_routine)(void *),
     *                    void *arg);
     *
     * 參數：
     *   thread: 線程 ID（輸出參數）
     *   attr: 線程屬性（NULL 使用默認）
     *   start_routine: 線程函數
     *   arg: 傳遞給線程函數的參數
     */
    printf("[主線程] 創建兩個線程...\n\n");

    if (pthread_create(&thread1, NULL, thread_function, &id1) != 0) {
        perror("pthread_create failed");
        return;
    }

    if (pthread_create(&thread2, NULL, thread_function, &id2) != 0) {
        perror("pthread_create failed");
        return;
    }

    printf("[主線程] 兩個線程已創建，主線程繼續執行其他任務...\n\n");
    sleep(2);

    /*
     * pthread_join() - 等待線程結束
     *
     * int pthread_join(pthread_t thread, void **retval);
     *
     * - 阻塞等待指定線程結束
     * - 回收線程資源
     * - 獲取線程返回值
     */
    printf("[主線程] 等待線程結束...\n");

    pthread_join(thread1, (void**)&result1);
    printf("[主線程] 線程 1 已結束，返回值: %d\n", *result1);
    free(result1);

    pthread_join(thread2, (void**)&result2);
    printf("[主線程] 線程 2 已結束，返回值: %d\n", *result2);
    free(result2);

    printf("\n[主線程] 所有線程已完成\n");
}

/*
 * 演示 2: 競爭條件（Race Condition）
 */
void demo_race_condition(void)
{
    pthread_t threads[5];
    int ids[5];

    printf("\n====== 演示 2: 競爭條件 ======\n\n");

    global_counter = 0;

    printf("[測試] 創建 5 個線程，每個線程將計數器加 100000 次\n");
    printf("[預期] 最終結果應該是 500000\n\n");

    // 創建 5 個線程
    for (int i = 0; i < 5; i++) {
        ids[i] = i + 1;
        pthread_create(&threads[i], NULL, unsafe_counter, &ids[i]);
    }

    // 等待所有線程完成
    for (int i = 0; i < 5; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("\n[結果] global_counter = %d\n", global_counter);

    if (global_counter != 500000) {
        printf("[分析] 結果不正確！這就是競爭條件造成的數據不一致\n");
        printf("[原因] counter++ 不是原子操作，包含：讀取、加一、寫回\n");
        printf("[說明] 多個線程可能同時讀取相同的值，導致部分增量丟失\n");
    } else {
        printf("[說明] 偶然得到正確結果（多次運行可能出現不同結果）\n");
    }
}

/*
 * 演示 3: 使用互斥鎖解決競爭條件
 */
void demo_mutex(void)
{
    pthread_t threads[5];
    int ids[5];

    printf("\n====== 演示 3: 使用互斥鎖 ======\n\n");

    global_counter = 0;

    printf("[測試] 創建 5 個線程，使用互斥鎖保護共享變量\n");
    printf("[預期] 最終結果應該是 500000\n\n");

    // 創建 5 個線程
    for (int i = 0; i < 5; i++) {
        ids[i] = i + 1;
        pthread_create(&threads[i], NULL, safe_counter, &ids[i]);
    }

    // 等待所有線程完成
    for (int i = 0; i < 5; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("\n[結果] global_counter = %d\n", global_counter);

    if (global_counter == 500000) {
        printf("[分析] 結果正確！互斥鎖成功保護了共享變量\n");
        printf("[說明] 同一時間只有一個線程能訪問臨界區\n");
    }
}

int main(void)
{
    printf("====== POSIX 線程演示 ======\n\n");

    // 演示 1: 基本線程操作
    demo_basic_thread();

    printf("\n按 Enter 繼續...");
    getchar();

    // 演示 2: 競爭條件
    demo_race_condition();

    printf("\n按 Enter 繼續...");
    getchar();

    // 演示 3: 互斥鎖
    demo_mutex();

    // 清理互斥鎖
    pthread_mutex_destroy(&mutex);

    printf("\n程序結束\n");
    return 0;
}

/*
 * 知識點總結：
 *
 * 1. 線程 vs 進程：
 *    - 線程共享：地址空間、文件描述符、全局變量
 *    - 線程私有：棧、寄存器、線程 ID
 *    - 線程創建開銷小於進程
 *
 * 2. 線程同步機制：
 *    - 互斥鎖 (Mutex): 保護臨界區
 *    - 條件變量 (Condition Variable): 線程間通訊
 *    - 讀寫鎖 (RW Lock): 多讀單寫
 *    - 信號量 (Semaphore): 資源計數
 *
 * 3. 互斥鎖操作：
 *    pthread_mutex_lock()    - 加鎖（阻塞）
 *    pthread_mutex_trylock() - 嘗試加鎖（非阻塞）
 *    pthread_mutex_unlock()  - 解鎖
 *
 * 4. 線程屬性：
 *    - 分離態 (detached): 結束後自動回收資源
 *    - 可結合態 (joinable): 需要 pthread_join() 回收
 *
 * 5. 常見錯誤：
 *    - 忘記初始化/銷毀互斥鎖
 *    - 死鎖（兩個線程互相等待對方的鎖）
 *    - 忘記解鎖
 *    - 訪問已結束線程的棧變量
 *
 * 6. 編譯注意：
 *    必須使用 -pthread 選項：
 *    gcc thread_demo.c -pthread -o thread_demo
 */
