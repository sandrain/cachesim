/* Copyright (C) Hyogi Sim <hyogi@cs.vt.edu>
 * 
 * ---------------------------------------------------------------------------
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 */
#include <string>
#include <linux/types.h>
#include <omnetpp.h>

using namespace std;

class StorageDevice : public cSimpleModule
{
    public:

    protected:
        virtual void initialize();
        virtual void finish();
        virtual void handleMessage(cMessage *msg);

    private:
        __u32 __blockSize;
        __u32 __blockCount;
        double __readLatency;
        double __writeLatency;
        __u64 __nReads;
        __u64 __nWrites;
};

Define_Module(StorageDevice);

void StorageDevice::initialize()
{
    __nReads = 0;
    __nWrites = 0;
    __blockSize = par("blockSize").longValue();
    __blockCount = par("blockCount").longValue();
    __readLatency = par("readLatency").doubleValue();
    __writeLatency = par("writeLatency").doubleValue();
}

void StorageDevice::finish()
{
    cerr << "read request  = " << __nReads << endl;
    cerr << "write request = " << __nWrites << endl;
}

void StorageDevice::handleMessage(cMessage *msg)
{
    if (msg->getName()[0] == 'R')
        __nReads++;
    else
        __nWrites++;

    delete msg;
}

