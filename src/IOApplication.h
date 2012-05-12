#ifndef __IOAPPLICATION_H__
#define __IOAPPLICATION_H__

#include <omnetpp.h>
#include "cachesim.h"

class IOApplication : public cSimpleModule
{
    public:
        IOApplication();
        ~IOApplication();

        void initialize();
        void final();
        void handleMessage(cMessage *msg);

    private:
        IOTrace trace;
};

#endif  /** __IOAPPLICATION_H__ */

