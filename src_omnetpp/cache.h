#ifndef	__CACHE_H__
#define __CACHE_H__ 

#include <vector>
#include "cachesim.h"

class Cache : public cSimpleModule
{
    public:
        Cache() {}
        ~Cache() {}

    protected:
        virtual void initialize() = 0;
        virtual void final() = 0;
        virtual void handleMessage() = 0;

        __u64 hits;
        __u64 misses;
        __u64 replacements;
        __u64 readRequests;
        __u64 writeRequests;

        int gateIn;
        int gateOut;
};

class LocalCache : public Cache
{
    public:
        LocalCache() {}

        LocalCache(const LocalCache& cache);

        LocalCache& operator=(const LocalCache& cache);

        ~LocalCache() {}

    protected:
        virtual void initialize() = 0;
        virtual void final() = 0;
        virtual void handleMessage() = 0;

        enum { GATE_INDEX_DRAM = 0, GATE_INDEX_SSD = 1, GATE_INDEX_HDD = 2 };

        std::vector<int> gateRequest;
        std::vector<int> gateResponse;
        std::vector<__u64> capacity;
};

#endif  /** __CACHE_H__ */

