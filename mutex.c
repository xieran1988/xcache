#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

void *functionC();
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
int  counter = 0;

sem_t sem;

void *proc1(void *_)
{
	sem_wait(&sem);
	printf("11\n");
	sleep(10);
	sem_wait(&sem);
	printf("12\n");
}

void *proc2(void *_)
{
	sem_post(&sem);
	sem_post(&sem);

	int val;
	sem_getvalue(&sem, &val);
	printf("%d\n", val);
	sem_post(&sem);
	sem_post(&sem);
}

main()
{
	int rc1, rc2;
	pthread_t thread1, thread2;

	sem_init(&sem, 0, 0);

	/* Create independent threads each of which will execute functionC */

	if( (rc1=pthread_create( &thread1, NULL, &proc1, NULL)) )
	{
		printf("Thread creation failed: %d\n", rc1);
	}

	if( (rc2=pthread_create( &thread2, NULL, &proc2, NULL)) )
	{
		printf("Thread creation failed: %d\n", rc2);
	}

	/* Wait till threads are complete before main continues. Unless we  */
	/* wait we run the risk of executing an exit which will terminate   */
	/* the process and all threads before the threads have completed.   */

	pthread_join( thread1, NULL);
	pthread_join( thread2, NULL); 

	exit(0);
}

void *functionC()
{
	pthread_mutex_lock( &mutex1 );
	counter++;
	printf("Counter value: %d\n",counter);
	pthread_mutex_unlock( &mutex1 );
}

