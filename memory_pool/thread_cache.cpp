#include "thread_cache.h"
#include "central_cache.h"
#include "page_cache.h"

namespace ekko{

void*
ThreadCache::Allocate(size_t obj_size)
{
    if (obj_size > (NPAGES<<PAGE_SHIFT))
        //too big
        return 0;
    
    if (obj_size > MAX_BYTES_FROM_CENTRAL_CACHE) {
        //directly get from page cache
        return (void*) (PageCache::GetInstance()->Allocate((obj_size + PAGE_SIZE - 1) >> PAGE_SHIFT)->pageid << PAGE_SHIFT);
    }

    size_t index;

    index = GetListIndex(obj_size);
    if (free_list_[index].Empty()) {
        //refill the free list
        void *start, *last;
        size_t batchSize = CentralCache::GetInstance()->BatchAllocate(obj_size, start, last);
        free_list_[index].PushFront(start, last, batchSize);
    }
        
    return free_list_[index].PopFront();
}

void
ThreadCache::Deallocate(void *ptr, size_t size)
{
    size = RoundUp(size);

    if (size > MAX_BYTES_FROM_CENTRAL_CACHE) {
        //directly release into page_cache
        PageCache *page_cache_ptr = PageCache::GetInstance();
        Span *span_ptr = page_cache_ptr->PageIdToSpan(((page_id_t) ptr) >> PAGE_SHIFT);
        page_cache_ptr->Deallocate(span_ptr);
        return;
    }

    size_t index, max_size;

    index = GetListIndex(size);
    free_list_[index].PushFront(ptr);
    
    max_size = GetBatchSizeFromCentralCache(size) << 1;
    if (free_list_[index].Size() > max_size)
        //list too long, return to central cache
        CentralCache::GetInstance()->BatchDeallocate(free_list_[index].PopFront(max_size >> 1));
}

ThreadCache::~ThreadCache()
{
    for (int i = 0; i < NLISTS; ++i)
        CentralCache::GetInstance()->BatchDeallocate(free_list_[i].PopFront(free_list_[i].Size()));
}
}