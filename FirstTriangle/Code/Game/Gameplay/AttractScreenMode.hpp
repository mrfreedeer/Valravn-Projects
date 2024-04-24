#pragma once

#include "Game/Gameplay/GameMode.hpp"


class AttractScreenMode : public GameMode {
public:
	AttractScreenMode(Game* game, Vec2 UISize) : GameMode(game, UISize) {m_UICenter = UISize * 0.5f; }
	~AttractScreenMode();

	virtual void Startup() override;
	virtual void Update(float deltaSeconds) override;
	virtual void Render() const override;
protected:
	virtual void UpdateInput(float deltaSeconds);


private:
	std::string m_currentText = "";
	std::vector	<Vertex_PCU> m_textAnimTriangles = std::vector<Vertex_PCU>();
	Rgba8 m_textAnimationColor = Rgba8(255, 255, 255, 255);
	Rgba8 m_startTextColor;
	Rgba8 m_endTextColor;
	bool m_useTextAnimation = true;
	bool m_transitionTextColor = false;
	Vec2 m_UICenter = {};
	float m_textAnimationPosPercentageTop = g_gameConfigBlackboard.GetValue("TEXT_ANIMATION_POS_PERCENTAGE_TOP", 0.98f);
	float m_textCellHeightAttractScreen = g_gameConfigBlackboard.GetValue("TEXT_CELL_HEIGHT_ATTRACT_SCREEN", 40.0f);
	float m_textAnimationTime = g_gameConfigBlackboard.GetValue("TEXT_ANIMATION_TIME", 4.0f);
	float m_textMovementPhaseTimePercentage = g_gameConfigBlackboard.GetValue("TEXT_MOVEMENT_PHASE_TIME_PERCENTAGE", 0.25f);
	float m_timeTextAnimation = 0.0f;
};