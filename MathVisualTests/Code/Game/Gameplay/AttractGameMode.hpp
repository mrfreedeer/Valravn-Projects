#include "Game/Gameplay/GameMode.hpp"


struct Vec2;
class AttractGameMode : public GameMode {
public:
	AttractGameMode(Game* pointerToGame, Vec2 const& cameraSize, Vec2 const& UICameraSize);

	virtual void Startup() override;
	virtual void Shutdown() override;

	virtual void Update(float deltaSeconds);
	virtual void Render() const override;

protected:
	virtual void UpdateInput(float deltaSeconds) override;
	virtual void UpdateInputFromKeyboard();
	virtual void UpdateInputFromController();
	float m_textCellHeightAttractScreen = g_gameConfigBlackboard.GetValue("TEXT_CELL_HEIGHT_ATTRACT_SCREEN", 40.0f);

};