#include "Game/Gameplay/TwinAttacker.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/PlayerShip.hpp"
#include "Game/Gameplay/Game.hpp"

TwinAttacker::TwinAttacker(Game* gamePointer, Vec2 const& startingPosition) :
	Entity(gamePointer, startingPosition)
{
	m_physicsRadius = TWIN_PHYSICS_RADIUS;
	m_cosmeticRadius = TWIN_COSMETIC_RADIUS;
	m_health = TWIN_HEALTH;

	m_shieldRadius = m_physicsRadius * SHIELD_PHYSICS_RADIUS_PERCENTAGE;


	InitializeLocalVerts();
}

TwinAttacker::~TwinAttacker()
{
	if (m_twin) {
		m_twin->m_twin = nullptr;
	}

	if (m_playingLaserSound) {
		g_theAudio->StopSound(m_laserSoundPlaybackID);
	}
}

void TwinAttacker::Update(float deltaSeconds)
{
	PlayerShip* nearestPlayer = m_game->GetNearestPlayer();

	if (nearestPlayer->IsAlive()) {

		Vec2 direction;

		

		if (m_twin) {
			
			UpdateLaserBeamSound();

			Vec2 displacementTwinPlayer = nearestPlayer->m_position - m_twin->m_position;
			Vec2 idealDisplacement = nearestPlayer->m_position + displacementTwinPlayer;

			direction = idealDisplacement - m_position;


			Vec2 dispFromTwin = m_twin->m_position - m_position;
			direction.Normalize();
			if (dispFromTwin.GetLength() < TWIN_DISTANCE_TO_OTHER_TWIN) {
				direction -= dispFromTwin.GetNormalized();
			}

			m_velocity = direction * TWIN_SPEED;

		}
		else {
			// Twin is dead, charge to the player
			direction = nearestPlayer->m_position - m_position;
			direction.Normalize();

			m_velocity = direction * 2.5 * TWIN_SPEED;
		}

		m_orientationDegrees = direction.GetOrientationDegrees();

	}

	Entity::Update(deltaSeconds);

}

void TwinAttacker::UpdateLaserBeamSound()
{
	PlayerShip* nearestPlayer = m_game->GetNearestPlayer();

	Vec2 middlePos = m_position + (m_twin->m_position - m_position) * 0.5f;

	float middleDistanceToPlayerSqr = GetDistanceSquared2D(nearestPlayer->m_position, middlePos);
	float sqrSoundRadius = TWIN_LASER_SOUND_RADIUS * TWIN_LASER_SOUND_RADIUS;

	if (!m_playingLaserSound && !m_twin->m_playingLaserSound) {
		if (middleDistanceToPlayerSqr < sqrSoundRadius) {
			m_playingLaserSound = true;
			m_twin->m_playingLaserSound = true;
			m_laserSoundPlaybackID = g_theAudio->StartSound(laserBeamSound, true, 0.0f);
			m_twin->m_laserSoundPlaybackID = m_laserSoundPlaybackID;
		}
	}

	if (m_playingLaserSound) {
		float soundVolume = RangeMapClamped(middleDistanceToPlayerSqr, sqrSoundRadius, 0.0f, 0.0f, 2.0f);
		g_theAudio->SetSoundPlaybackVolume(m_laserSoundPlaybackID, soundVolume);
	}
}


void TwinAttacker::Render() const
{
	Vertex_PCU worldVerts[TWIN_ATTACKER_AMOUNT_VERTS] = {};
	for (int vertIndex = 0; vertIndex < TWIN_ATTACKER_AMOUNT_VERTS; vertIndex++) {
		worldVerts[vertIndex] = m_verts[vertIndex];
	}

	TransformVertexArrayXY3D(TWIN_ATTACKER_AMOUNT_VERTS, worldVerts, 1.0f, m_orientationDegrees, m_position);
	g_theRenderer->DrawVertexArray(TWIN_ATTACKER_AMOUNT_VERTS, worldVerts);

	// Laser Beam
	if (m_twin) {
		DebugDrawLine(m_position, m_twin->m_position, 0.5f, Rgba8(0, 255, 255, 255));
	}
	RenderShield();
}

void TwinAttacker::InitializeLocalVerts()
{
	Vec2 uvTexCoords;

	Rgba8 red(255, 0, 0, 255);
	Rgba8 blue(0, 0, 255, 255);

	m_color = red;

	// Back Wings
	//Right
	m_verts[0] = Vertex_PCU(Vec3(-1.0f, 0.0f, 0.0f), m_color, uvTexCoords);
	m_verts[1] = Vertex_PCU(Vec3(-2.0f, -2.0f, 0.0f), m_color, uvTexCoords);
	m_verts[2] = Vertex_PCU(Vec3(2.0f, -1.0f, 0.0f), m_color, uvTexCoords);
	// Left
	m_verts[3] = Vertex_PCU(Vec3(-1.0f, 0.0f, 0.0f), m_color, uvTexCoords);
	m_verts[4] = Vertex_PCU(Vec3(-2.0f, 2.0f, 0.0f), m_color, uvTexCoords);
	m_verts[5] = Vertex_PCU(Vec3(2.0f, 1.0f, 0.0f), m_color, uvTexCoords);

	// Middle Section
	// Right
	m_verts[6] = Vertex_PCU(Vec3(-1.0f, 0.0f, 0.0f), blue, uvTexCoords);
	m_verts[7] = Vertex_PCU(Vec3(2.0f, -1.0f, 0.0f), blue, uvTexCoords);
	m_verts[8] = Vertex_PCU(Vec3(1.0f, 0.0f, 0.0f), blue, uvTexCoords);

	// Left
	m_verts[9] = Vertex_PCU(Vec3(-1.0f, 0.0f, 0.0f), blue, uvTexCoords);
	m_verts[10] = Vertex_PCU(Vec3(2.0f, 1.0f, 0.0f), blue, uvTexCoords);
	m_verts[11] = Vertex_PCU(Vec3(1.0f, 0.0f, 0.0f), blue, uvTexCoords);

	// Tip
	// Right
	m_verts[12] = Vertex_PCU(Vec3(2.0f, -1.0f, 0.0f), m_color, uvTexCoords);
	m_verts[13] = Vertex_PCU(Vec3(4.0f, 0.0f, 0.0f), m_color, uvTexCoords);
	m_verts[14] = Vertex_PCU(Vec3(3.0f, 0.0f, 0.0f), m_color, uvTexCoords);

	// Left
	m_verts[15] = Vertex_PCU(Vec3(2.0f, 1.0f, 0.0f), m_color, uvTexCoords);
	m_verts[16] = Vertex_PCU(Vec3(4.0f, 0.0f, 0.0f), m_color, uvTexCoords);
	m_verts[17] = Vertex_PCU(Vec3(3.0f, 0.0f, 0.0f), m_color, uvTexCoords);
}
