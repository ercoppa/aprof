#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>

#define DEBUG     0
#define STOP      10000
#define BUF_SIZE  32
#define RECURSIVE 1

int buffer[BUF_SIZE] = {0};
int acked = -1;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void * consumer(void * round) {

    int version;
    
    #if RECURSIVE == 0
again:
    #endif
    
    version = *((int *)round);
    if (version == STOP) return NULL;
    
    #if DEBUG
    //printf("Consumer round %d\n", version);
    #endif
    
    #if DEBUG
    pthread_mutex_lock(&mutex);
    int old_acked = acked;
    pthread_mutex_unlock(&mutex);
    #endif
    
    /* read input */
    while (1) {
        
        usleep(10);
        pthread_mutex_lock(&mutex);
        
        #if DEBUG
        assert(acked == old_acked);
        #endif
        
        int i, ver;
        for (i = 0; i < BUF_SIZE; i++) {
            
            #if DEBUG
            if (i > 0) assert(ver == buffer[i]);
            assert(acked == old_acked);
            #endif
            
            ver = buffer[i];
        }
        
        #if DEBUG
        //printf("Consumer reads %d version of input\n", ver);
        assert(acked == old_acked);
        assert(ver == acked || ver == acked + 1);
        #endif
        
        /* new version of the input? */
        if (ver > acked) {
            acked = ver;
            break;
        }
        pthread_mutex_unlock(&mutex);
        
    }
    
    #if DEBUG
    assert(acked > old_acked);
    #endif
    
    pthread_mutex_unlock(&mutex);
    (*((int *)round))++;
    
    #if RECURSIVE
    consumer(round);
    #else
    goto again;
    #endif
    
    return NULL;
}

void * producer(void * round) {
    
    int version;
    
    #if RECURSIVE == 0
again2:
    #endif
    
    version = *((int *)round);
    if (version == STOP) return NULL;
    
    #if DEBUG
    //printf("Producer round %d\n", version);
    #endif
    
    /* wait until consumer reads input */
    while (1) {
        usleep(10);
        pthread_mutex_lock(&mutex);
        if (version == acked) break;
        //printf("acked %d ~ version %d\n", acked, version);
        assert(version -1 == acked);
        pthread_mutex_unlock(&mutex);
    }
    
    int i;
    for (i = 0; i < BUF_SIZE; i++)
        buffer[i] = version + 1;
    
    #if DEBUG
    //printf("Producer writes %d version of input\n", version + 1);
    #endif
    
    pthread_mutex_unlock(&mutex);
    (*((int *)round))++;
    
    #if RECURSIVE
    producer(round);
    #else
    goto again2;
    #endif
    
    return NULL;
}



int main() {
    
    pthread_t thread1, thread2;
    
    int i;
    for (i = 0; i < BUF_SIZE; i++)
        buffer[i] = 0;
    
    int r1 = 0, r2 = 0;
    pthread_create(&thread1, NULL, consumer, &r1);
    pthread_create(&thread2, NULL, producer, &r2);
    
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    
    return 0;
}
