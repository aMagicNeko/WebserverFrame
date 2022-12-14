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
	ekko::thread_cache_ptr = new ekko::ThreadCache;
	void* arr[NOBJS];
	for (int i = 0; i < NOBJS; ++i) {
		arr[i] = ekko::thread_cache_ptr->Allocate(SIZE);
		*(int*)arr[i] = i;
	}
	long long *res = (long long*) malloc(sizeof(long long));
	*res = 0;
	for (int i = 0; i < NOBJS; ++i) {
		*res += *(int*) arr[i];
		ekko::thread_cache_ptr->Deallocate(arr[i], SIZE);
	}
	delete ekko::thread_cache_ptr;
	printf("done \n");

	pthread_exit(res);
}

void
test(size_t npthreads)
{
	pthread_t threadId[npthreads];
	for (int i = 0; i < npthreads; ++i)
		pthread_create(threadId+i, 0, _test, 0);
	long long res = 0;
	for (pthread_t x:threadId) {
		void *tmp = 0;
		pthread_join(x, &tmp);
		res += *((long long*) (tmp));
	}
	printf("res: %d", res);
}

int main()
{
	test(10);

}