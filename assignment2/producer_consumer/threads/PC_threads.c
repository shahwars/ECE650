#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

/* Define to limit maximum size of integer generated. */
#define MAX_NUMBER                      10000

/* Time period after which number of requests are calculated. */
#define TIME_PERIOD_SECS                10

/* Global variables to contain program parameters and to manipulate them. */
int run_time, buffer_size, p_i, Pt_min, Pt_max, Ct1_min, Ct1_max, Ct2_min, Ct2_max, Rs_min, Rs_max;
int num_pro, num_con;

/* Some variables to keep track of program timing. */
int run_time, end_program = 0;
int req_num = 1;

/* Some static shared variables between producers and consumers to calculate requests. */
int num_req_consumed, periodic_req_count, producer_blocked, consumer_blocked;
long double producer_block_time, consumer_block_time; 

/* Buffer pointer to which will point to shared memory. */
struct request_int *bufferPtr;

/* Variables to implement bounded buffer. */
int front = 0;
int  end  = 0;

/* Mutex locks for thread synchronization. */
pthread_mutex_t mutex_p = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_c = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_req_num = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_request_rcv = PTHREAD_MUTEX_INITIALIZER;

/* 
    Basic unit of a request. This chunk consists of 1 integer in a request.
    In order to send multiple integers in request, we will have to send this
    chunk multiple times. 
*/
struct request_int
{
    int request_chunk;
    int request_number;
    int req_size;
};

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
}

/*
    Function to print console messages when the progam is about to terminate.
*/
void console(void)
{
        printf("CONSOLE MESSAGE: Total requests completed: %d\n", num_req_consumed );
        printf("CONSOLE MESSAGE: Number of times producers got blocked: %d\n", producer_blocked);
        printf("CONSOLE MESSAGE: Summation of producers block time: %Lf seconds\n", producer_block_time);
        printf("CONSOLE MESSAGE: Average block time per producer: %Lf seconds\n", producer_block_time/num_pro);
        printf("CONSOLE MESSAGE: Number of times consumers got blocked: %d\n", consumer_blocked);
        printf("CONSOLE MESSAGE: Summation of consumers block time: %Lf seconds\n", consumer_block_time);
        printf("CONSOLE MESSAGE: Average block time per consumer: %Lf seconds\n", consumer_block_time/num_con);
}

/* Function to start producer thread*/
void *producer(void * thread_num)
{
    while(!end_program)
    {
        /* 
            Producer is supposed to generate a delay at the start. 
            Or before generating a request. 
        */
        usleep(generate_Pt()*1000000);

        int R_S = generate_Rs();
        

        struct request_int *req = malloc(sizeof(struct request_int) * R_S);

        /* IMPORTANT: 
            Here we are going to write in the shared memory chunk by chunk. 
            Every request is broken into chunks. Every chunk contains information:
            1. integer
            2. request size
            3. request number
        */
        int r;
        for (r = 0; r < R_S; r++)
        {
            struct timeval start_p, end_p;
            int blocked = 0;
            long double block_time = 0.0;

            /* Prepare the chunk of request. */
            struct request_int temp;
            temp.request_chunk = rand() % MAX_NUMBER;
            temp.request_number = req_num;
            temp.req_size = R_S;

            /* Start measuring time because we want to measure time everytime a consumer is blocked. */
            gettimeofday(&start_p, NULL);

            /* Check if the buffer is already full.*/
            while(((front+1) % buffer_size) == end)
            {
                /* Set a blocked flag if the consumer os blocked. */
                blocked = 1;
            }

            /* As soon as producer is out of block state, temp chunk is written into the memory. */
            req[r] = temp;

            if (blocked)
            {
                /* Calculate time again only of producer was blocked. */
                gettimeofday(&end_p, NULL);
                block_time = time_calculation(start_p, end_p);
            }

            bufferPtr[front] = req[r];
            pthread_mutex_lock(&mutex_p);
            front = (front + 1) % buffer_size;
            producer_block_time = producer_block_time + block_time;
            producer_blocked = producer_blocked + blocked;
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
        struct request_int chunk = bufferPtr[end];
        int R_S = chunk.req_size;

        /* Allocate some memory to receive */
        struct request_int *received_req = malloc(sizeof(struct request_int) * R_S);
        
        int r;
        for (r = 0; r < R_S; r++)
        {
            int blocked = 0;
            long double block_time = 0.0;
            struct timeval start_c, end_c;

            /* Start measuring time because we want to measure time everytime a consumer is blocked. */
            gettimeofday(&start_c, NULL);
            while (front == end)
            {
                /* Set a blocked flag if the consumer os blocked. */
                blocked = 1;
            }

            /* Right after consumer gets unblocked receive the chunk. */
            struct request_int temp;
            temp.request_chunk = bufferPtr[end].request_chunk;
            temp.request_number = bufferPtr[end].request_number;
            temp.req_size = bufferPtr[end].req_size;

            /* Place the chunk in the request assebler. */
            received_req[r] = temp;

            if (blocked)
            {
                /* Calculate time again only of consumer was blocked. */
                gettimeofday(&end_c, NULL);
                block_time = time_calculation(start_c, end_c);
            }

            pthread_mutex_lock(&mutex_c);
            end = (end + 1) % buffer_size;
            consumer_block_time = consumer_block_time + block_time;
            consumer_blocked = consumer_blocked + blocked;
            pthread_mutex_unlock(&mutex_c);
        }

        /* Delay which models the time for which received request is being processed. */
        usleep(select_Ct1_Ct2(generate_Ct1(), generate_Ct2())*1000000);

        /* 
            Accumulate blocking time and number of blocks calculation. 
            This is done under mutexes because these are shared variables.
        */
        pthread_mutex_lock(&mutex_request_rcv);
        periodic_req_count++;
        num_req_consumed++;
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

    /* Initialize all the variables. */
    initialize_globals(argv);

    /* 
        Buffer allocation for shared memory between threads
        As we are receiving buffer_size as number of integers. 
        Also, struct request_int only sends 1 integer. 

        Therefore buffer_size * sizeof(struct request_int)
        sends buffer_size number of integers. 
    */
    bufferPtr = malloc(buffer_size * sizeof(struct request_int));

    
    /* Thread attribute initialization */
    pthread_attr_init(&attr);

    /* Creating consumer and producer threads here*/

    long i = 0;
    while ( i++ < num_pro)
        pthread_create(&producer_id, &attr, producer, (void *)i);

    i = 0;
    while ( i++ < num_con )
        pthread_create(&consumer_id, &attr, consumer, (void *)i);

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
            printf("CONSOLE MESSAGE: 10 Seconds period requests completed: %d\n", periodic_req_count );
            periodic_req_count = 0;
        }
    }

    /*Set global end program variable to kill every thread. */
    end_program = 1;

    /* Print out results. */
    console();

}