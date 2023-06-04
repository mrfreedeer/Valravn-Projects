#include "ThreadSafeStructures.hpp"


//------------------------------------------------------------------------------------------------
// Thread-safe globals
//
ThreadSafe_TurnStatus				g_threadSafe_turnStatus;	// Thread-safe wrapper around eTurnStatus
ThreadSafe_ArenaTurnStateForPlayer	g_threadSafe_turnInfo;		// Middle "hand-off" buffer of triple-buffering scheme
ThreadSafe_PlayerTurnOrders			g_threadSafe_turnOrders;	// Middle "hand-off" buffer of triple-buffering scheme
std::atomic<int>					g_threadSafe_turnNumberOfLatestUpdateReceived = 0;
std::atomic<int>					g_threadSafe_turnNumberOfLatestTurnOrdersSent = 0;



//////////////////////////////////////////////////////////////////////////////////////////////////
// ThreadSafe_TurnStatus
//////////////////////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------------------------
const char* GetNameForTurnStatus( eTurnStatus turnStatus )
{
	switch( turnStatus )
	{
	case TURN_STATUS_WAITING_FOR_PREGAME_STARTUP:	return "WAITING_FOR_PREGAME_STARTUP";
	case TURN_STATUS_WAITING_FOR_NEXT_UPDATE:		return "WAITING_FOR_NEXT_UPDATE";
	case TURN_STATUS_PROCESSING_UPDATE:				return "PROCESSING_UPDATE";
	case TURN_STATUS_WORKING_ON_ORDERS:				return "WORKING_ON_ORDERS";
	case TURN_STATUS_ORDERS_READY:					return "ORDERS_READY";
	default:	
		Errorf( "Unknown turnStatus #%i", turnStatus );
		return "UNKNOWN";
	}
}


//------------------------------------------------------------------------------------------------
void ThreadSafe_TurnStatus::Assert( eTurnStatus shouldBeTurnState )
{
	eTurnStatus turnState = Get();
	if( turnState != shouldBeTurnState )
	{
		Errorf( "ThreadSafeTurnState should be %s(%i), was %s(%i)", GetNameForTurnStatus(shouldBeTurnState), shouldBeTurnState, GetNameForTurnStatus(turnState), turnState );
	}
}


//------------------------------------------------------------------------------------------------
void ThreadSafe_TurnStatus::Set( eTurnStatus newState )
{
	m_mutex.lock();
	m_turnState = newState;
	m_mutex.unlock();
}


//------------------------------------------------------------------------------------------------
eTurnStatus ThreadSafe_TurnStatus::Get() const
{
	m_mutex.lock();
	eTurnStatus turnState = m_turnState;
	m_mutex.unlock();
	return turnState;
}



//////////////////////////////////////////////////////////////////////////////////////////////////
// ThreadSafe_TurnInfo
//////////////////////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------------------------
void ThreadSafe_ArenaTurnStateForPlayer::CopyFrom( const ArenaTurnStateForPlayer& copyFrom )
{
	m_mutex.lock();
	m_arenaState = copyFrom;
	m_mutex.unlock();
}


//------------------------------------------------------------------------------------------------
void ThreadSafe_ArenaTurnStateForPlayer::CopyTo( ArenaTurnStateForPlayer& copyTo ) const
{
	m_mutex.lock();
	copyTo = m_arenaState;
	m_mutex.unlock();
}



//////////////////////////////////////////////////////////////////////////////////////////////////
// ThreadSafe_TurnOrders
//////////////////////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------------------------
void ThreadSafe_PlayerTurnOrders::CopyFrom( const PlayerTurnOrders& copyFrom )
{
	m_mutex.lock();
	m_turnOrders = copyFrom;
	m_mutex.unlock();
}


//------------------------------------------------------------------------------------------------
void ThreadSafe_PlayerTurnOrders::CopyTo( PlayerTurnOrders& copyTo ) const
{
	m_mutex.lock();
	copyTo = m_turnOrders;
	m_mutex.unlock();
}


