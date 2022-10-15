#ifndef __PAGE_CACHE_H__
#define __PAGE_CACHE_H__

#include "memory_pool.h"
#include <unordered_map>
#include <unistd.h>

#define REFILL_SIZE 524288
#define NPAGES 128

namespace ekko {

class page_cache {
private:
    page_cache(){}
    page_cache(const page_cache&)=delete;
    page_cache& operator=(const page_cache&)=delete;

    span_list spanList[NPAGES];
    std::unordered_map<pageID_t, span*> spanMap;
    pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;
    
    /// @brief refill from heap
    void refill();

    /// @brief map all pages in the span into spanMap
    void map_span(span *spanPtr);
public:

    /// @brief get the only instance
    static page_cache* get_instance() {
        static page_cache _inst;
        return &_inst;
    }

    /// @brief get a span from the page cache
    /// @param[in] npages number of pages in the span
    span* allocate(size_t npages);

    /// @brief return the span to the page cache
    void deallocate(span *spanPtr);

    /// @brief find the span with pageID
    span* pageID_to_span(pageID_t pageID);
};

}
#endif