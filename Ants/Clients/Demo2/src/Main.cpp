#include "Main.hpp"
#include "Common.hpp"
#include "Colony.hpp"
#include "ThreadSafeStructures.hpp"


//------------------------------------------------------------------------------------------------
// Thread-safe globals
//
DebugInterface*		g_debug = nullptr;				// Server function interface, use from any thread
std::atomic<bool>	g_isExiting = false;			// Set to true only once; signals all threads to exit
std::atomic<int>	g_threadSafe_threadCount = 0;	// How many threads this DLL was given by the Arena


//------------------------------------------------------------------------------------------------
// Entry point for our primary thread and any number of secondary threads.
//
// The primary thread "owns" the Colony instance; only the primary thread can directly use
//	g_colony, whose methods and data are NOT considered thread-safe (and are not for use by
//	other secondary worker threads) UNLESS they are specifically named "threadSafe".
//
// Both primary and secondary threads process async work jobs; secondary threads do this ONLY.
//
void ThreadMain( bool isPrimaryThread )
{
	++ g_threadSafe_threadCount;
	while( !g_isExiting )
	{
		if( isPrimaryThread )
		{
			// This shouldn't block for any more than 1ms (ideally 0.5ms or less).
			// Heavy work such as A*, distance field generation, strategic analysis, etc.
			//	should be done asynchonously in other (non-primary) threads.
			g_colony->Update();
		}
		else
		{
			// Grant path requests, etc.
			DoAsyncWork();
		}
	}
	-- g_threadSafe_threadCount;
}


//------------------------------------------------------------------------------------------------
void DoAsyncWork()
{
	std::this_thread::yield();
	// #ToDo: What sort of work can we have the non-primary thread(s) do?  Generate paths and
	//	distance fields?  Solve the Hungarian Problem for worker-food assignment?  Calculate pi?
}

