#pragma once
#using <System.dll>
#include "SMObjects.h"
#include "NetworkedModule.h"

using namespace System;
using namespace System::Threading;
using namespace System::Diagnostics;
using namespace System::Net::Sockets;
using namespace System::Text;

#define CRC32_POLYNOMIAL 0xEDB88320L

ref class GNSS : public NetworkedModule
{
public:

    GNSS(SM_ThreadManagement^ SM_TM, SM_GNSS^ SM_GNSS);
    void threadFunction() override;
    error_state processHeartbeats();
    void shutdownThreads();
    bool getShutdownFlag() override;
    error_state communicate() override;
    error_state checkData();
    error_state  processSharedMemory() override;
    error_state connect(String^ hostName, int portNumber) override;

private:

    SM_GNSS^ SM_GNSS_;
    Stopwatch^ Watch;
    array<unsigned char>^ SendData;
    #define CRC32_POLYNOMIAL 0xEDB88320L
    unsigned int CRC;
    double Northing;
    double Easting;
    double Height;
    unsigned int CalculatedCRC;
};