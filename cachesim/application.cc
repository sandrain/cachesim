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

#include <fstream>
#include <string>
#include <omnetpp.h>

using namespace std;

class IOApplication
{
    public:
        IOApplication();
        ~IOApplication();

        void setTraceFile(const char *fileName);
        void run();

        void initialize();
        void finish();
        void handleMessage(cMessage *msg);

    private:
        ifstream trace;
}

IOApplication::IOApplication()
{
}

IOApplication::~IOApplication()
{
}

void IOApplication::setTraceFile(const char *fileName)
{
    this->trace.open(fileName);
}

void IOApplication::run()
{
    string line;

    while (getline(trace, line))
        send(new cMessage(line), "out");
}

void StorageDevice::initialize()
{
}

void StorageDevice::finish()
{
    trace.close();
}

void IOApplication::handleMessage(cMessage *msg)
{
    /** at this moment, this should not happen */
}

