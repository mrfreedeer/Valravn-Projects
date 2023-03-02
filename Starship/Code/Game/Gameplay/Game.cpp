#include "Engine/Math/AABB2.hpp"
#include "Game/Framework/App.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/Game.hpp"
#include "Game/Gameplay/PlayerShip.hpp"
#include "Game/Gameplay/Asteroid.hpp"
#include "Game/Gameplay/Bullet.hpp"
#include "Game/Gameplay/Beetle.hpp"
#include "Game/Gameplay/Wasp.hpp"
#include "Game/Gameplay/TimeWarper.hpp"
#include "Game/Gameplay/TwinAttacker.hpp"
#include "Game/Gameplay/Debris.hpp"
#include "Game/Gameplay/PickUp.hpp"


extern bool g_drawDebug;
extern App* g_theApp;
extern AudioSystem* g_theAudio;


SoundID playerShootingSound;
SoundID playerExplosionSound;
SoundID	playerFlameSound;
SoundID clockTickingSound;
SoundID laserBeamSound;
SoundID pickUpSound;
SoundID respawnSound;
SoundID winGameSound;
SoundID loseGameSound;
SoundID entityDamageSound;
SoundID entityDeathSound;


Game::Game(App* appPointer):
	m_theApp(appPointer)
{
}

Game::~Game()
{
	
}

void Game::Startup()
{
	m_playerShip = new PlayerShip(this, Vec2(WORLD_CENTER_X, WORLD_CENTER_Y));
	m_enemiesAlive = 0;
	m_wave = 0;

	LoadSoundFiles();
	StartUpParallax();
}

void Game::ShutDown()
{
	m_playerShip->StopAllSounds();
	delete m_playerShip;
	m_playerShip = nullptr;

	for (int asteroidIndex = 0; asteroidIndex < MAX_ASTEROIDS; asteroidIndex++) {
		Asteroid*& asteroid = m_asteroids[asteroidIndex];
		if (asteroid) {
			delete asteroid;
			asteroid = nullptr;
		}
	}

	for (int bulletIndex = 0; bulletIndex < MAX_BULLETS; bulletIndex++) {
		Bullet*& bullet = m_bullets[bulletIndex];
		if (bullet) {
			delete bullet;
			bullet = nullptr;
		}
	}

	for (int enemyIndex = 0; enemyIndex < MAX_ENEMIES; enemyIndex++)
	{
		Entity*& enemy = m_enemies[enemyIndex];
		if (enemy) {
			delete enemy;
			enemy = nullptr;
		}
	}

	for (int pickUpIndex = 0; pickUpIndex < MAX_PICKUPS; pickUpIndex++)
	{
		PickUp*& pickUp = m_pickUps[pickUpIndex];
		if (pickUp) {
			delete pickUp;
			pickUp = nullptr;
		}
	}

	if (m_loseGameSoundID != -1) {
		g_theAudio->StopSound(m_loseGameSoundID);
	}

	if (m_winGameSoundID != -1) {
		g_theAudio->StopSound(m_winGameSoundID);
	}
}

void Game::LoadSoundFiles() {
	playerShootingSound = g_theAudio->CreateOrGetSound("Data/Audio/ShootingSound.wav");
	playerExplosionSound = g_theAudio->CreateOrGetSound("Data/Audio/PlayerExplosionSound.wav");
	playerFlameSound = g_theAudio->CreateOrGetSound("Data/Audio/PlayerFlameSound.wav");
	clockTickingSound = g_theAudio->CreateOrGetSound("Data/Audio/TickingClock.wav");
	laserBeamSound = g_theAudio->CreateOrGetSound("Data/Audio/LaserBeamSound.wav");
	pickUpSound = g_theAudio->CreateOrGetSound("Data/Audio/PickUpSound.wav");
	respawnSound = g_theAudio->CreateOrGetSound("Data/Audio/RespawnSound.wav");
	winGameSound = g_theAudio->CreateOrGetSound("Data/Audio/WinGameSound.wav");
	loseGameSound = g_theAudio->CreateOrGetSound("Data/Audio/LoseGameSound.wav");
	entityDamageSound = g_theAudio->CreateOrGetSound("Data/Audio/EntityDamageSound.wav");
	entityDeathSound = g_theAudio->CreateOrGetSound("Data/Audio/EntityDeathSound.wav");
}

void Game::StartUpParallax()
{
	// The following ranges and factors are totally arbitrary. It just felt good

	Vec2 innerParallaxPlanetBounds;
	Vec2 outerParallaxPlanetBounds = Vec2(PARALLAX_PLANET_CENTER_X, PARALLAX_PLANET_CENTER_Y) +
		Vec2(3 * PARALLAX_PLANET_RING_RADIUS + PARALLAX_PLANET_RING_THICKNESS, 3 * PARALLAX_PLANET_RING_RADIUS + PARALLAX_PLANET_RING_THICKNESS);

	AABB2 parallaxPlanetBounds(innerParallaxPlanetBounds, outerParallaxPlanetBounds);


	for (int starIndex = 0; starIndex < PARALLAX_STARS_AMOUNT; starIndex++) {
		float randX = rng.GetRandomFloatInRange(-WORLD_CENTER_X * PARALLAX_FACTOR, WORLD_SIZE_X + WORLD_CENTER_X * PARALLAX_FACTOR);
		float randY = rng.GetRandomFloatInRange(-WORLD_SIZE_Y * PARALLAX_FACTOR, WORLD_SIZE_Y + WORLD_CENTER_Y * PARALLAX_FACTOR);

		if (parallaxPlanetBounds.IsPointInside(Vec2(randX, randY))) {

			int directionToPush = rng.GetRandomIntInRange(0, 1);
			float amountToPush = PARALLAX_PLANET_RING_RADIUS + PARALLAX_PLANET_RING_THICKNESS * 2.0f;
			if (directionToPush) {

				randX += amountToPush;
			}
			else {
				randY -= amountToPush;
			}
		}

		m_starsPositionParallax[starIndex] = Vec2(randX, randY);
	}
}


void Game::Update(float deltaSeconds)
{
	if (g_theConsole->GetMode() == DevConsoleMode::SHOWN) {
		g_theInput->ConsumeAllKeysJustPressed();
	}
	switch (m_state)
	{
	case AttractScreen:
		UpdateAttractScreen(deltaSeconds);
		break;
	case Play:
		UpdateGame(deltaSeconds);
		break;
	default:
		break;
	}

	UpdateUICamera(deltaSeconds);
}

void Game::UpdateGame(float deltaTime)
{
	float gameDeltaTime = static_cast<float>(m_clock.GetDeltaTime());
	ManageWaves(gameDeltaTime);

	UpdateDeveloperCheatCodes(deltaTime);

	UpdateEntities(gameDeltaTime);

	CheckCollisions();
	DeleteGarbageEntities();

	if (m_playerShip->m_playerDeathTime >= TIME_TO_RETURN_TO_ATTRACT_SCREEN
		|| secondsAfterWinning >= TIME_TO_RETURN_TO_ATTRACT_SCREEN
		|| g_theInput->WasKeyJustPressed(KEYCODE_ESC)) {

		ShutDown();
		Startup();
		m_state = AttractScreen;
	}

	UpdateCameras(gameDeltaTime);

	if (m_pickupsInGame < MAX_PICKUPS) {

		if (secondsToSpawnPickup <= 0.0f) {
			secondsToSpawnPickup = PICKUP_TIME_TO_SPAWN;
			m_pickupsInGame++;
			SpawnPickUp();
		}

		secondsToSpawnPickup -= gameDeltaTime;
	}

	if (m_useTextAnimation) {
		UpdateTextAnimation(gameDeltaTime, m_currentText, Vec2(UI_CENTER_X, UI_SIZE_Y * TEXT_ANIMATION_POS_PERCENTAGE_TOP));
	}
}

void Game::UpdateAttractScreen(float deltaSeconds)
{
	m_attractModeOrientation += -1.5f;
	m_playButtonAlpha = 55 + static_cast<unsigned char>(fabsf(sinf(m_timeAlive * 4)) * 0.5f * 200.0f);
	m_AttractScreenCamera.SetOrthoView(Vec2(0.0f, 0.0f), Vec2(WORLD_SIZE_X, WORLD_SIZE_Y));

	UpdateInputAttractScreen(deltaSeconds);
	m_timeAlive += deltaSeconds;
}

void Game::UpdateEntities(float deltaTime)
{
	m_playerShip->Update(deltaTime);
	for (int asteroidIndex = 0; asteroidIndex < MAX_ASTEROIDS; asteroidIndex++) {
		Asteroid*& asteroid = m_asteroids[asteroidIndex];
		if (asteroid) {
			asteroid->Update(deltaTime);
		}
	}

	for (int bulletIndex = 0; bulletIndex < MAX_BULLETS; bulletIndex++) {
		Bullet*& bullet = m_bullets[bulletIndex];
		if (bullet) {
			bullet->Update(deltaTime);
			if (bullet->IsOffScreen()) {
				bullet->Die();
			}
		}
	}

	for (int enemyIndex = 0; enemyIndex < MAX_ENEMIES; enemyIndex++) {
		Entity* enemy = m_enemies[enemyIndex];
		if (enemy) {
			enemy->Update(deltaTime);
		}
	}

	for (int debrisIndex = 0; debrisIndex < DEBRIS_MAX_AMOUNT; debrisIndex++) {
		Debris* debris = m_debris[debrisIndex];
		if (debris) {
			debris->Update(deltaTime);
		}
	}
}

void Game::UpdateInputAttractScreen(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	XboxController controller = g_theInput->GetController(0);

	bool exitAttractMode = false;
	exitAttractMode = g_theInput->WasKeyJustPressed(KEYCODE_SPACE);
	exitAttractMode = exitAttractMode || g_theInput->WasKeyJustPressed('N');
	exitAttractMode = exitAttractMode || controller.WasButtonJustPressed(XboxButtonID::Start);
	exitAttractMode = exitAttractMode || controller.WasButtonJustPressed(XboxButtonID::A);

	if (exitAttractMode) {
		m_state = Play;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) || controller.WasButtonJustPressed(XboxButtonID::Back)) {
		m_theApp->HandleQuitRequested();
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_TILDE)) {
		g_theConsole->ToggleMode(DevConsoleMode::SHOWN);
	}
}

void Game::UpdateDeveloperCheatCodes(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	XboxController controller = g_theInput->GetController(0);

	if (g_theInput->WasKeyJustPressed('I') || controller.WasButtonJustPressed(XboxButtonID::LeftShoulder)) {
		SpawnRandomAsteroid();
	}

	if (g_theInput->WasKeyJustPressed('T') || controller.IsButtonDown(XboxButtonID::RightShoulder)) {
		m_clock.SetTimeDilation(0.1);
	}

	if (g_theInput->WasKeyJustPressed('O') || controller.WasButtonJustPressed(XboxButtonID::Up)) {
		m_clock.StepFrame();
	}

	if (g_theInput->WasKeyJustPressed('P') || controller.WasButtonJustPressed(XboxButtonID::Back)) {
		m_clock.TogglePause();
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F1) || controller.WasButtonJustPressed(XboxButtonID::LeftThumb)) {
		g_drawDebug = !g_drawDebug;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F6) || controller.WasButtonJustPressed(XboxButtonID::Down)) {
		KillAllEnemies();
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F8) || controller.WasButtonJustPressed(XboxButtonID::RightThumb)) {
		ShutDown();
		Startup();
		m_state = Play;
	}

	if (g_theInput->WasKeyJustReleased('T') || controller.WasButtonJustReleased(XboxButtonID::RightShoulder)) {
		m_clock.SetTimeDilation(1);
	}

	if (g_theInput->WasKeyJustPressed('K')) {
		SoundID testSound = g_theAudio->CreateOrGetSound("Data/Audio/TestSound.mp3");
		g_theAudio->StartSound(testSound);
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_TILDE)) {
		g_theConsole->ToggleMode(DevConsoleMode::SHOWN);
	}

}

void Game::UpdateCameras(float deltaSeconds)
{
	UpdateWorldCamera(deltaSeconds);

	UpdateParallaxCamera();
}

void Game::UpdateWorldCamera(float deltaSeconds)
{
	m_worldCamera.SetOrthoView(Vec2(), Vec2(WORLD_SIZE_X, WORLD_SIZE_Y));
	if (m_screenShake) {
		float easingInScreenShake = m_timeShakingScreen / m_screenShakeDuration;
		easingInScreenShake *= (m_screenShakeTranslation * 0.95f);

		float randAmountX = rng.GetRandomFloatInRange(-1.0f, 1.0f) * (m_screenShakeTranslation - easingInScreenShake);
		float randAmountY = rng.GetRandomFloatInRange(-1.0f, 1.0f) * (m_screenShakeTranslation - easingInScreenShake);

		m_worldCamera.TranslateCamera(randAmountX, randAmountY);

		m_timeShakingScreen += deltaSeconds;

		if (m_timeShakingScreen > m_screenShakeDuration) {
			m_screenShake = false;
			m_timeShakingScreen = 0.0f;;
		}
	}
}

void Game::UpdateUICamera(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	m_UICamera.SetOrthoView(Vec2(), Vec2(UI_SIZE_X, UI_SIZE_Y));
}

void Game::UpdateParallaxCamera()
{
	m_parallaxCamera.SetOrthoView(Vec2(), Vec2(WORLD_SIZE_X, WORLD_SIZE_Y));

	Vec2 parallaxCameraMovement = m_playerShip->m_position * PARALLAX_FACTOR;
	if (m_playerShip->IsAlive()) {
		m_lastParallaxPosition = parallaxCameraMovement;
		m_parallaxCamera.TranslateCamera(parallaxCameraMovement);
	}
	else {
		m_parallaxCamera.TranslateCamera(m_lastParallaxPosition);
	}
}

void Game::UpdateTextAnimation(float deltaTime, std::string text, Vec2 textLocation)
{
	m_timeTextAnimation += deltaTime;
	m_textAnimTriangles.clear();
	AddVertsForTextTriangles2D(m_textAnimTriangles, text, Vec2(), TEXT_CELL_HEIGHT, m_textAnimationColor);

	float fullTextWidth = GetSimpleTriangleStringWidth(text, TEXT_CELL_HEIGHT);
	float textWidth = GetTextWidthForAnim(fullTextWidth);

	Vec2 iBasis = GetIBasisForTextAnim();
	Vec2 jBasis = GetJBasisForTextAnim();

	TransformText(iBasis, jBasis, Vec2(textLocation.x - textWidth * 0.5f, textLocation.y));

	if (m_timeTextAnimation >= TEXT_ANIMATION_TIME) {
		m_useTextAnimation = false;
		m_timeTextAnimation = 0.0f;
	}

}

void Game::KillAllEnemies()
{
	for (int enemyIndex = 0; enemyIndex < MAX_ENEMIES; enemyIndex++) {
		Entity* enemy = m_enemies[enemyIndex];
		if (enemy) {
			enemy->Die();
		}
	}
	m_enemiesAlive = 0;
}

void Game::TransformText(Vec2 iBasis, Vec2 jBasis, Vec2 translation)
{
	for (int vertIndex = 0; vertIndex < int(m_textAnimTriangles.size()); vertIndex++) {
		Vec3& vertPosition = m_textAnimTriangles[vertIndex].m_position;
		TransformPositionXY3D(vertPosition, iBasis, jBasis, translation);
	}
}

float Game::GetTextWidthForAnim(float fullTextWidth) {

	float timeAsFraction = GetFractionWithin(m_timeTextAnimation, 0.0f, TEXT_ANIMATION_TIME);
	float textWidth = 0.0f;
	if (timeAsFraction < TEXT_MOVEMENT_PHASE_TIME_PERCENTAGE) {
		if (timeAsFraction < TEXT_MOVEMENT_PHASE_TIME_PERCENTAGE * 0.5f) {
			textWidth = RangeMapClamped(timeAsFraction, 0.0f, TEXT_MOVEMENT_PHASE_TIME_PERCENTAGE * 0.5f, 0.0f, fullTextWidth);
		}
		else {
			return fullTextWidth;
		}
	}
	else if (timeAsFraction > TEXT_MOVEMENT_PHASE_TIME_PERCENTAGE && timeAsFraction < 1 - TEXT_MOVEMENT_PHASE_TIME_PERCENTAGE) {
		return fullTextWidth;
	}
	else {
		if (timeAsFraction < 1.0f - TEXT_MOVEMENT_PHASE_TIME_PERCENTAGE * 0.5f) {
			return fullTextWidth;
		}
		else {
			textWidth = RangeMapClamped(timeAsFraction, 1.0f - TEXT_MOVEMENT_PHASE_TIME_PERCENTAGE * 0.5f, 1.0f, fullTextWidth, 0.0f);
		}
	}
	return textWidth;

}

Vec2 const Game::GetIBasisForTextAnim()
{
	float timeAsFraction = GetFractionWithin(m_timeTextAnimation, 0.0f, TEXT_ANIMATION_TIME);
	float iScale = 0.0f;
	if (timeAsFraction < TEXT_MOVEMENT_PHASE_TIME_PERCENTAGE) {
		if (timeAsFraction < TEXT_MOVEMENT_PHASE_TIME_PERCENTAGE * 0.5f) {
			iScale = RangeMapClamped(timeAsFraction, 0.0f, TEXT_MOVEMENT_PHASE_TIME_PERCENTAGE * 0.5f, 0.0f, 1.0f);
		}
		else {
			iScale = 1.0f;
		}
	}
	else if (timeAsFraction > TEXT_MOVEMENT_PHASE_TIME_PERCENTAGE && timeAsFraction < 1 - TEXT_MOVEMENT_PHASE_TIME_PERCENTAGE) {
		iScale = 1.0f;
	}
	else {
		if (timeAsFraction < 1.0f - TEXT_MOVEMENT_PHASE_TIME_PERCENTAGE * 0.5f) {
			iScale = 1.0f;
		}
		else {
			iScale = RangeMapClamped(timeAsFraction, 1.0f - TEXT_MOVEMENT_PHASE_TIME_PERCENTAGE * 0.5f, 1.0f, 1.0f, 0.0f);
		}
	}

	return Vec2(iScale, 0.0f);
}

Vec2 const Game::GetJBasisForTextAnim()
{
	float timeAsFraction = GetFractionWithin(m_timeTextAnimation, 0.0f, TEXT_ANIMATION_TIME);
	float jScale = 0.0f;
	if (timeAsFraction < TEXT_MOVEMENT_PHASE_TIME_PERCENTAGE) {
		if (timeAsFraction < TEXT_MOVEMENT_PHASE_TIME_PERCENTAGE * 0.5f) {
			jScale = 0.05f;
		}
		else {
			jScale = RangeMapClamped(timeAsFraction, TEXT_MOVEMENT_PHASE_TIME_PERCENTAGE * 0.5f, TEXT_MOVEMENT_PHASE_TIME_PERCENTAGE, 0.05f, 1.0f);

		}
	}
	else if (timeAsFraction > TEXT_MOVEMENT_PHASE_TIME_PERCENTAGE && timeAsFraction < 1 - TEXT_MOVEMENT_PHASE_TIME_PERCENTAGE) {
		jScale = 1.0f;
	}
	else {
		if (timeAsFraction < 1.0f - TEXT_MOVEMENT_PHASE_TIME_PERCENTAGE * 0.5f) {
			jScale = RangeMapClamped(timeAsFraction, 1.0f - TEXT_MOVEMENT_PHASE_TIME_PERCENTAGE, 1.0f - TEXT_MOVEMENT_PHASE_TIME_PERCENTAGE * 0.5f, 1.0f, 0.05f);
		}
		else {
			jScale = 0.05f;
		}
	}

	return Vec2(0.0f, jScale);
}




void Game::ManageWaves(float deltaTime)
{
	if (m_enemiesAlive <= 0 && m_wave <= NUM_WAVES)
	{
		m_wave++;

		if (m_playerShip->m_health > 0) {
			m_currentText = Stringf("WAVE %i", m_wave);
			m_textAnimationColor = Rgba8(255, 255, 255, 255);
		}

		if (m_wave != NUM_WAVES + 1) {
			SpawnEnemies();
		}

		m_timeTextAnimation = 0.0f;
		m_useTextAnimation = true;
	}
	else if (m_wave >= NUM_WAVES + 1 && m_enemiesAlive <= 0)
	{
		secondsAfterWinning += deltaTime;
		if (m_currentText != "YOU WON!!!!") {
			m_currentText = "YOU WON!!!!";
			m_useTextAnimation = true;
			m_textAnimationColor = Rgba8(0, 255, 0, 255);
		}

		if (!m_playingWinGameSound) {
			m_playingWinGameSound = true;
			m_winGameSoundID = g_theAudio->StartSound(winGameSound);
		}
	}
}

void Game::Render() const
{
	switch (m_state)
	{
	case AttractScreen:
		RenderAttractScreen();
		break;
	case Play:
		RenderGame();
		break;
	default:
		break;
	}

	RenderDevConsole();
}



void Game::DeleteGarbageEntities()
{
	for (int asteroidIndex = 0; asteroidIndex < MAX_ASTEROIDS; asteroidIndex++) {
		Asteroid*& asteroid = m_asteroids[asteroidIndex];
		if (asteroid && asteroid->m_isGarbage) {
			delete asteroid;
			asteroid = nullptr;
		}
	}

	for (int bulletIndex = 0; bulletIndex < MAX_BULLETS; bulletIndex++) {
		Bullet*& bullet = m_bullets[bulletIndex];
		if (bullet && bullet->m_isGarbage) {
			delete bullet;
			bullet = nullptr;
		}
	}

	for (int enemyIndex = 0; enemyIndex < MAX_ENEMIES; enemyIndex++) {
		Entity*& enemy = m_enemies[enemyIndex];
		if (enemy && enemy->m_isGarbage) {
			delete enemy;
			enemy = nullptr;
		}
	}

	for (int debrisIndex = 0; debrisIndex < DEBRIS_MAX_AMOUNT; debrisIndex++) {
		Debris*& debris = m_debris[debrisIndex];
		if (debris && debris->m_isGarbage) {
			delete debris;
			debris = nullptr;
		}
	}

	for (int pickupIndex = 0; pickupIndex < MAX_PICKUPS; pickupIndex++) {
		PickUp*& pickup = m_pickUps[pickupIndex];
		if (pickup && pickup->m_isGarbage) {
			delete pickup;
			pickup = nullptr;
		}
	}

}

void Game::RenderGame() const
{
	RenderParallax();

	RenderEntities();

	RenderUI();
}

void Game::RenderAttractScreen() const
{
	g_theRenderer->BeginCamera(m_AttractScreenCamera);
	g_theRenderer->BindShader(nullptr);

	g_theRenderer->ClearScreen(Rgba8(0, 0, 0, 1));

	PlayerShip::DrawPlayerShip(m_attractModePos, m_attractModeOrientation, 2.0f, 10.0f);
	PlayerShip::DrawPlayerShip(m_attractModePos, -m_attractModeOrientation, 2.0f, 10.0f);

	Vec3 playButtonBottom = Vec3(WORLD_CENTER_X - playButtonWidth * 0.5f, WORLD_CENTER_Y - playButtonHeight * 0.5f, 0.0f);
	Vec3 playButtonRight = Vec3(WORLD_CENTER_X + playButtonWidth * 0.5f, WORLD_CENTER_Y, 0.0f);
	Vec3 playButtonTop = Vec3(WORLD_CENTER_X - playButtonWidth * 0.5f, WORLD_CENTER_Y + playButtonHeight * 0.5f, 0.0f);

	Vertex_PCU playButtonVerts[3];
	Rgba8 playButtonColor(0, 255, 0, m_playButtonAlpha);
	playButtonVerts[0] = Vertex_PCU(playButtonBottom, playButtonColor, Vec2());
	playButtonVerts[1] = Vertex_PCU(playButtonRight, playButtonColor, Vec2());
	playButtonVerts[2] = Vertex_PCU(playButtonTop, playButtonColor, Vec2());


	g_theRenderer->DrawVertexArray(3, playButtonVerts);
	g_theRenderer->EndCamera(m_AttractScreenCamera);
}

void Game::RenderDevConsole() const
{
	AABB2 consoleBounds(m_UICamera.GetOrthoBottomLeft(), m_UICamera.GetOrthoTopRight());
	g_theRenderer->BeginCamera(m_UICamera);
	g_theRenderer->BindShader(nullptr);

	g_theConsole->Render(consoleBounds);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->EndCamera(m_UICamera);
}

void Game::RenderEntities() const
{
	g_theRenderer->BeginCamera(m_worldCamera);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->BindTexture(nullptr);


	//g_theRenderer->ClearScreen(Rgba8::BLACK);

	for (int asteroidIndex = 0; asteroidIndex < MAX_ASTEROIDS; asteroidIndex++) {
		Asteroid const* asteroid = m_asteroids[asteroidIndex];
		if (asteroid) {
			asteroid->Render();
		}
	}

	m_playerShip->Render();

	for (int bulletIndex = 0; bulletIndex < MAX_BULLETS; bulletIndex++) {
		Bullet const* bullet = m_bullets[bulletIndex];
		if (bullet) {
			bullet->Render();
		}
	}

	for (int enemyIndex = 0; enemyIndex < MAX_ENEMIES; enemyIndex++) {
		Entity const* enemy = m_enemies[enemyIndex];
		if (enemy) {
			enemy->Render();
		}
	}

	for (int debrisIndex = 0; debrisIndex < DEBRIS_MAX_AMOUNT; debrisIndex++) {
		Debris const* debris = m_debris[debrisIndex];
		if (debris) {
			debris->Render();
		}
	}

	if (g_drawDebug) {
		RenderDegubEntities();
	}


	for (int pickupIndex = 0; pickupIndex < MAX_PICKUPS; pickupIndex++) {
		PickUp const* pickup = m_pickUps[pickupIndex];
		if (pickup) {
			pickup->Render();
		}
	}
	g_theRenderer->EndCamera(m_worldCamera);

}

void Game::RenderDegubEntities() const {
	float lineThickness = 0.15f;
	Rgba8 darkGrey(50, 50, 50, 255);

	for (int asteroidIndex = 0; asteroidIndex < MAX_ASTEROIDS; asteroidIndex++) {
		Asteroid const* asteroid = m_asteroids[asteroidIndex];
		if (asteroid) {
			DebugDrawLine(asteroid->m_position, m_playerShip->m_position, lineThickness, darkGrey);
			asteroid->DrawDebug();
		}
	}

	for (int bulletIndex = 0; bulletIndex < MAX_BULLETS; bulletIndex++) {
		Bullet const* bullet = m_bullets[bulletIndex];
		if (bullet) {
			DebugDrawLine(bullet->m_position, m_playerShip->m_position, lineThickness, darkGrey);
			bullet->DrawDebug();
		}
	}

	for (int enemyIndex = 0; enemyIndex < MAX_ENEMIES; enemyIndex++)
	{
		Entity const* enemy = m_enemies[enemyIndex];
		if (enemy) {
			DebugDrawLine(enemy->m_position, m_playerShip->m_position, lineThickness, darkGrey);
			enemy->DrawDebug();
		}
	}

	for (int debrisIndex = 0; debrisIndex < DEBRIS_MAX_AMOUNT; debrisIndex++) {
		Debris const* debris = m_debris[debrisIndex];
		if (debris) {
			DebugDrawLine(debris->m_position, m_playerShip->m_position, lineThickness, darkGrey);
			debris->DrawDebug();
		}
	}
	m_playerShip->DrawDebug();

}

void Game::RenderUI() const
{

	g_theRenderer->BeginCamera(m_UICamera);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->BindTexture(nullptr);

	//g_theRenderer->ClearScreen(Rgba8::BLACK);

	m_playerShip->RenderLives();

	if (m_playerShip->m_isSlowedDown && !m_playerShip->m_isSpedUp) {
		RenderSlowMoClock();
	}

	if (!m_playerShip->m_isSlowedDown && m_playerShip->m_isSpedUp) {
		RenderSpedUpClock();
	}

	if (m_useTextAnimation) {
		if (m_wave <= NUM_WAVES || (m_wave > NUM_WAVES) && (m_currentText != Stringf("WAVE: ", m_wave))) {

			RenderTextAnimation();
		}
	}

	g_theRenderer->EndCamera(m_UICamera);
}

void Game::RenderParallax() const
{

	g_theRenderer->BeginCamera(m_parallaxCamera);
	g_theRenderer->ClearScreen(Rgba8::BLACK);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->BindTexture(nullptr);

	DrawCircle(Vec2(PARALLAX_PLANET_CENTER_X, PARALLAX_PLANET_CENTER_Y), PARALLAX_PLANET_RADIUS, Rgba8(255, 0, 0, 128));
	DebugDrawRing(Vec2(PARALLAX_PLANET_CENTER_X, PARALLAX_PLANET_CENTER_Y), PARALLAX_PLANET_RING_RADIUS, PARALLAX_PLANET_RING_THICKNESS, Rgba8(0, 0, 255, 255));

	for (int starIndex = 0; starIndex < PARALLAX_STARS_AMOUNT; starIndex++) {
		DrawCircle(m_starsPositionParallax[starIndex], 0.1f, Rgba8());
	}

	g_theRenderer->EndCamera(m_parallaxCamera);

}

void Game::RenderSlowMoClock() const
{
	constexpr float clockDetailsSizeInPercentage = 1.0f - 0.2f;

	Rgba8 red(255, 0, 0, 255);

	RenderClock();

	Vec2 arrowsCenter(UI_CLOCK_CENTER_X - (1.0f - clockDetailsSizeInPercentage) * UI_CLOCK_RADIUS, UI_CLOCK_CENTER_Y);
	Vec2 uvTexCoords;

	Vertex_PCU backwardArrows[6] = {};
	backwardArrows[0] = Vertex_PCU(Vec3(arrowsCenter.x - UI_CLOCK_RADIUS * clockDetailsSizeInPercentage, arrowsCenter.y, 0.0f), red, uvTexCoords);
	backwardArrows[1] = Vertex_PCU(Vec3(arrowsCenter.x, arrowsCenter.y - UI_CLOCK_RADIUS * clockDetailsSizeInPercentage, 0.0f), red, uvTexCoords);
	backwardArrows[2] = Vertex_PCU(Vec3(arrowsCenter.x, arrowsCenter.y + UI_CLOCK_RADIUS * clockDetailsSizeInPercentage, 0.0f), red, uvTexCoords);

	backwardArrows[3] = Vertex_PCU(Vec3(arrowsCenter.x, arrowsCenter.y, 0.0f), red, uvTexCoords);
	backwardArrows[4] = Vertex_PCU(Vec3(arrowsCenter.x + UI_CLOCK_RADIUS * clockDetailsSizeInPercentage, arrowsCenter.y - UI_CLOCK_RADIUS * clockDetailsSizeInPercentage, 0.0f), red, uvTexCoords);
	backwardArrows[5] = Vertex_PCU(Vec3(arrowsCenter.x + UI_CLOCK_RADIUS * clockDetailsSizeInPercentage, arrowsCenter.y + UI_CLOCK_RADIUS * clockDetailsSizeInPercentage, 0.0f), red, uvTexCoords);

	g_theRenderer->DrawVertexArray(6, backwardArrows);
}

void Game::RenderSpedUpClock() const
{
	constexpr float clockDetailsSizeInPercentage = 1.0f - 0.2f;

	Rgba8 blue(0, 0, 255, 255);

	RenderClock();

	Vec2 arrowsCenter(UI_CLOCK_CENTER_X + (1.0f - clockDetailsSizeInPercentage) * UI_CLOCK_RADIUS, UI_CLOCK_CENTER_Y);
	Vec2 uvTexCoords;

	Vertex_PCU backwardArrows[6] = {};
	backwardArrows[0] = Vertex_PCU(Vec3(arrowsCenter.x - UI_CLOCK_RADIUS * clockDetailsSizeInPercentage, arrowsCenter.y - UI_CLOCK_RADIUS * clockDetailsSizeInPercentage, 0.0f), blue, uvTexCoords);
	backwardArrows[1] = Vertex_PCU(Vec3(arrowsCenter.x, arrowsCenter.y, 0.0f), blue, uvTexCoords);
	backwardArrows[2] = Vertex_PCU(Vec3(arrowsCenter.x - UI_CLOCK_RADIUS * clockDetailsSizeInPercentage, arrowsCenter.y + UI_CLOCK_RADIUS * clockDetailsSizeInPercentage, 0.0f), blue, uvTexCoords);

	backwardArrows[3] = Vertex_PCU(Vec3(arrowsCenter.x, arrowsCenter.y - UI_CLOCK_RADIUS * clockDetailsSizeInPercentage, 0.0f), blue, uvTexCoords);
	backwardArrows[4] = Vertex_PCU(Vec3(arrowsCenter.x + UI_CLOCK_RADIUS * clockDetailsSizeInPercentage, arrowsCenter.y, 0.0f), blue, uvTexCoords);
	backwardArrows[5] = Vertex_PCU(Vec3(arrowsCenter.x, arrowsCenter.y + UI_CLOCK_RADIUS * clockDetailsSizeInPercentage, 0.0f), blue, uvTexCoords);

	g_theRenderer->DrawVertexArray(6, backwardArrows);

}

void Game::RenderClock() const
{
	Rgba8 gold(212, 175, 55, 255);
	Rgba8 black = Rgba8(0, 0, 0, 255);
	constexpr float clockDetailsSizeInPercentage = 0.2f;

	DrawCircle(m_clockPosition, UI_CLOCK_RADIUS, Rgba8());
	DrawCircle(m_clockPosition, UI_CLOCK_RADIUS * clockDetailsSizeInPercentage * 0.25f, black);


	DebugDrawRing(m_clockPosition, UI_CLOCK_RADIUS * 1.1f, UI_CLOCK_RADIUS * clockDetailsSizeInPercentage, gold);

	Vertex_PCU squareOnTop[6] = {};
	Vec2 squarePosition = Vec2(UI_CLOCK_CENTER_X, UI_CLOCK_RADIUS * (1.0f + clockDetailsSizeInPercentage) + UI_CLOCK_CENTER_Y);

	squareOnTop[0] = Vertex_PCU(Vec3(squarePosition.x - UI_CLOCK_RADIUS * clockDetailsSizeInPercentage * 0.5f, squarePosition.y, 0.0f), gold, Vec2());
	squareOnTop[1] = Vertex_PCU(Vec3(squarePosition.x + UI_CLOCK_RADIUS * clockDetailsSizeInPercentage * 0.5f, squarePosition.y, 0.0f), gold, Vec2());
	squareOnTop[2] = Vertex_PCU(Vec3(squarePosition.x + UI_CLOCK_RADIUS * clockDetailsSizeInPercentage * 0.5f, squarePosition.y + UI_CLOCK_RADIUS * clockDetailsSizeInPercentage, 0.0f), gold, Vec2());

	squareOnTop[3] = Vertex_PCU(Vec3(squarePosition.x - UI_CLOCK_RADIUS * clockDetailsSizeInPercentage * 0.5f, squarePosition.y, 0.0f), gold, Vec2());
	squareOnTop[4] = Vertex_PCU(Vec3(squarePosition.x + UI_CLOCK_RADIUS * clockDetailsSizeInPercentage * 0.5f, squarePosition.y + UI_CLOCK_RADIUS * clockDetailsSizeInPercentage, 0.0f), gold, Vec2());
	squareOnTop[5] = Vertex_PCU(Vec3(squarePosition.x - UI_CLOCK_RADIUS * clockDetailsSizeInPercentage * 0.5f, squarePosition.y + UI_CLOCK_RADIUS * clockDetailsSizeInPercentage, 0.0f), gold, Vec2());

	g_theRenderer->DrawVertexArray(6, squareOnTop);

	Vec2 clockTop(UI_CLOCK_CENTER_X, UI_CLOCK_CENTER_Y + UI_CLOCK_RADIUS);
	Vec2 clockBottom(UI_CLOCK_CENTER_X, UI_CLOCK_CENTER_Y - UI_CLOCK_RADIUS);

	Vec2 clockLeft(UI_CLOCK_CENTER_X - UI_CLOCK_RADIUS, UI_CLOCK_CENTER_Y);
	Vec2 clockRight(UI_CLOCK_CENTER_X + UI_CLOCK_RADIUS, UI_CLOCK_CENTER_Y);

	DebugDrawLine(clockTop, Vec2(clockTop.x, clockTop.y - UI_CLOCK_RADIUS * clockDetailsSizeInPercentage), UI_CLOCK_RADIUS * clockDetailsSizeInPercentage, black);
	DebugDrawLine(clockBottom, Vec2(clockBottom.x, clockBottom.y + UI_CLOCK_RADIUS * clockDetailsSizeInPercentage), UI_CLOCK_RADIUS * clockDetailsSizeInPercentage, black);

	DebugDrawLine(clockLeft, Vec2(clockLeft.x + UI_CLOCK_RADIUS * clockDetailsSizeInPercentage, clockLeft.y), UI_CLOCK_RADIUS * clockDetailsSizeInPercentage, black);
	DebugDrawLine(clockRight, Vec2(clockRight.x - UI_CLOCK_RADIUS * clockDetailsSizeInPercentage, clockRight.y), UI_CLOCK_RADIUS * clockDetailsSizeInPercentage, black);
}

void Game::RenderTextAnimation() const
{
	if (m_textAnimTriangles.size() > 0) {
		g_theRenderer->DrawVertexArray(int(m_textAnimTriangles.size()), &m_textAnimTriangles[0]);
	}
}





void Game::CheckCollisions()
{
	CheckCollisionAsteroids();
	CheckCollisionEntities();
	CheckCollisionLaserBeams();
	CheckCollisionPickUps();
}

void Game::CheckCollisionEntities()
{
	for (int enemyIndex = 0; enemyIndex < MAX_ENEMIES; enemyIndex++) {
		Entity* enemy = m_enemies[enemyIndex];
		if (!enemy || !enemy->IsAlive()) {
			continue;
		}

		for (int bulletIndex = 0; bulletIndex < MAX_BULLETS; bulletIndex++) {
			Bullet* bullet = m_bullets[bulletIndex];
			if (!bullet) {
				continue;
			}
			if (DoDiscsOverlap(enemy->m_position, enemy->GetCollisionRadius(), bullet->m_position, bullet->m_physicsRadius)) {
				enemy->TakeAHit(bullet->m_position);
				if (!enemy->IsAlive()) {
					m_enemiesAlive--;
				}
				bullet->Die();
			}
		}

		if (!m_playerShip->IsAlive()) {
			continue;
		}


		if (DoDiscsOverlap(enemy->m_position, enemy->GetCollisionRadius(), m_playerShip->m_position, m_playerShip->GetCollisionRadius())) {
			if (m_playerShip->m_shieldHealth <= 0 && enemy->m_shieldHealth > 0) {
				enemy->TakeAHit(m_playerShip->m_position);
			}
			else {
				enemy->Die();
				m_enemiesAlive--;
			}
			m_playerShip->TakeAHit(enemy->m_position);
		}
	}
}

void Game::CheckCollisionAsteroids()
{
	for (int asteroidIndex = 0; asteroidIndex < MAX_ASTEROIDS; asteroidIndex++) {
		Asteroid* asteroid = m_asteroids[asteroidIndex];
		if (!asteroid || !asteroid->IsAlive()) {
			continue;
		}

		for (int bulletIndex = 0; bulletIndex < MAX_BULLETS; bulletIndex++) {
			Bullet* bullet = m_bullets[bulletIndex];
			if (!bullet) {
				continue;
			}

			if (DoDiscsOverlap(asteroid->m_position, asteroid->m_physicsRadius, bullet->m_position, bullet->m_physicsRadius)) {
				asteroid->TakeAHit();
				bullet->Die();
			}

		}

		if (!m_playerShip->IsAlive()) {
			continue;
		}


		if (DoDiscsOverlap(asteroid->m_position, asteroid->m_physicsRadius, m_playerShip->m_position, m_playerShip->GetCollisionRadius())) {
			m_playerShip->TakeAHit(asteroid->m_position);
			asteroid->Die();
		}


	}
}

void Game::CheckCollisionLaserBeams()
{
	// Collision Player vs Twins' Laser Beam
	for (int twinIndex = 0; twinIndex < MAX_ENEMIES; twinIndex++) {
		TwinAttacker* twinOne = dynamic_cast<TwinAttacker*>(m_enemies[twinIndex]);

		if (!twinOne || !twinOne->m_twin) {
			continue;
		}

		TwinAttacker*& twinTwo = twinOne->m_twin;

		Vec2 beam = twinTwo->m_position - twinOne->m_position;
		Vec2 beamDir = beam.GetNormalized();
		Vec2 dispToPlayer = m_playerShip->m_position - twinOne->m_position;

		float distanceToPlayerOnRay = DotProduct2D(dispToPlayer, beamDir);

		distanceToPlayerOnRay = Clamp(distanceToPlayerOnRay, 0.0f, beam.GetLength());

		Vec2 nearestPointToPlayer = twinOne->m_position + (beamDir * distanceToPlayerOnRay);

		if (IsPointInsideDisc2D(nearestPointToPlayer, m_playerShip->m_position, m_playerShip->GetCollisionRadius())) {
			Vec2 posBetweenTwins = (twinOne->m_position + twinTwo->m_position) * 0.5f;
			Vec2 dispToPlayerBetTwins = m_playerShip->m_position - posBetweenTwins;
			dispToPlayerBetTwins.Normalize();
			m_playerShip->m_velocity = dispToPlayerBetTwins * 20.0f;

			m_playerShip->TakeAHit(nearestPointToPlayer);
		}

	}
}

void Game::CheckCollisionPickUps()
{
	for (int pickUpIndex = 0; pickUpIndex < MAX_PICKUPS; pickUpIndex++) {
		PickUp* const pickUp = m_pickUps[pickUpIndex];
		if (!pickUp) {
			continue;
		}

		if (DoDiscsOverlap(pickUp->m_position, pickUp->m_physicsRadius, m_playerShip->m_position, m_playerShip->m_physicsRadius)) {
			if (m_playerShip->IsAlive()) {
				pickUp->Die();
				m_playerShip->m_shieldHealth += SHIELD_DEFAULT_HEALTH;

			}
		}

		for (int entityIndex = 0; entityIndex < MAX_ENEMIES; entityIndex++) {
			Entity* const entity = m_enemies[entityIndex];
			if (!entity) {
				continue;
			}

			if (DoDiscsOverlap(pickUp->m_position, pickUp->m_physicsRadius, entity->m_position, entity->m_physicsRadius)) {
				pickUp->Die();
				entity->m_shieldHealth += SHIELD_DEFAULT_HEALTH;
			}
		}

	}

}


void Game::SpawnRandomAsteroid()
{
	float randX = rng.GetRandomFloatInRange(-WRAP_AROUND_SPACE, -ASTEROID_COSMETIC_RADIUS);
	float randY = rng.GetRandomFloatInRange(-WRAP_AROUND_SPACE, -ASTEROID_COSMETIC_RADIUS);

	for (int asteroidIndex = 0; asteroidIndex < MAX_ASTEROIDS; asteroidIndex++) {
		if (!m_asteroids[asteroidIndex]) {
			m_asteroids[asteroidIndex] = new Asteroid(this, Vec2(randX, randY));
			return;
		}
	}

	ERROR_RECOVERABLE("CANNOT SPAWN A NEW ASTEROID; ALL SLOTS ARE FULL!");

}

void Game::SpawnBullet(const Vec2& pos, float forwardDegrees)
{
	if (!m_playerShip->IsAlive()) return;
	for (int bulletIndex = 0; bulletIndex < MAX_BULLETS; bulletIndex++) {
		Bullet*& bullet = m_bullets[bulletIndex];
		if (!bullet) {
			bullet = new Bullet(this, pos);
			bullet->m_orientationDegrees = forwardDegrees;
			bullet->m_velocity.SetOrientationDegrees(forwardDegrees);
			return;
		}
	}

	ERROR_RECOVERABLE("CANNOT SPAWN MORE BULLETS; ALL SLOTS ARE FULL!");
}

void Game::SpawnDebrisCluster(int count, const Vec2& pos, Rgba8 color, const Vec2& baseVelocity,
	float minRndScale, float maxRndScale, float minRndSpeed, float maxRndSpeed)
{
	for (int spawnIndex = 0; spawnIndex < count; spawnIndex++) {
		float scale = rng.GetRandomFloatInRange(minRndScale, maxRndScale);
		float speed = rng.GetRandomFloatInRange(minRndSpeed, maxRndSpeed);

		SpawnRandomDebris(pos, color, baseVelocity, scale, speed);
	}
}

void Game::SpawnRandomDebris(const Vec2& pos, Rgba8 color, const Vec2& velocity, float scale, float speed)
{
	float randomDegrees = rng.GetRandomFloatInRange(-1.0f, 1.0f) * 90.0f;
	Vec2 newVel = velocity.GetRotatedDegrees(randomDegrees);
	for (int debrisIndex = 0; debrisIndex < DEBRIS_MAX_AMOUNT; debrisIndex++) {
		Debris*& debris = m_debris[debrisIndex];
		if (!debris) {
			debris = new Debris(this, pos, color, newVel, scale, speed);
			return;
		}
	}
}

Vec2 const Game::GetRandomSpawnPosition()
{
	int spawnArea = rng.GetRandomIntInRange(0, 3);
	Vec2 spawnPosition = Vec2();
	float randX = 0.0f;
	float randY = 0.0f;
	float spaceMultiplier = static_cast<float>(rng.GetRandomIntInRange(10, 15)); // To give player time to breath
	switch (spawnArea)
	{
	case 0:
		randX = rng.GetRandomFloatInRange(-WRAP_AROUND_SPACE * spaceMultiplier, -WRAP_AROUND_SPACE);
		randY = rng.GetRandomFloatInRange(0, WORLD_SIZE_Y);
		break;
	case 1:
		randX = rng.GetRandomFloatInRange(0, WORLD_SIZE_X);
		randY = rng.GetRandomFloatInRange(-WRAP_AROUND_SPACE * spaceMultiplier, -WRAP_AROUND_SPACE);
		break;
	case 2:
		randX = rng.GetRandomFloatInRange(WORLD_SIZE_X + WRAP_AROUND_SPACE * spaceMultiplier, WORLD_SIZE_X + WRAP_AROUND_SPACE);
		randY = rng.GetRandomFloatInRange(0, WORLD_SIZE_Y);
		break;
	case 3:
		randX = rng.GetRandomFloatInRange(0, WORLD_SIZE_X);
		randY = rng.GetRandomFloatInRange(WORLD_SIZE_Y + WRAP_AROUND_SPACE * spaceMultiplier, WORLD_SIZE_Y + WRAP_AROUND_SPACE);
		break;
	}

	return Vec2(randX, randY);
}

void Game::SpawnEnemies()
{

	constexpr int waspAmountPerWave[5] = { 0, 2, 3, 10, 4 };
	constexpr int beetleAmountPerWave[5] = { 0, 3, 4, 4, 6 };
	constexpr int timeWarperAmountPerWave[5] = { 0, 3, 4, 4, 6 };
	constexpr int twinsAmountPerWave[5] = { 2, 3, 4, 4, 6 };
	constexpr int asteroidAmountPerWave[5] = { 6, 3, 4, 5, 7 };

	for (int enemyIndex = 0; enemyIndex < beetleAmountPerWave[m_wave - 1]; enemyIndex++, m_enemiesAlive++)
	{
		SpawnRandomBeetle();
	}

	for (int enemyIndex = 0; enemyIndex < waspAmountPerWave[m_wave - 1]; enemyIndex++, m_enemiesAlive++)
	{
		SpawnRandomWasp();
	}

	for (int enemyIndex = 0; enemyIndex < timeWarperAmountPerWave[m_wave - 1]; enemyIndex++, m_enemiesAlive++)
	{
		SpawnRandomTimeWarper();
	}

	for (int enemyIndex = 0; enemyIndex < twinsAmountPerWave[m_wave - 1]; enemyIndex += 2, m_enemiesAlive += 2)
	{
		SpawnRandomTwins();
	}

	for (int asteroidIndex = 0; asteroidIndex < asteroidAmountPerWave[m_wave - 1]; asteroidIndex++)
	{
		SpawnRandomAsteroid();
	}
}

void Game::SpawnRandomTimeWarper() {
	for (int enemyIndex = 0; enemyIndex < MAX_ENEMIES; enemyIndex++)
	{
		Entity*& enemy = m_enemies[enemyIndex];
		if (!enemy) {
			Vec2 randPos = GetRandomSpawnPosition();
			m_enemies[enemyIndex] = new TimeWarper(this, randPos);
			return;
		}
	}

	ERROR_RECOVERABLE("CANNOT SPAWN MORE TIMEWARPERS; ALL SLOTS ARE FULL!");
}

void Game::SpawnRandomTwins()
{
	int twinOneSpot = -1;
	int twinTwoSpot = -1;
	for (int enemyIndex = 0; enemyIndex < MAX_ENEMIES; enemyIndex++)
	{
		Entity*& enemy = m_enemies[enemyIndex];
		if (!enemy) {
			if (twinOneSpot == -1) {
				twinOneSpot = enemyIndex;
			}
			else {
				twinTwoSpot = enemyIndex;
				break;
			}
		}
	}

	if (twinOneSpot == -1 || twinTwoSpot == -1) {
		ERROR_RECOVERABLE("CANNOT SPAWN MORE WASPS; ALL SLOTS ARE FULL!");
		return;
	}
	else {
		Vec2 randomLocation = GetRandomSpawnPosition();


		TwinAttacker* twinTwo = new TwinAttacker(this, randomLocation);
		TwinAttacker* twinOne = new TwinAttacker(this, randomLocation + Vec2(1.0f, 1.0f));
		twinOne->SetTwin(twinTwo);
		twinTwo->SetTwin(twinOne);

		m_enemies[twinOneSpot] = twinOne;
		m_enemies[twinTwoSpot] = twinTwo;
	}

}

void Game::SpawnRandomWasp() {
	for (int enemyIndex = 0; enemyIndex < MAX_ENEMIES; enemyIndex++)
	{
		Entity*& enemy = m_enemies[enemyIndex];
		if (!enemy) {
			Vec2 randPos = GetRandomSpawnPosition();
			m_enemies[enemyIndex] = new Wasp(this, randPos);
			return;
		}
	}

	ERROR_RECOVERABLE("CANNOT SPAWN MORE WASPS; ALL SLOTS ARE FULL!");

}

void Game::SpawnRandomBeetle() {
	for (int enemyIndex = 0; enemyIndex < MAX_ENEMIES; enemyIndex++)
	{
		Entity*& enemy = m_enemies[enemyIndex];
		if (!enemy) {
			Vec2 randPos = GetRandomSpawnPosition();
			m_enemies[enemyIndex] = new Beetle(this, randPos);
			return;
		}
	}

	ERROR_RECOVERABLE("CANNOT SPAWN MORE BEETLES; ALL SLOTS ARE FULL!");

}

void Game::SpawnPickUp()
{
	float randX = rng.GetRandomFloatInRange(SHIELD_PICKUP_RADIUS, WORLD_SIZE_X - SHIELD_PICKUP_RADIUS);
	float randY = rng.GetRandomFloatInRange(SHIELD_PICKUP_RADIUS, WORLD_SIZE_Y - SHIELD_PICKUP_RADIUS);

	for (int pickupIndex = 0; pickupIndex < MAX_PICKUPS; pickupIndex++) {
		PickUp*& pickup = m_pickUps[pickupIndex];
		if (!pickup) {
			pickup = new PickUp(this, Vec2(randX, randY));
			return;
		}
	}

	ERROR_RECOVERABLE("CANNOT SPAWN MORE PICKUPS");

}

void Game::ShakeScreenCollision()
{
	m_screenShakeDuration = SCREENSHAKE_DURATION;
	m_screenShakeTranslation = MAX_SCREENSHAKE_TRANSLATION;
	m_screenShake = true;
}

void Game::ShakeScreenDeath()
{
	m_screenShake = true;
	m_screenShakeDuration = SCREENSHAKE_DEATH_DURATION;
	m_screenShakeTranslation = MAX_SCREENSHAKE_TRANSLATION_DEATH;
}

void Game::ShakeScreenPlayerDeath()
{
	m_screenShake = true;
	m_screenShakeDuration = SCREENSHAKE_PLAYER_DEATH_DURATION;
	m_screenShakeTranslation = MAX_SCREENSHAKE_TRANSLATION_PLAYER_DEATH;
}

void Game::StopShakeScreen() {
	m_screenShake = false;
	m_screenShakeDuration = 0.0f;
	m_screenShakeTranslation = 0.0f;
}
