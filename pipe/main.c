#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

int main(int argc, char *argv[])
{
    pid_t pid;
    char buf[1024];
    int pipfd[2];
    char *p = "test for pipe\n";

    if (pipe(pipfd) == -1)
    {
        // sys_err("pipe");
        // ERROR_EXIT("pipe error");
        printf("error");
    }

    pid = fork();
    if (pid < 0)
    {
        // sys_err("fork err");
        printf("fork err");
    }
    else if (pid == 0)
    {
        close(pipfd[1]);
        int len = read(pipfd[0], buf, sizeof(buf));
        write(STDOUT_FILENO, buf, len);
        close(pipfd[0]);
    }else{
        close(pipfd[0]);
        write(pipfd[1],p,strlen(p));
        wait(NULL);
        close(pipfd[1]);
    }

    printf("hello world\n");
    return 0;
}
