#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/mman.h>

#define MAX_NUMBER						10000
#define TIME_PERIOD_SECS				10

int run_time, p_i, Pt_min, Pt_max, Ct1_min, Ct1_max, Ct2_min, Ct2_max, Rs_min, Rs_max;
int buffer_size;

int msqid;

static int *request_number;
static int *num_req_consumed;
static int *periodic_req_count;

static int *producer_blocked;
static int *consumer_blocked;

static long double *producer_block_time;
static long double *consumer_block_time;


struct request
{
    long mtype;
    int req_num;
    int req_size;
    int request_content[];
};

void die(char *s)
{
  perror(s);
  exit(1);
}

int generate_Rs()
{
    int Rs = (rand() % Rs_max) + Rs_min;
	return Rs;
}

long double generate_Pt()
{  
	int Pt = ( rand() % Pt_max) + Pt_min;
	return Pt/1000.0;
}

long double generate_Ct1 ()
{
	int Ct1 = ( rand() % Ct1_max ) + Ct1_min;
	return Ct1/1000.0;
}

long double generate_Ct2()
{
	int Ct2 = ( rand() % Ct2_max ) + Ct2_min;
	return Ct2/1000.0;
}

long double select_Ct1_Ct2(long double ct1, long double ct2)
{
	int gen_p = (rand() % 100 + 1);
    
    if (gen_p < p_i)
    {
        return ct1;
    }
    else
    {
    	return ct2;
    }
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

void initialize_globals(void)
{
    request_number = mmap(NULL, sizeof *request_number, PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    num_req_consumed = mmap(NULL, sizeof *num_req_consumed, PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    periodic_req_count = mmap(NULL, sizeof *periodic_req_count, PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    producer_blocked = mmap(NULL, sizeof *producer_blocked, PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    consumer_blocked = mmap(NULL, sizeof *consumer_blocked, PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    producer_block_time = mmap(NULL, sizeof *producer_block_time, PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    consumer_block_time = mmap(NULL, sizeof *consumer_block_time, PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
}

void producer(int producer_num)
{
    srand(time(NULL) * producer_num);
    while (1)
    {
    	usleep(generate_Pt()*1000000);
    	int R_S = generate_Rs();
    	struct request *req = malloc( sizeof(struct request) + (R_S)* sizeof(int) );
    	int blocked = 0;
    	long double block_time = 0.0;

    	struct timeval start_p, end_p;
		/* Set the message type message type */
	    req->mtype = 1;
	    req->req_num = (*request_number)++;
	    req->req_size = R_S;

	    for (int r = 0; r < R_S; r++)
        	req->request_content[r] = rand() % MAX_NUMBER;

		/* 
	        If the queue is currently filled, just keep waiting 
	        and trying to send the current random number. 
	    */

        gettimeofday(&start_p, NULL);

	    while(msgsnd(msqid, req, sizeof(int) * (R_S + 2), IPC_NOWAIT) == -1)
	    {
	    	blocked  = 1;
	    }
        
        if (blocked)
        {
        	gettimeofday(&end_p, NULL);
        	block_time = time_calculation(start_p, end_p);
        }

        (*producer_block_time) = (*producer_block_time) + block_time;
        (*producer_blocked) = (*producer_blocked) + blocked;
        printf("Produced request number : %d, size = %d\n", req->req_num, req->req_size);
        free(req);
	}
}

void consumer(int consumer_num)
{
	while (1)
	{
		struct request *rcvbuffer = malloc( sizeof(struct request) + (buffer_size * sizeof(int)));
    	int blocked = 0;
    	long double block_time = 0.0;
    	struct timeval start_c, end_c;

		long double Ct = select_Ct1_Ct2(generate_Ct1(), generate_Ct2());
        usleep(Ct*1000000);

        gettimeofday(&start_c, NULL);
	    /* If the queue is empty keep on trying if a message is present */
       	while(msgrcv(msqid, rcvbuffer, buffer_size * sizeof(int), 1, IPC_NOWAIT) == -1)
       	{
       		blocked = 1;
       	}

       	if (blocked)
        {
        	gettimeofday(&end_c, NULL);
        	block_time = time_calculation(start_c, end_c);
        }

        (*consumer_block_time) = (*consumer_block_time) + block_time;
       	(*consumer_blocked) = (*consumer_blocked) + blocked;
	    *num_req_consumed = (*num_req_consumed)+ 1;
	    *periodic_req_count = (*periodic_req_count)+1;
	    printf("Consumeed request number : %d, size = %d\n", rcvbuffer->req_num, rcvbuffer->req_size);

        free(rcvbuffer);
	}
}

int main(int argc, char *argv[])
{
    int num_pro, num_con, time_spent;

    struct timeval start_t, end_t, p_t;

    /* Declare Message queue related variables */
    int msgflg = IPC_CREAT | 0666;
    key_t key;
    struct msqid_ds msqid_buf;

    initialize_globals();

    gettimeofday(&start_t, NULL);

    /* Error check if there are more or less arguments than required*/
    if ( argc != 14) 
    {
         fprintf(stderr, "Usage: ./producer_consumer <T> <B> <P> <C> <Rs_MIN> <Rs_MAX> <Pt_min> <Pt_max> <Ct1_min> <Ct1_max> <Ct2_min> <Ct2_max> <p_i>\n");
        return -1;
    }

    /* Error check if one of the arguments is negative.*/
    if ( atoi(argv[1]) < 0 || atoi(argv[2]) < 0 || atoi(argv[3]) < 0 || atoi(argv[4]) < 0  || 
    	 atoi(argv[5]) < 0 || atoi(argv[6]) < 0 || atoi(argv[7]) < 0 || atoi(argv[8]) < 0  || 
    	 atoi(argv[9]) < 0 || atoi(argv[10]) < 0 || atoi(argv[11]) < 0 || atoi(argv[12]) < 0 || atoi(argv[13]) < 0) 
    { 
        fprintf(stderr, "Every argument must be a positive integer.\n");
        return -1;
    }
    
    /* Extract the arguments to integers.*/
    run_time = atoi(argv[1]);
    buffer_size = atoi(argv[2]);
    num_pro = atoi(argv[3]);
    num_con = atoi(argv[4]);
    Rs_min = atoi(argv[5]);
    Rs_max = atoi(argv[6]);
    Pt_min = atoi(argv[7]);
    Pt_max = atoi(argv[8]);
    Ct1_min = atoi(argv[9]);
    Ct1_max = atoi(argv[10]);
    Ct2_min = atoi(argv[11]);
    Ct2_max = atoi(argv[12]);
    p_i = atoi(argv[13]);

    pid_t pid;

    *request_number = 1;
    *num_req_consumed = 0;
    *periodic_req_count = 0;
    *producer_blocked = 0;
    *consumer_blocked = 0;
    *producer_block_time = 0.0;
    *consumer_block_time = 0.0;

	/* Arbitrary value for key to initialize the message queue */ 
    key = 1234;

    /* Get the message queue ID for the given key. */
    if ((msqid = msgget(key, msgflg )) < 0)   
           die("msgget");

    /* Get the values of structure filled in msqid_buf */
    msgctl(msqid, IPC_STAT, &msqid_buf);

    /* 
       Set the buffer value of the message queue.
       So that Kernel never runs out of memory when 
       we set a bound on the queue buffer.
    */
    msqid_buf.msg_qbytes =  buffer_size * sizeof(int);

    /*
        Set the buffer values inside the msqid.
    */
    msgctl(msqid, IPC_SET, &msqid_buf);
    
    for (int i = 1; i <= num_pro + num_con; i++) 
    {
	    pid = fork();
	    if (pid) 
	    {
            continue;
	    } 
	    else if (pid == 0) 
	    {
	    	if (i <= num_pro)
		    	producer(i);
		    else
		    	consumer(i-num_pro);

	        break;
	    } 
	    else 
	    {
	        printf("Could not fork.\n");
	        exit(1);
	    }
	}

	if(pid)
	{
		while ( time_spent != run_time )
		{
			sleep(TIME_PERIOD_SECS);
			gettimeofday(&end_t, NULL);
			time_spent = (int) time_calculation(start_t, end_t);

			if (time_spent % TIME_PERIOD_SECS == 0 )
			{
				printf("CONSOLE MESSAGE: 10 Seconds period requests completed: %d\n", *periodic_req_count );
				*periodic_req_count = 0;
			}
   		}

    	printf("CONSOLE MESSAGE: Total requests completed: %d\n", *num_req_consumed );
        printf("CONSOLE MESSAGE: Number of times producers got blocked: %d\n", *producer_blocked);
        printf("CONSOLE MESSAGE: Number of times consumers got blocked: %d\n", *consumer_blocked);
        printf("CONSOLE MESSAGE: Summation of producers block time: %Lf seconds\n", *producer_block_time);
        printf("CONSOLE MESSAGE: Average block time per producer: %Lf seconds\n", (*producer_block_time)/(num_pro));
        printf("CONSOLE MESSAGE: Summation of consumers block time: %Lf seconds\n", *consumer_block_time);
        printf("CONSOLE MESSAGE: Average block time per consumer: %Lf seconds\n", (*consumer_block_time)/(num_con));

        msgctl(msqid, IPC_RMID, (struct msqid_ds *) 0);
        kill(0, SIGTERM);

	}
	
}