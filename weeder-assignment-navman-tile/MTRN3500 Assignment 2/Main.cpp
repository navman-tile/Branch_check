// commit

#include "TMM.h"
#include "Display.h"

int main()
{
	ThreadManagement^ myTMT = gcnew ThreadManagement();

	Console::WriteLine("Main is running");

	myTMT->setupSharedMemory();

	myTMT->threadFunction();

	Console::ReadKey();
	Console::ReadKey();

	return 0;
}