#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define STOP 1000
#define BUF_SIZE 1024

int buffer[BUF_SIZE] = {0};
int acked = -1;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void * consumer(void * round) {
	
	int version = *((int *)round);
	if (version == STOP) return NULL;
	
	printf("Consumer round %d\n", version);
	
	/* read input */
	while (1) {
		pthread_mutex_lock(&mutex);
		
		int i, ver;
		for (i = 0; i < BUF_SIZE; i++)
			ver = buffer[i];
		
		printf("Consumer reads %d version of input\n", ver);
		
		/* new version of the input? */
		if (ver > acked) {
			acked = ver;
			break;
		}
		pthread_mutex_unlock(&mutex);
		//usleep(1);
	}
	
	pthread_mutex_unlock(&mutex);
	(*((int *)round))++;
	consumer(round);
	return NULL;
}

void * producer(void * round) {
	
	int version = *((int *)round);
	if (version == STOP) return NULL;
	
	printf("Producer round %d\n", version);
	
	/* wait until consumer read input */
	while (1) {
		pthread_mutex_lock(&mutex);
		if (version == acked) break;
		pthread_mutex_unlock(&mutex);
		//usleep(100);
	}
	
	int i;
	for (i = 0; i < BUF_SIZE; i++)
		buffer[i] = version + 1;
	
	printf("Producer writes %d version of input\n", version + 1);
	
	pthread_mutex_unlock(&mutex);
	(*((int *)round))++;
	producer(round);
	return NULL;
}



int main() {
	
	pthread_t thread1, thread2;
	
	int r1 = 0, r2 = 0;
	pthread_create(&thread1, NULL, consumer, &r1);
	pthread_create(&thread2, NULL, producer, &r2);
	
	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL);
	
	return 0;
}
