#using <System.dll>
#include "LiDAR.h"

using namespace System;
using namespace System::Threading;
using namespace System::Diagnostics;

LiDAR::LiDAR(SM_ThreadManagement^ SM_TM, SM_LiDAR^ SM_lidar)
{
	LiDAR_SM_TM_ = SM_TM;
	LiDAR_SM_LiDAR_ = SM_lidar;
	LiDAR_Stopwatch = gcnew Stopwatch;
}

error_state LiDAR::connect(String^ hostName, int portNumber)
{
	try
	{
		Console::WriteLine("LiDAR: Attempting to connect to {0}:{1}", hostName, portNumber);
		Client = gcnew TcpClient(hostName, portNumber);
		Console::WriteLine("LiDAR: TCP connection established");
	}
	catch (Exception^ e)
	{
		shutdownThreads();
		Console::WriteLine("LiDAR: Connection failed - " + e->Message);
		return ERR_CONNECTION;
	}

	Client->NoDelay = true;
	Client->ReceiveTimeout = 2500;
	Client->SendTimeout = 2500;
	Client->ReceiveBufferSize = 1024;
	Client->SendBufferSize = 1024;
	Stream = Client->GetStream();

	SendData = gcnew array<unsigned char>(2048);
	ReadData = gcnew array<unsigned char>(2048);


	return SUCCESS;
}
error_state LiDAR::communicate()
{
	String^ Command = "sRN LMDscandata";
	try
	{
		SendData = Encoding::ASCII->GetBytes(Command);
		Stream->WriteByte(StartByte);
		Stream->Write(SendData, 0, SendData->Length);
		Stream->WriteByte(EndByte);
		Thread::Sleep(50);
		Stream->Read(ReadData, 0, ReadData->Length);
	}
	catch (int error)
	{
		Console::WriteLine("Error: Failed to write request to Laser err_code=%d", error);
		shutdownThreads();
		return ERR_WRITE;
	}
	Thread::Sleep(10);
	try
	{
		ResponseData = System::Text::Encoding::ASCII->GetString(ReadData);
	}
	catch (int error)
	{
		Console::WriteLine("Error: Failed to read response from Laser err_code=%d", error);
		shutdownThreads();
		return ERR_READ;
	}
	return SUCCESS;
}

error_state LiDAR::authenticate()
{
	try {
		Console::WriteLine("LiDAR: Authenticating...");

		String^ authString = "5417679\n"; 
		SendData = Encoding::ASCII->GetBytes(authString);
		Stream->Write(SendData, 0, SendData->Length);
		Stream->Flush();

		Thread::Sleep(100);

		array<unsigned char>^ response = gcnew array<unsigned char>(10);
		int bytesRead = Stream->Read(response, 0, response->Length);

		if (bytesRead == 0) {
			Console::WriteLine("LiDAR: No authentication response");
			return ERR_CONNECTION;
		}

		String^ responseStr = Encoding::ASCII->GetString(response, 0, bytesRead);
		Console::WriteLine("LiDAR: Auth response: {0}", responseStr->Trim());

		if (responseStr->Contains("OK"))
		{
			Console::WriteLine("LiDAR: Authenticated successfully");
			return SUCCESS;
		}
		else
		{
			Console::WriteLine("LiDAR: Authentication failed - unexpected response");
			return ERR_CONNECTION;
		}
	}
	catch (Exception^ e) {
		Console::WriteLine("LiDAR: Authentication error - " + e->Message);
		return ERR_CONNECTION;
	}
}


error_state LiDAR::processSharedMemory()
{
	int NumPoints;

	array<String^>^ Fragments;
	String^ ResponseData = Encoding::ASCII->GetString(ReadData);


	if (ResponseData->Length > 361) {
		Fragments = ResponseData->Split(' ');

		double StartAngle = Convert::ToInt32(Fragments[23], 16);
		double Resolution = (double)Convert::ToInt32(Fragments[24], 16) / 10000.0;
		NumPoints = Convert::ToInt32(Fragments[25], 16);
		// Lock SM so nothing reads it at this point
		Monitor::Enter(LiDAR_SM_TM_->lockObject);

		Console::WriteLine("Angle Increment: {0}", Resolution);

		for (int i = 0; i < NumPoints; i++)
		{
			LiDAR_SM_LiDAR_->x[i] = (double)Convert::ToInt32(Fragments[26 + i], 16) * Math::Cos(i * Resolution * Math::PI / 180.0);
			LiDAR_SM_LiDAR_->y[i] = (double)Convert::ToInt32(Fragments[26 + i], 16) * Math::Sin(i * Resolution * Math::PI / 180.0);

			//Console::WriteLine("i:{0}, x:{1, 0:F4} y:{2, 0:F4}", i, LiDAR_SM_LiDAR_->x[i], LiDAR_SM_LiDAR_->y[i]);
			//Thread::Sleep(200);

			if (i == 271) {
				Console::WriteLine();
			}

		}
		Console::WriteLine("{0:D5} {1:D5}", NumPoints, Fragments->Length);

	}
	else
		return ERR_INVALID_DATA;

	//unlock SM
	Monitor::Exit(LiDAR_SM_TM_->lockObject);

	return SUCCESS;
}

error_state LiDAR::setupShareMemory() { return error_state::SUCCESS; }

void LiDAR::threadFunction()
{
	Console::WriteLine("LiDAR Thread is starting");

	if (connect(gcnew String(WEEDER_ADDRESS), 23000) != SUCCESS) {
		Console::WriteLine("LiDAR: Connection failed");
		shutdownThreads();
		return;
	}

	if (authenticate() != SUCCESS) {
		Console::WriteLine("LiDAR: Authentication failed");
		shutdownThreads();
		return;
	}

	Console::WriteLine("LiDAR: Connected and authenticated successfully");


	//wait for the barrier
	LiDAR_SM_TM_->ThreadBarrier->SignalAndWait();

	//start the stopwatch
	LiDAR_Stopwatch->Start();

	//start the thread loop
	while (!getShutdownFlag())
	{
		Console::WriteLine("LiDAR Thread is running");
		processHeartbeats();
		if (communicate() == SUCCESS && checkData() == SUCCESS)
		{
			processSharedMemory();
		}
		Thread::Sleep(20);
	}

	Console::WriteLine("LiDAR Thread is terminated");
}

error_state LiDAR::processHeartbeats()
{
	//check if the LiDAR bit in the hearbeat byte is low
	if ((LiDAR_SM_TM_->heartbeat & bit_LASER) == 0)
	{
		//put the LiDAR bit high
		LiDAR_SM_TM_->heartbeat |= bit_LASER;
		//reset the stopwatch
		LiDAR_Stopwatch->Restart();
	}

	else
	{
		//check if the time elasped exceed the crash time limit
		if (LiDAR_Stopwatch->ElapsedMilliseconds > CRASH_LIMIT)
		{
			//shut down all threads
			shutdownThreads();
			return error_state::ERR_TMT_FAILURE;
		}
	}
	return error_state::SUCCESS;
}

void LiDAR::shutdownThreads() { LiDAR_SM_TM_->shutdown = 0xFF; }

bool LiDAR::getShutdownFlag() { return LiDAR_SM_TM_->shutdown & bit_LASER; }

error_state LiDAR::checkData() { return error_state::SUCCESS; }