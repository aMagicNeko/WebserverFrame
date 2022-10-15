#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__


#include <pthread.h>
#include <memory.h>
#include <malloc.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

namespace ekko{
struct thread_pool_t;

struct worker_t {
    struct worker_t *next;
    pthread_t thread;
    struct thread_pool_t *threadPoolPtr;
    int isTerm;
};

struct task_t {
    struct task_t *next;
    void (*func)(void*);
    void* arg;
};

struct thread_pool_t {
    struct worker_t *workers;
    struct task_t *tasks;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};


int thread_pool_init(struct thread_pool_t *threadPoolPtr, int numThreads);

int thread_pool_push_task(struct thread_pool_t *threadPoolPtr, void (*func)(void*), void *arg);

void thread_pool_destroy(struct thread_pool_t *threadPoolPtr);

}
#endif