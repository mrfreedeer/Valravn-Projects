#include "Engine/Core/VertexUtils.hpp"
#include "Game/Gameplay/DelaunayShape3D.hpp"
#include "Game/Framework/GameCommon.hpp"

Rgba8 const GetRandomColor()
{
	unsigned char randomR = static_cast<unsigned char>(rng.GetRandomIntInRange(128, 255));
	unsigned char randomG = static_cast<unsigned char>(rng.GetRandomIntInRange(128, 255));
	unsigned char randomB = static_cast<unsigned char>(rng.GetRandomIntInRange(128, 255));
	unsigned char randomA = static_cast<unsigned char>(rng.GetRandomIntInRange(190, 255));
	return Rgba8(randomR, randomG, randomB, randomA);
}

DelaunayShape3D::DelaunayShape3D(std::vector<Vec3> const& vertexes, std::vector<Face> const& faces) :
	m_polygon(vertexes, faces)
{
	m_position = m_polygon.m_middlePoint;
	for (int faceIndex = 0; faceIndex < faces.size(); faceIndex++) {
		m_faceColors.push_back(GetRandomColor());
	}
}

void DelaunayShape3D::Update(float deltaSeconds)
{
	Vec3 prevPos = m_position;
	m_position += m_velocity * deltaSeconds;
	Vec3 dispToNewPos = m_position - prevPos;
	m_polygon.Translate(dispToNewPos);
}

void DelaunayShape3D::Translate(Vec3 const& disp)
{
	m_position += disp;
	m_polygon.Translate(disp);
}

void DelaunayShape3D::AddVertsForShape(std::vector<Vertex_PCU>& verts, Rgba8 const& color) const
{
	m_polygon.AddVertsForPoly(verts, color);
}


void DelaunayShape3D::AddVertsForShape(std::vector<Vertex_PCU>& verts) const
{
	m_polygon.AddVertsForPoly(verts, m_faceColors);
}

void DelaunayShape3D::AddVertsForWireShape(std::vector<Vertex_PCU>& verts, Rgba8 const& color) const
{
	m_polygon.AddVertsForWirePoly(verts, color);
}
