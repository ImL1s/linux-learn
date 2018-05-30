#include<sys/types.h>
#include<sys/stat.h>
#include<unistd.h>
#include<fcntl.h>
#include<stdio.h>
#include<errno.h>
#include<string.h>
#include<signal.h>

int main(int argc,char *argv[])
{
    
    int pipfd[2];
    if(pipe(pipfd)){
        ERROR_EXIT("pipe error");           
    }


}
