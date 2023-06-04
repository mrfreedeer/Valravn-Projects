#pragma once
#include "Common.hpp"


//------------------------------------------------------------------------------------------------
class ThreadSafe_TurnStatus;
class ThreadSafe_ArenaTurnStateForPlayer;
class ThreadSafe_PlayerTurnOrders;


//------------------------------------------------------------------------------------------------
// Thread-safe globals
//
extern ThreadSafe_TurnStatus				g_threadSafe_turnStatus;
extern ThreadSafe_ArenaTurnStateForPlayer	g_threadSafe_turnInfo;
extern ThreadSafe_PlayerTurnOrders			g_threadSafe_turnOrders;


//------------------------------------------------------------------------------------------------
enum eTurnStatus : uint8_t
{
	TURN_STATUS_WAITING_FOR_PREGAME_STARTUP,
	TURN_STATUS_WAITING_FOR_NEXT_UPDATE,
	TURN_STATUS_PROCESSING_UPDATE,
	TURN_STATUS_WORKING_ON_ORDERS,
	TURN_STATUS_ORDERS_READY,
	NUM_TURN_STATUSES
};
const char* GetNameForTurnStatus( eTurnStatus turnStatus );


//////////////////////////////////////////////////////////////////////////////////////////////////
// ThreadSafe_TurnStatus
//
// A thread-safe (self-protecting) version of the eTurnStatus enum.  Could be an atomic instead.
//////////////////////////////////////////////////////////////////////////////////////////////////
class ThreadSafe_TurnStatus
{
public:
	void Assert( eTurnStatus shouldBeTurnState );
	void Set( eTurnStatus newState );
	eTurnStatus Get() const;

private:
	eTurnStatus			m_turnState = TURN_STATUS_WAITING_FOR_PREGAME_STARTUP;
	mutable std::mutex	m_mutex;
};


//////////////////////////////////////////////////////////////////////////////////////////////////
// ThreadSafe_ArenaTurnStateForPlayer
//
// A thread-safe (self-protecting) version of the ArenaTurnStateForPlayer struct.
// Used as middle buffer in triple-buffering scheme (to avoid lengthy blocking on read or write).
//////////////////////////////////////////////////////////////////////////////////////////////////
class ThreadSafe_ArenaTurnStateForPlayer
{
public:
	void CopyFrom( const ArenaTurnStateForPlayer& copyFrom );
	void CopyTo( ArenaTurnStateForPlayer& copyTo ) const;

private:
	ArenaTurnStateForPlayer		m_arenaState;
	mutable std::mutex			m_mutex;
};


//////////////////////////////////////////////////////////////////////////////////////////////////
// ThreadSafe_PlayerTurnOrders
//
// A thread-safe (self-protecting) version of the PlayerTurnOrders struct.
// Used as middle buffer in triple-buffering scheme (to avoid lengthy blocking on read or write).
//////////////////////////////////////////////////////////////////////////////////////////////////
class ThreadSafe_PlayerTurnOrders
{
public:
	void CopyFrom( const PlayerTurnOrders& copyFrom );
	void CopyTo( PlayerTurnOrders& copyTo ) const;

private:
	PlayerTurnOrders			m_turnOrders;
	mutable std::mutex			m_mutex;
};


