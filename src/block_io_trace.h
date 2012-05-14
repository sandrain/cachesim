#ifndef __BLOCK_IO_TRACE_H__
#define __BLOCK_IO_TRACE_H__

#include <string>
#include <ifstream>
#include "cachesim.h"

class BlockIORequest : public cMessage
{
    public:
        BlockIORequest() {}

        BlockIORequest(int type, __u64 offset, __u64 count)
            : kind(type), _blockOffset(offset), _blockCount(count) {}

        /** Default will be enough here
        BlockIORequest(const BlockIORequest& request);
        BlockIORequest& operator=(const BlockIORequest& request);
        */

        ~BlockIORequest() {}

        enum { IOREQ_EOF = 0, IOREQ_READ = 1, IOREQ_WRITE = 2 };

    private:
        __u64 _blockOffset;
        __u64 _blockCount;
};

class BlockIOTrace
{
    public:
        BlockIOTrace() {}

        BlockIOTrace(const std::string& fileName) : _traceFile(fileName) {}

        BlockIOTrace(const BlockIOTrace& trace);
        BlockIOTrace& operator=(const BlockIOTrace& trace);

        ~BlockIOTrace() {}

        void initialize();
        void initialize(const std::string& fileName);

        static const BlockIORequest eof(BlockIORequest::IOREQ_EOF, 0, 0);

    private:
        std::string _traceFile;
        std::ifstream _trace;
};

#endif  /** __BLOCK_IO_TRACE_H__ */

