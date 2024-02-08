//-----------------------------------------------------------------------------------------------
// ArenaPlayerInterface.hpp
//
// Common interface used by both the Arena Server and by each Arena Player (DLL).
// Defines enums, types, structures, and functions necessary to communicate with Arena Server.
//
#pragma once

//-----------------------------------------------------------------------------------------------
// Revision History
// 1: DLL Interface created
// 2: Separated combatPriority and sacrificePriority
// 3: Assorted changes made due to logic going in - mostly error codes 
// 4: Added nutrients lost due to faults to the report
// 5: Removed sand and gravel tile types (as well as STATE_HOLDING_SAND, etc.)
// 6: Queen suffocation penalty added
// 7: Sudden death nutrient upkeep added
// 8: New agent order for failed move due to queens sharing tile
// 9: Added a RequestPauseAfterTurn() to the debug interface; 
// 10: changed eOrderCode storage type to unsigned char (was default-int)
// 11: Removed some redundant order result codes, and added nutrient loss information to given stats
// 12: Exposed SetMoodText() function in DebugInterface struct; changes one-line player emote text
// (12): Added GetMouseCoords to DebugInterface
//-----------------------------------------------------------------------------------------------
constexpr int	COMMON_INTERFACE_VERSION_NUMBER		= 12;


//-----------------------------------------------------------------------------------------------
// Macros
//-----------------------------------------------------------------------------------------------
#if defined( ARENA_SERVER )
	#define DLL __declspec( dllimport )
#else // ARENA_PLAYER
	#define DLL __declspec( dllexport )
#endif

//-----------------------------------------------------------------------------------------------
// Common Typedefs
//-----------------------------------------------------------------------------------------------
typedef unsigned char	TeamID;		// 200+
typedef unsigned char	PlayerID;	// 100+
typedef unsigned int	AgentID;	// unique per agent, highest byte is agent's owning playerID


//-----------------------------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------------------------

// Hard limits (will never be exceeded); actual limits will be provided during
// startup, and may (typically) be much lower.
constexpr short	MAX_ARENA_WIDTH						= 256;
constexpr int	MAX_ARENA_TILES						= (MAX_ARENA_WIDTH * MAX_ARENA_WIDTH);
constexpr char	MAX_PLAYERS							= 32;
constexpr char	MAX_TEAMS							= MAX_PLAYERS;
constexpr char	MAX_PLAYERS_PER_TEAM				= MAX_PLAYERS;

constexpr int	MAX_AGENTS_PER_PLAYER				= 256;
constexpr int	MAX_ORDERS_PER_PLAYER				= MAX_AGENTS_PER_PLAYER;
constexpr int	MAX_REPORTS_PER_PLAYER				= 2 * MAX_AGENTS_PER_PLAYER;
constexpr int	MAX_AGENTS_TOTAL					= (MAX_PLAYERS * MAX_AGENTS_PER_PLAYER);

// special penalty values for digging/moving
constexpr int DIG_IMPOSSIBLE						= -1; 
constexpr int TILE_IMPASSABLE						= -1; 

//-----------------------------------------------------------------------------------------------
// Enums
//-----------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------
// "Flagrant Fouls" the server can award for illegal orders or bad behavior on the part of player
//	DLLs.  The server may or may not impose penalties (e.g. starvations) or ejection as a result.
enum eFaultType : unsigned char 
{
	FAULT_NONE		= 0, 
	FAULT_TURN_START_DURATION_ELAPSED,	// Player DLL exceeded allowed time to return from main-thread blocking call to ReceiveTurnState()
	FAULT_ORDER_FETCH_DURATION_ELAPSED, // Player DLL exceeded allowed time to return from main-thread blocking call to TurnOrderRequest()
	FAULT_TOTAL_TURN_DURATION_ELAPSED,	// (Currently unused)
	FAULT_INVALID_ORDER,				// Illegal order code, or other general order corruption.
	FAULT_DOUBLE_ORDERED_AGENT,			// Sent two orders for the same AgentID in the same turn.
	FAULT_DID_NOT_OWN_AGENT,			// Sent orders for AgentID not owned by the player
	FAULT_INVALID_AGENT_ID,				// Sent orders for an invalid (or possibly dead) AgentID
};

//-----------------------------------------------------------------------------------------------
// Agent (player ant entity) types allowed in the Arena.
enum eAgentType : unsigned char
{
	AGENT_TYPE_SCOUT,	// Typically cheapest upkeep, farthest visibility, fastest movement (e.g. may move through dirt freely).
	AGENT_TYPE_WORKER,	// Typically the only ant type which can carry & drop tiles and food; used mostly to feed the Queen(s).
	AGENT_TYPE_SOLDIER,	// Typically the highest combat value (beats everything except Soldier).  Mostly useful in multiplayer.
	AGENT_TYPE_QUEEN,	// The only type which can consume food and (typically) birth new ants (perhaps even Queens).  High upkeep.

	NUM_AGENT_TYPES, 
	INVALID_AGENT_TYPE = 0xff
};

//-----------------------------------------------------------------------------------------------
// Orders that can be issued by player DLLs; up to one for each living ant they control per turn.
enum eOrderCode : unsigned char
{
	ORDER_HOLD = 0,				// Stand still; do not move.  A valid order, and the default order if none is specified.

	ORDER_MOVE_EAST,			// Moving into some tile types (e.g. Dirt) may cause exhaustion (# of forced-rest turns afterward);
	ORDER_MOVE_NORTH,			//	check the ant's AgentTypeInfo.moveExhaustPenalties[tileType] for expected move exhaustion costs.
	ORDER_MOVE_WEST,			// Typically most ants can only move into TILE_TYPE_AIR; some ants (e.g. Scouts, Workers) may be
	ORDER_MOVE_SOUTH,			//	allowed to move into other tile types (e.g. TILE_TYPE_DIRT), with/without an exhaustion penalty.

	ORDER_DIG_HERE,				// Exception: does not cause dig exhaustion!
	ORDER_DIG_EAST,				
	ORDER_DIG_NORTH,			// Some ants (e.g. Workers) can dig (destroy) certain adjacent tile types (e.g. Dirt);
	ORDER_DIG_WEST,				//	digging may cause exhaustion (# of forced-rest turns afterward); 
	ORDER_DIG_SOUTH,			//	check the ant's AgentTypeInfo.moveExhaustPenalties[tileType] for expected move exhaustion costs.

	ORDER_PICK_UP_FOOD,			// Some ants (e.g. Workers) may carry either a single food or a single tile.
	ORDER_PICK_UP_TILE,			// Tiles can be carried & dropped to build defensive structures, etc.
	ORDER_DROP_CARRIED_OBJECT,	// Food or tile is dropped here; if food is dropped onto Queen, she is fed.

	ORDER_BIRTH_SCOUT,			// Some ants (e.g. Queens) can give birth to other ants;
	ORDER_BIRTH_WORKER,			//	check AgentTypeInfo.costToBirth for actual birthing nutrient cost for each ant type;
	ORDER_BIRTH_SOLDIER,		//	note that most ant types have a nutrient upkeep cost that must be paid each turn by the colony.
	ORDER_BIRTH_QUEEN,			// Even Queens may be birthable; Queens eat & birth independently but nutrients are shared by all.

	ORDER_SUICIDE,				// Ants may willingly suicide if they become a liability, or to prevent a random death during starvation.

	// Emotes; purely cosmetic, may be ignored by server.
	ORDER_EMOTE_HAPPY,
	ORDER_EMOTE_SAD,
	ORDER_EMOTE_ANGRY,
	ORDER_EMOTE_TAUNT,
	ORDER_EMOTE_DEPRESSED,
	ORDER_EMOTE_CONFUSED, 
	ORDER_EMOTE_SCARED, 
	ORDER_EMOTE_ASTONISHED, 

	NUM_ORDERS
};

//-----------------------------------------------------------------------------------------------
// Every eOrderCode submitted the previous turn will receive at least one (and at most two) result codes.
enum eAgentOrderResult : unsigned char
{
	// events
	AGENT_WAS_CREATED,								// "Order" result given on an agent's first turn in existence.
	AGENT_KILLED_BY_ENEMY,							// Agent was killed by contacting an enemy of equal or greater combat strength.
	AGENT_KILLED_BY_WATER,							// Agent drowned by entering TILE_TYPE_WATER, converting it to TILE_TYPE_CORPSE_BRIDGE.
	AGENT_KILLED_BY_SUFFOCATION,					// Agent dies if found inside a tile it can't enter; Queens take nutrient damage instead.
	AGENT_KILLED_BY_STARVATION,						// If the colony cannot pay its nutrients upkeep, an agent is chosen to starve and die.
	AGENT_KILLED_BY_PENALTY,						// If the Player DLL commits a flagrant foul (eFaultType), agents may be killed as penalty.

	// responses
	AGENT_ORDER_SUCCESS_HELD,						// ORDER_HOLD was successful
	AGENT_ORDER_SUCCESS_MOVED,						// ORDER_MOVE_XXX was successful
	AGENT_ORDER_SUCCESS_DUG,						// ORDER_DIG_XXX was successful (and a tile was destroyed by the agent)
	AGENT_ORDER_SUCCESS_PICKUP,						// ORDER_PICK_UP_XXX was successful; the agent is now carrying the (food / tile) picked up
	AGENT_ORDER_SUCCESS_DROP,						// ORDER_DROP_CARRIED_OBJECT was successful; this may cause suffocations or Queen-damage
	AGENT_ORDER_SUCCESS_GAVE_BIRTH,					// ORDER_BIRTH_XXX was successful; the new agent reports in this list with AGENT_WAS_CREATED
	AGENT_ORDER_SUCCESS_SUICIDE,					// ORDER_SUICIDE was successful; the agent is dead and will no longer be referenced

	AGENT_ORDER_ERROR_BAD_ANT_ID,					// Order was submitted for an illegal AgentID (dead, not-yours, or invalid)
	AGENT_ORDER_ERROR_EXHAUSTED,					// Order failed; agent is still exhausted (from previous action) and cannot act yet
	AGENT_ORDER_ERROR_CANT_CARRY_FOOD,				// ORDER_PICK_UP_FOOD failed; this agent type cannot carry food (typically only Workers can)
	AGENT_ORDER_ERROR_CANT_CARRY_TILE,				// ORDER_PICK_UP_TILE failed; this agent type cannot carry tiles (typically only Workers can)
	AGENT_ORDER_ERROR_CANT_BIRTH,					// ORDER_BIRTH_XXX failed; this agent type cannot birth new agents (typically only Queens can)
	AGENT_ORDER_ERROR_CANT_DIG_INVALID_TILE,		// ORDER_DIG_XXX failed; this agent cannot dig the attempted tile type
	AGENT_ORDER_ERROR_CANT_DIG_WHILE_CARRYING,		// ORDER_DIG_XXX failed; agents must be empty-handed to dig (and was carrying something)
	AGENT_ORDER_ERROR_MOVE_BLOCKED_BY_TILE,			// ORDER_MOVE_XXX failed; agent movement was blocked by tile type not passable to this agent type
	AGENT_ORDER_ERROR_MOVE_BLOCKED_BY_QUEEN,		// ORDER_MOVE_XXX failed for a Queen; Queens may not co-occupy the same space (adjudicated simultaneously)
	AGENT_ORDER_ERROR_OUT_OF_BOUNDS,				// ORDER_MOVE_XXX failed; agents may not move to coordinates outside the arena
	AGENT_ORDER_ERROR_NO_FOOD_PRESENT,				// ORDER_PICK_UP_FOOD failed; agent tried to pick up food, but none is here (picked up by someone else?)
	AGENT_ORDER_ERROR_ALREADY_CARRYING_FOOD,		// ORDER_PICK_UP_FOOD failed; an agent can carry at most one food/tile at a time
	AGENT_ORDER_ERROR_NOT_CARRYING,					// ORDER_DROP_CARRIED_OBJECT failed; but be carrying something in order to drop it
	AGENT_ORDER_ERROR_INSUFFICIENT_FOOD,			// ORDER_BIRTH_XXX failed; insufficient colony nutrients to pay the birthing cost
	AGENT_ORDER_ERROR_MAXIMUM_POPULATION_REACHED,	// ORDER_BIRTH_XXX failed; maximum colony population reached (use ORDER_SUICIDE to reduce)

	NUM_AGENT_STATUSES
};

//-----------------------------------------------------------------------------------------------
enum eAgentState : unsigned char 
{
	STATE_NORMAL = 0,		// Agent is alive and empty-handed
	STATE_DEAD,				// Agent is dead
	STATE_HOLDING_FOOD,		// Agent is alive and carrying food
	STATE_HOLDING_DIRT,		// Agent is alive and carrying a [Dirt] tile

	NUM_AGENT_STATES
};


//-----------------------------------------------------------------------------------------------
enum eTileType : unsigned char 
{
	TILE_TYPE_AIR,				// Open space, traversable by most/all agent types

	TILE_TYPE_DIRT,				// Semi-solid; may be dug, traversed, and/or carried by certain ant types
	TILE_TYPE_STONE,			// Impassable and indestructible

	TILE_TYPE_WATER,			// Lethal; turns into a corpse bridge when ant walks onto it, killing the ant
	TILE_TYPE_CORPSE_BRIDGE,	// Traversable space (like AIR), but can be dug to revert it back to water

	NUM_TILE_TYPES,
	TILE_TYPE_UNSEEN = 0xff		// Not currently visible to any ant in the colony, e.g. obscured by fog of war
};

// predeclared types;
struct DebugInterface;

//------------------------------------------------------------------------------------------------
// Structures
//-----------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------
struct Color8 // Similar to an Rgba8, but simpler.  Used for debug drawing, team colors, etc.
{
	Color8()
		: r(0)
		, g(0)
		, b(0)
		, a(255) 
	{}

	Color8( unsigned char redByte, 
		unsigned char greenByte, 
		unsigned char blueByte, 
		unsigned char alphaByte = 255 )
		: r(redByte)
		, g(greenByte)
		, b(blueByte)
		, a(alphaByte)
	{}

	unsigned char r, g, b, a; 
};


//-----------------------------------------------------------------------------------------------
struct VertexPC // Used for debug drawing.
{
	float x, y; 
	Color8 rgba; 
};


//-----------------------------------------------------------------------------------------------
struct AgentTypeInfo // Information about a given agent type (e.g. AGENT_TYPE_WORKER)
{
	const char*	name;			// Proper-case name for this type, e.g. "Worker" or "Queen"
	int		costToBirth;		// Colony pays this many nutrients for a queen to birth one of these
	int		exhaustAfterBirth;	// Queen who birthed an agent of this type gains +exhaustion
	int		upkeepPerTurn;		// Colony pays this many nutrients per turn per one of these alive
	int		visibilityRange;	// Sees tiles and agents up to this far away (taxicab distance)
	int		combatStrength;		// Survives a duel if enemy agent has a lower combatStrength
	int		combatPriority;		// Types with highest priority fight first
	int		sacrificePriority;	// Types with highest priority are killed first when a sacrifice is required (water bridge or starvation)
	bool	canCarryFood;		// Can pick up & drop food / porous tiles (ORDER_PICK_UP_, ORDER_DROP_)
	bool	canCarryTiles;		// Can pick up & drop food / porous tiles (ORDER_PICK_UP_, ORDER_DROP_)
	bool	canBirth;			// Can birth other ants (ORDER_BIRTH_)

	int		moveExhaustPenalties[NUM_TILE_TYPES]; // exhaustion gained after moving into each tile type
	int		digExhaustPenalties[NUM_TILE_TYPES];  // exhaustion gained after remote-digging each tile type
};


//-----------------------------------------------------------------------------------------------
struct MatchInfo // Information about the match about to be played
{
	int numPlayers;		// Number of players (with unique PlayerIDs) in this match
	int numTeams;		// <numPlayers> in brawl, 2 for 5v5, 1 for co-op survival
	
	short mapWidth;		// Width & height of [always square] map; tileX,tileY < mapWidth
	
	bool fogOfWar;				// If false, all tiles & agents are always visible
	bool teamSharedVision;		// If true, teammates share combined visibility
	bool teamSharedResources;	// If true, teammates share a single combined "nutrients" score

	int nutrientsEarnedPerFoodEatenByQueen;	// When a queen moves onto food, or food drops on her
	int nutrientLossPerAttackerStrength;	// Colony loses nutrients whenever queen is attacked
	int nutrientLossForQueenSuffocation;	// When queen is suffocated by a dropped tile - how many nutrients in damage she takes
	int numTurnsBeforeSuddenDeath;			// No new food appears after this turn, and upkeep increases continuously thereafter
	int suddenDeathTurnsPerUpkeepIncrease;	// Total upkeep increases by +1 every N turns after Sudden Death
	int colonyMaxPopulation;				// Cannot birth new agents if population is at max (can suicide to make room if need be)
	int startingNutrients;					// Each colony starts with this many nutrients

	int foodCarryExhaustPenalty;	// Exhaustion gained after each move while carrying food
	int tileCarryExhaustPenalty;	// Exhaustion gained after each move while carrying a tile

	int combatStrengthQueenAuraBonus;		// Agents may gain +combatStrength near a friendly queen
	int combatStrengthQueenAuraDistance;	// Number of tiles away (taxicab) a queen's combat bonus extends outward

	AgentTypeInfo agentTypeInfos[NUM_AGENT_TYPES];	// Stats and capabilities for each agent type
};


//------------------------------------------------------------------------------------------------
struct PlayerInfo // Server-assigned information about your Player instance in this match context
{
	PlayerID		playerID;	// 100+
	TeamID			teamID;		// 200+
	unsigned char	teamSize;	// 1 in free-for-all, 5 in 5v5, <numPlayers> in co-op survival
	Color8			color;		// Use this in your debug drawing, etc.
};


//-----------------------------------------------------------------------------------------------
struct AgentReport // Structure given for each of your agents (and/or each of your orders just previously issued)
{
	AgentID				agentID;		// This agent's unique ID #

	short				tileX;			// Current tile coordinates in map; (0,0) is bottom-left
	short				tileY;
	short				exhaustion;		// Number of turns inactive; non-HOLD orders fail if > 0

	short				receivedCombatDamage;		// Amount of damage received this turn 
	short				receivedSuffocationDamage;	// Suffocation damage received this turn (1 if you suffocated)

	eAgentType			type;			// Type of agent (permanent/unique per agent),	e.g. AGENT_TYPE_WORKER
	eAgentState			state;			// Special status of agent,						e.g. STATE_HOLDING_FOOD
	eAgentOrderResult	result;			// Result of agent's previously issued order,	e.g. AGENT_ORDER_ERROR_MOVE_BLOCKED_BY_TILE
}; 

// -----------------------------------------------------------------------------------------------
struct ObservedAgent // An agent (not one of yours) you were able to directly observe this turn
{
	AgentID		agentID;			// Another Player's agent's unique ID #
	PlayerID	playerID;			// Player who owns this agent
	TeamID		teamID;				// Team the agent's Player is on

	short		tileX;				// Agent was just observed at these tile coordinates; (0,0) is bottom-left
	short		tileY;
	
	short		receivedCombatDamage;		// 0 or 1 in most cases, or nutrient damage received by queen
	short		receivedSuffocationDamage;	// 0 or 1 in most cases, or nutrient damage received by queen

	eAgentType	type;				// Observed agent's type										e.g. AGENT_TYPE_WORKER
	eAgentState	state;				// Special status of agent (carrying something, etc.)			e.g. STATE_HOLDING_FOOD
	eOrderCode	lastObservedAction;	// What this agent just did / was trying to do (last orders)	e.g. ORDER_MOVE_EAST
}; 


//------------------------------------------------------------------------------------------------
// An order you issue for one of your agents.
// Issuing more than one order for a given agent in the same turn is an illegal FAULT.
struct AgentOrder
{
	AgentID agentID;
	eOrderCode order; 
};

//------------------------------------------------------------------------------------------------
// Fill this out when requested by TurnOrderRequest() to tell the server what actions each of
//	your agents is taking this turn.
//
// Orders [0] through [numberOfOrders-1] should each be a valid order for a valid, unique AgentID
//	that you own and is currently alive.  Agents not receiving orders will assume ORDER_HOLD.
struct PlayerTurnOrders
{
	AgentOrder orders[MAX_ORDERS_PER_PLAYER];
	int numberOfOrders; 
}; 


//------------------------------------------------------------------------------------------------
// Information about the server and match provided when PreGameStartup() is called by the server.
typedef void (*EventFunc)( const char* line );
typedef void (*RegisterEventFunc)( const char* eventName, EventFunc func );
struct StartupInfo
{
	MatchInfo	matchInfo;			// Info about the match itself (numPlayers, mapWidth, etc.)
	PlayerInfo	yourPlayerInfo;		// Info about your Player instance in this match context

	int expectedThreadCount;		// How many calls to PlayerThreadEntry() you will get (minimum of 1, typically 2+)
	double	maxTurnSeconds;			// You must always return from ReceiveTurnState/TurnOrderRequest within this period
	int freeFaultCount;				// Illegal faults allowed before Player is ejected (default 0)
	int nutrientPenaltyPerFault;	// Nutrients lost per illegal fault committed (default infinity)
	int agentsKilledPerFault;		// Agents killed as punishment per illegal fault (default infinity)

	DebugInterface* debugInterface; // Debug draw/log function interface for your Player to use

	RegisterEventFunc RegisterEvent; // You may call this during PreGameStartup to register events & console commands

	// #ToDo: provide a pre-allocated fixed memory pool that AI can (must?) use for "heap" allocations
	// void*	memoryPool;
	// size_t	memoryPoolSizeInBytes; 
};


//-----------------------------------------------------------------------------------------------
// Passed to you by the server whenever it calls ReceiveTurnState() to give you a report each turn.
struct ArenaTurnStateForPlayer
{
	// Information at the start of this turn; your next PlayerTurnOrders will be for this turn number
	int turnNumber;
	int currentNutrients;

	// fault reporting to clients
	int numFaults; 
	int nutrientsLostDueToFault;
	int nutrientsLostDueToQueenDamage;
	int nutrientsLostDueToQueenSuffocation;

	// Status reports for each of your living agents (and/or for each order you submitted)
	AgentReport agentReports[MAX_REPORTS_PER_PLAYER];
	int numReports;

	// List of all agents (besides yours) within your current visibility
	ObservedAgent observedAgents[MAX_AGENTS_TOTAL];
	int numObservedAgents; 

	// Copy of the entire map; tile type is TILE_TYPE_UNSEEN for tiles not currently visible.
	//	Unseen tiles always report "false" for food.
	// Maps are always square (width x width), with (0,0) in bottom-left; +X is east, +Y is north.
	// Map tiles are indexed x-minor y-major from bottom-left (left-to-right, then up).
	//	For example, observedTiles[11] on a 8x8 map would be tile coordinates (3,1) - 3 right, 1 up.
	eTileType observedTiles[MAX_ARENA_TILES]; // Only the first (width*width) entries are used
	bool tilesThatHaveFood[MAX_ARENA_TILES]; // Indexed identically to observerdTiles[]
}; 

// -----------------------------------------------------------------------------------------------
struct MatchResults
{
	// #ToDo: Do we want to inform the Player DLLs of victory/loss?
};


//------------------------------------------------------------------------------------------------
// Debug Render Interface
//
// Call debugInterface->LogText() to printf colored text to the dev console and log file
// Call debugInterface->QueueXXX() functions to request asynchronous debug draws on the server;
// Call debugInterface->FlushXXX() when finished; server will thereafter draw all queued items,
//	and continue to do so until your next call to FlushXXX(), which clears the previous items.
// !!! Items Queued for drawing will not appear until a Flush has been called afterwards. !!!
//
// Debug drawing is done in world space; each Tile is 1x1, and (x,y) is the center of tile X,Y.
// For example, tile 7,3 extends from mins(6.5,2.5) to maxs(7.5,3.5), with center at (7.0,3.0).
// All drawing is clipped to arena space, i.e. mins(-.5,-.5) to maxs(mapWidth-.5,mapWidth-.5).
//-----------------------------------------------------------------------------------------------
typedef void (*RequestPauseFunc)(); 
typedef void (*LogTextFunc)( char const* format, ... );
typedef void (*SetMoodTextFunc)( char const* format, ... );
typedef void (*DrawVertexArrayFunc)( int count, const VertexPC* vertices );
typedef void (*DrawWorldTextFunc)( 
	float posX, float posY,
	float anchorU, float anchorV, 
	float height, // 1.0 would be text whose characters are one tile high
	Color8 color, 
	char const* format, ... ); 
typedef void (*FlushQueuedDrawsFunc)(); 
typedef bool (*IsDebuggingFunc)();
typedef bool (*IsKeyDownFunc)( char const* keyShortName );
typedef void (*GetMouseWorldPosFunc)( float& out_mouseWorldX, float& out_mouseWorldY );
//------------------------------------------------------------------------------------------------
struct DebugInterface
{
	RequestPauseFunc		RequestPause;			// Pause the simulation (can be ignored by game)
	LogTextFunc				LogText;				// Print to dev console (and possibly log file)
	SetMoodTextFunc			SetMoodText;			// Call anytime to change "mood" text on player panel

	DrawWorldTextFunc		QueueDrawWorldText;		// Draw (aligned) overlay text in world space
	DrawVertexArrayFunc		QueueDrawVertexArray;	// Draw untextured geometry in world space
	FlushQueuedDrawsFunc	FlushQueuedDraws;		// Call after queuing to commit and show draws

	// These functions were added as of interface version 12 (previous clients may see a smaller struct, missing these)
	IsDebuggingFunc			IsDebugging;			// If false, calls to debug functions may be ignored by the Arena
	IsKeyDownFunc			IsKeyDown;				// True if the named key is held down; e.g. "ESC", "LMB", "W", "Left", "Space"
	GetMouseWorldPosFunc	GetMouseWorldPos;		// Reports the mouse position in world space
};


//-----------------------------------------------------------------------------------------------
// DLL-EXE Interface
//
// All of these functions are exported by the DLL so that the server can find them using
//	LoadLibrary() and GetProcAddress() and call them.
//
// All functions should return immediately, except PlayerThreadEntry() which is called from
//	within a private thread created by the server solely for this DLL to use to do asynchronous
//	work.  PlayerThreadEntry() should loop infinitely, doing general AI work for your ant colony,
//	until the server calls PostGameShutdown(), after which you should exit your loop and finally
//	return from PlayerThreadEntry().  Note that PlayerThreadEntry() may (likely) be called 2+
//	times, once for/in each thread created for this DLL player by the server.
//-----------------------------------------------------------------------------------------------
#if !defined(ARENA_SERVER)
	// Functions exported by the DLL for the server (.EXE) to call
	extern "C"
	{
		// Pre-game player information collection (server asks DLLs about themselves)
		DLL int GiveCommonInterfaceVersion();	// DLL should return COMMON_INTERFACE_VERSION_NUMBER
		DLL const char* GivePlayerName();		// DLL should return the name of the AI (can be whatever)
		DLL const char* GiveAuthorName();		// DLL should return the actual human author's name

		// Game start & end notifications from the server
		DLL void PreGameStartup( const StartupInfo& info );			// Server provides DLL with player/match info
		DLL void PostGameShutdown( const MatchResults& results );	// Match has ended (or player is being ejected); DLL should exit all loops/threads

		// This is the server calling YOUR DLL's "main" entry function; this was called in its own private
		//	thread that was created just for this player to keep and use for general AI work.
		// If multiple threads were created for this player, this entry function will be called once for (from within) each
		//	of those threads.
		DLL void PlayerThreadEntry( int yourThreadIdx );

		// Functions called on DLL by server once per Turn, to receive new orders from the DLL and to send update reports from server.
		DLL void ReceiveTurnState( const ArenaTurnStateForPlayer& state );					// Server tells you what you see.
		DLL bool TurnOrderRequest( int turnNumber, PlayerTurnOrders* out_ordersToFill );	// You tell server what you do.  You can return false if you have no orders ready, and the server MAY ask you a second time after asking everyone else.
	}
#else
	// Function pointer types, used by server (only) to type-cast found exported DLL function pointers acquired via GetProcAddress()
	typedef int (*GiveCommandInterfaceVersionFunc)();
	typedef const char* (*GivePlayerNameFunc)();
	typedef const char* (*GiveAuthorNameFunc)();
	typedef void (*PreGameStartupFunc)( const StartupInfo& info );
	typedef void (*PostGameShutdownFunc)( const MatchResults& results );
	typedef void (*PlayerThreadEntryFunc)( int yourThreadIdx );
	typedef void (*ReceiveTurnStateFunc)( const ArenaTurnStateForPlayer& state );
	typedef bool (*TurnOrderRequestFunc)( int turnNumber, PlayerTurnOrders* ordersToFill ); 
#endif
