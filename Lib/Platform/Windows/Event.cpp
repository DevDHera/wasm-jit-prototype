#include "WAVM/Platform/Event.h"
#include "WAVM/Inline/Assert.h"
#include "WAVM/Inline/BasicTypes.h"
#include "WAVM/Platform/Event.h"

#define NOMINMAX
#include <Windows.h>

using namespace WAVM;
using namespace WAVM::Platform;

U64 Platform::getMonotonicClock()
{
	LARGE_INTEGER performanceCounter;
	LARGE_INTEGER performanceCounterFrequency;
	QueryPerformanceCounter(&performanceCounter);
	QueryPerformanceFrequency(&performanceCounterFrequency);

	const U64 wavmFrequency = 1000000;

	return performanceCounterFrequency.QuadPart > wavmFrequency
			   ? performanceCounter.QuadPart
					 / (performanceCounterFrequency.QuadPart / wavmFrequency)
			   : performanceCounter.QuadPart
					 * (wavmFrequency / performanceCounterFrequency.QuadPart);
}

Platform::Event::Event()
{
	handle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	errorUnless(handle);
}

Platform::Event::~Event() { errorUnless(CloseHandle(handle)); }

bool Platform::Event::wait(U64 untilTime)
{
	U64 currentTime = getMonotonicClock();
	const U64 startProcessTime = currentTime;
	while(true)
	{
		const U64 timeoutMicroseconds = currentTime > untilTime ? 0 : (untilTime - currentTime);
		const U64 timeoutMilliseconds64 = timeoutMicroseconds / 1000;
		const U32 timeoutMilliseconds32
			= timeoutMilliseconds64 > UINT32_MAX ? (UINT32_MAX - 1) : U32(timeoutMilliseconds64);

		const U32 waitResult = WaitForSingleObject(handle, timeoutMilliseconds32);
		if(waitResult != WAIT_TIMEOUT)
		{
			errorUnless(waitResult == WAIT_OBJECT_0);
			return true;
		}
		else
		{
			currentTime = getMonotonicClock();
			if(currentTime >= untilTime) { return false; }
		}
	};
}

void Platform::Event::signal() { errorUnless(SetEvent(handle)); }
