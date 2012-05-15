#ifndef	__CACHE_H__
#define __CACHE_H__ 

#include <vector>
#include "cachesim.h"

class Cache : public cSimpleModule
{
    friend class CacheAlgorithm;

    public:
        Cache() {}
        ~Cache() {}

    protected:
        virtual void initialize() = 0;
        virtual void final() = 0;
        virtual void handleMessage() = 0;

        CacheAlgorithm algorithm;

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

        void initialize();
        void final();
        void handleMessage();

        enum { GATE_INDEX_DRAM = 0, GATE_INDEX_SSD = 1, GATE_INDEX_HDD = 2 };

    private:
        std::vector<int> _gateRequest;
        std::vector<int> _gateResponse;
};

#endif  /** __CACHE_H__ */

