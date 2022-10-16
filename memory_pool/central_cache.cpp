#include "central_cache.h"
#include "page_cache.h"

namespace ekko{
span*
central_cache::get_span_from_page_cache(size_t objSize)
{   void *cur;
    span *spanPtr;
    size_t nObj;

    spanPtr = page_cache::get_instance()->allocate(get_npages_from_page_cache(objSize));
    spanPtr->objSize = objSize;
    nObj = (spanPtr->nPages << PAGE_SHIFT) / objSize;

    //init the free object list 
    spanPtr->_list = (void*) (spanPtr->pageID << PAGE_SHIFT);
    cur = spanPtr->_list;
    while (--nObj) {
        //printf("nObjs %d cur: %x\t", nObj, cur);
        NEXT_OBJ(cur) = cur+objSize;
        cur = NEXT_OBJ(cur);
    }
    NEXT_OBJ(cur) = 0;
    //printf("get a span from pagecache: %x\n", spanPtr->pageID);
    return spanPtr;
}

size_t
central_cache::batch_allocate(size_t objSize, void *&start, void *&last)
{
    size_t index, batchSize, curBatchSize;
    span *spanPtr;
    void *cur, *prev;

    index = get_list_index(objSize);
    batchSize = get_batch_size_from_central_cache(objSize);

    spanList[index].lock();

    if (!spanList[index].isfree()) {
        spanList[index].push_front(get_span_from_page_cache(objSize));
        //printf("getSpanSpace:%x\n", get_span_from_page_cache(objSize)->_list);
    }
    spanPtr = spanList[index].pop_front();
    cur = start = spanPtr->_list;

    curBatchSize = 0;
    do {
        //printf("order: %d, cur: %x\n", curBatchSize, cur);
        prev = cur;
        cur = NEXT_OBJ(cur);
        ++curBatchSize;
    } while (cur && curBatchSize < batchSize);
    spanPtr->_list = cur;
    spanPtr->useCount += curBatchSize;
    NEXT_OBJ(prev) = 0;
    last = prev;

    if (cur == 0)
        // put the empty span in the back
        spanList[index].push_back(spanPtr);
    else
        spanList[index].push_front(spanPtr);

    spanList[index].unlock();

    return curBatchSize;
}

void
central_cache::batch_deallocate(void *start)
{
    span *spanPtr;
    size_t index;
    void *cur, *tmp;
    page_cache *pageCachPtr;

    pageCachPtr = page_cache::get_instance();

    for (cur = start; cur; cur = tmp) {
        tmp = NEXT_OBJ(cur);
        spanPtr = pageCachPtr->pageID_to_span(((pageID_t) cur) >> PAGE_SHIFT);
        index = get_list_index(spanPtr->objSize);

        spanList[index].lock();

        spanList[index].erase(spanPtr);
        NEXT_OBJ(cur) = spanPtr->_list;
        spanPtr->_list = cur;

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