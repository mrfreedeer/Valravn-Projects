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
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/DevConsole.hpp"

#include <math.h>

extern Renderer* g_theRenderer;
extern InputSystem* g_theInput;
extern AudioSystem* g_theAudio;
extern DevConsole* g_theConsole;
extern RandomNumberGenerator rng;

void DebugDrawRing(Vec2 const& ringCenter, float radius, float thickness, Rgba8 const& color);
void DebugDrawLine(Vec2 const& lineStart, Vec2 const& lineEnd, float thickness, Rgba8 const& color);
void DrawCircle(Vec2 const& circleCenter, float radius, Rgba8 const& color);
void DrawCircleShieldWithHitEffect(Vec2 const& circleCenter, float radius, Rgba8 const& color, Vec2 const& hitPosition, float elapsedEffectTime);

constexpr int NUM_STARTING_ASTEROIDS = 6;
constexpr int MAX_ASTEROIDS = 50;
constexpr int MAX_BULLETS = 100;

constexpr int MAX_ENEMIES = 100;

constexpr float UI_SIZE_X = 1600.0f;
constexpr float UI_CENTER_X = UI_SIZE_X * 0.5f;
constexpr float UI_SIZE_Y = 800.0f;
constexpr float UI_CENTER_Y = UI_SIZE_Y * 0.5f;
constexpr float UI_DRAWING_SPACE_BUFFER = 30.0f;
constexpr float UI_CLOCK_CENTER_X = 150.0f;
constexpr float UI_CLOCK_CENTER_Y = UI_SIZE_Y - UI_DRAWING_SPACE_BUFFER *0.8f;
constexpr float UI_CLOCK_RADIUS = UI_DRAWING_SPACE_BUFFER * 0.5f;

constexpr float WORLD_SIZE_X = 200.f;
constexpr float WORLD_SIZE_Y = 100.f;
constexpr float WORLD_CENTER_X = WORLD_SIZE_X / 2.f;
constexpr float WORLD_CENTER_Y = WORLD_SIZE_Y / 2.f;

constexpr float ASTEROID_SPEED = 10.f;
constexpr float ASTEROID_PHYSICS_RADIUS = 1.6f;
constexpr float ASTEROID_COSMETIC_RADIUS = 2.0f;

constexpr float BEETLE_SPEED = 12.0f;
constexpr float BEETLE_COSMETIC_RADIUS = 3.0f;
constexpr float BEETLE_PHYSICS_RADIUS = 2.6f;
constexpr int BEETLE_HEALTH = 3;

constexpr float WASP_ACCELERATION = 10.0f;
constexpr float WASP_COSMETIC_RADIUS = 3.1f;
constexpr float WASP_PHYSICS_RADIUS = 2.8f;
constexpr float MAX_WASP_SPEED = 35.0f;
constexpr int WASP_HEALTH = 3;

constexpr float TWIN_SPEED = 10.0f;
constexpr float TWIN_COSMETIC_RADIUS = 3.0f;
constexpr float TWIN_PHYSICS_RADIUS = 2.6f;
constexpr int TWIN_HEALTH = 3;
constexpr float TWIN_DISTANCE_TO_OTHER_TWIN = 20.0f;

constexpr float BULLET_LIFETIME_SECONDS = 2.0f;
constexpr float BULLET_SPEED = 50.f;
constexpr float BULLET_PHYSICS_RADIUS = 0.5f;
constexpr float BULLET_COSMETIC_RADIUS = 2.0f;

constexpr float PLAYER_SHIP_ACCELERATION = 30.f;
constexpr float PLAYER_SHIP_TURN_SPEED = 300.f;
constexpr float PLAYER_SHIP_PHYSICS_RADIUS = 1.75F;
constexpr float PLAYER_SHIP_COSMETIC_RADIUS = 2.25f;
constexpr int PLAYER_DEBRIS_AMOUNT = 30;

constexpr float CLIENT_ASPECT = 2.0f; // We are requesting a 1:1 aspect (square) window area
constexpr float WRAP_AROUND_SPACE = 5.0f;

constexpr float DEBRIS_LIFE_TIME = 2.0f;
constexpr float DEBRIS_MIN_SCALE = 0.3f;
constexpr float DEBRIS_MAX_SCALE = 0.8f;
constexpr float DEBRIS_MIN_SCALE_DEATH = 1.0f;
constexpr float	DEBRIS_MAX_SCALE_DEATH = 2.0f;
constexpr int DEBRIS_MAX_AMOUNT = 200;
constexpr int DEBRIS_AMOUNT_COLLISION = 2;
constexpr int DEBRIS_AMOUNT_DEATH = 10;

constexpr int NUM_WAVES = 5;

constexpr float TIME_TO_RETURN_TO_ATTRACT_SCREEN = 4.0f;


constexpr float SCREENSHAKE_DURATION = 0.2f;
constexpr float SCREENSHAKE_DEATH_DURATION = 0.5f;
constexpr float SCREENSHAKE_PLAYER_DEATH_DURATION = 2.5f;

constexpr float MAX_SCREENSHAKE_TRANSLATION = 1.0f;
constexpr float MAX_SCREENSHAKE_TRANSLATION_DEATH = 5.0f;
constexpr float MAX_SCREENSHAKE_TRANSLATION_PLAYER_DEATH = 15.0f;

constexpr float PARALLAX_FACTOR = 0.2f;
constexpr int PARALLAX_STARS_AMOUNT = 150;
constexpr float PARALLAX_PLANET_CENTER_X = 10.0f;
constexpr float PARALLAX_PLANET_CENTER_Y = 100.0f;
constexpr float PARALLAX_PLANET_RADIUS = 50.0f;
constexpr float PARALLAX_PLANET_RING_RADIUS = 70.0f;
constexpr float PARALLAX_PLANET_RING_THICKNESS = 10.0f;

constexpr float TIMEWARPER_EFFECT_RADIUS = 25.0f;
constexpr float TIMEWARPER_SPEED = 10.0f;
constexpr float TIMEWARPER_COSMETIC_RADIUS = 3.5f;
constexpr float TIMEWARPER_PHYSICS_RADIUS = 2.5f;
constexpr float TIMEWARPER_TURNSPEED = 15.0f;
constexpr int TIMEWARPER_HEALTH = 2;
constexpr float TWIN_LASER_SOUND_RADIUS = 50.0f;

constexpr int SHIELD_DEFAULT_HEALTH = 3;
constexpr float SHIELD_PHYSICS_RADIUS_PERCENTAGE = 1.5f;
constexpr float SHIELD_TIME_HIT_EFFECT = 0.2f;
constexpr float SHIELD_EFFECT_ANGLE = 30.0f;
constexpr float SHIELD_PICKUP_RADIUS = 2.0f;
constexpr float PICKUP_TIME_TO_SPAWN = 15.0f;
constexpr int MAX_PICKUPS = 5;

constexpr float TEXT_ANIMATION_TIME = 4.0f;
constexpr float TEXT_ANIMATION_POS_PERCENTAGE_TOP = 0.8f;
constexpr float TEXT_CELL_HEIGHT = 50.0f;
constexpr float TEXT_MOVEMENT_PHASE_TIME_PERCENTAGE = 0.25f;
constexpr float TEXT_STILL_PHASE_TIME_PERCENTAGE = 1 - 0.25f;