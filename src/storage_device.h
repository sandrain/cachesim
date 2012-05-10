#ifndef	__STORAGE_DEVICE_H__
#define	__STORAGE_DEVICE_H__

#include <omnetpp.h>
#include "cachesim.h"

class StorageDevice : public cSimpleModule
{
    public:
        StorageDevice();
        ~StorageDevice();

    protected:
        virtual void initialize();
        virtual void finish();
        virtual void handleMessage(cMessage *msg);

    private:
        double readLatency;
        double writeLatency;
        __u64 blockSize;
        __u64 blockCount;
        __u64 readRequests;
        __u64 writeRequests;
};

#endif	/** __STORAGE_DEVICE_H__ */

