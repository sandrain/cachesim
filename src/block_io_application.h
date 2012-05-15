#ifndef __BLOCK_IO_APPLICATION__
#define __BLOCK_IO_APPLICATION__

#include "cachesim.h"

class BlockIOApplication : public cSimpleModule
{
    public:
        BlockIOApplication() {}

        BlockIOApplication(const BlockIOApplication& app)
        {
            _trace = BlockIOTrace(app._trace);
        }

        BlockIOApplication& operator=(const BlockIOApplication& app)
        {
            _trace = BlockIOTrace(app._trace);
            return *this;
        }

        bool operator==(const BlockIOApplication& other) const
        {
            return _trace.name == other._trace.name;
        }

        bool operator!=(const BlockIOApplication& other) const
        {
            return !(*this == other);
        }

        ~BlockIOApplication() {}

        void initialize();
        void final();
        void handleMessage(cMessage *msg);

    private:
        BlockIOTrace _trace;
        int _requestGate;
        int _responseGate;
};

#endif  /** __BLOCK_IO_APPLICATION__ */

