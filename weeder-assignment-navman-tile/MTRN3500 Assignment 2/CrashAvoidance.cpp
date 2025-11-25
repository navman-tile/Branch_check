#include "CrashAvoidance.h"

using namespace System;
using namespace System::Threading;
using namespace System::Diagnostics;

CrashAvoidance::CrashAvoidance(SM_ThreadManagement^ SM_TM, SM_VehicleControl^ SM_VC, SM_LiDAR^ SM_Laser) {
	SM_TM_ = SM_TM;
	SM_LiDAR_ = SM_Laser;
	SM_VehicleControl_ = SM_VC;
	Watch = gcnew Stopwatch;
}
bool CrashAvoidance::getShutdownFlag() { return SM_TM_->shutdown & bit_CONTROLLER; }

void CrashAvoidance::shutdownThreads() { SM_TM_->shutdown = 0xff; }

error_state CrashAvoidance::checkData() { return error_state::SUCCESS; }

error_state CrashAvoidance::processSharedMemory() { return error_state::SUCCESS; }

error_state CrashAvoidance::connect(String^ hostName, int portNumber)  { return SUCCESS; }

error_state CrashAvoidance::processHeartbeats()
{
	if ((SM_TM_->heartbeat & bit_CRASHAVOIDANCE) == 0)
	{
		SM_TM_->heartbeat |= bit_CRASHAVOIDANCE;
		Watch->Restart();
	}
	else
	{
		if (Watch->ElapsedMilliseconds > CRASH_LIMIT)
		{
			return ERR_GNSS_FAILURE;
		}
	}
	return SUCCESS;
}
void CrashAvoidance::threadFunction()
{
	Console::WriteLine("CrashAvoidance		Thread is starting");
	Watch = gcnew Stopwatch;
	SM_TM_->ThreadBarrier->SignalAndWait();
	Watch->Start();
	int i = 0;

	while (!getShutdownFlag()) {
		Console::WriteLine("CrashAvoidance		Thread is running.");
		processHeartbeats();

		processSharedMemory();
		communicate();
		if (SM_VehicleControl_->CrashIndicator == 1) {
			Console::WriteLine("\n\n\n\CRASH IS INBOUND\n\n\n\n");
			Console::WriteLine("\n\n\n\FirstTimeCrash: {0}\n\n\n\n", FirstTimeCrash);
		}
		Thread::Sleep(50);
	}
	Console::WriteLine("CrashAvoidance		thread is terminating");

}
error_state CrashAvoidance::communicate()
{
	String^ steering = String::Format("{0}", SM_VehicleControl_->Steering);
	int steeringINT = SM_VehicleControl_->Steering;
	//made it negative to see difference
	steeringINT = -steeringINT;
	Console::WriteLine("\n\n the steering right now is: {0} \n\n", steeringINT);
	//Need to change steering angle into laser data points now
	int conversion = 180;

	if (steeringINT < 0.0) {
		conversion += static_cast<int>(-steeringINT * 2);
		Console::WriteLine("\n\n YAY, IT'S NEGATIVE \n\n");
	}
	else if (steeringINT > 0.0) {
		conversion -= static_cast<int>(steeringINT * 2);
		Console::WriteLine("\n\n POSITIVE! \n\n");
	}

	// Define conversion bounds
	int conversionlower = conversion - 20;
	int conversionupper = conversion + 20;

	//If goes under 0 or over 361 its not valid so chill
	if (conversionlower < 0) {
		conversionlower = 0;
	}
	if (conversionlower > 361) {
		conversionlower = 361;
	}
	if (conversionupper < 0) {
		conversionupper = 0;
	}
	if (conversionupper > 361) {
		conversionupper = 361;
	}
	// Loop through and print laser data
	for (int i = conversionlower; i <= conversionupper; ++i) {
		//Console::WriteLine("i:{0}, x:{1,0:F4} y:{2,0:F4}", i, SM_Laser_->x[i], SM_Laser_->y[i]);
		double x_len = SM_LiDAR_->x[i];
		double y_len = SM_LiDAR_->y[i];
		double distance = Math::Sqrt(x_len * x_len + y_len * y_len);
		if (distance < 1000.0 && distance != 0.0) {
			Console::WriteLine("FAIL: Distance less than 1 meter detected at i:{0}", i);
			SM_VehicleControl_->CrashIndicator = 1;

			if (FirstTimeCrash == 0) {
				Console::WriteLine("In the first time crash loop");
				SM_VehicleControl_->Speed = 0;
				SM_VehicleControl_->Steering = 0;
				FirstTimeCrash = 1;
			}
		}
	}


	//Console::WriteLine("i:{0}, x:{1, 0:F4} y:{2, 0:F4}", i, SM_Laser_->x[i], SM_Laser_->y[i]);

	return SUCCESS;
}