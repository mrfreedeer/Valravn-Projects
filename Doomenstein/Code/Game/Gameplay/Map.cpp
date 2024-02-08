#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Renderer/DebugRendererSystem.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Game/Gameplay/Game.hpp"
#include "Game/Gameplay/Map.hpp"
#include "Game/Gameplay/MapDefinition.hpp"
#include "Game/Gameplay/TileSetDefinition.hpp"
#include "Game/Gameplay/TileDefinition.hpp"
#include "Game/Gameplay/TileMaterialDefinition.hpp"
#include "Game/Gameplay/Actor.hpp"
#include "Game/Gameplay/ActorDefinition.hpp"
#include "Game/Gameplay/Player.hpp"
#include "Game/Gameplay/AI.hpp"

IntVec2 stepNorth(0, 1);
IntVec2 stepSouth(0, -1);
IntVec2 stepEast(1, 0);
IntVec2 stepWest(-1, 0);

IntVec2 stepNorthEast(1, 1);
IntVec2 stepSouthEast(1, -1);
IntVec2 stepNorthWest(-1, 1);
IntVec2 stepSouthWest(-1, -1);

Map::Map(Game* game, const MapDefinition* definition, std::vector<PlayerInfo> const& playersInfo) :
	m_game(game),
	m_definition(definition),
	m_dimensions(definition->m_image->GetDimensions()),
	m_solidMap(m_dimensions)
{
	m_spawnClock.SetParent(m_game->m_clock);
	CreateTiles();
	CreateGeometry();
	CreateBuffers();

	/*SpawnInfo actorOneSpawnInfo = {};
	actorOneSpawnInfo.m_position = Vec3(7.5f, 8.5f, 0.375f);
	actorOneSpawnInfo.m_orientation = EulerAngles::ZERO;
	actorOneSpawnInfo.m_definition = ActorDefinition::GetByName("Demon");

	SpawnInfo actorTwoSpawnInfo = {};
	actorTwoSpawnInfo.m_position = Vec3(8.5f, 8.5f, 0.25f);
	actorTwoSpawnInfo.m_orientation = EulerAngles::ZERO;
	actorTwoSpawnInfo.m_definition = ActorDefinition::GetByName("Demon");

	SpawnInfo actorThreeSpawnInfo = {};
	actorThreeSpawnInfo.m_position = Vec3(9.5f, 8.5f, 0.125f);
	actorThreeSpawnInfo.m_orientation = EulerAngles::ZERO;
	actorThreeSpawnInfo.m_definition = ActorDefinition::GetByName("Demon");

	Actor* actorOne = new Actor(this, actorOneSpawnInfo);
	Actor* actorTwo = new Actor(this, actorTwoSpawnInfo);
	Actor* actorThree = new Actor(this, actorThreeSpawnInfo);

	m_actors.push_back(actorOne);
	m_actors.push_back(actorTwo);
	m_actors.push_back(actorThree);*/
	m_actorSalt = 0;

	SpawnActorsInMap(playersInfo);

	if (m_definition->m_isHordeMode) {
		m_game->m_useTextAnimation = true;
		m_game->m_currentText = Stringf("Wave %d", m_currentWave + 1);
	}

	PopulateSolidHeatMap();

	//Light newLight = {};

	//newLight.Position = Vec3(5.0f, 10.0f, 0.9f);
	//DebugAddWorldPoint(newLight.Position, 0.1f, -1.0f, Rgba8::BLUE, Rgba8::BLUE, DebugRenderMode::ALWAYS);
	//Rgba8::BLUE.GetAsFloats(newLight.Color);
	//newLight.ConstantAttenuation = 0.1f;
	//newLight.LinearAttenuation = 0.2f;
	//newLight.QuadraticAttenuation = 0.5f;
	//newLight.Direction = Vec3(0.0f, 0.0f, -1.0f);
	//newLight.Enabled = true;

	//Light newLight2 = {};
	//newLight2.Position = Vec3(6.5f, 10.0f, 0.9f);
	//DebugAddWorldPoint(newLight2.Position, 0.1f, -1.0f, Rgba8::BLUE, Rgba8::BLUE, DebugRenderMode::ALWAYS);
	//Rgba8::RED.GetAsFloats(newLight2.Color);
	//newLight2.ConstantAttenuation = 0.1f;
	//newLight2.LinearAttenuation = 0.2f;
	//newLight2.QuadraticAttenuation = 0.5f;
	//newLight2.Direction = Vec3(0.0f, 0.0f, -1.0f);
	//newLight2.Enabled = true;

	//


	//g_theRenderer->SetLight(newLight, 0);
	//g_theRenderer->SetLight(newLight2, 1);
}

Map::~Map()
{
	delete m_vertexBuffer;
	m_vertexBuffer = nullptr;

	delete m_indexBuffer;
	m_indexBuffer = nullptr;

	for (int actorIndex = 0; actorIndex < m_actors.size(); actorIndex++) {
		Actor*& actor = m_actors[actorIndex];
		if (actor) {
			delete actor;
			actor = nullptr;
		}
	}

	for (int controllerIndex = 0; controllerIndex < m_playerControllers.size(); controllerIndex++) {
		Player*& controller = m_playerControllers[controllerIndex];
		if (controller) {
			delete controller;
			controller = nullptr;
		}
	}

	for (int AIControllerIndex = 0; AIControllerIndex < m_AIControllers.size(); AIControllerIndex++) {
		AI*& controller = m_AIControllers[AIControllerIndex];
		if (controller) {
			delete controller;
			controller = nullptr;
		}
	}

	for (int actorFactionIndex = 0; actorFactionIndex < (int)Faction::NUM_FACTIONS; actorFactionIndex++) {
		for (int actorIndex = 0; actorIndex < m_actorsByFaction[actorFactionIndex].size(); actorIndex++) {
			Actor*& actor = m_actorsByFaction[actorFactionIndex][actorIndex];
			actor = nullptr;
		}
	}
}

void Map::Update(float deltaSeconds)
{
	UpdatePlayerControllers(deltaSeconds);
	UpdateAIControllers(deltaSeconds);
	UpdateActors(deltaSeconds);
	UpdateActorsPhyiscs(deltaSeconds);

	CollideActors();
	CollideActorsWithMap();

	UpdatePlayerCameras();

	DeleteDestroyedActors();

	UpdateDeveloperCheatCodes();
	UpdateListeners();


	m_enemiesStillAlive = 0;
	for (int demonIndex = 0; demonIndex < m_actorsByFaction[(int)Faction::DEMON].size(); demonIndex++) {
		Actor* demon = m_actorsByFaction[(int)Faction::DEMON][demonIndex];
		if (IsActorAlive(demon)) {
			m_enemiesStillAlive++;
		}
	}

	if (m_definition->m_isHordeMode) {
		if (m_currentWave >= m_definition->m_wavesInfos.size()) {
			m_game->GoToVictoryScreen();
			return;
		}

		WaveInfo const& currentWaveInfo = m_definition->m_wavesInfos[m_currentWave];
		int const& currentAmountOfEnemies = currentWaveInfo.m_amountOfEnemies;

		if ((m_enemiesSpawned >= currentAmountOfEnemies) && !m_hasFinishedSpawningWave) {
			m_spawnClock.SetTimeDilation(0.0);
			m_waveStopwatch.Start(&m_game->m_clock, currentWaveInfo.m_timeBeforeNextWave);
			m_hasFinishedSpawningWave = true;
		}

		if ((m_waveStopwatch.HasDurationElapsed() || (m_enemiesStillAlive <= 0)) && m_hasFinishedSpawningWave) {
			m_enemiesSpawned = 0;
			m_spawnClock.SetTimeDilation(1.0);
			m_waveStopwatch.Stop();
			m_hasFinishedSpawningWave = false;
			m_currentWave++;
			m_game->m_useTextAnimation = true;
			m_game->m_currentText = Stringf("Wave %d", m_currentWave + 1);
		}
	}

}

void Map::UpdateActors(float deltaSeconds)
{
	for (int actorListIndex = 0; actorListIndex < (int)Faction::NUM_FACTIONS; actorListIndex++) {
		std::vector<Actor*>& actorList = m_actorsByFaction[actorListIndex];
		for (int actorIndex = 0; actorIndex < actorList.size(); actorIndex++) {
			Actor* actor = actorList[actorIndex];
			if (actor) {
				actor->Update(deltaSeconds);
			}
		}
	}
}

void Map::UpdateActorsPhyiscs(float deltaSeconds)
{
	for (int actorListIndex = 0; actorListIndex < (int)Faction::NUM_FACTIONS; actorListIndex++) {
		std::vector<Actor*>& actorList = m_actorsByFaction[actorListIndex];
		for (int actorIndex = 0; actorIndex < actorList.size(); actorIndex++) {
			Actor* actor = actorList[actorIndex];
			if (actor) {
				actor->UpdatePhysics(deltaSeconds);
			}
		}
	}
}

void Map::UpdateListeners()
{
	for (int playerIndex = 0; playerIndex < m_players.size(); playerIndex++) {

		Camera const* playerCamera = m_players[playerIndex]->m_worldCamera;

		Vec3 playerForward = Vec3::ZERO;
		Vec3 playerUp = Vec3::ZERO;
		Vec3 playerLeft = Vec3::ZERO;
		playerCamera->GetViewOrientation().GetVectors_XFwd_YLeft_ZUp(playerForward, playerLeft, playerUp);

		g_theAudio->UpdateListener(playerIndex, playerCamera->GetViewPosition(), Vec3::ZERO, playerForward, playerUp);
	}
}

void Map::UpdatePlayerCameras()
{
	for (int playerControllerIndex = 0; playerControllerIndex < m_playerControllers.size(); playerControllerIndex++) {
		Player* player = m_playerControllers[playerControllerIndex];
		player->UpdateCameras();
	}
}

void Map::Render(Camera const& camera, int playerIndex) const
{
	g_theRenderer->BindMaterial(m_material);
	g_theRenderer->BindTexture(m_texture);

	g_theRenderer->SetModelMatrix(Mat44());
	g_theRenderer->SetModelColor(Rgba8::WHITE);


	Vec3 directionalLight = m_directionalLightOrientation.GetXForward();
	g_theRenderer->SetDirectionalLight(directionalLight);
	g_theRenderer->SetAmbientIntensity(m_ambientLightIntensities[playerIndex]);
	g_theRenderer->SetDirectionalLightIntensity(m_directionalLightIntensities[playerIndex]);

	g_theRenderer->BindLightConstants();

	g_theRenderer->DrawIndexedVertexBuffer(m_vertexBuffer, m_indexBuffer, (int)m_indexes.size());

	for (int actorIndex = 0; actorIndex < m_actors.size(); actorIndex++) {
		Actor* actor = m_actors[actorIndex];
		if (actor) {
			actor->Render(camera);
		}
	}

}

void Map::CreateTiles()
{
	Image const* mapImage = m_definition->m_image;
	for (int yIndex = 0; yIndex < m_dimensions.y; yIndex++) {
		for (int xIndex = 0; xIndex < m_dimensions.x; xIndex++) {
			Rgba8 const pixelColor = mapImage->GetTexelColor(xIndex, yIndex);
			TileSetDefinition const* tileSetDef = m_definition->m_tileSetDefinition;
			TileDefinition const* tileDef = tileSetDef->GetTileDefinitionByColor(pixelColor);

			Vec3 boundsStart((float)xIndex, (float)yIndex, 0.0f);
			Vec3 boundsEnd(boundsStart);
			boundsEnd.x += 1.0f;
			boundsEnd.y += 1.0f;
			boundsEnd.z = 1.0f;

			AABB3 tileBounds(boundsStart, boundsEnd);
			IntVec2 coords(xIndex, yIndex);
			m_tiles.emplace_back(tileBounds, coords, tileDef);
		}
	}

}

void Map::CreateGeometry()
{
	for (int tileIndex = 0; tileIndex < m_tiles.size(); tileIndex++) {
		Tile const& tile = m_tiles[tileIndex];
		AddVertsForTile(tile, GetCoordsForTileIndex(tileIndex));
	}
}

void Map::CreateBuffers()
{
	BufferDesc vBufferDesc = {};
	vBufferDesc.data = nullptr;
	vBufferDesc.descriptorHeap = nullptr;
	vBufferDesc.memoryUsage = MemoryUsage::Dynamic;
	vBufferDesc.owner = g_theRenderer;
	vBufferDesc.size = sizeof(Vertex_PNCU);
	vBufferDesc.stride = sizeof(Vertex_PNCU);

	BufferDesc iBufferDesc = {};
	iBufferDesc.data = nullptr;
	iBufferDesc.descriptorHeap = nullptr;
	iBufferDesc.memoryUsage = MemoryUsage::Dynamic;
	iBufferDesc.owner = g_theRenderer;
	iBufferDesc.size = sizeof(unsigned int);
	iBufferDesc.stride = sizeof(unsigned int);

	m_vertexBuffer = new VertexBuffer(vBufferDesc);
	m_indexBuffer = new IndexBuffer(iBufferDesc);

	m_vertexBuffer->CopyCPUToGPU(m_vertexes.data(), m_vertexes.size() * m_vertexBuffer->GetStride());
	m_indexBuffer->CopyCPUToGPU(m_indexes.data(), m_indexes.size() * sizeof(unsigned int));
}

void Map::RaycastVsMap(Vec3 const& start, Vec3 const& direction, float distance, Player* playerToExclude) const
{
	Actor* actorToExclude = (playerToExclude) ? playerToExclude->GetActor() : nullptr;
	RaycastResultDoomenstein closestImpact = RaycastAll(start, direction, distance, { actorToExclude });

	if (closestImpact.m_didImpact) {
		DebugAddWorldPoint(closestImpact.m_impactPos, 0.06f, 10.0f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::USEDEPTH);
		DebugAddWorldArrow(closestImpact.m_impactPos, closestImpact.m_impactPos + closestImpact.m_impactNormal * 0.3f, 0.03f, 10.0f, Rgba8::BLUE, Rgba8::BLUE, Rgba8::BLUE, DebugRenderMode::USEDEPTH);
	}
}

void Map::CollideActors()
{
	for (int actorIndex = 0; actorIndex < m_actors.size(); actorIndex++) {
		Actor* actorA = m_actors[actorIndex];
		if (!IsActorAlive(actorA) || !actorA->CanCollideWithActors()) continue;
		for (int otherActorIndex = actorIndex + 1; otherActorIndex < m_actors.size(); otherActorIndex++) {
			Actor* actorB = m_actors[otherActorIndex];
			if (!IsActorAlive(actorB) || !actorA->CanCollideWithActors()) continue;

			if (actorA->m_definition->m_flying && actorB->m_definition->m_flying) continue;

			CollideActors(actorA, actorB);
		}
	}
}

void Map::CollideActors(Actor* actorA, Actor* actorB)
{
	if (actorA->m_owner == actorB) return;
	if (actorB->m_owner == actorA) return;


	Vec2 actorAXYCenter(actorA->m_position.x, actorA->m_position.y);
	Vec2 actorBXYCenter(actorB->m_position.x, actorB->m_position.y);

	if (DoDiscsOverlap(actorAXYCenter, actorA->GetPhysicsRadius(), actorBXYCenter, actorB->GetPhysicsRadius())) {
		PushDiscsOutOfEachOther2D(actorAXYCenter, actorA->GetPhysicsRadius(), actorBXYCenter, actorB->GetPhysicsRadius());
		actorA->m_position.x = actorAXYCenter.x;
		actorA->m_position.y = actorAXYCenter.y;

		actorB->m_position.x = actorBXYCenter.x;
		actorB->m_position.y = actorBXYCenter.y;

		actorA->OnCollide(actorB);
		actorB->OnCollide(actorA);
	}
}

void Map::SpawnActorsInMap(std::vector<PlayerInfo> const& playersInformation)
{
	std::vector<SpawnInfo>const& listOfSpawns = m_definition->m_spawnInfos;
	for (int spawnIndex = 0; spawnIndex < listOfSpawns.size(); spawnIndex++) {
		SpawnInfo const& spawnInfo = listOfSpawns[spawnIndex];
		Actor* createdActor = SpawnActor(spawnInfo);

		if (spawnInfo.m_definition->m_name == "SpawnPoint") {
			m_spawnPoints.push_back(createdActor);
		}
	}

	if (m_spawnPoints.size() > 0) {



		for (int playerIndex = 0; playerIndex < playersInformation.size(); playerIndex++) {

			PlayerInfo const& playerInfo = playersInformation[playerIndex];

			Actor const* randomSpawn = GetRandomSpawn();

			SpawnInfo marineSpawnInfo = {};
			marineSpawnInfo.m_definition = ActorDefinition::GetByName("Marine");
			marineSpawnInfo.m_position = randomSpawn->m_position;
			marineSpawnInfo.m_orientation = randomSpawn->m_orientation;

			Actor* spawnedMarine = SpawnActor(marineSpawnInfo);
			Player* playerController = new Player(this, &m_game->GetPlayerCamera(playerIndex), &m_game->GetPlayerUICamera(playerIndex), playerInfo.m_playerIndex, playerInfo.m_controller);
			m_playerControllers.push_back(playerController);

			playerController->Possess(spawnedMarine);

			m_players.push_back(playerController);
			m_ambientLightIntensities.push_back(m_defaultAmbientLightIntensity);
			m_directionalLightIntensities.push_back(m_defaultDirectionalLightIntensity);
		}

	}
}

RaycastResultDoomenstein Map::RaycastAll(Vec3 const& start, Vec3 const& direction, float distance, RaycastFilter const& filter) const
{
	RaycastResultDoomenstein raycastVsActorsResult = RaycastWorldActors(start, direction, distance, filter);
	RaycastResultDoomenstein raycastVsWorldXY = RaycastWorldXY(start, direction, distance);
	RaycastResultDoomenstein raycastVsWorldZ = RaycastWorldZ(start, direction, distance);

	RaycastResultDoomenstein closestImpact = {};
	if (raycastVsActorsResult.m_didImpact) {
		closestImpact = raycastVsActorsResult;
	}

	if (raycastVsWorldXY.m_didImpact) {
		if (closestImpact.m_didImpact) {
			closestImpact = (raycastVsWorldXY.m_impactDist < closestImpact.m_impactDist) ? raycastVsWorldXY : closestImpact;
		}
		else {
			closestImpact = raycastVsWorldXY;
		}
	}

	if (raycastVsWorldZ.m_didImpact) {
		if (closestImpact.m_didImpact) {
			closestImpact = (raycastVsWorldZ.m_impactDist < closestImpact.m_impactDist) ? raycastVsWorldZ : closestImpact;
		}
		else {
			closestImpact = raycastVsWorldZ;
		}
	}

	return closestImpact;
}

RaycastResultDoomenstein Map::RaycastWorldActors(Vec3 const& start, Vec3 const& direction, float distance, RaycastFilter const& filter) const
{
	RaycastResultDoomenstein closestImpact = {};
	closestImpact.m_impactDist = FLT_MAX;
	for (int actorIndex = 0; actorIndex < m_actors.size(); actorIndex++) {
		Actor* actor = m_actors[actorIndex];
		if (!actor || actor == filter.m_ignoreActor || actor->m_definition->m_flying) continue;
		RaycastResult3D baseRaycastVsActor = RaycastVsZCylinder(start, direction, distance, actor->m_position, actor->m_physicsRadius, actor->m_physicsHeight);
		RaycastResultDoomenstein raycastVsActor(baseRaycastVsActor);

		if (raycastVsActor.m_didImpact) {
			raycastVsActor.m_impactActor = actor;
			if (closestImpact.m_didImpact) {
				if (raycastVsActor.m_impactDist < closestImpact.m_impactDist) {
					closestImpact = raycastVsActor;
				}
			}
			else {
				closestImpact = raycastVsActor;
			}
		}
	}
	return closestImpact;
}

RaycastResultDoomenstein Map::RaycastWorldXY(Vec3 const& start, Vec3 const& direction, float distance) const
{
	RaycastResultDoomenstein raycastResult = {};
	raycastResult.m_forwardNormal = direction;
	raycastResult.m_startPosition = start;
	raycastResult.m_maxDistance = distance;

	Vec3 rayEnd = start + (direction * distance);
	Vec2 rayStart2D(start.x, start.y);
	Vec2 rayFwdXY = Vec2(direction.x, direction.y);
	rayFwdXY.Normalize();

	float maxXYDist = (Vec2(rayEnd.x, rayEnd.y) - Vec2(start.x, start.y)).GetLength();
	float rayHeight = rayEnd.z - start.z;
	IntVec2 tilePosition = GetCoordsForPosition(start);

	FloatRange allowedXBounds(0.99f, (float)m_dimensions.x - 1.0f);
	FloatRange allowedYBounds(0.99f, (float)m_dimensions.y - 1.0f);
	FloatRange allowedHeight(0.0f, 1.0f);

	Tile const* currTile = GetTileForCoords(tilePosition);

	if (allowedXBounds.IsOnRange(start.x) && allowedYBounds.IsOnRange(start.y) && allowedHeight.IsOnRange(start.z)) {
		if (currTile && currTile->IsSolid()) {
			raycastResult.m_didImpact = true;
			raycastResult.m_impactDist = 0.0f;
			raycastResult.m_impactPos = start;
			raycastResult.m_impactNormal = direction;
		}
	}


	int tileStepDirectionX = (direction.x > 0) ? 1 : -1;
	int tileStepDirectionY = (direction.y > 0) ? 1 : -1;

	float fwdDistPerXCrossing = (direction.x == 0) ? ARBITRARILY_LARGE_VALUE : (1.0f / fabsf(rayFwdXY.x));
	float fwdDistPerYCrossing = (direction.y == 0) ? ARBITRARILY_LARGE_VALUE : (1.0f / fabsf(rayFwdXY.y));

	float xAtFirstCrossing = tilePosition.x + static_cast<float>((tileStepDirectionX + 1) / 2);
	float xDistToFirstCrossing = xAtFirstCrossing - start.x;
	float yAtFirstCrossing = tilePosition.y + static_cast<float>((tileStepDirectionY + 1) / 2);
	float yDistToFirstCrossing = yAtFirstCrossing - start.y;

	float fwdDistAtNextXCrossing = fabsf(xDistToFirstCrossing) * fwdDistPerXCrossing;
	float fwdDistAtNextYCrossing = fabsf(yDistToFirstCrossing) * fwdDistPerYCrossing;

	while (!raycastResult.m_didImpact) {
		if ((fwdDistAtNextXCrossing > maxXYDist) && (fwdDistAtNextYCrossing > maxXYDist)) {
			raycastResult.m_maxDistanceReached = true;
			return raycastResult;
		}

		float usedDistanceAtNextCrossing = 0.0f;
		if (fwdDistAtNextXCrossing < fwdDistAtNextYCrossing) {
			tilePosition.x += tileStepDirectionX;
			usedDistanceAtNextCrossing = fwdDistAtNextXCrossing;
		}
		else {
			tilePosition.y += tileStepDirectionY;
			usedDistanceAtNextCrossing = fwdDistAtNextYCrossing;
		}

		Tile const* tile = GetTileForCoords(tilePosition);

		bool isTileSolid = (tile && tile->IsSolid());

		bool keepRaycastGoing = !isTileSolid;

		if (isTileSolid) {
			float impactZLength = (rayHeight * usedDistanceAtNextCrossing) / (maxXYDist);
			float zPos = start.z + impactZLength;
			float rayLengthAtHit = sqrtf((impactZLength * impactZLength) + (usedDistanceAtNextCrossing * usedDistanceAtNextCrossing));

			Vec2 impactPosXY = rayStart2D + (rayFwdXY * usedDistanceAtNextCrossing);
			Vec3 impactPos(impactPosXY.x, impactPosXY.y, zPos);
			if (allowedXBounds.IsOnRange(impactPos.x) && allowedYBounds.IsOnRange(impactPos.y) && allowedHeight.IsOnRange(impactPos.z)) {
				raycastResult.m_didImpact = true;
				raycastResult.m_impactDist = rayLengthAtHit;
				raycastResult.m_impactPos = start + direction * rayLengthAtHit;
				raycastResult.m_impactFraction = raycastResult.m_impactDist / distance;

				Vec3 impactNormal = Vec3(0.0f, 0.0f, 1.0f);
				if ((usedDistanceAtNextCrossing == fwdDistAtNextXCrossing) && (rayFwdXY.x > 0.0f)) impactNormal = Vec3(-1.0f, 0.0f, 0.0f);
				if ((usedDistanceAtNextCrossing == fwdDistAtNextXCrossing) && (rayFwdXY.x <= 0.0f)) impactNormal = Vec3(1.0f, 0.0f, 0.0f);
				if ((usedDistanceAtNextCrossing == fwdDistAtNextYCrossing) && (rayFwdXY.y > 0.0f)) impactNormal = Vec3(0.0f, -1.0f, 0.0f);
				if ((usedDistanceAtNextCrossing == fwdDistAtNextYCrossing) && (rayFwdXY.y <= 0.0f)) impactNormal = Vec3(0.0f, 1.0f, 0.0f);

				raycastResult.m_impactNormal = impactNormal;
				return raycastResult;
			}
			else {
				keepRaycastGoing = true;
			}
		}

		if (keepRaycastGoing) {
			if (usedDistanceAtNextCrossing == fwdDistAtNextXCrossing) {
				fwdDistAtNextXCrossing += fwdDistPerXCrossing;
			}
			else {
				fwdDistAtNextYCrossing += fwdDistPerYCrossing;
			}
		}
	}

	raycastResult.m_maxDistanceReached = true;

	return raycastResult;
}

RaycastResultDoomenstein Map::RaycastWorldZ(Vec3 const& start, Vec3 const& direction, float distance) const
{
	RaycastResultDoomenstein raycastResult = {};
	raycastResult.m_forwardNormal = direction;
	raycastResult.m_startPosition = start;
	raycastResult.m_maxDistance = distance;

	float distanceRayToEdge = (direction.z > 0) ? (1.0f - start.z) : (start.z);

	Vec3 end = start + (direction * distance);

	float rayHeight = fabsf(end.z - start.z);

	float rayLengthAtImpact = ((distanceRayToEdge * distance) / rayHeight);

	FloatRange mapBounds(1.0f, m_dimensions.x - 1.0f);

	Vec3 impactPos = start + (direction * rayLengthAtImpact);
	if (rayLengthAtImpact > distance) return raycastResult;
	if (mapBounds.IsOnRange(impactPos.x) && mapBounds.IsOnRange(impactPos.y)) {
		raycastResult.m_didImpact = true;
		raycastResult.m_forwardNormal = direction;
		raycastResult.m_impactNormal = (direction.z > 0.0f) ? Vec3(0.0, 0.0, -1.0f) : Vec3(0.0, 0.0, 1.0f);
		raycastResult.m_impactDist = rayLengthAtImpact;
		raycastResult.m_impactFraction = raycastResult.m_impactDist / distance;
		raycastResult.m_impactPos = impactPos;
		return raycastResult;
	}

	raycastResult.m_maxDistanceReached = true;
	return raycastResult;
}

void Map::AddVertsForTile(Tile const& tile, IntVec2 const& tileCoords)
{
	TileDefinition const* tileDef = tile.m_definition;
	TileMaterialDefinition const* tileCeilingDef = tileDef->m_ceilingMaterialDefinition;
	TileMaterialDefinition const* tileFloorDef = tileDef->m_floorMaterialDefinition;
	TileMaterialDefinition const* tileWallDef = tileDef->m_wallMaterialDefinition;

	if ((!m_material && !m_texture) && tileCeilingDef) {
		m_material = tileCeilingDef->m_material;
		m_texture = tileCeilingDef->m_texture;
	}

	Vec3 corners[8];
	tile.m_bounds.GetCorners(corners);

	Vec3 const& rightFrontBottom = corners[0];
	Vec3 const& rightBackBottom = corners[1];
	Vec3 const& rightBackTop = corners[2];
	Vec3 const& rightFrontTop = corners[3];
	Vec3 const& leftFrontBottom = corners[4];
	Vec3 const& leftBackBottom = corners[5];
	Vec3 const& leftBackTop = corners[6];
	Vec3 const& leftFrontTop = corners[7];


	if (tileWallDef) {
		AABB2 wallUVS = tileWallDef->m_uv;
		Tile const* northTile = GetTileForCoords(tileCoords + stepNorth);
		Tile const* southTile = GetTileForCoords(tileCoords + stepSouth);
		Tile const* eastTile = GetTileForCoords(tileCoords + stepEast);
		Tile const* westTile = GetTileForCoords(tileCoords + stepWest);

		if (northTile && northTile->HasFloor()) {
			AddVertsForIndexedQuad3D(m_vertexes, m_indexes, leftBackBottom, leftFrontBottom, leftFrontTop, leftBackTop, Rgba8::WHITE, wallUVS); // North
		}
		if (southTile && southTile->HasFloor()) {
			AddVertsForIndexedQuad3D(m_vertexes, m_indexes, rightFrontBottom, rightBackBottom, rightBackTop, rightFrontTop, Rgba8::WHITE, wallUVS); // South
		}
		if (eastTile && eastTile->HasFloor()) {
			AddVertsForIndexedQuad3D(m_vertexes, m_indexes, rightBackBottom, leftBackBottom, leftBackTop, rightBackTop, Rgba8::WHITE, wallUVS); // Right
		}
		if (westTile && westTile->HasFloor()) {
			AddVertsForIndexedQuad3D(m_vertexes, m_indexes, leftFrontBottom, rightFrontBottom, rightFrontTop, leftFrontTop, Rgba8::WHITE, wallUVS); // Left
		}
	}

	if (tileCeilingDef) {
		AddVertsForIndexedQuad3D(m_vertexes, m_indexes, leftBackTop, rightBackTop, rightFrontTop, leftFrontTop, Rgba8::WHITE, tileCeilingDef->m_uv); // Top
	}

	if (tileFloorDef) {
		AddVertsForIndexedQuad3D(m_vertexes, m_indexes, leftFrontBottom, rightFrontBottom, rightBackBottom, leftBackBottom, Rgba8::WHITE, tileFloorDef->m_uv); // Bottom;
	}
}

Vec3 const Map::GetPositionForTileCoords(IntVec2 const& tileCoords) const
{

	return Vec3((float)tileCoords.x + 0.5f, (float)tileCoords.y + 0.5f, 0.0f);
}


IntVec2 const Map::GetCoordsForTileIndex(int index) const
{
	int y = index / m_dimensions.x;
	int x = index - y * m_dimensions.x;
	return IntVec2(x, y);
}

IntVec2 const Map::GetCoordsForPosition(Vec3 const& position) const
{
	int x = RoundDownToInt(position.x);
	int y = RoundDownToInt(position.y);
	return IntVec2(x, y);
}

Tile const* Map::GetTileForCoords(int x, int y) const
{
	if (x < 0 || y < 0 || x > m_dimensions.x || y > m_dimensions.y) return nullptr;
	int tileIndex = y * m_dimensions.x + x;
	if (tileIndex >= (int)m_tiles.size()) return nullptr;

	return &m_tiles[tileIndex];
}

Tile const* Map::GetTileForCoords(IntVec2 const& coords) const
{
	return GetTileForCoords(coords.x, coords.y);
}

Actor* Map::SpawnActor(const SpawnInfo& spawnInfo)
{
	int actorIndex = 0;

	bool foundIndex = false;
	while ((actorIndex < m_actors.size()) && (!foundIndex)) {
		Actor* actor = m_actors[actorIndex];
		if (!actor) {
			foundIndex = true;
		}
		else {
			actorIndex++;
		}
	}

	std::vector<Actor*>& actorFactionList = m_actorsByFaction[(int)spawnInfo.m_definition->m_faction];

	Actor* newActor = nullptr;
	if (foundIndex) {
		newActor = new Actor(this, spawnInfo);
		newActor->m_uid = ActorUID(actorIndex, m_actorSalt);
		m_actors[actorIndex] = newActor;
	}
	else {
		if (m_actors.size() < 0x0000ffff) { // four bits of index are used, so it vector size can't reach 65535
			newActor = new Actor(this, spawnInfo);
			newActor->m_uid = ActorUID(actorIndex, m_actorSalt);
			m_actors.push_back(newActor);

		}
		else {
			ERROR_AND_DIE("NO MORE SPACE FOR NEW ACTORS");
		}
	}

	m_actorSalt++;

	if (m_actorSalt == 0x0000fffe) { // four bits of index are used, so it should wrap around at 65535 - 1, this is before is invalid salt. 
		m_actorSalt = 0;
	}

	actorFactionList.push_back(newActor);

	if (newActor->m_definition->m_aiEnabled) {
		AI* aiController = new AI(this);
		aiController->Possess(newActor);
		m_AIControllers.push_back(aiController);
	}

	return newActor;
}

void Map::CollideActorsWithMap()
{
	for (int actorIndex = 0; actorIndex < m_actors.size(); actorIndex++) {
		Actor* actor = m_actors[actorIndex];
		if (!actor || !actor->CanCollideWithWorld()) continue;
		//#ToDo check for projectile death within collide function
		CollideActorWithMap(*actor);
	}
}

void Map::CollideActorWithMap(Actor& actor)
{
	IntVec2 tileCoord = GetCoordsForPosition(actor.m_position);

	// First cardinal points to avoid toe stubbing
	PushActorOutOfWall(actor, tileCoord + stepNorth);
	PushActorOutOfWall(actor, tileCoord + stepSouth);
	PushActorOutOfWall(actor, tileCoord + stepEast);
	PushActorOutOfWall(actor, tileCoord + stepWest);

	PushActorOutOfWall(actor, tileCoord + stepNorthEast);
	PushActorOutOfWall(actor, tileCoord + stepSouthEast);
	PushActorOutOfWall(actor, tileCoord + stepNorthWest);
	PushActorOutOfWall(actor, tileCoord + stepSouthWest);

	if (actor.m_definition->m_flying) {

		if (actor.m_position.z + actor.m_physicsHeight >= 1.0f) {
			actor.m_position.z = 1.0f - actor.m_physicsHeight;
			actor.OnCollide(nullptr);
		}

		if (actor.m_position.z < 0.0f) {
			actor.m_position.z = 0.0f;
			actor.OnCollide(nullptr);
		}
	}
}

void Map::PushActorOutOfWall(Actor& actor, IntVec2 const& tileCoords)
{
	if (tileCoords.x < 0 || tileCoords.y < 0) return;
	Tile const* tile = GetTileForCoords(tileCoords);
	if (!tile) return;
	bool isNotSolid = !tile->IsSolid();

	if (isNotSolid) return;
	AABB2 tileAABB2 = tile->m_bounds.GetXYBounds();
	Vec2 actorXYPos(actor.m_position.x, actor.m_position.y);

	bool pushed = PushDiscOutOfAABB2D(actorXYPos, actor.m_physicsRadius, tileAABB2);

	if (pushed && actor.m_definition->m_flying) {
		actor.OnCollide(nullptr);
	}

	actor.m_position.x = actorXYPos.x;
	actor.m_position.y = actorXYPos.y;

}

void Map::RemoveFromLists(Actor& deletedActor)
{
	int factionIndex = (int)deletedActor.m_definition->m_faction;
	std::vector<Actor*> actorList = m_actorsByFaction[factionIndex];
	for (int actorIndex = 0; actorIndex < actorList.size(); actorIndex++) {
		Actor*& actor = m_actorsByFaction[factionIndex][actorIndex];
		if (actor && actor->m_uid == deletedActor.m_uid) {
			actor = nullptr;
			return;
		}
	}
}

void Map::UpdateDeveloperCheatCodes()
{
	if (g_theInput->WasKeyJustPressed(KEYCODE_F1)) {
		float currentAmbientLightIntesity[4];
		float currentDirectionalLightIntesity[4];
		m_ambientLightIntensities[0].GetAsFloats(currentAmbientLightIntesity);
		m_directionalLightIntensities[0].GetAsFloats(currentDirectionalLightIntesity);


		std::string lightingInfo = Stringf("Ambient: %.2f Directional: %.2f Pitch: %.1f", currentAmbientLightIntesity[0], currentDirectionalLightIntesity[0], m_directionalLightOrientation.m_pitchDegrees);
		DebugAddMessage(lightingInfo, 5.0f, Rgba8::WHITE, Rgba8::LIGHTBLUE);
	}

	float messageDuration = 0.5f;

	float changeAmount =  0.01f;
	if (g_theInput->IsKeyDown(KEYCODE_F2)) {
		float currentAmbientLightIntesity[4];
		m_ambientLightIntensities[0].GetAsFloats(currentAmbientLightIntesity);
		float newIntensity = currentAmbientLightIntesity[0] - changeAmount;
		newIntensity = ClampZeroToOne(newIntensity);
		m_ambientLightIntensities[0] = Rgba8(newIntensity);

		std::string lightingInfo = Stringf("Ambient: %.2f", newIntensity);
		DebugAddMessage(lightingInfo, messageDuration, Rgba8::WHITE, Rgba8::LIGHTBLUE);
	}

	if (g_theInput->IsKeyDown(KEYCODE_F3)) {
		float currentAmbientLightIntesity[4];
		m_ambientLightIntensities[0].GetAsFloats(currentAmbientLightIntesity);
		float newIntensity = currentAmbientLightIntesity[0] + changeAmount;
		newIntensity = ClampZeroToOne(newIntensity);
		m_ambientLightIntensities[0] = Rgba8(newIntensity);

		std::string lightingInfo = Stringf("Ambient: %.2f", newIntensity);
		DebugAddMessage(lightingInfo, messageDuration, Rgba8::WHITE, Rgba8::LIGHTBLUE);
	}

	if (g_theInput->IsKeyDown(KEYCODE_F4)) {
		float currentDirecitonalLightIntensity[4];
		m_directionalLightIntensities[0].GetAsFloats(currentDirecitonalLightIntensity);
		float newIntensity = currentDirecitonalLightIntensity[0] - changeAmount;
		newIntensity = ClampZeroToOne(newIntensity);
		m_directionalLightIntensities[0] = Rgba8(newIntensity);

		std::string lightingInfo = Stringf("Directional: %.2f", newIntensity);
		DebugAddMessage(lightingInfo, messageDuration, Rgba8::WHITE, Rgba8::LIGHTBLUE);
	}

	if (g_theInput->IsKeyDown(KEYCODE_F5)) {
		float currentDirecitonalLightIntensity[4];
		m_directionalLightIntensities[0].GetAsFloats(currentDirecitonalLightIntensity);
		float newIntensity = currentDirecitonalLightIntensity[0] + changeAmount;
		newIntensity = ClampZeroToOne(newIntensity);
		m_directionalLightIntensities[0] = Rgba8(newIntensity);

		std::string lightingInfo = Stringf("Directional: %.2f", newIntensity);
		DebugAddMessage(lightingInfo, messageDuration, Rgba8::WHITE, Rgba8::LIGHTBLUE);
	}

	if (g_theInput->IsKeyDown(KEYCODE_F6)) {
		float& currentPitch = m_directionalLightOrientation.m_pitchDegrees;
		currentPitch -= 1.0f;

		currentPitch = Clamp(currentPitch, 0.0f, 180.0f);
		DebugAddMessage(Stringf("Pitch: %.2f", currentPitch), messageDuration, Rgba8::WHITE, Rgba8::BLUE);
	}

	if (g_theInput->IsKeyDown(KEYCODE_F7)) {
		float& currentPitch = m_directionalLightOrientation.m_pitchDegrees;
		currentPitch += 1.0f;

		currentPitch = Clamp(currentPitch, 0.0f, 180.0f);
		DebugAddMessage(Stringf("Pitch: %.2f", currentPitch), messageDuration, Rgba8::WHITE, Rgba8::BLUE);
	}
}

void Map::DeleteDestroyedActors()
{
	for (int actorIndex = 0; actorIndex < m_actors.size(); actorIndex++) {
		Actor*& actor = m_actors[actorIndex];
		if (!actor) continue;
		if (actor->m_isPendingDelete) {
			RemoveFromLists(*actor);
			delete actor;
			actor = nullptr;
		}
	}
}

void Map::DestroyActor(const ActorUID uid)
{
	Actor* actor = GetActorByUID(uid);

	if (!actor) return;

	actor->m_isPendingDelete = true;
}

Actor* Map::GetActorByUID(const ActorUID uid) const
{
	bool isActorUIDValid = uid.IsValid();
	if (!isActorUIDValid) return nullptr;

	int index = uid.GetIndex();
	Actor* actor = m_actors[index];
	if (!actor) return nullptr;

	if (actor->m_uid != uid) return nullptr;

	return actor;
}

Actor* Map::GetClosestVisibleEnemy(Actor* actor)
{
	if (!actor) return nullptr;
	Actor* closestActor = nullptr;
	float closestDist = FLT_MAX;

	std::vector<Actor*> enemyList = (actor->m_definition->m_faction == Faction::MARINE) ? m_actorsByFaction[(int)Faction::DEMON] : m_actorsByFaction[(int)Faction::MARINE];

	for (int actorIndex = 0; actorIndex < enemyList.size(); actorIndex++) {
		Actor* otherActor = enemyList[actorIndex];
		if (!otherActor) continue;
		Vec3 forward = actor->GetForward();
		Vec2 forwardXY(forward.x, forward.y);

		Vec3 dispToOtherActor = otherActor->m_position - actor->m_position;
		Vec2 dispToOtherActorXY(dispToOtherActor.x, dispToOtherActor.y);

		if (GetAngleDegreesBetweenVectors2D(forwardXY, dispToOtherActorXY) > (actor->m_definition->m_sightAngle * 0.5f)) continue;

		Vec3 direction = dispToOtherActor.GetNormalized();

		float distToOtherActor = GetDistanceSquared3D(actor->m_position, otherActor->m_position);
		if (distToOtherActor >= (closestDist * closestDist)) continue;
		RaycastResultDoomenstein raycastVsActor = RaycastWorldXY(actor->m_position, direction, actor->m_definition->m_sightRadius);
		float impactDistSqr = raycastVsActor.m_impactDist * raycastVsActor.m_impactDist;

		if ((!raycastVsActor.m_didImpact) || (distToOtherActor < impactDistSqr)) {
			if (raycastVsActor.m_impactDist < closestDist) {
				closestDist = raycastVsActor.m_impactDist;
				closestActor = otherActor;
			}

		}
	}

	return closestActor;
}

std::vector<Actor*> Map::GetActorsWithinRadius(Actor const* actor, float radius) const
{
	float sqrRadius = radius * radius;
	Vec3 const& actorPos = actor->m_position;
	std::vector<Actor*> actorList;
	actorList.reserve(m_actors.size());
	for (int actorIndex = 0; actorIndex < m_actors.size(); actorIndex++) {
		Actor* otherActor = m_actors[actorIndex];
		if (actor == otherActor) continue;
		if (!otherActor) continue;
		float distanceToActor = GetDistanceSquared3D(actorPos, otherActor->m_position);
		if (distanceToActor <= sqrRadius) {
			actorList.push_back(otherActor);
		}
	}

	return actorList;
}

Player* Map::GetPlayer(int index) const
{
	if (index > m_players.size()) return nullptr;
	return m_players[index];
}

Player const* Map::GetConstPlayer(int index) const
{
	if (index > m_players.size()) return nullptr;
	return m_players[index];
}


Player* Map::GetPlayerWithKeyboardInput() const
{
	for (int playerIndex = 0; playerIndex < m_players.size(); playerIndex++) {
		Player* player = m_players[playerIndex];
		if (player->m_controllerIndex == -1) {
			return player;
		}
	}

	return nullptr;
}

Game* Map::GetGame()
{
	return m_game;
}

Actor* Map::GetNextPossessableActor(Actor* currentlyPossessedActor) const
{

	int currentIndex = (currentlyPossessedActor) ? currentlyPossessedActor->m_uid.GetIndex() : 0;

	int nextActorIndex = currentIndex + 1;
	if (nextActorIndex >= m_actors.size()) nextActorIndex = 0;

	while (currentIndex != nextActorIndex) {
		Actor* nextActor = m_actors[nextActorIndex];
		if (nextActor && nextActor->m_definition->m_canBePossessed) {
			return nextActor;
		}

		nextActorIndex++;
		if (nextActorIndex >= m_actors.size()) nextActorIndex = 0;
	}

	return nullptr;
}

bool Map::IsActorAlive(Actor* actor) const
{
	return actor && !actor->m_isDead;
}

Actor const* Map::GetRandomSpawn() const
{
	std::vector<bool> takenSpawns;

	for (int spawnIndex = 0; spawnIndex < m_spawnPoints.size(); spawnIndex++) {
		takenSpawns.push_back(false);
	}

	int randomSpawnInd = -1;
	for (int attemptIndex = 0; attemptIndex < 10; attemptIndex++) {
		if (randomSpawnInd != -1) continue;
		randomSpawnInd = rng.GetRandomIntInRange(0, (int)m_spawnPoints.size() - 1);
		if (takenSpawns[randomSpawnInd]) {
			randomSpawnInd = -1;
		}
	}

	if (randomSpawnInd == -1) {
		ERROR_AND_DIE("CANNOT FIND PLACE TO SPAWN PLAYER!");
	}

	Actor* randomSpawn = m_spawnPoints[randomSpawnInd];
	return randomSpawn;
}

void Map::KillAllDemons() const
{
	for (int demonIndex = 0; demonIndex < m_actorsByFaction[(int)Faction::DEMON].size(); demonIndex++) {
		Actor* demon = m_actorsByFaction[(int)Faction::DEMON][demonIndex];
		if (demon) {
			demon->Die();
		}
	}
}

void Map::SetDirectionalLightIntensity(Rgba8 const& newIntensity, int playerIndex)
{
	m_directionalLightIntensities[playerIndex] = newIntensity;
}

void Map::SetAmbientLightIntensity(Rgba8 const& newIntensity, int playerIndex)
{
	m_ambientLightIntensities[playerIndex] = newIntensity;
}

bool Map::CanEnemyBeSpawned() const
{
	return !m_hasFinishedSpawningWave && (m_enemiesSpawned < m_definition->m_wavesInfos[m_currentWave].m_amountOfEnemies);
}

bool Map::IsHordeMode() const
{
	return m_definition->m_isHordeMode;
}

float Map::GetRemainingWaveTime() const
{
	float timeLeft = 1.0f - m_waveStopwatch.GetElapsedFraction();
	return timeLeft * (float)m_waveStopwatch.m_duration;
}

float Map::GetTotalWaveTime() const
{
	return (float)m_waveStopwatch.m_duration;
}

void Map::PopulateSolidHeatMap()
{
	for (int tileIndex = 0; tileIndex < m_tiles.size(); tileIndex++) {
		bool isSolid = m_tiles[tileIndex].IsSolid();
		if (isSolid) {
			m_solidMap.SetValue(tileIndex, 1.0f);
		}
	}
}

void Map::PopulateDistanceField(TileHeatMap& out_distanceField, IntVec2 const& referenceCoords, float maxCost)
{
	out_distanceField.SetAllValues(maxCost);
	out_distanceField.SetValue(referenceCoords, 0.0f);

	float currTileValue = 0.0f;
	bool modifiedTile = true;
	while (modifiedTile) {

		modifiedTile = false;
		for (int tileIndex = 0; tileIndex < m_tiles.size(); tileIndex++) {
			if (m_tiles[tileIndex].IsSolid()) continue;

			float tileHeatValue = out_distanceField.GetValue(tileIndex);
			if (tileHeatValue == currTileValue) {

				IntVec2 const& currTileCoords = m_tiles[tileIndex].m_coords;

				bool modifiedNorth = SetTileHeatMapValue(out_distanceField, currTileValue, currTileCoords + stepNorth);
				bool modifiedSouth = SetTileHeatMapValue(out_distanceField, currTileValue, currTileCoords + stepSouth);
				bool modifiedEast = SetTileHeatMapValue(out_distanceField, currTileValue, currTileCoords + stepEast);
				bool modifiedWest = SetTileHeatMapValue(out_distanceField, currTileValue, currTileCoords + stepWest);

				modifiedTile = modifiedTile || modifiedNorth || modifiedSouth || modifiedEast || modifiedWest;
			}

		}
		currTileValue++;
	}
}

bool Map::SetTileHeatMapValue(TileHeatMap& out_distanceField, float currentValue, IntVec2 const& tileCoords)
{
	Tile const* tile = GetTileForCoords(tileCoords);
	bool isTileConsideredSolid = tile->IsSolid();

	if (!isTileConsideredSolid && (out_distanceField.GetValue(tileCoords) > currentValue + 1.0f)) {
		out_distanceField.SetValue(tileCoords, currentValue + 1.0f);
		return true;
	}

	return false;
}

void Map::UpdatePlayerControllers(float deltaSeconds)
{
	for (int playerControllerIndex = 0; playerControllerIndex < m_playerControllers.size(); playerControllerIndex++) {
		Player* player = m_playerControllers[playerControllerIndex];
		player->Update(deltaSeconds);

	}
}

void Map::UpdateAIControllers(float deltaSeconds)
{
	for (int AIControllerIndex = 0; AIControllerIndex < m_AIControllers.size(); AIControllerIndex++) {
		AI* ai = m_AIControllers[AIControllerIndex];
		ai->Update(deltaSeconds);

	}
}
