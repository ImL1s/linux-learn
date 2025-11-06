/*
 * 檔案名稱: rwlock_basic.c  
 * 功能說明: pthread 讀寫鎖 (Read-Write Lock) 基礎演示
 *
 * 知識點:
 *   1. pthread_rwlock_t 讀寫鎖
 *   2. pthread_rwlock_rdlock() 讀鎖定
 *   3. pthread_rwlock_wrlock() 寫鎖定
 *   4. pthread_rwlock_unlock() 解鎖
 *   5. 多讀者單寫者模型
 *   6. 讀者優先 vs 寫者優先
 *
 * 編譯方式: gcc -o rwlock_basic rwlock_basic.c -pthread
 * 執行方式: ./rwlock_basic
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;
int shared_data = 0;

void* reader(void* arg)
{
    int id = *(int*)arg;
    
    for (int i = 0; i < 3; i++) {
        printf("[讀者 %d] 請求讀鎖...\n", id);
        pthread_rwlock_rdlock(&rwlock);
        
        printf("[讀者 %d] 獲得讀鎖，讀取數據: %d\n", id, shared_data);
        sleep(1); // 模擬讀取操作
        
        pthread_rwlock_unlock(&rwlock);
        printf("[讀者 %d] 釋放讀鎖\n", id);
        
        usleep(500000);
    }
    
    return NULL;
}

void* writer(void* arg)
{
    int id = *(int*)arg;
    
    for (int i = 0; i < 2; i++) {
        printf("[寫者 %d] 請求寫鎖...\n", id);
        pthread_rwlock_wrlock(&rwlock);
        
        shared_data++;
        printf("[寫者 %d] 獲得寫鎖，寫入數據: %d\n", id, shared_data);
        sleep(1); // 模擬寫入操作
        
        pthread_rwlock_unlock(&rwlock);
        printf("[寫者 %d] 釋放寫鎖\n", id);
        
        sleep(2);
    }
    
    return NULL;
}

int main(void)
{
    pthread_t readers[3], writers[2];
    int reader_ids[3] = {1, 2, 3};
    int writer_ids[2] = {1, 2};
    
    printf("====== 讀寫鎖基礎演示 ======\n\n");
    printf("規則：\n");
    printf("  • 多個讀者可以同時持有讀鎖\n");
    printf("  • 只有一個寫者可以持有寫鎖\n");
    printf("  • 讀鎖和寫鎖互斥\n\n");
    
    // 創建讀者線程
    for (int i = 0; i < 3; i++) {
        pthread_create(&readers[i], NULL, reader, &reader_ids[i]);
    }
    
    // 創建寫者線程
    for (int i = 0; i < 2; i++) {
        pthread_create(&writers[i], NULL, writer, &writer_ids[i]);
    }
    
    // 等待所有線程完成
    for (int i = 0; i < 3; i++) {
        pthread_join(readers[i], NULL);
    }
    for (int i = 0; i < 2; i++) {
        pthread_join(writers[i], NULL);
    }
    
    printf("\n最終數據: %d\n", shared_data);
    pthread_rwlock_destroy(&rwlock);
    
    return 0;
}
