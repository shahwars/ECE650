#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>

#define MAX_NUMBER 2000
int N, B;


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

void consumer(void)
{
    struct msg rcvbuffer;
    int msqid; 
    key_t key = 1234;
    if ((msqid = msgget(key, 0666)) < 0)
        die("msgget()");
    
    int num;
    int i = 0;
    while (i < N) 
    {
        //Receive an answer of message type 1.
        if (msgrcv(msqid, &rcvbuffer, sizeof(int), 1, 0) < 0)
            die("msgrcv");
        
        printf("Consumed : %d\n", rcvbuffer.mesg);
        num = rcvbuffer.mesg;
        i++;
    }
    printf("received %d messages\n", i);
}


void producer(void)
{
    int msqid;
    int msgflg = IPC_CREAT | 0666;
    key_t key;
    struct msg msg_buf;
    struct msqid_ds msqid_buf;

    /* Arbitrary value for key to initialize the message queue */ 
    key = 1234;

    if ((msqid = msgget(key, msgflg )) < 0)   //Get the message queue ID for the given key
           die("msgget");

        if (msgctl(msqid, IPC_STAT, &msqid_buf) == -1)
        {
            perror("msgctl: msgctl failed");
            exit(1);
        }

        msqid_buf.msg_qbytes =  B*sizeof(int);

        if (msgctl(msqid, IPC_SET, &msqid_buf) == -1) 
        {
            perror("msgctl: msgctl failed");
            exit(1);
        }
       
        //Message Type
        msg_buf.m_type = 1;

        int i = 0;
        while ( i < N)
        {
            msg_buf.mesg = rand() % MAX_NUMBER;

            while(msgsnd(msqid, &msg_buf, sizeof(int), IPC_NOWAIT) < 0);
            printf("Produced: %d\n", msg_buf.mesg);
            i++;
        }
     //   wait(NULL);
        printf("Transmitted %d messages\n", i);
        wait(NULL);
}

int main(int argc, char *argv[])
{
    pid_t pid;
    srand(time(NULL));

    struct timeval start_t, init_t, end_t;
    long double interval_usec = 0.0;
    long double initialzation_t_usec, transmission_t_usec, init_t_usec, start_t_usec, end_t_usec;

   /* Start measuring time for initialization */
    gettimeofday(&start_t, NULL);

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

    /* Check point for initialization time measurement */
    gettimeofday(&init_t, NULL);

    pid = fork();

    if (pid<0)
    {
        fprintf(stderr, "Fork Failed");
    }
    else if (pid == 0)
    {
        /*Child process should call the consumer.*/
        consumer();
        /* checkpoint for threads ending */
        gettimeofday(&end_t, NULL);

        /* timing extraction from the data structure */
        start_t_usec = (long double) (start_t.tv_sec * 1000000 + start_t.tv_usec);
        init_t_usec = (long double) (init_t.tv_sec * 1000000 + init_t.tv_usec);

        /* Keep the initialization time as a difference */
        initialzation_t_usec = init_t_usec - start_t_usec;

        init_t_usec = (long double) (init_t.tv_sec * 1000000 + init_t.tv_usec);
        end_t_usec = (long double) (end_t.tv_sec * 1000000 + end_t.tv_usec);

        transmission_t_usec = end_t_usec - init_t_usec;

        printf("Initialization time = %Lf in seconds %d\n", (initialzation_t_usec / 1000000), pid);
        printf("Time to transfer data = %Lf seconds %d \n", (transmission_t_usec / 1000000), pid);
    }
    else
    {   
        /* Parent process should call the producer.*/
        producer();
    }
    wait(NULL);
    exit(0);
}


