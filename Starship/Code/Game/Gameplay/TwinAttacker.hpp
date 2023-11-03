#pragma once
#include "Game/Gameplay/Entity.hpp"

typedef size_t SoundPlaybackID;
constexpr int TWIN_ATTACKER_AMOUNT_VERTS = 18;

class TwinAttacker : public Entity {

public:
	TwinAttacker(Game* gamePointer, Vec2 const& startingPosition);
	~TwinAttacker();
	void SetTwin(TwinAttacker* pointerToTwin) { m_twin = pointerToTwin; }
	void Update(float deltaSeconds);
	void UpdateLaserBeamSound();
	void Render() const;

	TwinAttacker* m_twin = nullptr;
	bool m_playingLaserSound;
	SoundPlaybackID m_laserSoundPlaybackID;

private:
	void InitializeLocalVerts();
	Vertex_PCU m_verts[TWIN_ATTACKER_AMOUNT_VERTS];
};