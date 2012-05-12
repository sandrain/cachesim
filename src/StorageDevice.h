#ifndef __STORAGEDEVICE_H__
#define __STORAGEDEVICE_H__

#include <omnetpp.h>
#include "cachesim.h"

class StorageDevice : public cSimpleModule
{
    public:
        StorageDevice();
        ~StorageDevice();

        virtual void initialize();
        virtual void final();
        virtual void handleMessage(cMessage *msg);

        static void setLatency(double read, double write);

        static double readLatency;
        static double writeLatency;

    protected:
        __u64 readRequests;
        __u64 writeRequests;
};

#endif  /** __STORAGEDEVICE_H__ */

