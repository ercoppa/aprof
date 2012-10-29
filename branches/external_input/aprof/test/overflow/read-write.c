#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define STOP 10

int x = 0, y = 0, z = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void do_work(int ric, int id) {
	
	if (ric == STOP) return;
	
	pthread_mutex_lock(&mutex);
	int q = x;
	x++;
	q++;
	printf("[%d] x = %d\n", id, q);
	pthread_mutex_unlock(&mutex);

	usleep(100);
	
	do_work(ric + 1, id);
	
}

void * worker(void * arg) {
	
	int * id = (int *) arg;
	do_work(0, *id);
	return NULL;
}

int main() {
	
	pthread_t thread1, thread2;
	
	int id1 = 1, id2 = 2;
	pthread_create(&thread1, NULL, worker, &id1);
	pthread_create(&thread2, NULL, worker, &id2);
	
	pthread_join( thread1, NULL);
	pthread_join( thread2, NULL);
	
	return 0;
}
