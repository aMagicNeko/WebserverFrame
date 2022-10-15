#include "thread_cache.h"
#include "central_cache.h"
#include "page_cache.h"

namespace ekko{

void*
thread_cache::allocate(size_t objSize)
{
    if (objSize > (NPAGES<<PAGE_SHIFT))
        //too big
        return 0;
    
    if (objSize > MAX_BYTES_FROM_CENTRAL_CACHE) {
        //directly get from page cache
        return (void*) (page_cache::get_instance()->allocate((objSize + PAGE_SIZE - 1) >> PAGE_SHIFT)->pageID << PAGE_SHIFT);
    }

    size_t index;

    index = get_list_index(objSize);
    if (freeList[index].empty()) {
        //refill the free list
        void *start, *last;
        size_t batchSize = central_cache::get_instance()->batch_allocate(objSize, start, last);
        freeList[index].push_front(start, last, batchSize);
    }
        
    return freeList[index].pop_front();
}

void
thread_cache::deallocate(void *ptr, size_t size)
{
    size = roundup_size(size);

    if (size > MAX_BYTES_FROM_CENTRAL_CACHE) {
        //directly release into page_cache
        page_cache *pageCachePtr = page_cache::get_instance();
        span *spanPtr = pageCachePtr->pageID_to_span(((pageID_t) ptr) >> PAGE_SHIFT);
        pageCachePtr->deallocate(spanPtr);
        return;
    }

    size_t index, max_size;

    index = get_list_index(size);
    freeList[index].push_front(ptr);
    
    max_size = get_batch_size_from_central_cache(size) << 1;
    if (freeList[index].size() > max_size)
        //list too long, return to central cache
        central_cache::get_instance()->batch_deallocate(freeList[index].pop_front(max_size >> 1));
}

thread_cache::~thread_cache()
{
    for (int i = 0; i < NLISTS; ++i)
        central_cache::get_instance()->batch_deallocate(freeList[i].pop_front(freeList[i].size()));
}
}