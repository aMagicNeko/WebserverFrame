#include "page_cache.h"

namespace ekko {

void
PageCache::MapSpan(Span *span_ptr)
{
    for (int i = 0; i < span_ptr->npages; ++i)
        span_map_[span_ptr->pageid+i] = span_ptr;
}

Span*
PageCache::PageIdToSpan(page_id_t page_id)
{
    pthread_rwlock_rdlock(&rwlock_);
    std::unordered_map<page_id_t, Span*>::iterator iter = span_map_.find(page_id);
    if (iter == span_map_.end()) {
        pthread_rwlock_unlock(&rwlock_);
        return 0;
    }
    pthread_rwlock_unlock(&rwlock_);
    return iter->second;
}

void
PageCache::Refill()
{
    Span *span_ptr;
    void *memPtr;
    page_id_t pID;

    if ( (memPtr = sbrk(REFILL_SIZE)) == (void*) -1) {
        //perror("sbrk error: %s", strerror(errno));
        return;
    }

    span_ptr = new Span;
    span_ptr->pageid = (page_id_t) memPtr >> PAGE_SHIFT;
    span_ptr->npages = NPAGES;
    span_list_[NPAGES-1].PushFront(span_ptr);
    MapSpan(span_ptr);

    //printf("refill page_size:%d\n", span_ptr->npages);
}

Span*
PageCache::Allocate(size_t page_size)
{
    size_t i;
    Span* span_ptr;

    if (page_size > NPAGES)
        page_size = NPAGES;
    
    pthread_rwlock_wrlock(&rwlock_);

    if (!span_list_[page_size-1].Empty()) {
        span_ptr = span_list_[page_size-1].PopFront();

        span_ptr->obj_size = 1;

        pthread_rwlock_unlock(&rwlock_);
        return span_ptr;
    }

    //find a spare span
    for (i = page_size; i < NPAGES; ++i) {
        if (span_list_[i].Empty())
            continue;
        span_ptr = span_list_[i].PopBack();

        //split the span
        span_ptr->npages = page_size;

        Span *span_split_ptr = new Span;
        span_split_ptr->npages = i + 1 - page_size;
        span_split_ptr->pageid = span_ptr->pageid + page_size;
        MapSpan(span_split_ptr);
        span_list_[span_split_ptr->npages-1].PushFront(span_split_ptr);
        span_ptr->obj_size = 1;

        pthread_rwlock_unlock(&rwlock_);

        //printf("span_ptr get: %d\n", span_ptr->npages);
        return span_ptr;
    }

    Refill();

    pthread_rwlock_unlock(&rwlock_);

    return Allocate(page_size);
}

void
PageCache::Deallocate(Span *span_ptr)
{
    page_id_t page_id;
    Span *span_cur_ptr;
    std::unordered_map<page_id_t, Span*>::iterator mapIter;
    size_t page_size;
    span_ptr->obj_size = 0;
    
    pthread_rwlock_wrlock(&rwlock_);

    //merge the span in front
    page_size = span_ptr->npages;
    while (1) {
        page_id = span_ptr->pageid - 1;
        mapIter = span_map_.find(page_id);
        if (mapIter == span_map_.end())
            break;
        span_cur_ptr = mapIter->second;

        //if in use?
        if (span_cur_ptr->obj_size)
            break;
        
        //if too big?
        if ( (page_size = page_size + span_cur_ptr->npages) > NPAGES)
            break;
    
        span_ptr->pageid = span_cur_ptr->pageid;
        span_ptr->npages = page_size;

        span_list_[span_cur_ptr->npages-1].Erase(span_cur_ptr);
        delete span_cur_ptr;
    }
    
    //merge the span in back
    page_size = span_ptr->npages;
    while (1) {
        page_id = span_ptr->pageid + span_ptr->npages;
        mapIter = span_map_.find(page_id);
        if (mapIter == span_map_.end())
            break;
        span_cur_ptr = mapIter->second;

        //if in use?
        if (span_cur_ptr->obj_size)
            break;
        
        //if too big?
        if ( (page_size = page_size + span_cur_ptr->npages) > NPAGES)
            break;
        
        span_ptr->npages = page_size;

        span_list_[span_cur_ptr->npages-1].Erase(span_cur_ptr);
        delete span_cur_ptr;
    }

    MapSpan(span_ptr);
    span_list_[span_ptr->npages-1].PushFront(span_ptr);

    pthread_rwlock_unlock(&rwlock_);
}


}