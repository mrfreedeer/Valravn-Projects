#pragma once
#include "Engine/Math/Vec2.hpp"
#include <vector>

struct DelaunayConvexPoly2D;
struct Vertex_PCU;


struct DelaunayRaycast2D {
	bool m_didImpact = false;
	Vec2 m_impactPos = Vec2::ZERO;
	Vec2 m_impactedEdgeVertexA = Vec2::ZERO;
	Vec2 m_impactedEdgeVertexB = Vec2::ZERO;
};

struct DelaunayEdge {
public:
	DelaunayEdge(Vec2 const& pointA, Vec2 const& pointB) :
		m_pointA(pointA),
		m_pointB(pointB)
	{}
	DelaunayEdge() = default;
	Vec2 m_pointA;
	Vec2 m_pointB;

	bool operator==(DelaunayEdge const& otherEdge) const {
		bool areEqualDirAB = (otherEdge.m_pointA == m_pointA) && (otherEdge.m_pointB == m_pointB);
		bool areEqualDirBA = (otherEdge.m_pointA == m_pointB) && (otherEdge.m_pointB == m_pointA);
		return areEqualDirAB || areEqualDirBA;
	}

	Vec2 const GetCenter() const {
		return (m_pointA + m_pointB) * 0.5f;
	}

	bool ContainsPoint(Vec2 const& point, float tolerance = 0.1f) const {
		Vec2 diffBetweenA = point - m_pointA;
		Vec2 diffBetweenB = point - m_pointB;

		return (diffBetweenA.GetLengthSquared() <= tolerance) || (diffBetweenB.GetLengthSquared() <= tolerance);
	}


	DelaunayRaycast2D const ClipEdge(DelaunayConvexPoly2D const& convexPoly);
};

struct DelaunayTriangle {
public:
	DelaunayTriangle() = default;
	DelaunayTriangle(std::vector<Vec2> const& vertexes);
	DelaunayTriangle(Vec2 const& pointA, Vec2 const& pointB, Vec2 const& pointC);
	bool IsPointInsideCircumcenter(Vec2 const& point) const;
	bool DoTrianglesShareEdge(DelaunayTriangle const& otherTriangle) const;

	// Modifiers
	void SetVertex(int vertexIndex, Vec2 const& vertex);
	void Translate(Vec2 const& translation);
	bool ReplaceVertex(Vec2 const& oldVertex, Vec2 const& newVertex);

	// Const
	bool IsPointAVertex(Vec2 const& point, float tolerance) const;
	void GetEdges(DelaunayEdge* edges) const;
	DelaunayEdge const GetClosestEdgeToPoint(Vec2 const& refPoint) const;
	Vec2 const GetCircumcenter() const { return m_circumcenter; }
	Vec2 const GetRemainingVertex(Vec2 const& vertexToExclude1, Vec2 const& vertexToExclude2) const; // Function used to quickly get other vertex that's not any of two other vertex
	float GetCircumcenterRadius() const { return m_circumcenterRadius; }
	bool ContainsAnyTriangleVertex(DelaunayTriangle const& otherTriangle) const;
	bool ContainsEdge(DelaunayEdge const& edge) const;

	void AddVertsForTriangleMesh(std::vector<Vertex_PCU>& verts, Rgba8 const& color, float lineThickness = 0.5f) const;

	bool operator==(DelaunayTriangle const& otherTriangle);
	bool IsTriangleCollapsed() const;
public:
	Vec2 m_vertexes[3] = {};
	Vec2 m_circumcenter = Vec2::ZERO;
	float m_circumcenterRadius = 0.0f;

private:
	void CalculateCircumcenter();
};

DelaunayTriangle const GetASuperTriangleFromPoly(DelaunayConvexPoly2D const& convexPoly);
std::vector<DelaunayTriangle> TriangulateConvexPoly2D(DelaunayConvexPoly2D const& convexPoly, std::vector<Vec2> const& artificialPoints, bool includeSuperTriangle = false, float toleranceVertexDistance = 0.1f, int maxSteps = -1);
std::vector<DelaunayTriangle> TriangulatePointSet2D(std::vector<Vec2> const& pointSet, bool includeSuperTriangle = false);
std::vector<DelaunayEdge> GetVoronoiDiagram(std::vector<DelaunayTriangle> const& triangleMesh);
std::vector<DelaunayEdge> GetVoronoiDiagramFromConvexPoly(std::vector<DelaunayTriangle> const& triangleMesh, DelaunayConvexPoly2D const& convexPoly, std::vector<Vec2> const& artificialPoints);
std::vector<DelaunayConvexPoly2D> SplitConvexPoly(std::vector<DelaunayEdge> const& convexPolyVoronoiEdges, DelaunayConvexPoly2D const& convexPoly);