//#using<System.dll>
#include "VC.h"

using namespace System;
using namespace System::Threading;
using namespace System::Diagnostics;

VC::VC(SM_ThreadManagement^ SM_TM, SM_VehicleControl^ SM_VC) {
	SM_VehicleControl_ = SM_VC;
	SM_TM_ = SM_TM;
	Flag = 0;
	Watch = gcnew Stopwatch;
}

void VC::shutdownThreads() { SM_TM_->shutdown = 0xff; };

bool VC::getShutdownFlag() { return (SM_TM_->shutdown & bit_VC); };

void VC::threadFunction() {
	Console::WriteLine("VC Thread is starting.");

	connect(WEEDER_ADDRESS, 25000);

	SM_TM_->ThreadBarrier->SignalAndWait();

	Watch->Start();

	while (!getShutdownFlag()) {
		Console::WriteLine("VC Thread is running.");
		processHeartbeats();
		communicate();
		Thread::Sleep(50);
	}

	processSharedMemory();
	Thread::Sleep(50);

	Console::WriteLine("VC Thread is terminating.");
	Stream->Close();
	Client->Close();
};

error_state VC::processHeartbeats() {
	// check if bit is down and reset if so
	if ((SM_TM_->heartbeat & bit_VC) == 0) {
		SM_TM_->heartbeat |= bit_VC;
		Watch->Restart();
	}
	else {
		// since critical shutdown program
		if (Watch->ElapsedMilliseconds > CRASH_LIMIT) {
			shutdownThreads();
			return ERR_VC_FAILURE;
		}
	}
	return SUCCESS;
};

error_state VC::communicate() {
	try {
		Thread::Sleep(10);
		// set flag
		if (Flag == 0) {
			Flag = 1;
		}
		else {
			Flag = 0;
		}

		String^ speed;
		String^ steering = String::Format("{0}", SM_VehicleControl_->Steering);
		String^ flag = String::Format("{0}", (int)Flag);

		// If CrashIndicator is 0, send regular command
		if (CrashIndicator == 0) {
			speed = String::Format("{0}", SM_VehicleControl_->Speed);  // Regular speed
		}
		else if (CrashIndicator == 1) {
			//speed = String::Format("{0}", 0);
			// If CrashIndicator is 1, ensure speed is negative
			if (SM_VehicleControl_->Speed < 0) {
				speed = String::Format("{0}", SM_VehicleControl_->Speed);
			}
		}
		String^ command = "# " + steering + " " + speed + " " + flag + " " + "#";
		Console::WriteLine("Sending command: {0}", command);
		SendData = Encoding::ASCII->GetBytes(command);
		Stream->Write(SendData, 0, SendData->Length); // sending command
		Thread::Sleep(10);
		return SUCCESS;
	}
	catch (Exception^ e) {
		Console::WriteLine("VC: Authentication error - " + e->Message);
		return ERR_CONNECTION;
	}
	
}

error_state VC::connect(String^ hostName, int portNumber) {
	try {
		Client = gcnew TcpClient(hostName, portNumber);
		Stream = Client->GetStream();
		Client->NoDelay = true;
		Client->ReceiveTimeout = 500;
		Client->SendTimeout = 500;
		Client->ReceiveBufferSize = 1024;
		Client->SendBufferSize = 1024;

		ReadData = gcnew array<unsigned char>(2048);
		SendData = gcnew array<unsigned char>(128);

		String^ verification = "5417679\n";
		SendData = Encoding::ASCII->GetBytes(verification);
		Stream->Write(SendData, 0, SendData->Length);
		System::Threading::Thread::Sleep(10);
		Stream->Read(ReadData, 0, ReadData->Length);
	}
	catch (Exception^ e) {
		Console::WriteLine("\n\nError connecting to Vehicle Control device: " + e->Message);
		return ERR_CONNECTION;
	}
	return SUCCESS;
};

error_state VC::checkData() { return error_state::SUCCESS; }

error_state VC::processSharedMemory() { return error_state::SUCCESS; }

error_state VC::authenticate() { return error_state::SUCCESS; }