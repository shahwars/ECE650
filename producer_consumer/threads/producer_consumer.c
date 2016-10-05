#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>


void *producer(void *params);
void *consumer(void *params);

int *buffer;
int N, B;

int front = 0;
int  end  = 0;

int main(int argc, char *argv[])
{
	pthread_t producer_id, consumer_id;
    pthread_attr_t attr;

    struct timeval start_t, end_t;
    long double interval_usec = 0.0;
    long double start_t_usec, end_t_usec;

    if ( argc != 3) 
    {
        fprintf(stderr, "usage: producer_consumer <Number of integers> <Buffer Size>");
        return -1;
    }

    if ( atoi(argv[1]) < 0 || atoi(argv[2]) < 0) 
    { 
        fprintf(stderr, "Both arguments must be positive integers\n");
        return -1;
    }

    if ( atoi(argv[1]) < atoi(argv[2]) ) 
    {
        fprintf(stderr, "Number of integers must be greater than buffer size.\n");
        return -1;
    }


    N = atoi(argv[1]);
    B = atoi(argv[2]);

	buffer = malloc(B * sizeof(int));

    pthread_attr_init(&attr);

    // Creating a thread here
    pthread_create(&producer_id, &attr, producer, argv[1]);
    pthread_create(&consumer_id, &attr, consumer, argv[1]);

    gettimeofday(&start_t, NULL);
    pthread_join(producer_id, NULL);
    pthread_join(consumer_id, NULL);
    gettimeofday(&end_t, NULL);


   start_t_usec = (long double) (start_t.tv_sec * 1000000 + start_t.tv_usec);
   end_t_usec = (long double) (end_t.tv_sec * 1000000 + end_t.tv_usec);

   interval_usec = end_t_usec - start_t_usec;

    printf("Initialization time = %Lf in seconds\n", (start_t_usec / 1000000));
    printf("Time to transfer data = %Lf seconds\n", (interval_usec / 1000000));

}

void *producer(void  *params)
{
	srand(time(NULL));
	int i = 0;
    while ( i<N )
    {
        while(((front+1) % B) == end);

        buffer[front] = rand() ;
        front = (front + 1) % B;
        i++;
    }
}

void *consumer(void *params)
{
	int i = 0;
	int num = 0;
	while ( i<N )
	{
		while (front == end);

        num = buffer[end];
		printf("\n%d\n", buffer[end]);
		end = (end + 1) % B;
		i++;
	}
}