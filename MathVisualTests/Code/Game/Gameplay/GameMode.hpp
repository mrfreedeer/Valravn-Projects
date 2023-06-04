#pragma once
#include "Game/Gameplay/Shape2D.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Renderer/DebugRendererSystem.hpp"
class Game;
struct Vec2;

class GameMode {
public:
	GameMode(Game* pointerToGame, Vec2 const& cameraSize, Vec2 const& UICameraSize);
	virtual ~GameMode();

	virtual void Startup() = 0;
	virtual void Shutdown() = 0;
	virtual void Update(float deltaSeconds);
	virtual void Render() const = 0;
	virtual void RenderUI() const;
	virtual void RenderTextAnimation() const;
	virtual Rgba8 const GetRandomColor() const;

	bool m_useTextAnimation = true;
	Rgba8 m_textAnimationColor = Rgba8::WHITE;

protected:
	virtual void UpdateInput(float deltaSeconds) = 0;
	virtual void UpdateInputFromKeyboard() = 0;
	virtual void UpdateInputFromController() = 0;
	virtual void UpdateTextAnimation(float deltaTime, std::string text, Vec2 const& textLocation, float textCellHeight);
	virtual void TransformText(Vec2 const& iBasis, Vec2 const& jBasis, Vec2 const& translation);
	virtual float GetTextWidthForAnim(float fullTextWidth);
	virtual Vec2 const GetIBasisForTextAnim();
	virtual Vec2 const GetJBasisForTextAnim();

	virtual void UpdateCameras(float deltaSeconds);
	virtual void UpdateWorldCamera(float deltaSeconds);
	virtual void UpdateUICamera(float deltaSeconds);


	virtual std::string GetCurrentGameStateAsText() const;
	virtual Vec2 const GetClampedRandomPositionInWorld() const;
	virtual Vec2 const GetClampedRandomPositionInWorld(float shapeRadius) const;
	virtual Vec2 const GetRandomPositionInWorld() const;
	

	Game* m_game = nullptr;
	Camera m_worldCamera;
	Camera m_UICamera;

	std::vector<Shape2D*> m_shapes;
	std::string m_currentText = "";
	Rgba8 m_startTextColor;
	Rgba8 m_endTextColor;
	bool m_transitionTextColor = false;
	Vec2 m_cameraSize = Vec2::ZERO;
	Vec2 m_UICameraSize = Vec2::ZERO;
	Vec2 m_UICameraCenter = Vec2::ZERO;
	Vec2 m_worldSize = Vec2::ZERO;

	std::vector	<Vertex_PCU> m_textAnimTriangles = std::vector<Vertex_PCU>();

	float m_textAnimationPosPercentageTop = g_gameConfigBlackboard.GetValue("TEXT_ANIMATION_POS_PERCENTAGE_TOP", 0.98f);
	float m_textCellHeight = g_gameConfigBlackboard.GetValue("TEXT_CELL_HEIGHT", 20.0f);
	float m_timeAlive = 0.0f;
	float m_timeElapsedTextAnimation = 0.0f;
	float m_textAnimationDuration = g_gameConfigBlackboard.GetValue("TEXT_ANIMATION_TIME", 4.0f);
	float m_textMovementPhaseTimePercentage = g_gameConfigBlackboard.GetValue("TEXT_MOVEMENT_PHASE_TIME_PERCENTAGE", 0.25f);
	float m_nearestPointRadius = 0.45f;

	bool m_screenShake = false;
	float m_screenShakeDuration = 0.0f;
	float m_screenShakeTranslation = 0.0f;
	float m_timeShakingScreen = 0.0f;

	std::string m_gameModeName = "";
	std::string m_helperText = "";
	Clock m_clock;
};