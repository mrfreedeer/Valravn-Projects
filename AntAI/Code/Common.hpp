#pragma once
#include "ArenaPlayerInterface.hpp"
#include <atomic>
#include <thread>
#include <mutex>
#include <vector>
#include <set>
#include "ThreadSafeStructures.hpp"
#include "Engine/Math/IntVec2.hpp"


class Colony;

struct DebugInterface;

extern Colony* g_colony;
extern DebugInterface* g_debug;
extern std::atomic<bool>	g_isExiting;		
extern std::atomic<int>	g_threadSafe_threadCount;