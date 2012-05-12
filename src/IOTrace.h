#ifndef __IOTRACE_H__
#define __IOTRACE_H__

#include <string>
#include <omnetpp.h>

#include "cachesim.h"

enum IORequestKind
{
    IOREQ_EOF   = 0,
    IOREQ_READ  = 1,
    IOREQ_WRITE = 2
};

class IORequest : public cMessage
{
    public:
        IORequest();
        IORequest(int type, BlockOffset off, BlockCount len);
        IORequest(std::string &str);

        ~IORequest();
};

class IOTrace
{
    public:
        IOTrace();
        IOTrace(std::string &fileName);

        ~IOTrace();

        void getNextRequest(IORequest &req);

    private:
        istream file;
};

#endif  /** __IOTRACE_H__ */

