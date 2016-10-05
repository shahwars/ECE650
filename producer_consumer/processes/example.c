#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


void die(char *s)
{
  perror(s);
  exit(1);
}

struct msg_type
{
    long    m_type;
    int     mesg;
};

int main(int argc, char *argv[])
{
    int msqid;
    int msgflg = IPC_CREAT | 0666;
    key_t key;
    struct msg_type msg_buf;
    size_t buflen;

    key = 1234;

    pid_t pid;

    if ( argc != 3) 
    {
        fprintf(stderr, "usage: producer_consumer <Number of integers> <Buffer Size>");
        return -1;
    }

    if ( atoi(argv[1]) < 0 || atoi(argv[2]) < 0 ) 
    {  
        fprintf(stderr, "Both arguments must be positive integers\n");
        return -1;
    }

    if ( atoi(argv[1]) < atoi(argv[2]) ) 
    {
        fprintf(stderr, "Number of integers must be greater than buffer size.\n");
        return -1;
    }

    N = atoi(argv[1]);
    B = atoi(argv[2]);

    pid = fork();

    if (pid<0)
    {
        fprintf(stderr, "Fork Failed");
    }
    else if (pid == 0)
    {
        execlp("./consumer", NULL);
    }
    else
    {
        if ((msqid = msgget(key, msgflg )) < 0)   //Get the message queue ID for the given key
           die("msgget");

        //Message Type
        sbuf.mtype = 1;

        printf("Enter a message to add to message queue : ");
        scanf("%[^\n]",sbuf.mtext);
        getchar();

        buflen = strlen(sbuf.mtext) + 1 ;

        if (msgsnd(msqid, &sbuf, buflen, IPC_NOWAIT) < 0)
        {
            printf ("%d, %d, %s, %d\n", msqid, sbuf.mtype, sbuf.mtext, buflen);
            die("msgsnd");
        }

        else
        {
            printf("Message Sent\n");
            wait(NULL);
        }
     }
    

    exit(0);
}


