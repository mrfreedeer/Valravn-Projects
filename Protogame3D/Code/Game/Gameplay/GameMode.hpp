#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Game/Gameplay/Entity.hpp"
#include "Game/Gameplay/Player.hpp"

class Game;

class GameMode {
public:
	GameMode(Game* game, Vec2 const& UISize) : m_game(game), m_UISize(UISize) { m_UICamera.SetOrthoView(Vec2::ZERO, UISize); }
	virtual ~GameMode();

	virtual void Startup();
	virtual void Update(float deltaSeconds);
	virtual void Render() const;
	virtual void Shutdown();

protected:
	virtual void UpdateDeveloperCheatCodes(float deltaSeconds);
	virtual void UpdateInput(float deltaSeconds) = 0;
	Rgba8 const GetRandomColor() const;
	void CheckIfWindowHasFocus();

	virtual void UpdateEntities(float deltaSeconds);
	virtual void RenderEntities() const;
	virtual void RenderUI() const;
protected:
	Game* m_game = nullptr;
	Camera m_worldCamera;
	Camera m_UICamera;
	Vec2 m_UISize = {};
	Vec2 m_worldSize = {};

	float m_timeAlive = 0.0f;
	Clock m_clock;

	EntityList m_allEntities;

	Player* m_player = nullptr;

	bool m_isCursorHidden = false;
	bool m_isCursorClipped = false;
	bool m_isCursorRelative = false;

	bool m_lostFocusBefore = false;

	float m_textCellHeight = g_gameConfigBlackboard.GetValue("TEXT_CELL_HEIGHT", 20.0f);


};