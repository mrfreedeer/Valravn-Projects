#pragma once
#include "Game/Gameplay/Entity.hpp"

struct Vec2;
typedef size_t SoundPlaybackID;

constexpr int PLAYER_SHIP_VERTEX_AMOUNT = 18;

class PlayerShip : public Entity {
public:
	PlayerShip(Game* gamePointer, const Vec2& startingPosition);
	~PlayerShip();
	
	float m_playerDeathTime = 0.0f;
	bool m_isSlowedDown = false; 
	bool m_isSpedUp = false;

	void Update(float deltaSeconds) override;
	void UpdateFromKeyboard(float deltaSeconds);
	void UpdateFromController(float deltaSeconds);
	void HandleSound();
	void Render() const;
	void RenderLives() const;
	void Die();
	void TakeAHit(Vec2 const& hitPosition);
	void Respawn();
	
	void StopAllSounds();

	static void GetLocalVerts(Vertex_PCU* storeArray, Rgba8& color, float verticalOffset = 0.0f);
	static void DrawPlayerShip(const Vec2& position, float orientationDegrees, float scale, float verticalOffset = 0.0f);
	

private:
	Vertex_PCU m_verts[PLAYER_SHIP_VERTEX_AMOUNT] = {};
	void InitializeLocalVerts();
	void BounceOffWalls();
	void SpawnDeathDebrisCluster() const;
	void Shoot();
	SoundPlaybackID m_deathSound;
	SoundPlaybackID m_currentShootingSound;
	SoundPlaybackID m_flameSound;
	SoundPlaybackID m_clockTickingSound;
	SoundPlaybackID m_respawnSound;
	float m_clockSoundSpeed = 1.0f;

	float m_thrustForce = 0.0f;
	bool m_playingFlameSound = false;
	bool m_playingClockSound = false;
};