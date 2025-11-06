/*
 * 檔案名稱: mmap_shared.c
 * 功能說明: mmap 匿名共享映射 - 父子進程通訊
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

int main(void) {
    // 創建匿名共享映射
    char *shared = mmap(NULL, 4096, PROT_READ|PROT_WRITE,
                        MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    
    if (shared == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    
    pid_t pid = fork();
    
    if (pid == 0) {
        // 子進程：寫入數據
        strcpy(shared, "Hello from child!");
        printf("[子進程] 寫入: %s\n", shared);
        exit(0);
    } else {
        // 父進程：等待並讀取
        wait(NULL);
        printf("[父進程] 讀取: %s\n", shared);
        munmap(shared, 4096);
    }
    
    return 0;
}
