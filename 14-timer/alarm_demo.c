#include <stdio.h>
#include <unistd.h>
#include <signal.h>

void alarm_handler(int sig) {
    (void)sig;
    printf("⏰ ALARM 觸發！\n");
}

int main(void) {
    signal(SIGALRM, alarm_handler);
    printf("設置 3 秒後觸發 alarm...\n");
    alarm(3);
    pause();
    printf("程序結束\n");
    return 0;
}
