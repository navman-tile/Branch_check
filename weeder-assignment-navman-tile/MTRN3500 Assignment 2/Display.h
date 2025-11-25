#pragma once
#include "SMObjects.h"
#include "NetworkedModule.h"
#using <System.dll>

using namespace System;
using namespace System::Threading;
using namespace System::Diagnostics;

ref class Display : public NetworkedModule
{
public:
    Display(SM_ThreadManagement^ SM_TM, SM_LiDAR^ SM_LiDAR, SM_GNSS^ SM_GNSS);
    void threadFunction() override;
    error_state processHeartbeats();
    void shutdownThreads();
    bool getShutdownFlag() override;
    error_state communicate() override;
    error_state processSharedMemory() override;
    error_state connect(String^ hostName, int portNumber) override;
    error_state checkData();
    void sendDisplayData();

private:
    array<unsigned char>^ SendData; 
    SM_LiDAR^ SM_LiDAR_;
    SM_GNSS^ SM_GNSS_;
    Stopwatch^ Watch;
};