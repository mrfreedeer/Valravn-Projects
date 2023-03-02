#pragma once

#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/ConvexPoly2D.hpp"
struct Vertex_PCU;
struct Rgba8;

class DelaunayShape2D {
public:
	DelaunayShape2D(std::vector<Vec2> vertexes);
	DelaunayShape2D(Vec2 const& startingPosition, std::vector<Vec2> vertexes);
	DelaunayShape2D(Vec2 const& startingPosition, DelaunayConvexPoly2D const& convexPoly);

	void Update(float deltaSeconds);
	void Translate(Vec2 const& disp);
	void AddVertsForShape(std::vector<Vertex_PCU>& verts, Rgba8 const& color) const;
	DelaunayConvexPoly2D& GetPoly() { return m_polygon; }

	bool m_isOverlaping = false;
	Vec2 m_velocity = Vec2::ZERO;
	Vec2 m_position = Vec2::ZERO;

private:
	DelaunayConvexPoly2D m_polygon;
};