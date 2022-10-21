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
typedef long long page_id_t;

struct Span {
    page_id_t pageid;
    size_t npages;

    Span *next = 0;
    Span *prev = 0;

    /// @brief free memory objects in the span
    void *_list = 0;

    /// @brief =0 when the span in the page_cache
    size_t obj_size = 0;

    ///@brief number of objects used
    size_t use_count = 0;
};

class SpanList {
public:
    bool Empty() const {
        return head_ptr_ == 0;
    }

    bool IsFree() const {
        return head_ptr_ != 0 && head_ptr_->_list != 0;
    }

    void PushBack(Span *span_ptr_arg) {
        span_ptr_arg->next = 0;
        if (tail_ptr_ == 0) {
            head_ptr_ = tail_ptr_ = span_ptr_arg;
            head_ptr_->prev = 0;
        }
        else {
            tail_ptr_->next = span_ptr_arg;
            span_ptr_arg->prev = tail_ptr_;
            tail_ptr_ = span_ptr_arg;
        }
    }

    Span* PopBack() {
        Span *span_ptr = tail_ptr_;
        if (tail_ptr_->prev == 0)
            tail_ptr_ = head_ptr_ = 0;
        else {
            tail_ptr_ = tail_ptr_->prev;
            tail_ptr_->next = 0;
        }
        span_ptr->prev = 0;
        return span_ptr;
    }

    void PushFront(Span *span_ptr_arg) {
        span_ptr_arg->prev = 0;
        if (head_ptr_ == 0) {
            head_ptr_ = tail_ptr_ = span_ptr_arg;
            tail_ptr_->next = 0;
        }
        else {
            head_ptr_->prev = span_ptr_arg;
            span_ptr_arg->next = head_ptr_;
            head_ptr_ = span_ptr_arg;
        }
    }

    Span* PopFront() {
        Span *span_ptr = head_ptr_;
        if (head_ptr_->next == 0)
            head_ptr_ = tail_ptr_ = 0;
        else {
            head_ptr_ = head_ptr_->next;
            head_ptr_->prev = 0;
        }
        span_ptr->next = 0;
        return span_ptr;
    }

    void Erase(Span *span_ptr_arg) {
        if (span_ptr_arg == head_ptr_)
            PopFront();
        else if (span_ptr_arg == tail_ptr_)
            PopBack();
        else {
            span_ptr_arg->prev->next = span_ptr_arg->next;
            span_ptr_arg->next->prev = span_ptr_arg->prev;
        }
        span_ptr_arg->next = span_ptr_arg->prev = 0;
    }

    void Lock() {
        pthread_mutex_lock(&mutex_);
    }

    void Unlock() {
        pthread_mutex_unlock(&mutex_);
    }

private:
    Span *head_ptr_ = 0;
    Span *tail_ptr_ = 0;
    pthread_mutex_t mutex_ = PTHREAD_MUTEX_INITIALIZER;

};

class FreeList {
private:
    size_t list_size_ = 0;
    void *head_ptr_ = 0;
public:
    bool Empty() const {
        return list_size_ == 0;
    }

    size_t Size() const {
        return list_size_;
    }

    void PushFront(void *start, void *last, size_t batch_size) {
        list_size_ += batch_size;
        NEXT_OBJ(last) = head_ptr_;
        head_ptr_ = start;
    }

    void PushFront(void *ptr) {
        ++list_size_;
        NEXT_OBJ(ptr) = head_ptr_;
        head_ptr_ = ptr;
    };

    void* PopFront() {
        --list_size_;
        void *ret = head_ptr_;
        head_ptr_ = NEXT_OBJ(head_ptr_);
        return ret;
    }

    void* PopFront(size_t batch_size) {
        if (batch_size == 0)
            return 0;
        void *prev, *cur, *ret;
        list_size_ -= batch_size;
        ret = head_ptr_;
        cur = head_ptr_;
        while (batch_size) {
            --batch_size;
            prev = cur;
            cur = NEXT_OBJ(cur);
        }
        NEXT_OBJ(prev) = 0;
        head_ptr_ = cur;
        return ret;
    }

};

/// @brief get the index of the given object size in the list
/// [1, 128]            8 bytes align       index[0, 15]
/// [129, 1024]         16 bytes align      index[16, 71]
/// [1025, 8*1024]      128 bytes align     index[72, 127]
/// [8*1024+1, 64*1024] 1024 bytes align    index[128, 183]
inline size_t
GetListIndex(size_t size)
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
GetBatchSizeFromCentralCache(size_t obj_size)
{
    if (obj_size == 0)
        return 0;
    size_t num = MAX_BYTES_FROM_CENTRAL_CACHE / obj_size;
    if (num > MAX_BATCH_SIZE)
        num = MAX_BATCH_SIZE;
    if (num < MIN_BATCH_SIZE)
        num = MIN_BATCH_SIZE;
    return num;
}

/// @brief get the number of pages allocated from page_cache given the object size
inline size_t
GetNPagesFromPageCache(size_t obj_size)
{
    return (obj_size * GetBatchSizeFromCentralCache(obj_size) + PAGE_SIZE - 1) >> PAGE_SHIFT;
}

}

/// @brief get the real size that thread_cache allocated
/// [1, 128]            8 bytes align       
/// [129, 1024]         16 bytes align      
/// [1025, 8*1024]      128 bytes align     
/// [8*1024+1, 64*1024] 1024 bytes align    
inline size_t
RoundUp(size_t size)
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