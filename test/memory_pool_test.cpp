#include "memory_pool.h"
#include "thread_cache.h"
#include <iostream>
#include <vector>
#include <pthread.h>
#include <unistd.h>
#define SIZE 16
#define NOBJS 10000

void*
_test (void*)
{
	sleep(10);
	void* arr[NOBJS];
	for (int i = 0; i < NOBJS; ++i) {
		arr[i] = ekko::threadCachePtr->allocate(SIZE);
		*(int*)arr[i] = i;
	}
	void *res = malloc(sizeof(long long));
	for (int i = 0; i < NOBJS; ++i) {
		*(int*) res += *(int*) arr[i];
		ekko::threadCachePtr->deallocate(arr[i], SIZE);
	}
	printf("done\n");
	return res;
}

void
test(size_t npthreads)
{
	pthread_t threadId[npthreads];
	for (int i = 0; i < npthreads; ++i)
		pthread_create(threadId+i, 0, _test, 0);
	int res = 0;
	for (pthread_t x:threadId) {
		void ** tmp = 0;
		pthread_join(x, tmp);
		res += (long long) (*tmp);
	}
	printf("res: %d", res);
}

int main()
{
	void* arr[NOBJS];
	ekko::threadCachePtr = new ekko::thread_cache;
	for (int i = 0; i < NOBJS; ++i) {
		arr[i] = ekko::threadCachePtr->allocate(SIZE);
		*(int*)arr[i] = i;
	}
	void *res = malloc(sizeof(long long));
	for (int i = 0; i < NOBJS; ++i) {
		*(int*) res += *(int*) arr[i];
		ekko::threadCachePtr->deallocate(arr[i], SIZE);
	}
	printf("done\n");

}