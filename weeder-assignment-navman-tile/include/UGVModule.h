#pragma once
/*****************
UGVModule.h
ALEXANDER CUNIO 2024
*****************/

/*
This file defines the base class for ALL UGV modules (TMM, Laser, GPS, Vehicle Control, Display, Controller).
Basic classes (TMM, Controller) will be derived directly from this class, whereas those that require networking
to external devices (GPS, Laser, VC, Display) will derive from the networked module class (which itself inherits from this class).
For clarification of inheritance requirements see diagram in the assignment specification.
*/

#include <iostream>
#include <SMObjects.h>

// ERROR HANDLING. Use this as the return value in your functions
enum error_state {
	SUCCESS,
	ERR_STARTUP,
	ERR_NO_DATA,
	ERR_INVALID_DATA,
	ERR_SM,
	ERR_CONNECTION,
	// Define your own additional error types as needed
	ERR_TMT_FAILURE,
	ERR_READ,
	ERR_WRITE,
	ERR_CRITICAL_THREAD_FAILURE,
	ERR_GNSS_FAILURE,
	ERR_DISPLAY_FAILURE,
	ERR_CONTROLLER_FAILURE,
	ERR_VC_FAILURE,
};

ref class UGVModule abstract
{
	public:
		/**
		 * Send/Recieve data from shared memory structures
		 *
		 * @returns the success of this function at completion
		*/
		virtual error_state processSharedMemory() = 0;

		/**
		 * Get Shutdown signal for the current, from Process Management SM
		 *
		 * @returns the current state of the shutdown flag
		*/
		virtual bool getShutdownFlag() = 0;

		/**
		 * Main runner for the thread to perform all required actions
		*/
		virtual void threadFunction() = 0;

		/**
		 * A helper function for printing a helpful error message based on the error type
		 * returned from a function
		 *
		 * @param error the error that should be printed out
		*/
		static void printError(error_state error)
		{
			switch (error)
			{
				case SUCCESS:
					std::cout << "Success." << std::endl;
					break;
				case ERR_NO_DATA:
					std::cout << "ERROR: No Data Available." << std::endl;
					break;
				case ERR_INVALID_DATA:
					std::cout << "ERROR: Invalid Data Received." << std::endl;
					break;
				case ERR_STARTUP:
					std::cout << "ERROR: Unable To Startup." << std::endl;
					break;
				case ERR_SM:
					std::cout << "ERROR: Unable To Access Shared Memory." << std::endl;
					break;
				case ERR_CONNECTION:
					std::cout << "ERROR: Unable To Connect." << std::endl;
					break;
				// ADD PRINTOUTS FOR OTHER ERROR TYPES
				case ERR_TMT_FAILURE:
					std::cout << "ERROR: TMT has failed. Stopping all threads..." << std::endl;
					break;
				case ERR_READ:
					std::cout << "ERROR: Failed to read request from Laser" << std::endl;
					break;
				case ERR_WRITE:
					std::cout << "ERROR: Failed to write request to Laser" << std::endl;
					break;
				case ERR_CRITICAL_THREAD_FAILURE: 
					std::cout << "ERROR: Critical Thread Failure, stopping all processes" << std::endl;
					break;
				case ERR_GNSS_FAILURE:
					std::cout << "ERROR: GNSS Failure, stopping GNSS" << std::endl;
					break;
				case ERR_DISPLAY_FAILURE:
					std::cout << "ERROR: Display Failure, stopping Display" << std::endl;
					break;
				case ERR_CONTROLLER_FAILURE:
					std::cout << "ERROR: Controller Failure, stopping controller" << std::endl;
					break;
				case ERR_VC_FAILURE:
					std::cout << "ERROR: Vehicle Control Failure, stopping all processes" << std::endl;
					break;
			};

		}

	protected:
		SM_ThreadManagement^ SM_TM_;
};
