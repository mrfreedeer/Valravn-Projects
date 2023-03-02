#pragma once
#include "Game/Gameplay/Entity.hpp"
#include "Game/Gameplay/Hair.hpp"

class VertexBuffer;
class UnorderedAccessBuffer;
class Shader;

class HairDisc : public HairObject {
public:
	HairDisc(Game* pointerToGame, HairObjectInit const& initParams);
	~HairDisc();
	void CreateHair() override;
	void InitializeHairUAV();

public:
	

};

class HairDiscTessellation : public HairDisc {
public:
	HairDiscTessellation(Game* pointerToGame, HairObjectInit const& initParams);
	~HairDiscTessellation();
	void Render() const override;
};