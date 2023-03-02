#pragma once
#include "Game/Gameplay/Hair.hpp"

class Game;
class Shader;

class HairSphere : public HairObject {
public:
	HairSphere(Game* pointerToGame, HairObjectInit const& initParams);
	~HairSphere();

	void CreateHair() override;
	void InitializeHairUAV() override;
	void Render() const override;
public:
	float m_radius = 0.0f;

	std::vector<Vertex_PNCU> m_sphereVertexes;
	VertexBuffer* m_sphereBuffer = nullptr;
	VertexBuffer* m_multInterpBuffer = nullptr;
	int m_amountOfMultInterpHair = 0;
};

class HairSphereTessellation : public HairSphere {
public:
	HairSphereTessellation(Game* pointerToGame, HairObjectInit const& initParams);
	~HairSphereTessellation();

public:
	void InitializeHairUAVMultInterp();
	void Update(float deltaSeconds) override;
	void Render() const override;
	int GetMultiStrandBaseCount() const override;
private:
	UnorderedAccessBuffer* m_hairInterpUAV = nullptr;
	UnorderedAccessBuffer* m_hairInterpPrevUAV = nullptr;
	UnorderedAccessBuffer* m_hairInterpVirtualUAV = nullptr;
};