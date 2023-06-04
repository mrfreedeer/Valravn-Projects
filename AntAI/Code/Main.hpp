#pragma once
#include "Common.hpp"


//------------------------------------------------------------------------------------------------
void ThreadMain( bool isPrimaryThread );	// Entry point for our "primary" thread and extra "worker" threads
void DoAsyncWork();							// i.e. "Claim and execute 1 async work job"
int  GetActiveThreadCount();				// How many of our own work threads (including primary/main) are running
