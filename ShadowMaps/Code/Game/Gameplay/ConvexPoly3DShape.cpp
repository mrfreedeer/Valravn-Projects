#include "Game/Gameplay/ConvexPoly3DShape.hpp"
#include "Game/Framework/GameCommon.hpp"

Rgba8 const GetRandomColor()
{
	unsigned char randomR = static_cast<unsigned char>(rng.GetRandomIntInRange(128, 255));
	unsigned char randomG = static_cast<unsigned char>(rng.GetRandomIntInRange(128, 255));
	unsigned char randomB = static_cast<unsigned char>(rng.GetRandomIntInRange(128, 255));
	unsigned char randomA = static_cast<unsigned char>(rng.GetRandomIntInRange(190, 255));
	return Rgba8(randomR, randomG, randomB, randomA);
}

ConvexPoly3DShape::ConvexPoly3DShape(Game* game, Vec3 position, ConvexPoly3D const& convexPoly):
	m_convexPoly(convexPoly),
	Entity(game, position)
{
	RandomizeFaceColors();
}

void ConvexPoly3DShape::Update(float deltaSeconds)
{
	UNUSED(deltaSeconds);
}

void ConvexPoly3DShape::Render() const
{
	std::vector<Vertex_PCU> polyVerts;

	g_theRenderer->SetModelMatrix(GetModelMatrix());

	m_convexPoly.AddVertsForPoly(polyVerts, m_faceColors);

	g_theRenderer->DrawVertexArray(polyVerts);
}

void ConvexPoly3DShape::RenderDiffuse() const
{
	std::vector<Vertex_PNCU> polyVerts;

	g_theRenderer->SetModelMatrix(GetModelMatrix());

	m_convexPoly.AddVertsForPoly(polyVerts, m_faceColors);

	g_theRenderer->DrawVertexArray(polyVerts);
}

void ConvexPoly3DShape::RenderHighlight() const
{
	std::vector<Vertex_PCU> polyVerts;

	g_theRenderer->SetModelMatrix(GetModelMatrix());

	m_convexPoly.AddVertsForPoly(polyVerts, Rgba8::ORANGE);

	g_theRenderer->DrawVertexArray(polyVerts);
}

void ConvexPoly3DShape::RenderHighlightDiffuse() const
{
	std::vector<Vertex_PNCU> polyVerts;

	g_theRenderer->SetModelMatrix(GetModelMatrix());

	m_convexPoly.AddVertsForPoly(polyVerts, Rgba8::ORANGE);

	g_theRenderer->DrawVertexArray(polyVerts);
}

void ConvexPoly3DShape::RandomizeFaceColors()
{
	m_faceColors.clear();
	for (int faceIndex = 0; faceIndex < m_convexPoly.m_faces.size(); faceIndex++) {
		m_faceColors.push_back(GetRandomColor());
	}
}

