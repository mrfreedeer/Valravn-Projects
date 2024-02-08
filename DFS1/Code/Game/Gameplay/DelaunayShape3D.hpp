#pragma once

#include "Engine/Math/Vec3.hpp"
#include "Game/Gameplay/ConvexPoly3D.hpp"

struct Vertex_PCU;
struct Rgba8;

class DelaunayShape3D {
public:
	DelaunayShape3D(std::vector<Vec3> const& vertexes, std::vector<Face> const& faces);

	void Update(float deltaSeconds);
	void Translate(Vec3 const& disp);
	void AddVertsForShape(std::vector<Vertex_PCU>& verts, Rgba8 const& color) const;
	void AddVertsForShape(std::vector<Vertex_PCU>& verts) const;
	void AddVertsForWireShape(std::vector<Vertex_PCU>& verts, Rgba8 const& color) const;
	ConvexPoly3D& GetPoly() { return m_polygon; }

	bool m_isOverlaping = false;
	Vec3 m_velocity = Vec3::ZERO;
	Vec3 m_position = Vec3::ZERO;

private:
	ConvexPoly3D m_polygon;
	std::vector<Rgba8> m_faceColors;
};