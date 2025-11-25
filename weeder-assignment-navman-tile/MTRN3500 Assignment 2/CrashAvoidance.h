#pragma once

#include "SMObjects.h"
#include "NetworkedModule.h"

using namespace System;
using namespace System::Threading;
using namespace System::Diagnostics;
using namespace System::Net::Sockets;
using namespace System::Text;

ref class CrashAvoidance : public UGVModule
{
public:
    CrashAvoidance(SM_ThreadManagement^ SM_TM, SM_VehicleControl^ SM_VC, SM_LiDAR^ SM_LiDAR);
    void threadFunction() override;
    error_state processHeartbeats();
    void shutdownThreads();
    bool getShutdownFlag() override;
    error_state communicate() override;
    error_state checkData();
    error_state processSharedMemory() override;
    error_state connect(String^ hostName, int portNumber) override;

private:
    SM_ThreadManagement^ SM_TM_;
    Stopwatch^ Watch;
    SM_VehicleControl^ SM_VehicleControl_;
    SM_LiDAR^ SM_LiDAR_;
    bool FirstTimeCrash = 0;
};