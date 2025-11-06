/*
 * 檔案名稱: setitimer_demo.c
 * 功能說明: setitimer 週期定時器演示
 */

#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>

int count = 0;

void timer_handler(int sig) {
    (void)sig;
    printf("⏰ 定時器觸發 (第 %d 次)\n", ++count);
    if (count >= 5) {
        printf("達到5次，停止定時器\n");
        struct itimerval timer = {0};
        setitimer(ITIMER_REAL, &timer, NULL);
    }
}

int main(void) {
    signal(SIGALRM, timer_handler);
    
    struct itimerval timer;
    timer.it_value.tv_sec = 1;      // 首次觸發：1秒
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 2;   // 週期：2秒
    timer.it_interval.tv_usec = 0;
    
    printf("設置週期定時器：首次1秒，之後每2秒\n");
    setitimer(ITIMER_REAL, &timer, NULL);
    
    while (count < 5) pause();
    
    printf("程序結束\n");
    return 0;
}
