#include<stdio.h>
#include<sys/types.h>
#include<unistd.h>

int main() 
{

pid_t pid;

    pid = fork();

    if (pid<0)
    {
        fprintf(stderr, "Fork Failed");
    }
    else if (pid == 0)
    {
        execlp("/home/shahwar/ECE650/producer_consumer/consumer", NULL);
    }
    else
    {
        wait(NULL);
        printf("Child Complete\n");
    }


    return 0;
}
