#pragma once
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
//
#include <math.h>

struct Vec2;
struct Rgba8;

enum class GAME_TEXTURE {
	TestUV,
	TextureTest,
	CompanionCube,
	NUM_TEXTURES
};

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
extern std::string g_soundSources[(int)GAME_SOUND::NUM_SOUNDS];
extern SoundID g_sounds[(int)GAME_SOUND::NUM_SOUNDS];
extern SoundPlaybackID g_soundPlaybackIDs[(int)GAME_SOUND::NUM_SOUNDS];
extern std::map<std::string, GAME_SOUND> g_soundEnumIDBySource;
extern BitmapFont* g_squirrelFont;
extern Texture* g_textures[(int)GAME_TEXTURE::NUM_TEXTURES];

void DebugDrawRing(Vec2 const& ringCenter, float radius, float thickness, Rgba8 const& color);
void DebugDrawLine(Vec2 const& lineStart, Vec2 const& lineEnd, float thickness, Rgba8 const& color);
void DrawCircle(Vec2 const& circleCenter, float radius, Rgba8 const& color);
 
void PlaySound(GAME_SOUND gameSound, float volume = 1.0f, bool looped = false, float soundBalance = 0.0f);
void PlaySoundAt(GAME_SOUND gameSound, Vec3 const& soundPosition, Vec3 const& soundVelocity = Vec3::ZERO, float volume = 1.0f, bool looped = false, float soundBalance = 0.0f);
GAME_SOUND GetSoundIDForSource(char const* source);
GAME_SOUND GetSoundIDForSource(std::string source);