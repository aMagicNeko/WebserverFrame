#ifndef __MEMORY_POOL_H__
#define __MEMORY_POOL_H__

#include <stdio.h>
#include <cerrno>
#include <cstring>
#include <pthread.h>

#define NLISTS 184
#define PAGE_SIZE 4096
#define PAGE_SHIFT 12
#define MAX_BYTES_FROM_CENTRAL_CACHE 65536
#define MAX_BATCH_SIZE 512
#define MIN_BATCH_SIZE 2
#define NEXT_OBJ(cur) *((void**) cur)

namespace ekko {
typedef long long pageID_t;

struct span {
    pageID_t pageID;
    size_t nPages;

    span *next = 0;
    span *prev = 0;

    /// @brief free memory objects in the span
    void *_list = 0;

    /// @brief =0 when the span in the page_cache
    size_t objSize = 0;

    ///@brief number of objects used
    size_t useCount = 0;
};

class span_list {
private:
    span *headPtr = 0;
    span *tailPtr = 0;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

public:
    bool empty() const {
        return headPtr == 0;
    }

    bool isfree() const {
        return headPtr != 0 && headPtr->_list != 0;
    }

    void push_back(span *spanPtrArg) {
        spanPtrArg->next = 0;
        if (tailPtr == 0) {
            headPtr = tailPtr = spanPtrArg;
            headPtr->prev = 0;
        }
        else {
            tailPtr->next = spanPtrArg;
            spanPtrArg->prev = tailPtr;
            tailPtr = spanPtrArg;
        }
    }

    span* pop_back() {
        span *spanPtr = tailPtr;
        if (tailPtr->prev == 0)
            tailPtr = headPtr = 0;
        else {
            tailPtr = tailPtr->prev;
            tailPtr->next = 0;
        }
        spanPtr->prev = 0;
        return spanPtr;
    }

    void push_front(span *spanPtrArg) {
        spanPtrArg->prev = 0;
        if (headPtr == 0) {
            headPtr = tailPtr = spanPtrArg;
            tailPtr->next = 0;
        }
        else {
            headPtr->prev = spanPtrArg;
            spanPtrArg->next = headPtr;
            headPtr = spanPtrArg;
        }
    }

    span* pop_front() {
        span *spanPtr = headPtr;
        if (headPtr->next == 0)
            headPtr = tailPtr = 0;
        else {
            headPtr = headPtr->next;
            headPtr->prev = 0;
        }
        spanPtr->next = 0;
        return spanPtr;
    }

    void erase(span *spanPtrArg) {
        if (spanPtrArg == headPtr)
            pop_front();
        else if (spanPtrArg == tailPtr)
            pop_back();
        else {
            spanPtrArg->prev->next = spanPtrArg->next;
            spanPtrArg->next->prev = spanPtrArg->prev;
        }
        spanPtrArg->next = spanPtrArg->prev = 0;
    }

    void lock() {
        pthread_mutex_lock(&mutex);
    }

    void unlock() {
        pthread_mutex_unlock(&mutex);
    }
};

class free_list {
private:
    size_t listSize = 0;
    void *headPtr = 0;
public:
    bool empty() const {
        return listSize == 0;
    }

    size_t size() const {
        return listSize;
    }

    void push_front(void *start, void *last, size_t batchSize) {
        listSize += batchSize;
        NEXT_OBJ(last) = headPtr;
        headPtr = start;
    }

    void push_front(void *ptr) {
        ++listSize;
        NEXT_OBJ(ptr) = headPtr;
        headPtr = ptr;
    };

    void* pop_front() {
        --listSize;
        void *ret = headPtr;
        headPtr = NEXT_OBJ(headPtr);
        return ret;
    }

    void* pop_front(size_t batchSize) {
        if (batchSize == 0)
            return 0;
        void *prev, *cur, *ret;
        listSize -= batchSize;
        ret = headPtr;
        cur = headPtr;
        while (batchSize) {
            --batchSize;
            prev = cur;
            cur = NEXT_OBJ(cur);
        }
        NEXT_OBJ(prev) = 0;
        headPtr = cur;
        return ret;
    }
};

/// @brief get the index of the given object size in the list
/// [1, 128]            8 bytes align       index[0, 15]
/// [129, 1024]         16 bytes align      index[16, 71]
/// [1025, 8*1024]      128 bytes align     index[72, 127]
/// [8*1024+1, 64*1024] 1024 bytes align    index[128, 183]
inline size_t
get_list_index(size_t size)
{
    if (size <= 128) {
        return (size - 1) >> 3;
    }
    if (size <= 1024) {
        return 16 + ((size - 129) >> 4);
    }
    if (size <= 8192) {
        return 72 + ((size - 1025) >> 7);
    }
    return 128 + ((size - 8193) >> 10);
}

/// @brief get the size of batch given the object size
inline size_t
get_batch_size_from_central_cache(size_t objSize)
{
    if (objSize == 0)
        return 0;
    size_t num = MAX_BYTES_FROM_CENTRAL_CACHE / objSize;
    if (num > MAX_BATCH_SIZE)
        num = MAX_BATCH_SIZE;
    if (num < MIN_BATCH_SIZE)
        num = MIN_BATCH_SIZE;
    return num;
}

/// @brief get the number of pages allocated from page_cache given the object size
inline size_t
get_npages_from_page_cache(size_t objSize)
{
    return (objSize * get_batch_size_from_central_cache(objSize) + PAGE_SIZE - 1) >> PAGE_SHIFT;
}

}

/// @brief get the real size that thread_cache allocated
/// [1, 128]            8 bytes align       
/// [129, 1024]         16 bytes align      
/// [1025, 8*1024]      128 bytes align     
/// [8*1024+1, 64*1024] 1024 bytes align    
inline size_t
roundup_size(size_t size)
{
    if (size <= 128)
        return ((size+7)>>3)<<3;
    if (size <= 1024)
        return ((size+15)>>4)<<4;
    if (size <= 8192)
        return ((size+127)>>7)<<7;
    if (size <= MAX_BYTES_FROM_CENTRAL_CACHE)
        return ((size+1023)>>10)<<10;
    return ((size+PAGE_SIZE-1)>>PAGE_SHIFT)<<PAGE_SHIFT;
}
#endif