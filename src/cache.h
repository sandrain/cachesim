#ifndef	__CACHE_H__
#define	__CACHE_H__

#include <omnetpp.h>
#include "cachesim.h"

class Cache : public cSimpleModule
{
    public:
        Cache();
        ~Cache();

    protected:
        virtual void initialize();
        virtual void finish();
        virtual void handleMessage(cMessage *msg);

    private:
        CacheDevice *cache;
};

#endif	/** __CACHE_H__ */

