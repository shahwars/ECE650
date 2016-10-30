#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>

#define MAX_REQUEST_SIZE 10
#define MIN_REQUEST_SIZE 1
#define MAX_PRODUCER_DELAY_CONST 200
#define MIN_PRODUCER_DELAY_CONST 10
#define MAX_CONSUMER_DELAY_CONST 200
#define MIN_CONSUMER_DELAY_CONST 10
#define MAX_NUMBER 10000

int run_time, p_i;
int buffer_size;

int end_program = 0;
struct request_int
{
    int request_chunk;
    int request_number;
    int req_size;
};

int generate_Rs()
{
    int Rs = (rand() % MAX_REQUEST_SIZE) + MIN_REQUEST_SIZE;
	return Rs;
}

long double generate_Pt()
{  
	int Pt = ( rand() % MAX_PRODUCER_DELAY_CONST ) + MIN_PRODUCER_DELAY_CONST;
	return Pt/100.0;
}

long double generate_Ct1 ()
{
	int Ct1 = ( rand() % MAX_CONSUMER_DELAY_CONST ) + MIN_CONSUMER_DELAY_CONST;
	return Ct1/100.0;
}

long double generate_Ct2()
{
	int Ct2 = ( rand() % MAX_CONSUMER_DELAY_CONST ) + MIN_CONSUMER_DELAY_CONST;
	return Ct2/100.0;
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

void producer(int producer_num)
{
    while (!end_program)
    {
		printf("Producer %d ep= %d\n", producer_num, end_program);

	}
}

void consumer(int consumer_num)
{
	while (!end_program)
	{
		printf("Consumer %d\n", consumer_num);
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

int main(int argc, char *argv[])
{
    pid_t pid;
    srand(time(NULL));

    int num_pro, num_con, time_spent;

    struct timeval start_t, end_t;

    gettimeofday(&start_t, NULL);
    // long double interval_usec = 0.0;
    // long double initialzation_t_usec, transmission_t_usec, init_t_usec, start_t_usec, end_t_usec;

   /* Start measuring time for initialization */
    // gettimeofday(&start_t, NULL);

    /* Error check if there are more or less arguments than required*/
    if ( argc != 6) 
    {
        fprintf(stderr, "usage: producer_consumer <Number of integers> <Buffer Size>");
        return -1;
    }

    /* Error check if one of the arguments is negative.*/
    if ( atoi(argv[1]) < 0 || atoi(argv[2]) < 0 || atoi(argv[3]) < 0 || atoi(argv[4]) < 0  || atoi(argv[5]) < 0 ) 
    { 
        fprintf(stderr, "Every argument must be a positive integer.\n");
        return -1;
    }

    /* Extract the arguments to integers.*/
    run_time = atoi(argv[1]);
    buffer_size = atoi(argv[2]);
    num_pro = atoi(argv[3]);
    num_con = atoi(argv[4]);
    p_i = atoi(argv[5]);

    
    for (int i = 1; i <= num_pro + num_con; i++) 
    {
	    pid = fork();
	    if (pid) 
	    {
	        while ( time_spent != run_time )
		    {
		    	gettimeofday(&end_t, NULL);
		    	time_spent = (int) time_calculation(start_t, end_t);
		    	printf("time_spent = %d\n", time_spent);
		    }

            return 0;
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
}