#pragma once
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Plane3D.hpp"
#include <vector>

class ConvexPoly3D;
struct Face {
	std::vector<int> m_faceIndexes;
	Vec3 m_middlePoint = Vec3::ZERO;
	ConvexPoly3D* m_owningPolygon = nullptr;
	Plane3D	m_plane = {};

	std::vector<Vec3> GetFaceVerts() const;
	void RecalculatePlane();

};

struct Vertex_PCU;
struct Vertex_PNCU;
struct Rgba8;

class ConvexPoly3D {
public:
	ConvexPoly3D() = default;
	ConvexPoly3D(std::vector<Vec3>const& vertexes, std::vector<Face> const& faces);

	void CalculatePolyInfo();
	void AddVertsForPoly(std::vector<Vertex_PCU>& verts, Rgba8 const& color) const;
	void AddVertsForPoly(std::vector<Vertex_PNCU>& verts, Rgba8 const& color) const;
	void AddVertsForPoly(std::vector<Vertex_PCU>& verts, std::vector<Rgba8> const& faceColors) const;
	void AddVertsForPoly(std::vector<Vertex_PNCU>& verts, std::vector<Rgba8> const& faceColors) const;
	void AddVertsForWirePoly(std::vector<Vertex_PCU>& verts, Rgba8 const& color) const;

	void Translate(Vec3 const& disp);
	bool IsPointAVertex(Vec3 const& point, float tolerance = 0.025f) const;

	std::vector<Vec3> m_vertexes;
	Vec3 m_middlePoint = Vec3::ZERO;
	std::vector<Face> m_faces;
};