#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
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
#include "Engine/Core/XmlUtils.hpp"

#include <math.h>

struct Vec3;

enum class GAME_TEXTURE {
	TestUV,
	CompanionCube,
	Terrain,
	Infected,
	Victory,
	Died,
	Doom_Face,
	NUM_TEXTURES
};

enum GAME_SOUND {
	UNDEFINED = -1,
	CLAIRE_DE_LUNE,
	AT_DOOMS_GATE,
	MAIN_MENU_IN_THE_DARK,
	CLICK,
	DEMON_ACTIVE,
	DEMON_ATTACK,
	DEMON_DEATH,
	DEMON_HURT,
	DEMON_SIGHT,
	DISCOVERED,
	PISTOL_FIRE,
	PLASMA_FIRE,
	PLASMA_HIT,
	PLAYER_DEATH_1,
	PLAYER_DEATH_2,
	PLAYER_GIBBED,
	PLAYER_HIT,
	PLAYER_HURT,
	TELEPORTER,
	DOOR_KNOCKING_SOUND,
	SPIDER_HURT,
	SPIDER_DEATH,
	SPIDER_WALK,
	SPIDER_ATTACK,
	GRAVITY_FIRE,
	GRAVITY_PULSE,
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
extern float g_masterVolume;


void DebugDrawRing(Vec2 const& ringCenter, float radius, float thickness, Rgba8 const& color);
void DebugDrawLine(Vec2 const& lineStart, Vec2 const& lineEnd, float thickness, Rgba8 const& color);
void DrawCircle(Vec2 const& circleCenter, float radius, Rgba8 const& color);
 
void PlaySound(GAME_SOUND gameSound, float volume = 1.0f, bool looped = false, float soundBalance = 0.0f);
void PlaySoundAt(GAME_SOUND gameSound, Vec3 const& soundPosition, Vec3 const& soundVelocity = Vec3::ZERO, float volume = 1.0f, bool looped = false, float soundBalance = 0.0f);
GAME_SOUND GetSoundIDForSource(char const* source);
GAME_SOUND GetSoundIDForSource(std::string source);