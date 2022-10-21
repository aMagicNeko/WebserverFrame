#ifndef __PAGE_CACHE_H__
#define __PAGE_CACHE_H__

#include "memory_pool.h"
#include <unordered_map>
#include <unistd.h>

#define REFILL_SIZE 524288
#define NPAGES 128

namespace ekko {

class PageCache {
public:
    /// @brief get the only instance
    static PageCache* GetInstance() {
        static PageCache _inst;
        return &_inst;
    }

    /// @brief get a span from the page cache
    /// @param[in] npages number of pages in the span
    Span* Allocate(size_t npages);

    /// @brief return the span to the page cache
    void Deallocate(Span *spanPtr);

    /// @brief find the span with pageID
    Span* PageIdToSpan(page_id_t pageID);

private:
    PageCache(){}
    PageCache(const PageCache&) = delete;
    PageCache& operator=(const PageCache&) = delete;

    SpanList span_list_[NPAGES];
    std::unordered_map<page_id_t, Span*> span_map_;
    pthread_rwlock_t rwlock_ = PTHREAD_RWLOCK_INITIALIZER;
    
    /// @brief refill from heap
    void Refill();

    /// @brief map all pages in the span into spanMap
    void MapSpan(Span *spanPtr);
};

}
#endif