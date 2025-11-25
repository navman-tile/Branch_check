#include "Controller.h"

using namespace System;
using namespace System::Threading;
using namespace System::Diagnostics;

Controller::Controller(SM_ThreadManagement^ SM_TM, SM_VehicleControl^ SM_VC) {
	SM_VehicleControl_ = SM_VC;
	SM_TM_ = SM_TM;
	Watch = gcnew Stopwatch;
}

error_state Controller::processSharedMemory() {
	Monitor::Enter(SM_VehicleControl_->lockObject);
	SM_VehicleControl_->Speed = speed;
	SM_VehicleControl_->Steering = angle;

	Monitor::Exit(SM_VehicleControl_->lockObject);
	return SUCCESS;
}

bool Controller::getShutdownFlag() { return SM_TM_->shutdown & bit_CONTROLLER; }

void Controller::shutdownThreads() { SM_TM_->shutdown = 0xff; }

error_state Controller::processHeartbeats() {
	if ((SM_TM_->heartbeat & bit_CONTROLLER) == 0) {
		SM_TM_->heartbeat |= bit_CONTROLLER;
		Watch->Restart();

	}
	else {
		if (Watch->ElapsedMilliseconds > CRASH_LIMIT) {
			shutdownThreads();
			return ERR_CONTROLLER_FAILURE;
		}
	}
	return SUCCESS;
}

void Controller::threadFunction() {
	Console::WriteLine("Controller thread is starting");
	DWORD playernum = 1;
	ControllerInterface myController(playernum, CONTROLLER);
	SM_TM_->ThreadBarrier->SignalAndWait();
	Watch->Start();
	while (!getShutdownFlag()) {
		Console::WriteLine("Controller Thread is running.");
		processHeartbeats();
		if (!myController.IsConnected()) {
			Console::WriteLine("\n\nController disconnected	exiting process...");
			shutdownThreads();
		}

		if (SM_VehicleControl_->CrashIndicator == 0) {
			speed = myController.GetState().rightTrigger - myController.GetState().leftTrigger;
		}
		else {
			speed = -myController.GetState().leftTrigger;
		}

		// steering angle
		if (myController.GetState().rightThumbX == 0) {
			angle = 0;
		}
		else {
			angle = -myController.GetState().rightThumbX * 40;
			Console::WriteLine("angle: {0}", angle);
			if (angle <= -40) {
				angle = -40;
			}
			if (angle >= 40) {
				angle = 40;
			}
		}

		if (myController.GetState().buttonX) {
			Console::WriteLine("\n\nBreak key pressed exiting process...");
			SM_VehicleControl_->Speed = 0;
			SM_VehicleControl_->Steering = 0;
			shutdownThreads();
			break;
		}

		processSharedMemory();
		myController.printControllerState(myController.GetState());
		Thread::Sleep(50);
	}
	Console::WriteLine("Controller thread is terminating");
}

error_state Controller::checkData() { return error_state::SUCCESS; }