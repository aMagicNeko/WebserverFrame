#include "page_cache.h"

namespace ekko {

void
page_cache::map_span(span *spanPtr)
{
    for (int i = 0; i < spanPtr->nPages; ++i)
        spanMap[spanPtr->pageID+i] = spanPtr;
}

span*
page_cache::pageID_to_span(pageID_t pageID)
{
    pthread_rwlock_rdlock(&rwlock);
    std::unordered_map<pageID_t, span*>::iterator iter = spanMap.find(pageID);
    if (iter == spanMap.end()) {
        pthread_rwlock_unlock(&rwlock);
        return 0;
    }
    pthread_rwlock_unlock(&rwlock);
    return iter->second;
}

void
page_cache::refill()
{
    span *spanPtr;
    void *memPtr;
    pageID_t pID;

    if ( (memPtr = sbrk(REFILL_SIZE)) == (void*) -1) {
        //perror("sbrk error: %s", strerror(errno));
        return;
    }

    spanPtr = new span;
    spanPtr->pageID = (pageID_t) memPtr >> PAGE_SHIFT;
    spanPtr->nPages = NPAGES;
    spanList[NPAGES-1].push_front(spanPtr);
    map_span(spanPtr);

    //printf("refill pageSize:%d\n", spanPtr->nPages);
}

span*
page_cache::allocate(size_t pageSize)
{
    size_t i;
    span* spanPtr;

    if (pageSize > NPAGES)
        pageSize = NPAGES;
    
    pthread_rwlock_wrlock(&rwlock);

    if (!spanList[pageSize-1].empty()) {
        spanPtr = spanList[pageSize-1].pop_front();

        spanPtr->objSize = 1;

        pthread_rwlock_unlock(&rwlock);
        return spanPtr;
    }

    //find a spare span
    for (i = pageSize; i < NPAGES; ++i) {
        if (spanList[i].empty())
            continue;
        spanPtr = spanList[i].pop_front();

        //split the span
        spanPtr->nPages = pageSize;

        span *spanSplitPtr = new span;
        spanSplitPtr->nPages = i + 1 - pageSize;
        spanSplitPtr->pageID = spanPtr->pageID + pageSize;
        map_span(spanSplitPtr);
        spanList[spanSplitPtr->nPages-1].push_front(spanSplitPtr);
        spanPtr->objSize = 1;

        pthread_rwlock_unlock(&rwlock);

        //printf("spanPtr get: %d\n", spanPtr->nPages);
        return spanPtr;
    }

    refill();

    pthread_rwlock_unlock(&rwlock);

    return allocate(pageSize);
}

void
page_cache::deallocate(span *spanPtr)
{
    pageID_t pageID;
    span *spanCurPtr;
    std::unordered_map<pageID_t, span*>::iterator mapIter;
    size_t pageSize;
    spanPtr->objSize = 0;
    
    pthread_rwlock_wrlock(&rwlock);

    //merge the span in front
    pageSize = spanPtr->nPages;
    while (1) {
        pageID = spanPtr->pageID - 1;
        mapIter = spanMap.find(pageID);
        if (mapIter == spanMap.end())
            break;
        spanCurPtr = mapIter->second;

        //if in use?
        if (spanCurPtr->objSize)
            break;
        
        //if too big?
        if ( (pageSize = pageSize + spanCurPtr->nPages) > NPAGES)
            break;
    
        spanPtr->pageID = spanCurPtr->pageID;
        spanPtr->nPages = pageSize;

        spanList[spanCurPtr->nPages-1].erase(spanCurPtr);
        delete spanCurPtr;
    }
    
    //merge the span in back
    pageSize = spanPtr->nPages;
    while (1) {
        pageID = spanPtr->pageID + spanPtr->nPages;
        mapIter = spanMap.find(pageID);
        if (mapIter == spanMap.end())
            break;
        spanCurPtr = mapIter->second;

        //if in use?
        if (spanCurPtr->objSize)
            break;
        
        //if too big?
        if ( (pageSize = pageSize + spanCurPtr->nPages) > NPAGES)
            break;
        
        spanPtr->nPages = pageSize;

        spanList[spanCurPtr->nPages-1].erase(spanCurPtr);
        delete spanCurPtr;
    }

    map_span(spanPtr);
    spanList[spanPtr->nPages-1].push_front(spanPtr);

    pthread_rwlock_unlock(&rwlock);
}


}