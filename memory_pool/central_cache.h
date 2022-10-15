#ifndef __CENTRAL_CACHE_H__
#define __CENTRAL_CACHE_H__

#include "memory_pool.h"

namespace ekko {

class central_cache {
private:
    central_cache() {}
    central_cache(const central_cache&) = delete;
    central_cache& operator=(const central_cache&) = delete;

    span_list spanList[NLISTS];

    /// @brief get a span from page cache
    static span* get_span_from_page_cache(size_t objSize);
public:
    static central_cache* get_instance() {
        static central_cache _inst;
        return &_inst;
    }

    /// @brief get a batch of objects
    /// @param[in] objSize objSize must have been aligned according to my rule
    /// @param[out] start start point of batch to get
    /// @param[out] last end point of batch to get
    /// @return batch size
    size_t batch_allocate(size_t objSize, void *&start, void *&last);

    /// @brief deallocate a batch of objects
    /// @param[in] start start point of a singly linked list, ended with NULL point
    void batch_deallocate(void* start);

};
}

#endif