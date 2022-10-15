#include "thread_pool.h"

namespace ekko{
void*
thread_callback(void *arg)
{
    struct worker_t *workerPtr = (struct worker_t*) arg;
    struct task_t *taskPtr;
    while (1) {
        pthread_mutex_lock(&workerPtr->threadPoolPtr->mutex);
        while (workerPtr->threadPoolPtr->tasks == 0 && workerPtr->isTerm == 0)
            pthread_cond_wait(&workerPtr->threadPoolPtr->cond, &workerPtr->threadPoolPtr->mutex);
        taskPtr = workerPtr->threadPoolPtr->tasks;
        if (taskPtr == 0) {
            pthread_mutex_unlock(&workerPtr->threadPoolPtr->mutex);
            pthread_exit(0);
        }
        workerPtr->threadPoolPtr->tasks = taskPtr->next;
        pthread_mutex_unlock(&workerPtr->threadPoolPtr->mutex);

        taskPtr->func(taskPtr->arg);
        free(taskPtr);
    }
}

/**
 * @return the number of threads in the pool
*/
int
thread_pool_init(struct thread_pool_t *threadPoolPtr, int numThreads)
{
    int idx = 0;
    struct worker_t* workerPtr;
    pthread_t threadId;

    if (numThreads < 1)
        numThreads = 1;
    memset(threadPoolPtr, 0, sizeof(struct thread_pool_t));

    pthread_mutex_init(&threadPoolPtr->mutex, 0);
    pthread_cond_init(&threadPoolPtr->cond, 0);

    for ( ; idx < numThreads; ++idx) {

        workerPtr = (struct worker_t*) malloc(sizeof(struct worker_t));
        if (workerPtr == 0) {
            perror("malloc error in thread_pool_init\n");
            return idx;
        }

        if ( pthread_create(&threadId, 0, thread_callback, workerPtr) != 0) {
            perror("pthread_create error in thread_pool_init\n");
            free(workerPtr);
            return idx;
        }

        workerPtr->isTerm = 0;
        workerPtr->threadPoolPtr = threadPoolPtr;
        workerPtr->thread = threadId;
        workerPtr->next = threadPoolPtr->workers;
        threadPoolPtr->workers = workerPtr;
    }

    return idx;
}

/**
 * @param arg pointer to our dynamic-cache, you should do free job yourself after task completed
 * @return success with 0, fail with -1
*/
int
thread_pool_push_task(struct thread_pool_t *threadPoolPtr, void (*func)(void*), void *arg)
{
    struct task_t *taskPtr = (struct task_t*) malloc(sizeof(struct task_t));
    if (taskPtr == 0) {
        perror("thread_pool_push_task error: malloc error\n");
        return -1;
    }
    taskPtr->arg = arg;
    taskPtr->func = func;
    taskPtr->next = 0;
    pthread_mutex_lock(&threadPoolPtr->mutex);
    taskPtr->next = threadPoolPtr->tasks;
    threadPoolPtr->tasks = taskPtr;
    pthread_cond_signal(&threadPoolPtr->cond);
    pthread_mutex_unlock(&threadPoolPtr->mutex);
    return 0;
}

void
thread_pool_destroy(struct thread_pool_t *threadPoolPtr)
{
    struct worker_t *workerPtr, *workerPrePtr;
    struct task_t *taskPtr, *taskPrePtr;
    for (workerPtr = threadPoolPtr->workers; workerPtr; workerPtr = workerPtr->next)
        workerPtr->isTerm = 1;
    pthread_mutex_lock(&threadPoolPtr->mutex);
    pthread_cond_broadcast(&threadPoolPtr->cond);
    pthread_mutex_unlock(&threadPoolPtr->mutex);

    for (workerPtr = threadPoolPtr->workers; workerPtr; ) {
        pthread_join(workerPtr->thread, 0);
        workerPrePtr = workerPtr;
        workerPtr = workerPtr->next;
        free(workerPrePtr);
    }

}
}