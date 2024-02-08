#pragma once
#include "Game/Gameplay/GameMode.hpp"

class BufferWriter;
class ConvexPoly3DShape;
class BufferParser;

class ModelsMode : public GameMode {
public:
	ModelsMode(Game* pointerToGame, Vec2 const& UICameraSize);
	~ModelsMode();

	virtual void Startup() override;
	virtual void Shutdown() override;

	virtual void Update(float deltaSeconds);
	virtual void Render() const override;

	bool SaveSceneToFile(EventArgs& args);
	bool LoadSceneFromFile(EventArgs& args);

protected:
	virtual Vec3 GetPlayerPosition() const override;
	virtual void UpdateInput(float deltaSeconds) override;
	virtual void UpdateInputFromKeyboard();
	virtual void UpdateInputFromController();
	virtual bool IsEntityMovable(Entity* entity) const override;


	float m_textCellHeightAttractScreen = g_gameConfigBlackboard.GetValue("TEXT_CELL_HEIGHT_ATTRACT_SCREEN", 40.0f);
	int m_selectedModelInd = 0;


};