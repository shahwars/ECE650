#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>

int *buffer;
int N, B;

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
    while ( i<N )
    {
        while(((front+1) % B) == end);

        buffer[front] = rand();

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
    while ( i<N )
    {
        while (front == end);

        num = buffer[end];
        //printf("\n%d\n", buffer[end]);
        pthread_mutex_lock(&mutex);

        end = (end + 1) % B;
        pthread_mutex_unlock(&mutex);

        i++;
    }
}

int main(int argc, char *argv[])
{
	pthread_t producer_id, consumer_id;
    pthread_attr_t attr;

    struct timeval start_t, init_t, end_t;
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
    if ( atoi(argv[1]) < 0 || atoi(argv[2]) < 0) 
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

    /* Buffer allocation for shared memory between threads */
	buffer = malloc(B * sizeof(int));

    /* Thread attribute initialization */
    pthread_attr_init(&attr);

    /* Creating consumer and producer threads here*/
    pthread_create(&producer_id, &attr, producer, argv[1]);
    pthread_create(&consumer_id, &attr, consumer, argv[1]);

    /* Check point for initialization time measurement */
    gettimeofday(&init_t, NULL);

    /* timing extraction from the data structure */
    start_t_usec = (long double) (start_t.tv_sec * 1000000 + start_t.tv_usec);
    init_t_usec = (long double) (init_t.tv_sec * 1000000 + init_t.tv_usec);

    /* Keep the initialization time as a difference */
    initialzation_t_usec = init_t_usec - start_t_usec;
    pthread_join(producer_id, NULL);
    pthread_join(consumer_id, NULL);

    /* checkpoint for threads ending */
    gettimeofday(&end_t, NULL);

    init_t_usec = (long double) (init_t.tv_sec * 1000000 + init_t.tv_usec);
    end_t_usec = (long double) (end_t.tv_sec * 1000000 + end_t.tv_usec);

    transmission_t_usec = end_t_usec - init_t_usec;

    printf("Initialization time = %Lf in seconds\n", (initialzation_t_usec / 1000000));
    printf("Time to transfer data = %Lf seconds\n", (transmission_t_usec / 1000000));

}