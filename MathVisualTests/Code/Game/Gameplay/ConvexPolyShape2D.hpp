#pragma once
#include "Engine/Math/ConvexPoly2D.hpp"
#include "Engine/Math/ConvexHull2D.hpp"
#include "Game/Gameplay/Shape2D.hpp"


class ConvexPolyShape2D : public Shape2D {
public:
	ConvexPolyShape2D(std::vector<Vec2> ccwPoints, Rgba8 const& color, Rgba8 const& highlighedColor);

	void AddVertsForEdges(std::vector <Vertex_PCU>& vertexes, Rgba8 const& borderColor) const;
	void AddVertsForFillings(std::vector<Vertex_PCU>& vertexes)const;
	virtual void Render() const override;
	virtual void RenderHighlighted() const override;
	virtual Vec2 GetNearestPoint(Vec2 const& refPoint) const override;
	virtual bool IsPointInside(Vec2 const& refPoint) const override;
	virtual void UpdateBoundingDisc();
public:
	ConvexPoly2D m_convexPoly2D;
	ConvexHull2D m_convexHull2D;
	float m_radius = 0.0f;
	Vec2 m_discCenter = Vec2::ZERO;
};