//------------------------------------------------------------------------------------------------
// ArenaPlayerImpl.cpp (AI_Demo2)
//
// Implementation of required arena interface as defined in common header ArenaPlayerInterface.hpp
// Function entry points for Ant AI Player DLL (for AntArena 2.0)
//
#include "Common.hpp"
#include "Main.hpp"
#include "Colony.hpp"
#include "ThreadSafeStructures.hpp"



//------------------------------------------------------------------------------------------------
int GiveCommonInterfaceVersion() // DLL should return COMMON_INTERFACE_VERSION_NUMBER
{
	return COMMON_INTERFACE_VERSION_NUMBER; // must match the server's compiled version
}


//------------------------------------------------------------------------------------------------
const char* GivePlayerName() // DLL should return the name of the AI (can be whatever)
{
	return "Chimera Ants";
}


//------------------------------------------------------------------------------------------------
const char* GiveAuthorName() // DLL should return the actual human author's name
{
	return "Juan Ospina";
}


//------------------------------------------------------------------------------------------------
void PreGameStartup(const StartupInfo& info) // Server provides player/match info
{
	g_colony = new Colony(info);
	g_threadSafe_turnStatus = TURN_STATUS_WAITING_FOR_NEXT_UPDATE;
	g_debug = info.debugInterface;

	g_debug->SetMoodText("Ants are creepy");
}


//------------------------------------------------------------------------------------------------
void PostGameShutdown(const MatchResults& results) // Server signals the end of the match
{
	results;
	// Signal all of our threads to exit their loops
	g_isExiting = true;

	// Wait (block) until all threads have completed (effectively joining them here)
	while (g_threadSafe_threadCount > 0)
	{
		std::this_thread::yield();
	}
}


//------------------------------------------------------------------------------------------------
void PlayerThreadEntry(int yourThreadIndex) // Called from its own private thread (now owned by us!)
{
	// Thread 0 is our "primary" thread, and runs our main logic loop (and also does async jobs)
	// Threads 1,2,3... are pure async worker threads (which ONLY do async jobs)
	bool isPrimaryThread = (yourThreadIndex == 0);
	ThreadMain(isPrimaryThread);
}


//------------------------------------------------------------------------------------------------
void ReceiveTurnState(const ArenaTurnStateForPlayer& turnInfo) // Server tells us what happened, and what we see
{
	g_threadSafe_turnInfo.CopyFrom(turnInfo); // Copy off the turn state info we've just been given
	g_threadSafe_turnStatus = TURN_STATUS_PROCESSING_UPDATE;
}


//------------------------------------------------------------------------------------------------
bool TurnOrderRequest(int turnNumber, PlayerTurnOrders* ordersToFill) // We tell server what we do next
{
	turnNumber;
	if (g_threadSafe_turnStatus != TURN_STATUS_ORDERS_READY)
	{
		return false; // New orders aren't ready yet!!
	}

	// #ToDo: copy / fill out ready orders into server's struct
	g_threadSafe_turnOrders.CopyTo(*ordersToFill);
	g_threadSafe_turnStatus = TURN_STATUS_WAITING_FOR_NEXT_UPDATE;
	return true;
}

