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

/* Define to limit maximum size of integer generated. */
#define MAX_NUMBER						10000

/* Time period after which number of requests are calculated. */
#define TIME_PERIOD_SECS				10

/* Global variables to contain program parameters and to manipulate them. */
int num_pro, num_con, time_spent, buffer_size;
int run_time, p_i, Pt_min, Pt_max, Ct1_min, Ct1_max, Ct2_min, Ct2_max, Rs_min, Rs_max;

/* Intger to hold ID for the message queue. */
int msqid;

/* Some static shared variables between producers and consumers to calculate requests. */
static int *request_number;
static int *num_req_consumed;
static int *periodic_req_count;
static int *producer_blocked;
static int *consumer_blocked;
static long double *producer_block_time;
static long double *consumer_block_time;


/* Request structure. This will contain all the data of each request.*/
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

/* Generate a uniformly distributed random Request size. */
int generate_Rs()
{
    int Rs = (rand() % Rs_max) + Rs_min;
	return Rs;
}

/* Generate a uniformly distributed random Producer delay. */
long double generate_Pt()
{  
	int Pt = ( rand() % Pt_max) + Pt_min;
	return Pt/1000.0;
}

/* Generate a uniformly distributed random CPU Bound Consumer delay.*/
long double generate_Ct1 ()
{
	int Ct1 = ( rand() % Ct1_max ) + Ct1_min;
	return Ct1/1000.0;
}

/* Generate a uniformly distributed random I/O Bound Consumer delay.*/
long double generate_Ct2()
{
	int Ct2 = ( rand() % Ct2_max ) + Ct2_min;
	return Ct2/1000.0;
}

/* 
    Small utility function to select between Ct1 or Ct2 with probability p_i.

    * returns ct1 or ct2 value after deciding
 */
long double select_Ct1_Ct2(long double ct1, long double ct2)
{
	int gen_p = (rand() % 100 + 1);
    
    if (gen_p < p_i)
        return ct1;    
    else
    	return ct2;
}

/* 
    Utility function just to calculate time and manipulate seconds and microseconds
    which are inside the struct timeval 
    
    * returns time value in seconds
*/
long double time_calculation(struct timeval start_t, struct timeval end_t)
{
    /* timing extraction from the data structure */
    long double start_t_usec = (long double) (start_t.tv_sec * 1000000 + start_t.tv_usec);
    long double end_t_usec = (long double) (end_t.tv_sec * 1000000 + end_t.tv_usec);

    /* Keep the initialization time as a difference */
    return (end_t_usec - start_t_usec)/1000000;
}

/*
    Function just to initialize all the global variables. 
*/
void initialize_globals(char *argv[])
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

    *request_number = 1;
    *num_req_consumed = 0;
    *periodic_req_count = 0;
    *producer_blocked = 0;
    *consumer_blocked = 0;
    *producer_block_time = 0.0;
    *consumer_block_time = 0.0;

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
}

/*
    Function to print console messages when the progam is about to terminate.
*/
void console(void)
{
        printf("CONSOLE MESSAGE: Total requests completed: %d\n", *num_req_consumed );
        printf("CONSOLE MESSAGE: Number of times producers got blocked: %d\n", *producer_blocked);
        printf("CONSOLE MESSAGE: Number of times consumers got blocked: %d\n", *consumer_blocked);
        printf("CONSOLE MESSAGE: Summation of producers block time: %Lf seconds\n", *producer_block_time);
        printf("CONSOLE MESSAGE: Average block time per producer: %Lf seconds\n", (*producer_block_time)/(num_pro));
        printf("CONSOLE MESSAGE: Summation of consumers block time: %Lf seconds\n", *consumer_block_time);
        printf("CONSOLE MESSAGE: Average block time per consumer: %Lf seconds\n", (*consumer_block_time)/(num_con));
}

void producer(int producer_num)
{
    /* Every producer must have its own random seed. 

        Here producer number generated in main is multiplied by time(NULL)
        to make the seed a unique parameter for every producer.

    */
    srand(time(NULL) * producer_num);

    while (1)
    {
        /* 
            Producer is supposed to generate a delay at the start. 
            Or before generating a request. 
        */
    	usleep(generate_Pt()*1000000);
    	
        int R_S = generate_Rs();
    	struct request *req = malloc( sizeof(struct request) + (R_S)* sizeof(int) );
    	int blocked = 0;
    	long double block_time = 0.0;

    	struct timeval start_p, end_p;
		
        /* Set the values of struct members e.g message type, request size and request number. */
	    req->mtype = 1;
	    req->req_num = (*request_number)++;
	    req->req_size = R_S;

        /* 
            Insert the numbers the request.
            This is actual content of the request. 
        */
        int r;
	    for (r = 0; r < R_S; r++)
        	req->request_content[r] = rand() % MAX_NUMBER;

        /* Start measuring time because we want to measure time everytime a producer is blocked. */
        gettimeofday(&start_p, NULL);

        /* 
            If the queue is currently filled, just keep waiting 
            and trying to send the current random number. 
        */
	    while(msgsnd(msqid, req, sizeof(int) * (R_S + 2), IPC_NOWAIT) == -1)
	    {
            /* Set a blocked flag if the consumer os blocked. */
	    	blocked  = 1;
	    }
        
        if (blocked)
        {
            /* Calculate time again only of producer was blocked. */
        	gettimeofday(&end_p, NULL);
        	block_time = time_calculation(start_p, end_p);
        }

        /* Accumulate blocking time and number of blocks calculation. */
        (*producer_block_time) = (*producer_block_time) + block_time;
        (*producer_blocked) = (*producer_blocked) + blocked;
        
        printf("Produced request number : %d, size = %d\n", req->req_num, req->req_size);
        
        /* We need to free the memory allocated previously to make program more efficient. */
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

        /* Start measuring time because we want to measure time everytime a consumer is blocked. */
        gettimeofday(&start_c, NULL);

	    /* If the queue is empty keep on trying if a message is present */
       	while(msgrcv(msqid, rcvbuffer, buffer_size * sizeof(int), 1, IPC_NOWAIT) == -1)
       	{
            /* Set a blocked flag if the consumer os blocked. */
       		blocked = 1;
       	}

        /* Delay which models the time for which received request is being processed. */
        usleep(select_Ct1_Ct2(generate_Ct1(), generate_Ct2())*1000000);
       	if (blocked)
        {
            /* Calculate time again only of consumer was blocked. */
        	gettimeofday(&end_c, NULL);
        	block_time = time_calculation(start_c, end_c);
        }

        /* Accumulate blocking time and number of blocks calculation. */
        (*consumer_block_time) = (*consumer_block_time) + block_time;
       	(*consumer_blocked) = (*consumer_blocked) + blocked;

        /* Accumulate total request consumed and periodic request count. */
	    *num_req_consumed = (*num_req_consumed)+ 1;
	    *periodic_req_count = (*periodic_req_count)+1;
	    printf("Consumeed request number : %d, size = %d\n", rcvbuffer->req_num, rcvbuffer->req_size);

        /* We need to free the memory allocated previously to make program more efficient. */
        free(rcvbuffer);
	}
}

int main(int argc, char *argv[])
{
    struct timeval start_t, end_t;

    pid_t pid;

    /* Declare Message queue related variables. */
    int msgflg = IPC_CREAT | 0666;
    key_t key;
    struct msqid_ds msqid_buf;

    /* Start measuring the time. */
    gettimeofday(&start_t, NULL);

    /* Error check if there are more or less arguments than required. */
    if ( argc != 14) 
    {
         fprintf(stderr, "Usage: ./producer_consumer <T> <B> <P> <C> <Rs_MIN> <Rs_MAX> <Pt_min> <Pt_max> <Ct1_min> <Ct1_max> <Ct2_min> <Ct2_max> <p_i>\n");
        return -1;
    }

    /* Error check if one of the arguments is negative. */
    if ( atoi(argv[1]) < 0 || atoi(argv[2]) < 0 || atoi(argv[3]) < 0 || atoi(argv[4]) < 0  || 
    	 atoi(argv[5]) < 0 || atoi(argv[6]) < 0 || atoi(argv[7]) < 0 || atoi(argv[8]) < 0  || 
    	 atoi(argv[9]) < 0 || atoi(argv[10]) < 0 || atoi(argv[11]) < 0 || atoi(argv[12]) < 0 || atoi(argv[13]) < 0) 
    { 
        fprintf(stderr, "Every argument must be a positive integer.\n");
        return -1;
    }
    
    /* Call the variable initializer. */
    initialize_globals(argv);

	/* Arbitrary value for key to initialize the message queue */ 
    key = 1234;

    /* Get the message queue ID for the given key. */
    if ((msqid = msgget(key, msgflg )) < 0)   
           die("msgget");

    /* 
       Get the values of structure filled in msqid_buf

       Set the buffer value of the message queue.
       So that Kernel never runs out of memory when 
       we set a bound on the queue buffer.

       Set the buffer values inside the msqid.
    */
    msgctl(msqid, IPC_STAT, &msqid_buf);
    msqid_buf.msg_qbytes =  buffer_size * sizeof(int);
    msgctl(msqid, IPC_SET, &msqid_buf);
    
    /* Generate P+C number of processes. 
       P being number of producers
       C being number of consumers
    */
    int i;
    for (i = 1; i <= num_pro + num_con; i++) 
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

    /* Some control operations for main process to complete. */
	if(pid)
	{  
        /* 
            Time measurement loop goes here. 
            When system is out of this loop, program will proceed to termination.
        */
		while ( time_spent != run_time )
		{
            /* 
                Add sleep equal to period number of seconds. 
                This prevents main process to unneccessarily eat up CPU time.
            */
			sleep(TIME_PERIOD_SECS);

            /* Calculate time spent in program after sleep. */
			gettimeofday(&end_t, NULL);
			time_spent = (int) time_calculation(start_t, end_t);

            /* Report number of requests completed in TIME_PERIOD_SECS seconds if time period has passed. */
			if (time_spent % TIME_PERIOD_SECS == 0 )
			{
				printf("CONSOLE MESSAGE: 10 Seconds period requests completed: %d\n", *periodic_req_count );
				*periodic_req_count = 0;
			}
   		}

        /* Print out messages which need to be reported upon program completion. */
        console();

        /* Delete the message queue upon exiting. */
        msgctl(msqid, IPC_RMID, (struct msqid_ds *) 0);

        /* Broadcast a termination signal for every child before exiting. */
        kill(0, SIGTERM);
	}
}