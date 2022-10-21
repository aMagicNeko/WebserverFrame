#ifndef __CENTRAL_CACHE_H__
#define __CENTRAL_CACHE_H__

#include "memory_pool.h"

namespace ekko {

class CentralCache {
public:
    static CentralCache* GetInstance() {
        static CentralCache _inst;
        return &_inst;
    }

    /// @brief get a batch of objects
    /// @param[in] obj_size obj_size must have been aligned according to my rule
    /// @param[out] start start point of batch to get
    /// @param[out] last end point of batch to get
    /// @return batch size
    size_t BatchAllocate(size_t obj_size, void *&start, void *&last);

    /// @brief deallocate a batch of objects
    /// @param[in] start start point of a singly linked list, ended with NULL point
    void BatchDeallocate(void* start);

private:
    CentralCache() {}
    CentralCache(const CentralCache&) = delete;
    CentralCache& operator=(const CentralCache&) = delete;

    SpanList span_list_[NLISTS];

    /// @brief get a span from page cache
    static Span* GetSpanFromPageCache(size_t obj_size);
};
}

#endif