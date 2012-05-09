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

class IOApplication : public cSimpleModule
{
    public:
        IOApplication();
        ~IOApplication();

        void setTraceFile(const char *fileName);

    protected:
        virtual void initialize();
        virtual void finish();
        virtual void handleMessage(cMessage *msg);

    private:
        void generateIO();

        string fileName;
        ifstream trace;
        cMessage *eof;
};

Define_Module(IOApplication);

IOApplication::IOApplication()
{
}

IOApplication::~IOApplication()
{
}

void IOApplication::generateIO()
{
    string line;
    cMessage *msg;

    if (getline(trace, line))
        msg = new cMessage(line.c_str());
    else
        msg = eof;

    scheduleAt(simTime()+5, msg);
}

void IOApplication::initialize()
{
    fileName = par("traceFile").stringValue();
    trace.open(fileName.c_str());

    generateIO();

    eof = new cMessage("eof");
}

void IOApplication::finish()
{
    trace.close();
}

void IOApplication::handleMessage(cMessage *msg)
{
    if (msg == eof)
        delete msg;
    else {
        send(msg, "out");
        generateIO();
    }
}

