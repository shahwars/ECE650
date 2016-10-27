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

    /* start to consume the integers */
    while (i < N) 
    {
        /* If the queue is empty keep on trying if a message is present */
        while(msgrcv(msqid, &rcvbuffer, sizeof(int), 1, 0) < 0);
        
        /* Consume the number and  move on */
        num = rcvbuffer.mesg;
        i++;
    }
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

    /* Get the message queue ID for the given key. */
    if ((msqid = msgget(key, msgflg )) < 0)   
           die("msgget");

    /* Get the values of structure filled in msqid_buf */
    if (msgctl(msqid, IPC_STAT, &msqid_buf) == -1)
    {
        perror("msgctl: msgctl failed");
        exit(1);
    }

    /* 
       Set the buffer value of the message queue.
       So that Kernel never runs out of memory when 
       we set a bound on the queue buffer.
    */
    msqid_buf.msg_qbytes =  B*sizeof(int);

    /*
        Set the buffer values inside the msqid.
    */
    if (msgctl(msqid, IPC_SET, &msqid_buf) == -1) 
    {
        perror("msgctl: msgctl failed");
        exit(1);
    }
       
    /* Set the message type message type */
    msg_buf.m_type = 1;

    int i = 0;
    while ( i < N)
    {
        msg_buf.mesg = rand() % MAX_NUMBER;

        /* 
           If the queue is currently filled, just keep waiting 
           and trying to send the current random number. 
        */
        while(msgsnd(msqid, &msg_buf, sizeof(int), IPC_NOWAIT) < 0);
        
        i++;
    }
    
    /* Just inform when all the numbers are produced */
    //printf("Transmitted %d messages\n", i);
    wait(NULL);
}

/* 
    Utility function just to calculate time and manipulate seconds and microseconds
    which are inside the struct timeval 
    returns time value in seconds
*/
long double time_calculation(struct timeval start_t, struct timeval end_t)
{
    /* timing extraction from the data structure */
    long double start_t_usec = (long double) (start_t.tv_sec * 1000000 + start_t.tv_usec);
    long double end_t_usec = (long double) (end_t.tv_sec * 1000000 + end_t.tv_usec);

    /* Keep the initialization time as a difference */
    return (end_t_usec - start_t_usec)/1000000;
}

int main(int argc, char *argv[])
{
    pid_t pid;
    srand(time(NULL));

    struct timeval start_t, init_t, end_t, after_fork;
    long double interval_usec = 0.0;
    long double initialzation_t_usec, transmission_t_usec, init_t_usec, start_t_usec, end_t_usec;

   /* Start measuring time for initialization */
    gettimeofday(&start_t, NULL);

    /* Error check if there are more or less arguments than required*/
    if ( argc != 3) 
    {
        fprintf(stderr, "usage: producer_consumer <Number of integers> <Buffer Size>");
        return -1;
    }

    /* Error check if one of the arguments is negative.*/
    if ( atoi(argv[1]) < 0 || atoi(argv[2]) < 0 ) 
    {  
        fprintf(stderr, "Both arguments must be positive integers\n");
        return -1;
    }

    /* Check if the arguments satisfy the condition N > B*/
    if ( atoi(argv[1]) < atoi(argv[2]) ) 
    {
        fprintf(stderr, "Number of integers must be greater than buffer size.\n");
        return -1;
    }

    /* Extract the arguments to integers.*/
    N = atoi(argv[1]);
    B = atoi(argv[2]);

    /* Check point for initialization time measurement */
    gettimeofday(&init_t, NULL);

    pid = fork();

    /* Fork call is left out as asked in the assignment description. */
    gettimeofday(&after_fork, NULL);

    /* Check if the process was created successfully or not */
    if (pid<0)
    {
        fprintf(stderr, "Fork Failed");
    }
    else if (pid == 0)
    {
        /*Child process should call the consumer.*/
        consumer();

        /* Measure time right after we get out of consumer loop.
           Checkpoint for consumer ending */
        gettimeofday(&end_t, NULL);

        printf("%d , %d , %Lf , %Lf\n", N, B, 
           time_calculation(start_t, init_t) , 
           time_calculation(init_t, end_t));
    }
    else
    {   
        /* Parent process should call the producer.*/
        producer();
    }
    wait(NULL);
    exit(0);
}