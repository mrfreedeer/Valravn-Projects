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
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Engine/Core/Clock.hpp"

#include <math.h>

class Game;
class SpriteSheet;

enum GAME_SOUND {
	UNDEFINED_SOUND = -1,
	CLAIRE_DE_LUNE,
	LIBRA_THEME,
	VICTORY,
	GAME_OVER,
	PLAYER_SHOOTING,
	PLAYER_HIT,
	ENEMY_SHOOTING,
	ENEMY_HIT,
	ENEMY_DIED,
	BULLET_BOUNCE,
	BULLET_RICOCHET,
	EXIT_MAP,
	PAUSE,
	UNPAUSE, 
	CLICK,
	DISCOVERED,
	NUM_SOUNDS
};

enum GAME_TEXTURE
{
	UNDEFINED_TEXTURE = -1,
	ATTRACT_SCREEN,
	VICTORY_SCREEN,
	GAME_OVER_SCREEN,
	ARIES,
	ENEMY_BOLT,
	ENEMY_BULLET,
	ENEMY_CANNON,
	ENEMY_GATLING,
	ENEMY_SHELL,
	ENEMY_TANK_0,
	ENEMY_TANK_1,
	ENEMY_TANK_2,
	ENEMY_TANK_3,
	ENEMY_TANK_4,
	ENEMY_TURRET_BASE,
	FRIENDLY_BOLT,
	FRIENDLY_BULLET,
	FRIENDLY_CANNON,
	FRIENDLY_GATLING,
	FRIENDLY_SHELL,
	FRIENDLY_TANK_0,
	FRIENDLY_TANK_1,
	FRIENDLY_TANK_2,
	FRIENDLY_TANK_3,
	FRIENDLY_TANK_4,
	FRIENDLY_TURRET_BASE,
	PLAYER_TANK_BASE,
	PLAYER_TANK_TOP,
	TERRAIN,
	EXPLOSION,
	WALLRUBBLE,
	ENEMYRUBBLE,
	NUM_TEXTURES
};

enum class ExplosionScale {
	SMALL,
	MEDIUM,
	LARGE,
	NUM_EXPLOSIONS_DEFS
};

extern Renderer* g_theRenderer;
extern InputSystem* g_theInput;
extern AudioSystem* g_theAudio;
extern RandomNumberGenerator rng;
extern Game* g_theGame;

extern bool g_drawDebug;
extern bool g_drawDebugDistanceField;
extern bool g_drawDebugHeatMapEntity;
extern bool g_ignorePhysics;
extern bool g_ignoreDamage;
extern bool g_debugCamera;

extern float g_masterVolume;

extern SpriteSheet* g_tileSpriteSheet;
extern SpriteSheet* g_explosionSpriteSheet;
extern SpriteAnimDefinition* g_explosionAnimDefinitions[(int)ExplosionScale::NUM_EXPLOSIONS_DEFS];

extern SoundID g_sounds[GAME_SOUND::NUM_SOUNDS];
extern SoundPlaybackID g_soundPlaybackIDs[GAME_SOUND::NUM_SOUNDS];
extern Texture* g_textures[GAME_TEXTURE::NUM_TEXTURES];
extern BitmapFont* g_squirrelFont;

void DebugDrawRing(Vec2 const& ringCenter, float radius, float thickness, Rgba8 const& color);
void DebugDrawLine(Vec2 const& lineStart, Vec2 const& lineEnd, float thickness, Rgba8 const& color);
void DrawCircle(Vec2 const& circleCenter, float radius, Rgba8 const& color);
void PlaySound(GAME_SOUND gameSound, float volume = 1.0f, bool looped = false, float soundBalance = 0.0f);
