//#using <System.dll>
#include "Display.h"

using namespace System;
using namespace System::Threading;
using namespace System::Diagnostics;

Display::Display(SM_ThreadManagement^ SM_TM, SM_LiDAR^ SM_LiDAR, SM_GNSS^ SM_GNSS) {
    SM_LiDAR_ = SM_LiDAR;
    SM_GNSS_ = SM_GNSS;
    SM_TM_ = SM_TM;
    Watch = gcnew Stopwatch;
}

void Display::threadFunction()
{
    connect(DISPLAY_ADDRESS, 28000);

    Console::WriteLine("Display thread is starting.");
    Watch = gcnew Stopwatch;
    SM_TM_->ThreadBarrier->SignalAndWait();
    Watch->Start();
    while (!getShutdownFlag())
    {
        Console::WriteLine("Display thread is running.");
        processHeartbeats();
        if (communicate() == SUCCESS && checkData() == SUCCESS)
        {
            processSharedMemory();
        }
        Thread::Sleep(20);

    }
    Console::WriteLine("Display	thread is terminating");

}

error_state Display::communicate() {
    sendDisplayData();
    return SUCCESS;
}


error_state Display::processHeartbeats()
{
    if ((SM_TM_->heartbeat & bit_DISPLAY) == 0)
    {
        SM_TM_->heartbeat |= bit_DISPLAY;
        Watch->Restart();
    }
    else
    {
        if (Watch->ElapsedMilliseconds > CRASH_LIMIT)
        {
            shutdownThreads();
            return ERR_DISPLAY_FAILURE;
        }
    }

    return SUCCESS;
}

void Display::shutdownThreads() { SM_TM_->shutdown = 0xFF; }

bool Display::getShutdownFlag() { return SM_TM_->shutdown & bit_DISPLAY; }

error_state Display::connect(String^ hostName, int portNumber)
{
    //Console::WriteLine("Meow it connected");
    Client = gcnew TcpClient(hostName, portNumber);
    Stream = Client->GetStream();
    Client->NoDelay = true;
    Client->ReceiveTimeout = 500;
    Client->SendTimeout = 500;
    Client->ReceiveBufferSize = 1024;
    Client->SendBufferSize = 1024;

    SendData = gcnew array<unsigned char>(64);
    ReadData = gcnew array<unsigned char>(64);
    return SUCCESS;
}

void Display::sendDisplayData()
{
    array<Byte>^ dataX = gcnew array<Byte>(SM_LiDAR_->x->Length * sizeof(double));
    Buffer::BlockCopy(SM_LiDAR_->x, 0, dataX, 0, dataX->Length);
    array<Byte>^ dataY = gcnew array<Byte>(SM_LiDAR_->y->Length * sizeof(double));
    Buffer::BlockCopy(SM_LiDAR_->y, 0, dataY, 0, dataY->Length);
    Stream->Write(dataX, 0, dataX->Length);
    Thread::Sleep(10);
    Stream->Write(dataY, 0, dataY->Length);
}

error_state Display::checkData() { return error_state::SUCCESS; }

error_state Display::processSharedMemory() { return error_state::SUCCESS; }
