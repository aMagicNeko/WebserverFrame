#include "central_cache.h"
#include "page_cache.h"

namespace ekko{
Span*
CentralCache::GetSpanFromPageCache(size_t obj_size)
{   void *cur;
    Span *span_ptr;
    size_t nobjs;

    span_ptr = PageCache::GetInstance()->Allocate(GetNPagesFromPageCache(obj_size));
    span_ptr->obj_size = obj_size;
    nobjs = (span_ptr->npages << PAGE_SHIFT) / obj_size;

    //init the free object list 
    span_ptr->_list = (void*) (span_ptr->pageid << PAGE_SHIFT);
    cur = span_ptr->_list;
    while (--nobjs) {
        //printf("nobjss %d cur: %x\t", nobjs, cur);
        NEXT_OBJ(cur) = cur+obj_size;
        cur = NEXT_OBJ(cur);
    }
    NEXT_OBJ(cur) = 0;
    //printf("get a span from pagecache: %x\n", span_ptr->pageID);
    return span_ptr;
}

size_t
CentralCache::BatchAllocate(size_t obj_size, void *&start, void *&last)
{
    size_t index, batch_size, curbatch_size;
    Span *span_ptr;
    void *cur, *prev;

    index = GetListIndex(obj_size);
    batch_size = GetBatchSizeFromCentralCache(obj_size);

    span_list_[index].Lock();

    if (!span_list_[index].IsFree()) {
        span_list_[index].PushFront(GetSpanFromPageCache(obj_size));
        //printf("getSpanSpace:%x\n", GetSpanFromPageCache(obj_size)->_list);
    }
    span_ptr = span_list_[index].PopFront();
    cur = start = span_ptr->_list;

    curbatch_size = 0;
    do {
        //printf("order: %d, cur: %x\n", curbatch_size, cur);
        prev = cur;
        cur = NEXT_OBJ(cur);
        ++curbatch_size;
    } while (cur && curbatch_size < batch_size);
    span_ptr->_list = cur;
    span_ptr->use_count += curbatch_size;
    NEXT_OBJ(prev) = 0;
    last = prev;

    if (cur == 0)
        // put the empty span in the back
        span_list_[index].PushBack(span_ptr);
    else
        span_list_[index].PushFront(span_ptr);

    span_list_[index].Unlock();

    return curbatch_size;
}

void
CentralCache::BatchDeallocate(void *start)
{
    Span *span_ptr;
    size_t index;
    void *cur, *tmp;
    PageCache *page_cache_ptr;

    page_cache_ptr = PageCache::GetInstance();

    for (cur = start; cur; cur = tmp) {
        tmp = NEXT_OBJ(cur);
        span_ptr = page_cache_ptr->PageIdToSpan(((page_id_t) cur) >> PAGE_SHIFT);
        index = GetListIndex(span_ptr->obj_size);

        span_list_[index].Lock();

        span_list_[index].Erase(span_ptr);
        NEXT_OBJ(cur) = span_ptr->_list;
        span_ptr->_list = cur;

        // all of the span is free?
        if (--span_ptr->use_count == 0)
            //return to the page cache
            page_cache_ptr->Deallocate(span_ptr);
        else
            //put the span in the front
            span_list_[index].PushFront(span_ptr);
        
        span_list_[index].Unlock();
    }



}
}