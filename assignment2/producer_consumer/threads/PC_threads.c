#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>


#define MAX_NUMBER 10000
#define TIME_PERIOD_SECS 10

int run_time, p_i, Pt_min, Pt_max, Ct1_min, Ct1_max, Ct2_min, Ct2_max, Rs_min, Rs_max;


struct request_int *bufferPtr;
int run_time, end_program = 0;

int num_req_consumed, periodic_req_count, producer_blocked, consumer_blocked;
long double producer_block_time, consumer_block_time; 


int req_num = 1;
int p_i;

/* Variabled to implement bounded buffer. */
int front = 0;
int  end  = 0;

int buffer_size;

pthread_mutex_t mutex_p = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_c = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_req_num = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_request_rcv = PTHREAD_MUTEX_INITIALIZER;

struct request_int
{
    int request_chunk;
    int request_number;
    int req_size;
};

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

long double time_calculation(struct timeval start_t, struct timeval end_t)
{
    /* timing extraction from the data structure */
    long double start_t_usec = (long double) (start_t.tv_sec * 1000000 + start_t.tv_usec);
    long double end_t_usec = (long double) (end_t.tv_sec * 1000000 + end_t.tv_usec);

    /* Keep the initialization time as a difference */
    return (end_t_usec - start_t_usec)/1000000;
}

/* Function to start producer thread*/
void *producer(void * thread_num)
{
    while(!end_program)
    {
    	usleep(generate_Pt()*1000000);
    	int R_S = generate_Rs();
    	struct timeval start_p, end_p;
        int blocked = 0;
    	long double block_time = 0.0;

    	struct request_int *req = malloc(sizeof(struct request_int) * R_S);


    	gettimeofday(&start_p, NULL);
        int r;
        for (r = 0; r < R_S; r++)
        {
        	while(((front+1) % buffer_size) == end)
        	{
        		blocked = 1;
        	}

        	struct request_int temp;

        	temp.request_chunk = rand() % MAX_NUMBER;
        	temp.request_number = req_num;
        	temp.req_size = R_S;

        	req[r] = temp;

       		bufferPtr[front] = req[r];
       		pthread_mutex_lock(&mutex_p);
        	front = (front + 1) % buffer_size;
        	pthread_mutex_unlock(&mutex_p);
        }

        if (blocked)
        {
        	gettimeofday(&end_p, NULL);
        	block_time = time_calculation(start_p, end_p);
        }
        pthread_mutex_lock(&mutex_req_num);
        req_num++;
        producer_block_time = producer_block_time + block_time;
       	producer_blocked = producer_blocked + blocked;
        pthread_mutex_unlock(&mutex_req_num);
        printf("Produced request number : %d, size = %d\n", req[0].request_number, req[0].req_size);
    }
}

/* Function to start comsumer */
void *consumer(void * thread_num)
{
    while(!end_program)
    {
    	int running_size;
        long double Ct = select_Ct1_Ct2(generate_Ct1(), generate_Ct2());
        struct timeval start_c, end_c;
        int blocked = 0;
    	long double block_time = 0.0;
        usleep(Ct*1000000);

        struct request_int chunk = bufferPtr[end];

        running_size = chunk.req_size;

		struct request_int *received_req = malloc(sizeof(struct request_int) * running_size);

		gettimeofday(&start_c, NULL);
        int r;
        for (r = 0; r < running_size; r++)
        {
        	while (front == end)
        	{
        		blocked = 1;
        	}

        	struct request_int temp;
        	temp.request_chunk = bufferPtr[end].request_chunk;
        	temp.request_number = bufferPtr[end].request_number;
        	temp.req_size = bufferPtr[end].req_size;

            received_req[r] = temp;

            pthread_mutex_lock(&mutex_c);
        	end = (end + 1) % buffer_size;
        	pthread_mutex_unlock(&mutex_c);
        }

        if (blocked)
        {
        	gettimeofday(&end_c, NULL);
        	block_time = time_calculation(start_c, end_c);
        }

        pthread_mutex_lock(&mutex_request_rcv);
        periodic_req_count++;
        num_req_consumed++;
        consumer_block_time = consumer_block_time + block_time;
       	consumer_blocked = consumer_blocked + blocked;
        pthread_mutex_unlock(&mutex_request_rcv);
        printf("Consumed request number : %d, size = %d\n", received_req[0].request_number, received_req[0].req_size);
    }
}

int main(int argc, char *argv[])
{
	pthread_t producer_id, consumer_id;
    pthread_attr_t attr;

	struct timeval start_t,  end_t;

	int time_spent  = 0;
    int num_pro, num_con;

	gettimeofday(&start_t, NULL);

    srand(time(NULL));

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

    num_req_consumed = 0;
    periodic_req_count = 0;
    producer_blocked = 0;
    consumer_blocked = 0;
    producer_block_time = 0.0;
    consumer_block_time = 0.0;

    /* Buffer allocation for shared memory between threads */
	bufferPtr = malloc(buffer_size * sizeof(struct request_int));

    
    printf("starting program %lu\n", sizeof(bufferPtr));
    /* Thread attribute initialization */
    pthread_attr_init(&attr);

    /* Creating consumer and producer threads here*/

    long i = 0;
    while ( i++ < num_pro)
       	pthread_create(&producer_id, &attr, producer, (void *)i);

    i = 0;
    while ( i++ < num_con )
    	pthread_create(&consumer_id, &attr, consumer, (void *)i);

    while ( time_spent != run_time )
	{
		sleep(TIME_PERIOD_SECS);
		gettimeofday(&end_t, NULL);
		time_spent = (int) time_calculation(start_t, end_t);

		if (time_spent % TIME_PERIOD_SECS == 0 )
		{
			printf("CONSOLE MESSAGE: 10 Seconds period requests completed: %d\n", periodic_req_count );
			periodic_req_count = 0;
		}
   	}
    end_program = 1;
    printf("CONSOLE MESSAGE: Total requests completed: %d\n", num_req_consumed );
        printf("CONSOLE MESSAGE: Number of times producers got blocked: %d\n", producer_blocked);
        printf("CONSOLE MESSAGE: Number of times consumers got blocked: %d\n", consumer_blocked);
        printf("CONSOLE MESSAGE: Summation of producers block time: %Lf seconds\n", producer_block_time);
        printf("CONSOLE MESSAGE: Average block time per producer: %Lf seconds\n", producer_block_time/num_pro);
        printf("CONSOLE MESSAGE: Summation of consumers block time: %Lf seconds\n", consumer_block_time);
        printf("CONSOLE MESSAGE: Average block time per consumer: %Lf seconds\n", consumer_block_time/num_con);

}