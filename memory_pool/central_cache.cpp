#include "central_cache.h"
#include "page_cache.h"

namespace ekko{
span*
central_cache::get_span_from_page_cache(size_t objSize)
{   void **cur;
    span *spanPtr;
    size_t nObj;

    spanPtr = page_cache::get_instance()->allocate(get_npages_from_page_cache(objSize));
    spanPtr->objSize = objSize;
    nObj = (spanPtr->nPages << PAGE_SHIFT) / objSize;

    //init the free object list 
    spanPtr->_list = (void*) (spanPtr->pageID << PAGE_SHIFT);
    cur = (void**) spanPtr->_list;
    while (--nObj) {
        *cur = cur+objSize;
        cur = (void**) *cur;
    }
    *cur = 0;
}

size_t
central_cache::batch_allocate(size_t objSize, void *&start, void *&last)
{
    size_t index, batchSize, curBatchSize;
    span *spanPtr;
    void **cur, **prev;

    index = get_list_index(objSize);
    batchSize = get_batch_size_from_central_cache(objSize);

    spanList[index].lock();

    if (!spanList[index].isfree())
        spanList[index].push_front(get_span_from_page_cache(objSize));
    
    start = spanList[index].get_space();
    cur = (void**) start;
    curBatchSize = 0;
    do {
        prev = cur;
        cur = (void**) *cur;
        ++curBatchSize;
    } while (cur && curBatchSize < batchSize);
    spanPtr->_list = cur;
    spanPtr->useCount += curBatchSize;
    *prev = 0;
    last = (void*) prev;

    // put the empty span in the back
    if (cur == 0) {
        spanPtr = spanList[index].pop_front();
        spanList[index].push_back(spanPtr);
    }

    spanList[index].unlock();

    return curBatchSize;
}

void
central_cache::batch_deallocate(void *start)
{
    span *spanPtr;
    size_t index;
    void **cur;
    page_cache *pageCachPtr;

    pageCachPtr = page_cache::get_instance();

    for (cur = (void**) start; cur; cur = (void**) *cur) {
        spanPtr = pageCachPtr->pageID_to_span(((pageID_t) cur) >> PAGE_SHIFT);
        index = get_list_index(spanPtr->objSize);

        spanList[index].lock();

        spanList[index].erase(spanPtr);
        *cur = spanPtr->_list;
        spanPtr->_list = (void*) cur;

        // all of the span is free?
        if (--spanPtr->useCount == 0)
            //return to the page cache
            pageCachPtr->deallocate(spanPtr);
        else
            //put the span in the front
            spanList[index].push_front(spanPtr);
        
        spanList[index].unlock();
    }



}
}