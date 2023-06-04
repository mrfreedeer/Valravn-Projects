#pragma once

#include "Game/Gameplay/GameMode.hpp"

class NearestPointGameMode : public GameMode {
public:
	NearestPointGameMode(Game* pointerToGame, Vec2 const& cameraSize, Vec2 const& UICameraSize, Vec2 const& worldSize);
	~NearestPointGameMode();

	virtual void Startup() override;
	virtual void Shutdown() override;
	virtual void Update(float deltaSeconds) override;
	virtual void Render() const override;


private:
	virtual void UpdateInput(float deltaSeconds) override;
	void UpdateShapesColors();
	void UpdateNearestPointsOnShapes();

	virtual void UpdateInputFromKeyboard();
	virtual void UpdateInputFromController();

	void RenderNormalColorShapes() const;
	void RenderHighlightedColorShapes() const;

	std::vector<Shape2D*> m_allShapes;
	std::vector<Shape2D*> m_highlightedShapes;
	std::vector<Shape2D*> m_normalColorShapes;
	LineSegment2 m_lineSegment;
	LineSegment2 m_infiniteLine;

	Vec2 m_cursorDiscPos = Vec2(g_gameConfigBlackboard.GetValue("WORLD_CENTER_X", 100.0f), g_gameConfigBlackboard.GetValue("WORLD_CENTER_Y", 50.0f));
	float m_cursorDiscRadius = 0.3f;
	float m_dotSpeed = g_gameConfigBlackboard.GetValue("DOT_SPEED", 40.0f);
	
	std::vector<Vec2> m_nearestPointsOnShapes;
};




