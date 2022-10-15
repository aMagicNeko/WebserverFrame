#ifndef __THREAD_CACHE_H__
#define __THREAD_CACHE_H__

#include "memory_pool.h"


namespace ekko{
class thread_cache{
private:
    free_list freeList[NLISTS];

public:
    /// @brief allocate API
    void* allocate(size_t n);

    /// @brief deallocate API
    void deallocate(void *ptr, size_t size);

    ~thread_cache();
};

thread_local static thread_cache *threadCachePtr = 0;

}

#endif