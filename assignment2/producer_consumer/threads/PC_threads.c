#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>



struct request_int
{
    int request_chunk;
    int request_number;
};

struct request_int *bufferPtr;

/* Function to start producer thread*/
void *producer(int params)
{
    // srand(time(NULL));
    // int i = 0;
    // int num = 0;
    // while ( i < N )
    // {
    //     while(((front+1) % B) == end);

    //     num = rand() % MAX_NUMBER;
    //     buffer[front] = num;
    //    // printf("Produced : %d\n", num);
    //     pthread_mutex_lock(&mutex);
    //     front = (front + 1) % B;
    //     pthread_mutex_unlock(&mutex);

    //     i++;
    // }

    //while(1)
    printf("Producer %d created", params);
}

/* Function to start comsumer */
void *consumer(int params)
{
    // int i = 0;
    // int num = 0;
    // while ( i < N )
    // {
    //     while (front == end);

    //     num = buffer[end];
    //    // printf("Consumed : %d\n", num);
        
    //     pthread_mutex_lock(&mutex);
    //     end = (end + 1) % Bdd;
    //     pthread_mutex_unlock(&mutex);

    //     i++;
    // }
    //while(1)
    printf("Consumer %d created", params);
}


int main(int argc, char *argv[])
{
	pthread_t producer_id, consumer_id;
    pthread_attr_t attr;

    int run_time, buffer_size, num_pro, num_con;

    /* Error check if there are more or less arguments than required*/
    if ( argc != 5) 
    {
        fprintf(stderr, "usage: producer_consumer <T:Run Time> <B: Buffer Size> <P: Producers> <C: Consumers> \n");
        return -1;
    }

    /* Error check if one of the arguments is negative.*/
    if ( atoi(argv[1]) < 0 || atoi(argv[2]) < 0 || atoi(argv[3]) < 0 || atoi(argv[4]) < 0 ) 
    { 
        fprintf(stderr, "Every argument must be a positive integer.\n");
        return -1;
    }

    /* Extract the arguments to integers.*/
    run_time = atoi(argv[1]);
    buffer_size = atoi(argv[2]);
    num_pro = atoi(argv[3]);
    num_con = atoi(argv[4]);

    /* Buffer allocation for shared memory between threads */
	bufferPtr = malloc(buffer_size * sizeof(struct request_int));


    /* Thread attribute initialization */
    pthread_attr_init(&attr);

    /* Creating consumer and producer threads here*/

    int i = 0;
    while ( i < num_pro)
    {
    	pthread_create(&producer_id, &attr, producer, i);
    	thread_join(producer_id, NULL);
    }

    i = 0;
    while ( i < num_con )
    {
    	pthread_create(&consumer_id, &attr, consumer, i);
    	pthread_join(consumer_id, NULL);
    }

    /* Check point for initialization time measurement */
    // gettimeofday(&init_t, NULL);

  
    // initialzation_t_usec = init_t_usec - start_t_usec;
    // pthread_join(producer_id, NULL);
    // pthread_join(consumer_id, NULL);

    /* checkpoint for threads ending. This will only execute when consumer will end */
    // gettimeofday(&end_t, NULL);

    
    // printf("%d , %d , %Lf , %Lf\n", N, B, 
    //        time_calculation(start_t, init_t) , 
    //        time_calculation(init_t, end_t));
}