#pragma once

#include "ControllerInterface.h" 
#include "SMObjects.h"       
#include "NetworkedModule.h"

using namespace System;
using namespace System::Threading;
using namespace System::Diagnostics;
using namespace System::Net::Sockets;
using namespace System::Text;

#define CONTROLLER 0
#define KEYBOARD 1


ref class Controller : public UGVModule
{
public:

    Controller(SM_ThreadManagement^ SM_TM, SM_VehicleControl^ SM_VC);
    error_state processSharedMemory() override;
    bool getShutdownFlag() override;
    void shutdownThreads();
    void threadFunction() override;
    error_state processHeartbeats();
    error_state checkData();

private:

    Stopwatch^ Watch;
    SM_VehicleControl^ SM_VehicleControl_;
    array<unsigned char>^ SendData;
    double speed;
    double angle;
};