#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>

#define MAX_NUMBER 2000

int *buffer;
int run_time, buffer_size;

/* Variabled to implement bounded buffer. */
int front = 0;
int  end  = 0;

/* Mutex lock for thread synchronization within critical sections. */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* Function to start producer thread*/
void *producer(void  *params)
{
    srand(time(NULL));
    int i = 0;
    int num = 0;
    while ( i < N )
    {
        while(((front+1) % B) == end);

        num = rand() % MAX_NUMBER;
        buffer[front] = num;
       // printf("Produced : %d\n", num);
        pthread_mutex_lock(&mutex);
        front = (front + 1) % B;
        pthread_mutex_unlock(&mutex);

        i++;
    }
}

/* Function to start comsumer */
void *consumer(void *params)
{
    int i = 0;
    int num = 0;
    while ( i < N )
    {
        while (front == end);

        num = buffer[end];
       // printf("Consumed : %d\n", num);
        
        pthread_mutex_lock(&mutex);
        end = (end + 1) % Bdd;
        pthread_mutex_unlock(&mutex);

        i++;
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
	pthread_t producer_id, consumer_id;
    pthread_attr_t attr;

    struct timeval start_t, init_t, end_t;
    
    /* Start measuring time for initialization */
    gettimeofday(&start_t, NULL);

    /* Error check if there are more or less arguments than required*/
    if ( argc != 5) 
    {
        fprintf(stderr, "usage: producer_consumer <Number of integers> <Buffer Size>");
        return -1;
    }

    /* Error check if one of the arguments is negative.*/
    if ( atoi(argv[1]) < 0 || atoi(argv[2]) < 0 || atoi(argv[3]) < 0 || atoi(argv[4]) < 0 ) 
    { 
        fprintf(stderr, "Every argument must be arguments must be positive integers\n");
        return -1;
    }


    /* Extract the arguments to integers.*/
    run_time = atoi(argv[1]);
    buffer_size = atoi(argv[2]);

    /* Buffer allocation for shared memory between threads */
	buffer = malloc(buffer_size * sizeof(int));

    /* Thread attribute initialization */
    pthread_attr_init(&attr);

    /* Creating consumer and producer threads here*/
    pthread_create(&producer_id, &attr, producer, argv[1]);
    pthread_create(&consumer_id, &attr, consumer, argv[1]);

    /* Check point for initialization time measurement */
    gettimeofday(&init_t, NULL);

  
    // initialzation_t_usec = init_t_usec - start_t_usec;
    pthread_join(producer_id, NULL);
    pthread_join(consumer_id, NULL);

    /* checkpoint for threads ending. This will only execute when consumer will end */
    gettimeofday(&end_t, NULL);

    
    printf("%d , %d , %Lf , %Lf\n", N, B, 
           time_calculation(start_t, init_t) , 
           time_calculation(init_t, end_t));
}