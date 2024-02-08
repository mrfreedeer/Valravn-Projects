#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Renderer/SimpleTriangleFont.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Window/Window.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Core/Clock.hpp"

#include <math.h>


enum GAME_SOUND {
	UNDEFINED = -1,
	CLAIRE_DE_LUNE,
	WINNING_SOUND,
	LOSING_SOUND,
	DOOR_KNOCKING_SOUND,
	NUM_SOUNDS
};

extern Renderer* g_theRenderer;
extern InputSystem* g_theInput;
extern AudioSystem* g_theAudio;
extern Window* g_theWindow;
extern RandomNumberGenerator rng;
extern bool g_drawDebug;
extern SoundID g_sounds[(int)GAME_SOUND::NUM_SOUNDS];
extern SoundPlaybackID g_soundPlaybackIDs[(int)GAME_SOUND::NUM_SOUNDS];
extern BitmapFont* g_squirrelFont;

void DebugDrawRing(Vec2 const& ringCenter, float radius, float thickness, Rgba8 const& color);
void DebugDrawLine(Vec2 const& lineStart, Vec2 const& lineEnd, float thickness, Rgba8 const& color);
void DrawCircle(Vec2 const& circleCenter, float radius, Rgba8 const& color);
 
