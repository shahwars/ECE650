#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#define MAX_REQUEST_SIZE 10
#define MIN_REQUEST_SIZE 1
#define MAX_PRODUCER_DELAY_CONST 200
#define MIN_PRODUCER_DELAY_CONST 10
#define MAX_CONSUMER_DELAY_CONST 200
#define MIN_CONSUMER_DELAY_CONST 10
#define MAX_NUMBER 10000

struct request_int
{
    int request_chunk;
    int request_number;
    int req_size;
};

struct request_int *bufferPtr;
int run_time, end_program = 0;


int req_num = 1;
int p_i;

/* Variabled to implement bounded buffer. */
int front = 0;
int  end  = 0;

int buffer_size;

pthread_mutex_t mutex_p = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_c = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_req_num = PTHREAD_MUTEX_INITIALIZER;
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

/* Function to start producer thread*/
void *producer(void * thread_num)
{
	int i = 1;
    while(!end_program)
    {
    	usleep(generate_Pt()*1000000);
    	int R_S = generate_Rs();
    	struct request_int *req = malloc(sizeof(struct request_int) * R_S);

        for (int r = 0; r < R_S; r++)
        {
        	while(((front+1) % buffer_size) == end);

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
        pthread_mutex_lock(&mutex_req_num);
        req_num++;
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
        usleep(Ct*1000000);

        struct request_int chunk = bufferPtr[end];

        running_size = chunk.req_size;

		struct request_int *received_req = malloc(sizeof(struct request_int) * running_size);

        for (int r = 0; r < running_size; r++)
        {

        	while (front == end);

        	struct request_int temp;
        	temp.request_chunk = bufferPtr[end].request_chunk;
        	temp.request_number = bufferPtr[end].request_number;
        	temp.req_size = bufferPtr[end].req_size;

            received_req[r] = temp;

            pthread_mutex_lock(&mutex_c);
        	end = (end + 1) % buffer_size;
        	pthread_mutex_unlock(&mutex_c);
        }
        printf("Consumed request number : %d, size = %d\n", received_req[1].request_number, received_req[1].req_size);
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
    if ( argc != 6) 
    {
        fprintf(stderr, "Usage: producer_consumer <T:Run Time> <B: Buffer Size> <P: Producers> <C: Consumers> <p_i in percent>\n");
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
    	gettimeofday(&end_t, NULL);
    	time_spent = (int) time_calculation(start_t, end_t);
   // 	time_spent = (int)(clock() - begin) / CLOCKS_PER_SEC;
    }

  
    end_program = 1;

}