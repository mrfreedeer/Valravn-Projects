#pragma once
#include "Engine/Math/Vec2.hpp"
#include <vector>

struct AABB2;
struct FloatRange;
struct Vertex_PCU;
struct Rgba8;

class ConvexHull2D;

struct ConvexPoly2DEdge {
	ConvexPoly2DEdge(Vec2 const& pointOne, Vec2 const& pointTwo) : m_pointOne(pointOne), m_pointTwo(pointTwo) {}
	Vec2 m_pointOne = Vec2::ZERO;
	Vec2 m_pointTwo = Vec2::ZERO;
};

/// <summary>
/// This Convex Poly 2D was designed and written explicitly for operations involved in Delaunay's Triangulation
/// Therefore, it's different than a normal ConvexPoly2D, and as such, they are 2 different implementations
/// </summary>
struct DelaunayConvexPoly2D {
public:
	DelaunayConvexPoly2D(std::vector<Vec2> vertexes);

	void Translate(Vec2 const& translation);
	void Rotate(float degrees);
	DelaunayConvexPoly2D const GetRotated(float degrees);
	Vec2 const GetMiddlePoint() const { return m_middlePoint; }
	AABB2 const GetEnclosingAABB2() const;
	std::vector<Vec2> const GetSortedVertexes() const;
	bool IsPointVertex(Vec2 const& point, float tolerance = 0.025f) const;
	int GetInsideDirection() const; // 1 right, -1 left

	void PurgeIdenticalVertexes(float distTolerance = 0.025f);
	bool operator==(DelaunayConvexPoly2D const& compareTo) const;
	FloatRange const ProjectOntoAxis(Vec2 const& normalizedAxis, Vec2 const& origin) const;

	std::vector<ConvexPoly2DEdge> const GetEdges() const;
public:
	std::vector<Vec2> m_vertexes;
	Vec2 m_middlePoint = Vec2::ZERO;

private:
	void CalculateMiddlePoint();

};


class ConvexPoly2D {
public:
	ConvexPoly2D() = default;
	ConvexPoly2D(std::vector<Vec2> const& ccwPoints);
	Vec2 const GetCenter() const;
	float GetBoudingDisc(Vec2& discCenter);

	ConvexHull2D GetConvexHull() const;
	void RotateAroundPoint(Vec2 const& refPoint, float deltaAngle);
	void ScaleAroundPoint(Vec2 const& refPoint, float scale);
	void Translate(Vec2 const& displacement);

	friend void AddVertsForConvexPoly2D(std::vector<Vertex_PCU>& verts, ConvexPoly2D const& convexPoly, Rgba8 const& color);
	friend void AddVertsForWireConvexPoly2D(std::vector<Vertex_PCU>& verts, ConvexPoly2D const& convexPoly, Rgba8 const& borderColor, float lineThickness);
	AABB2 GetBoundingBox() const;

	void WritePolyToBuffer(std::vector<unsigned char>& buffer) const;
private:
	std::vector<Vec2> m_ccwPoints;

};