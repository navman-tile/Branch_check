#pragma once
#using <System.dll>
#include "SMObjects.h"
#include "NetworkedModule.h"
//#include "UGVModule.h"

using namespace System;
using namespace System::Threading;
using namespace System::Diagnostics;

ref class LiDAR : public NetworkedModule
{
public:
	LiDAR(SM_ThreadManagement^ SM_TM, SM_LiDAR^ SM_LiDAR);
	error_state setupShareMemory();
	void threadFunction() override;
	error_state processHeartbeats();
	void shutdownThreads(); 
	bool getShutdownFlag() override;
	error_state communicate() override;
	error_state checkData();
	error_state processSharedMemory() override;
	// added
	error_state connect(String^ hostName, int portNumber) override;
	error_state LiDAR::authenticate();

private:
	SM_ThreadManagement^ LiDAR_SM_TM_;
	SM_LiDAR^ LiDAR_SM_LiDAR_;
	Stopwatch^ LiDAR_Stopwatch;
	// added
	array<unsigned char>^ SendData;
	String^ ResponseData;
	bool bytesRequired;
	int StartByte = 0x02;
	int EndByte = 0x03;
	String^ Command;
};

//ref class SM_LiDAR
//{
//public:
//	SM_LiDAR(SM_ThreadManagement^ SM_TM, SM_LiDAR^ SM_LiDAR);
//
//	error_state setupShareMemory();
//	void threadFunction();
//	error_state processHeartbeats();
//	void shutdownThreads();
//	bool getShutdownFlag();
//	error_state communicate();
//	error_state checkData();
//	error_state processSharedMemory();
//
//private:
//	SM_ThreadManagement^ LiDAR_SM_TM_;
//	SM_LiDAR^ LiDAR_SM_LiDAR_;
//	Stopwatch^ LiDAR_Stopwatch;
//};