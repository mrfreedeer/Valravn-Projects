#pragma once
#include "Engine/Math/Vec3.hpp"
#include <vector>

class ConvexPoly3D;

struct ConvexPoly3DEdge {
	ConvexPoly3DEdge() = default;
	ConvexPoly3DEdge(Vec3 const& pointOne, Vec3 const& pointTwo): m_pointOne(pointOne), m_pointTwo(pointTwo){}
	Vec3 m_pointOne = Vec3::ZERO;
	Vec3 m_pointTwo = Vec3::ZERO;
};

struct Face {
	std::vector<int> m_faceIndexes;
	Vec3 m_middlePoint = Vec3::ZERO;
	ConvexPoly3D* m_owningPolygon = nullptr;
	Vec3 m_normal = Vec3::ZERO;

	std::vector<Vec3> GetFaceVerts() const;
	std::vector<ConvexPoly3DEdge> GetFaceEdges() const;
};

struct Vertex_PCU;
struct Rgba8;

class ConvexPoly3D {
public:
	ConvexPoly3D(std::vector<Vec3>const& vertexes, std::vector<Face> const& faces);

	void AddVertsForPoly(std::vector<Vertex_PCU>& verts, Rgba8 const& color) const;
	void AddVertsForPoly(std::vector<Vertex_PCU>& verts, std::vector<Rgba8> const& faceColors) const;
	void AddVertsForWirePoly(std::vector<Vertex_PCU>& verts, Rgba8 const& color) const;

	void Translate(Vec3 const& disp);
	bool IsPointAVertex(Vec3 const& point, float tolerance = 0.025f) const;

	std::vector<Vec3> m_vertexes;
	Vec3 m_middlePoint = Vec3::ZERO;
	std::vector<Face> m_faces;
};