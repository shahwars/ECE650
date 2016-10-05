#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>


void die(char *s)
{
  perror(s);
  exit(1);
}

struct msg
{
    long    m_type;
    int     mesg;
};

int main(int argc, char *argv[])
{
    int msqid;
    int msgflg = IPC_CREAT | 0666;
    key_t key;
    struct msg msg_buf;
    struct msqid_ds msqid_buf;
    int N,B;

    key = 1234;

    pid_t pid;

    srand(time(NULL));

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
       // execlp("./consumer", NULL);
      struct msg rcvbuffer;
      if ((msqid = msgget(key, 0666)) < 0)
      die("msgget()");

      int i = 0;
      while (i < N) 
      {
         //Receive an answer of message type 1.
         if (msgrcv(msqid, &rcvbuffer, sizeof(int), 1, 0) < 0)
            die("msgrcv");

         printf("%d\n", rcvbuffer.mesg);
         i++;
      }
     
    }
    else
    {   
        if ((msqid = msgget(key, msgflg )) < 0)   //Get the message queue ID for the given key
           die("msgget");

        if (msgctl(msqid, IPC_STAT, &msqid_buf) == -1)
        {
            perror("msgctl: msgctl failed");
            exit(1);
        }

        msqid_buf.msg_qbytes =  B;

        if (msgctl(msqid, IPC_SET, &msqid_buf) == -1) 
        {
            perror("msgctl: msgctl failed");
            exit(1);
        }
        //Message Type
        printf("In parent");
        msg_buf.m_type = 1;

        int i = 0;
        while ( i < N)
        {
           
            msg_buf.mesg = rand();
            if (msgsnd(msqid, &msg_buf, sizeof(int), IPC_NOWAIT) < 0)
            {
                 printf ("%d, %d, %d, %d\n", msqid, msg_buf.m_type, msg_buf.mesg, sizeof(int));
                 die("msgsnd");
            }
            else
            {
                 i++;
            }
        }
     }
    

    exit(0);
}


