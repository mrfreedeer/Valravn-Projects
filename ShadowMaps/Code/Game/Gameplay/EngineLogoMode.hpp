#pragma once
#include "Game/Gameplay/GameMode.hpp"

class EngineLogoMode : public GameMode {
public:
	EngineLogoMode(Game* pointerToGame, Vec2 const& UICameraSize);

	virtual void Startup() override;
	virtual void Shutdown() override;

	virtual void Update(float deltaSeconds);
	virtual void Render() const override;

protected:
	virtual void UpdateInput(float deltaSeconds) override;
	virtual void UpdateInputFromKeyboard();
	virtual void UpdateInputFromController();
	virtual bool IsEntityMovable(Entity*) const override { return false; }
	virtual void SaveScene(std::string const& fileName) const {UNUSED(fileName)};

	float m_textCellHeightAttractScreen = g_gameConfigBlackboard.GetValue("TEXT_CELL_HEIGHT_ATTRACT_SCREEN", 40.0f);
	Texture* m_logoTexture = nullptr;
	double m_timeShowingLogo = 0.0;
	double m_engineLogoLength = g_gameConfigBlackboard.GetValue("ENGINE_LOGO_LENGTH", 2.0);
};