#pragma once
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/Entity.hpp"

class PlayerShip;
class Asteroid;
class Bullet;
class Debris;
class PickUp;

extern SoundID playerShootingSound;
extern SoundID playerExplosionSound;
extern SoundID playerFlameSound;
extern SoundID clockTickingSound;
extern SoundID laserBeamSound;
extern SoundID pickUpSound;
extern SoundID respawnSound;
extern SoundID winGameSound;
extern SoundID loseGameSound;
extern SoundID entityDamageSound;
extern SoundID entityDeathSound;

enum GameState {
	AttractScreen,
	Play
};

class App;

class Game {

public:
	Game(App* appPointer);
	~Game();
	
	void Startup();
	void ShutDown();
	void LoadSoundFiles();
	void StartUpParallax();

	void Update(float deltaSeconds);
	void UpdateDeveloperCheatCodes(float deltaSeconds);

	void Render() const;

	void SpawnRandomAsteroid();
	void SpawnBullet(const Vec2& pos, float forwardDegrees);
	void SpawnDebrisCluster(int count, const Vec2& pos, Rgba8 color, const Vec2& baseVelocity,
		float minRndScale, float maxRndScale, float minRndSpeed, float maxRndSpeed);

	PlayerShip* GetNearestPlayer() { return m_playerShip; }
	void KillAllEnemies();


	void ShakeScreenCollision();
	void ShakeScreenDeath();
	void ShakeScreenPlayerDeath();
	void StopShakeScreen();

	bool m_useTextAnimation = true;
	Rgba8 m_textAnimationColor = Rgba8(255, 255, 255, 255);
	std::string m_currentText = "";
	bool m_returnToAttactMode = false;
	int m_pickupsInGame = 0;

	SoundPlaybackID m_winGameSoundID = {};
	SoundPlaybackID m_loseGameSoundID = {};

private:
	App* m_theApp = nullptr;

	void DeleteGarbageEntities();

	void UpdateGame(float deltaSeconds);

	void UpdateAttractScreen(float deltaSeconds);
	void UpdateInputAttractScreen(float deltaSeconds);

	void UpdateEntities(float deltaSeconds);
	void ManageWaves(float deltaTime);

	void UpdateCameras(float deltaSeconds);
	void UpdateParallaxCamera();
	void UpdateUICamera(float deltaSeconds);
	void UpdateWorldCamera(float deltaSeconds);

	void UpdateTextAnimation(float deltaTime, std::string text, Vec2 textLocation);
	void TransformText(Vec2 iBasis, Vec2 jBasis, Vec2 translation);
	float GetTextWidthForAnim(float fullTextWidth);
	Vec2 const GetIBasisForTextAnim();
	Vec2 const GetJBasisForTextAnim();
	
	void RenderGame() const;
	void RenderAttractScreen() const;

	void RenderDevConsole() const;
	void RenderEntities() const;
	void RenderDegubEntities() const;
	void RenderUI() const;
	void RenderParallax() const;
	void RenderSlowMoClock() const;
	void RenderSpedUpClock() const;
	void RenderClock() const;
	void RenderTextAnimation() const;

	void CheckCollisions();
	void CheckCollisionEntities();
	void CheckCollisionAsteroids();
	void CheckCollisionLaserBeams();
	void CheckCollisionPickUps();

	void SpawnRandomDebris(const Vec2& pos, Rgba8 color, const Vec2& velocity, float scale, float speed);
	void SpawnEnemies();
	void SpawnRandomWasp();
	void SpawnRandomBeetle();
	void SpawnRandomTimeWarper();
	void SpawnRandomTwins();
	void SpawnPickUp();

	Vec2 const GetRandomSpawnPosition();

	Asteroid* m_asteroids[MAX_ASTEROIDS] = {};
	Bullet* m_bullets[MAX_BULLETS] = {};
	PlayerShip* m_playerShip = nullptr;
	Entity* m_enemies[MAX_ENEMIES] = {};
	Debris* m_debris[DEBRIS_MAX_AMOUNT] = {};
	PickUp* m_pickUps[MAX_PICKUPS] = {};

	Camera m_worldCamera;
	Camera m_UICamera;
	Camera m_parallaxCamera;

	Vec2 m_lastParallaxPosition;
	Vec2 m_starsPositionParallax[PARALLAX_STARS_AMOUNT] = {};

	bool m_playingWinGameSound = false;
	bool m_playingLoseGameSound = false;
	

	bool m_screenShake = false;
	float m_screenShakeDuration = 0.0f;
	float m_screenShakeTranslation = 0.0f;
	float m_timeShakingScreen = 0.0f;
	
	int m_wave = 0;
	int m_enemiesAlive = 0;
	
	float secondsAfterWinning = 0.0f;
	float secondsToSpawnPickup = 0.0f;

	Vec2 m_clockPosition = Vec2(UI_CLOCK_CENTER_X, UI_CLOCK_CENTER_Y);
	
	std::vector	<Vertex_PCU> m_textAnimTriangles = std::vector<Vertex_PCU>();
	float m_timeTextAnimation = 0.0f;

	GameState m_state = AttractScreen;

	unsigned char m_playButtonAlpha = 0;

	float m_attractModeOrientation = 0.0f;

	float m_timeAlive = 0.0f;
	Vec2 m_attractModePos = Vec2(WORLD_CENTER_X - 1.9F, WORLD_CENTER_Y);
	Camera m_AttractScreenCamera;

	Clock m_clock;
};