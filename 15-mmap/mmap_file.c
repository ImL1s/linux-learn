#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

int main(void) {
    const char *filepath = "test.txt";
    const char *text = "Hello, mmap!\n";

    int fd = open(filepath, O_RDWR | O_CREAT, 0644);
    if (write(fd, text, strlen(text)) == -1) {
        perror("write");
        close(fd);
        return 1;
    }
    
    struct stat sb;
    fstat(fd, &sb);
    
    char *map = mmap(NULL, sb.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    printf("映射內容: %s", map);
    
    map[0] = 'h'; // 修改
    msync(map, sb.st_size, MS_SYNC);
    
    munmap(map, sb.st_size);
    close(fd);
    return 0;
}
