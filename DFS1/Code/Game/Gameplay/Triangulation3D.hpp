#pragma once
#include "Engine/Math/Vec3.hpp"
#include <vector>

struct Vertex_PCU;
struct Rgba8;

class ConvexPoly3D;

struct DelaunayRaycast3D {
	bool m_didImpact = false;
	Vec3 m_impactPos = Vec3::ZERO;
	Vec3 m_impactedEdgeVertexA = Vec3::ZERO;
	Vec3 m_impactedEdgeVertexB = Vec3::ZERO;
};

struct DelaunayEdge3D {
public:
	DelaunayEdge3D(Vec3 const& pointA, Vec3 const& pointB) :
		m_pointA(pointA),
		m_pointB(pointB)
	{}
	DelaunayEdge3D() = default;
	Vec3 m_pointA;
	Vec3 m_pointB;

	bool operator==(DelaunayEdge3D const& otherEdge) const {
		bool areEqualDirAB = (otherEdge.m_pointA == m_pointA) && (otherEdge.m_pointB == m_pointB);
		bool areEqualDirBA = (otherEdge.m_pointA == m_pointB) && (otherEdge.m_pointB == m_pointA);
		return areEqualDirAB || areEqualDirBA;
	}

	Vec3 const GetCenter() const {
		return (m_pointA + m_pointB) * 0.5f;
	}

	bool ContainsPoint(Vec3 const& point, float tolerance = 0.1f) const {
		Vec3 diffBetweenA = point - m_pointA;
		Vec3 diffBetweenB = point - m_pointB;

		return (diffBetweenA.GetLengthSquared() <= tolerance) || (diffBetweenB.GetLengthSquared() <= tolerance);
	}

	bool IsCollapsed(float tolerance = 0.025f) {
		float diffBetweenPoints = GetDistanceSquared3D(m_pointA, m_pointB);

		return (diffBetweenPoints < (tolerance * tolerance));
	}

};


struct DelaunayFace3D {
public:
	DelaunayFace3D(Vec3 const& pointA, Vec3 const& pointB, Vec3 const& pointC) :
		m_pointA(pointA),
		m_pointB(pointB),
		m_pointC(pointC)
	{}
	DelaunayFace3D() = default;
	Vec3 m_pointA;
	Vec3 m_pointB;
	Vec3 m_pointC;

	bool operator==(DelaunayFace3D const& otherFace) const {
		bool pointAFound = (m_pointA == otherFace.m_pointA) || (m_pointA == otherFace.m_pointB) || (m_pointA == otherFace.m_pointC);
		bool pointBFound = (m_pointB == otherFace.m_pointA) || (m_pointB == otherFace.m_pointB) || (m_pointB == otherFace.m_pointC);
		bool pointCFound = (m_pointC == otherFace.m_pointA) || (m_pointC == otherFace.m_pointB) || (m_pointC == otherFace.m_pointC);

		return pointAFound && pointBFound && pointCFound;
	}

	Vec3 const GetCenter() const {
		return (m_pointA + m_pointB + m_pointC)  / 3.0f;
	}

	bool ContainsPoint(Vec3 const& point, float tolerance = 0.1f) const {
		Vec3 diffBetweenA = point - m_pointA;
		Vec3 diffBetweenB = point - m_pointB;
		Vec3 diffBetweenC = point - m_pointC;

		return (diffBetweenA.GetLengthSquared() <= tolerance) || (diffBetweenB.GetLengthSquared() <= tolerance) || (diffBetweenC.GetLengthSquared() <= tolerance);
	}

	bool AreAnyPointsCollapsed(float tolerance = 0.025f) const {
		float toleranceSqr = tolerance * tolerance;

		bool AAndBCollapsed = (m_pointA - m_pointB).GetLengthSquared() < toleranceSqr;
		bool AAndCCollapsed = (m_pointA - m_pointC).GetLengthSquared() < toleranceSqr;
		bool BAndCCollapsed = (m_pointB - m_pointC).GetLengthSquared() < toleranceSqr;

		return AAndBCollapsed || AAndCCollapsed || BAndCCollapsed;
	}

};



struct DelaunayTetrahedron {
public:
	DelaunayTetrahedron() = default;
	DelaunayTetrahedron(std::vector<Vec3> const& vertexes);
	DelaunayTetrahedron(Vec3 const& pointA, Vec3 const& pointB, Vec3 const& pointC, Vec3 const& pointD);

	void AddVertsForFaces(std::vector<Vertex_PCU>& verts, Rgba8 const& faceColor) const;
	void AddVertsForWireframe(std::vector<Vertex_PCU>& verts, Rgba8 const& wireColor, float thickness = 0.05f) const;
	bool IsPointAVertex(Vec3 const& vertex, float tolerance = 0.025f) const;
	bool IsPointInsideCircumcenter(Vec3 const& vertex, float tolerance = 0.025f) const;
	bool DoTetrahedronsShareFaces(DelaunayTetrahedron const& otherTetrahedron) const;

	bool ContainsAnyTetrahedronVertex(DelaunayTetrahedron const& otherTetrahedron) const;
	void GetEdges(DelaunayEdge3D* edges) const;
	void GetFaces(DelaunayFace3D* faces) const;
public:
	Vec3 m_vertexes[4] = {};
	Vec3 m_circumcenter = Vec3::ZERO;
	float m_circumcenterRadius = 0.0f;

private:
	void CalculateCircumcenter();
	void AddVertsForFace(std::vector<Vertex_PCU>& verts, Rgba8 const& faceColor, Vec3 const& pointA, Vec3 const& pointB, Vec3 const& pointC) const;
};

std::vector<DelaunayTetrahedron> TriangulateConvexPoly3D(ConvexPoly3D const& convexPoly, bool includeSuperTriangle, float toleranceVertexDistance, int maxStep = -1);
DelaunayTetrahedron const GetASuperTetrahedronPoly(ConvexPoly3D const& convexPoly);
std::vector<DelaunayEdge3D> GetVoronoiDiagramFromConvexPoly3D(std::vector<DelaunayTetrahedron> const& tetrahedronMesh, ConvexPoly3D const& convexPoly, bool includeEdgeProjections = true);