#ifndef __THREAD_CACHE_H__
#define __THREAD_CACHE_H__

#include "memory_pool.h"


namespace ekko{
class ThreadCache{
public:
    /// @brief allocate API
    void* Allocate(size_t n);

    /// @brief deallocate API
    void Deallocate(void *ptr, size_t size);

    ~ThreadCache();

private:
    FreeList free_list_[NLISTS];
};

thread_local static ThreadCache *thread_cache_ptr = 0;

}

#endif