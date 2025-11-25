#using <System.dll>
#include "Controller.h"
#include "TMM.h"
#include "Lidar.h"
#include "GNSS.h"
#include "Display.h"
#include "VC.h"


error_state ThreadManagement::setupSharedMemory()
{
	SM_TM_ = gcnew SM_ThreadManagement;
	SM_LiDAR_ = gcnew SM_LiDAR;
	SM_GNSS_ = gcnew SM_GNSS;
	SM_VehicleControl_ = gcnew SM_VehicleControl;

	return error_state::SUCCESS;
}

error_state ThreadManagement::processSharedMemory() { return error_state::SUCCESS; }

error_state ThreadManagement::processHeartbeats()
{
	//check the heartbeat flag of the ith thread (is it high?)
	for (int i = 0; i < ThreadList->Length; i++)
	{
		if (SM_TM_->heartbeat & ThreadPropertiesList[i]->BitID)
		{
			//if high, put the ith bit (flag) down
			SM_TM_->heartbeat &= ~ThreadPropertiesList[i]->BitID;
			//reset the stopwatch
			StopwatchList[i]->Restart();
		}

		else
		{
			//check the stopwatch, is the time exceed the crash limit?
			if (StopwatchList[i]->ElapsedMilliseconds > CRASH_LIMIT)
			{
				//is the ith thread a critical one?
				if (ThreadPropertiesList[i]->Critical)
				{
					Console::WriteLine(ThreadPropertiesList[i]->ThreadName + " (critical) failure. Shutting down all threads");
					shutdownModules();//shut down all
					return error_state::ERR_CRITICAL_THREAD_FAILURE;
				}

				else
				{
					Console::WriteLine(ThreadPropertiesList[i]->ThreadName + " failed...Restarting");
					ThreadList[i]->Abort();
					ThreadList[i] = gcnew Thread(ThreadPropertiesList[i]->ThreadStart_);
					SM_TM_->ThreadBarrier = gcnew Barrier(1);
					ThreadList[i]->Start();
				}
			}
		}
	}
	return error_state::SUCCESS;
}

void ThreadManagement::shutdownModules() { SM_TM_->shutdown = 0xFF; }

bool ThreadManagement::getShutdownFlag() { return (SM_TM_->shutdown & bit_TM); }

void ThreadManagement::threadFunction() {
	Console::WriteLine("TMT Thread is starting");

	ThreadPropertiesList = gcnew array<ThreadProperties^>{
		//gcnew ThreadProperties(gcnew ThreadStart(gcnew Controller(SM_TM_, SM_VehicleControl_), &Controller::threadFunction), true, bit_CONTROLLER, "Controller Thread"),
		gcnew ThreadProperties(gcnew ThreadStart(gcnew LiDAR(SM_TM_, SM_LiDAR_), &LiDAR::threadFunction), true, bit_LASER, "LiDAR Thread"),
		gcnew ThreadProperties(gcnew ThreadStart(gcnew GNSS(SM_TM_, SM_GNSS_), &GNSS::threadFunction), false, bit_GPS, "GNSS Thread"),
		gcnew ThreadProperties(gcnew ThreadStart(gcnew VC(SM_TM_, SM_VehicleControl_), &VC::threadFunction), true, bit_VC, "Vehicle Control Thread"),
		gcnew ThreadProperties(gcnew ThreadStart(gcnew Display(SM_TM_, SM_LiDAR_, SM_GNSS_), &Display::threadFunction), true, bit_DISPLAY, "Display Thread"),
	};

	//make a list of thread
	ThreadList = gcnew array<Thread^>(ThreadPropertiesList->Length);

	//make the stopwatch list
	StopwatchList = gcnew array<Stopwatch^>(ThreadPropertiesList->Length);

	//make thread barriers
	SM_TM_->ThreadBarrier = gcnew Barrier(ThreadPropertiesList->Length + 1);

	//start all the threads
	for (int i = 0; i < ThreadPropertiesList->Length; i++)
	{
		StopwatchList[i] = gcnew Stopwatch;
		ThreadList[i] = gcnew Thread(ThreadPropertiesList[i]->ThreadStart_);
		ThreadList[i]->Start();
	}

	//wait at the TMT thread barrier
	SM_TM_->ThreadBarrier->SignalAndWait();

	//start all the stopwatches
	for (int i = 0; i < ThreadList->Length; i++)
	{
		StopwatchList[i]->Start();
	}

	//start the thread loop + keep checking the heartbeats
 	while (!getShutdownFlag())
	{
		Console::WriteLine("TMT Thread is running");
		checkQuit();
		processHeartbeats();
		Thread::Sleep(50);
	}

	shutdownModules();

	//join all threads
	for (int i = 0; i < ThreadPropertiesList->Length; i++)
	{
		ThreadList[i]->Join();
	}

	Console::WriteLine("TMT Thread is terminated");
}

void ThreadManagement::checkQuit() {
	if (Console::KeyAvailable) {
		ConsoleKeyInfo key = Console::ReadKey(true);

		if (key.KeyChar == 'q' || key.KeyChar == 'Q') {
			Console::WriteLine("\n'Q' pressed - initiating shutdown...");
			shutdownModules();
		}
	}
}