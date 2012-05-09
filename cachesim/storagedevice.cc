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

class StorageDevice : cSimpleModule
{
    public:
        void initialize();
        void finish();
        void handleMessage(cMessage *msg);

        void setSize(__u32 blockSize, __u32 blockCount);
        void setLatency(double readLatency, double writeLatency);

    private:
        __u32 __blockSize;
        __u32 __blockCount;
        double __readLatency;
        double __writeLatency;
};

void StorageDevice::setSize(__u32 blockSize, __u32 blockCount)
{
    this->__blockSize = blockSize;
    this->__blockCount = blockCount;
}

void StorageDevice::setLatency(double readLatency, double writeLatency)
{
    this->__readLatency = readLantecy;
    this->__writeLatency = writeLatency;
}

void StorageDevice::initialize()
{
}

void StorageDevice::finish()
{
}

void StorageDevice::handleMessage(cMessage *msg)
{
}

