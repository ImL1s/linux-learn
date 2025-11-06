/*
 * 檔案名稱: rwlock_demo.c
 * 功能說明: 讀寫鎖 - 多讀者單寫者完整演示
 */

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#define READERS 5
#define WRITERS 2

pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;
int data = 0;
int read_count = 0, write_count = 0;

void* reader_thread(void* arg) {
    int id = *(int*)arg;
    for (int i = 0; i < 3; i++) {
        pthread_rwlock_rdlock(&rwlock);
        printf("[讀者%d] 讀取: %d (第%d次)\n", id, data, ++read_count);
        sleep(1);
        pthread_rwlock_unlock(&rwlock);
        usleep(500000);
    }
    return NULL;
}

void* writer_thread(void* arg) {
    int id = *(int*)arg;
    for (int i = 0; i < 2; i++) {
        pthread_rwlock_wrlock(&rwlock);
        data += 10;
        printf("[寫者%d] 寫入: %d (第%d次)\n", id, data, ++write_count);
        sleep(2);
        pthread_rwlock_unlock(&rwlock);
        sleep(1);
    }
    return NULL;
}

int main(void) {
    pthread_t readers[READERS], writers[WRITERS];
    int r_ids[READERS], w_ids[WRITERS];
    
    printf("====== 讀寫鎖演示 ======\n");
    printf("配置: %d讀者 + %d寫者\n\n", READERS, WRITERS);
    
    for (int i = 0; i < READERS; i++) {
        r_ids[i] = i+1;
        pthread_create(&readers[i], NULL, reader_thread, &r_ids[i]);
    }
    for (int i = 0; i < WRITERS; i++) {
        w_ids[i] = i+1;
        pthread_create(&writers[i], NULL, writer_thread, &w_ids[i]);
    }
    
    for (int i = 0; i < READERS; i++) pthread_join(readers[i], NULL);
    for (int i = 0; i < WRITERS; i++) pthread_join(writers[i], NULL);
    
    printf("\n統計: 讀%d次, 寫%d次\n", read_count, write_count);
    pthread_rwlock_destroy(&rwlock);
    return 0;
}
