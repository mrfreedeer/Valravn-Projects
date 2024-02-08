#include "Game/Gameplay/ConvexPoly3D.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/Vertex_PNCU.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/MathUtils.hpp"

ConvexPoly3D::ConvexPoly3D(std::vector<Vec3> const& vertexes, std::vector<Face> const& faces) :
	m_vertexes(vertexes),
	m_faces(faces)
{
	CalculatePolyInfo();
}

void ConvexPoly3D::CalculatePolyInfo()
{
	Vec3 avgPos = Vec3::ZERO;
	for (int vertIndex = 0; vertIndex < m_vertexes.size(); vertIndex++) {
		avgPos += m_vertexes[vertIndex];
	}

	m_middlePoint = avgPos / (float)m_vertexes.size();

	for (int faceIndex = 0; faceIndex < m_faces.size(); faceIndex++) { // Calculating the face middle point can helping with further calculations thus, it's precomputed here
		Face& face = m_faces[faceIndex];
		face.m_owningPolygon = this;

		Vec3 faceAvgPoint = Vec3::ZERO;

		for (int faceVertIndex = 0; faceVertIndex < face.m_faceIndexes.size(); faceVertIndex++) {
			int vertInd = face.m_faceIndexes[faceVertIndex];
			Vec3 vertex = m_vertexes[vertInd];
			faceAvgPoint += vertex;
		}

		face.m_middlePoint = faceAvgPoint / (float)face.m_faceIndexes.size();

		face.RecalculatePlane();

		/*int faceVertIndOne = face.m_faceIndexes[0];
		int faceVertIndTwo = face.m_faceIndexes[1];

		Vec3 const& faceVertOne = m_vertexes[faceVertIndOne];
		Vec3 const& faceVertTwo = m_vertexes[faceVertIndTwo];

		Vec3 dirToPointOne = (faceVertOne - face.m_middlePoint).GetNormalized();
		Vec3 dirToPointTwo = (faceVertTwo - face.m_middlePoint).GetNormalized();

		face.m_normal = CrossProduct3D(dirToPointOne, dirToPointTwo).GetNormalized();*/

	}
}

void ConvexPoly3D::AddVertsForPoly(std::vector<Vertex_PCU>& verts, Rgba8 const& color) const
{
	for (int faceIndex = 0; faceIndex < m_faces.size(); faceIndex++) { // Calculating the face middle point can helping with further calculations thus, it's precomputed here
		Face const& face = m_faces[faceIndex];

		Vec3 facePrevPoint = m_vertexes[face.m_faceIndexes[0]];

		for (int faceVertIndex = 1; faceVertIndex < face.m_faceIndexes.size(); faceVertIndex++) {
			int vertInd = face.m_faceIndexes[faceVertIndex];
			Vec3 vertex = m_vertexes[vertInd];

			verts.emplace_back(face.m_middlePoint, color, Vec2::ZERO);
			verts.emplace_back(facePrevPoint, color, Vec2::ZERO);
			verts.emplace_back(vertex, color, Vec2::ZERO);

			facePrevPoint = vertex;

		}
		int firstFaceVert = face.m_faceIndexes[0];
		verts.emplace_back(face.m_middlePoint, color, Vec2::ZERO);
		verts.emplace_back(facePrevPoint, color, Vec2::ZERO);
		verts.emplace_back(m_vertexes[firstFaceVert], color, Vec2::ZERO);

	}

}
void ConvexPoly3D::AddVertsForPoly(std::vector<Vertex_PNCU>& verts, Rgba8 const& color) const
{
	for (int faceIndex = 0; faceIndex < m_faces.size(); faceIndex++) { // Calculating the face middle point can helping with further calculations thus, it's precomputed here
		Face const& face = m_faces[faceIndex];

		Vec3 facePrevPoint = m_vertexes[face.m_faceIndexes[0]];

		for (int faceVertIndex = 1; faceVertIndex < face.m_faceIndexes.size(); faceVertIndex++) {
			int vertInd = face.m_faceIndexes[faceVertIndex];
			Vec3 vertex = m_vertexes[vertInd];

			verts.emplace_back(face.m_middlePoint, face.m_plane.m_planeNormal, color, Vec2::ZERO);
			verts.emplace_back(facePrevPoint, face.m_plane.m_planeNormal, color, Vec2::ZERO);
			verts.emplace_back(vertex, face.m_plane.m_planeNormal, color, Vec2::ZERO);

			facePrevPoint = vertex;

		}
		int firstFaceVert = face.m_faceIndexes[0];
		verts.emplace_back(face.m_middlePoint, face.m_plane.m_planeNormal, color, Vec2::ZERO);
		verts.emplace_back(facePrevPoint, face.m_plane.m_planeNormal, color, Vec2::ZERO);
		verts.emplace_back(m_vertexes[firstFaceVert], face.m_plane.m_planeNormal, color, Vec2::ZERO);

	}
}

void ConvexPoly3D::AddVertsForPoly(std::vector<Vertex_PCU>& verts, std::vector<Rgba8> const& faceColors) const
{
	for (int faceIndex = 0; faceIndex < m_faces.size(); faceIndex++) { // Calculating the face middle point can helping with further calculations thus, it's precomputed here
		Face const& face = m_faces[faceIndex];

		Vec3 facePrevPoint = m_vertexes[face.m_faceIndexes[0]];
		Rgba8 const& currentColor = faceColors[faceIndex];

		for (int faceVertIndex = 1; faceVertIndex < face.m_faceIndexes.size(); faceVertIndex++) {
			int vertInd = face.m_faceIndexes[faceVertIndex];
			Vec3 vertex = m_vertexes[vertInd];

			verts.emplace_back(face.m_middlePoint, currentColor, Vec2::ZERO);
			verts.emplace_back(facePrevPoint, currentColor, Vec2::ZERO);
			verts.emplace_back(vertex, currentColor, Vec2::ZERO);

			facePrevPoint = vertex;

		}

		int firstFaceVert = face.m_faceIndexes[0];
		verts.emplace_back(face.m_middlePoint, currentColor, Vec2::ZERO);
		verts.emplace_back(facePrevPoint, currentColor, Vec2::ZERO);
		verts.emplace_back(m_vertexes[firstFaceVert], currentColor, Vec2::ZERO);

	}

}

void ConvexPoly3D::AddVertsForPoly(std::vector<Vertex_PNCU>& verts, std::vector<Rgba8> const& faceColors) const
{
	for (int faceIndex = 0; faceIndex < m_faces.size(); faceIndex++) { // Calculating the face middle point can helping with further calculations thus, it's precomputed here
		Face const& face = m_faces[faceIndex];

		Vec3 facePrevPoint = m_vertexes[face.m_faceIndexes[0]];
		Rgba8 const& currentColor = faceColors[faceIndex];

		for (int faceVertIndex = 1; faceVertIndex < face.m_faceIndexes.size(); faceVertIndex++) {
			int vertInd = face.m_faceIndexes[faceVertIndex];
			Vec3 vertex = m_vertexes[vertInd];

			verts.emplace_back(face.m_middlePoint, face.m_plane.m_planeNormal, currentColor, Vec2::ZERO);
			verts.emplace_back(facePrevPoint, face.m_plane.m_planeNormal, currentColor, Vec2::ZERO);
			verts.emplace_back(vertex, face.m_plane.m_planeNormal, currentColor, Vec2::ZERO);

			facePrevPoint = vertex;

		}

		int firstFaceVert = face.m_faceIndexes[0];
		verts.emplace_back(face.m_middlePoint, face.m_plane.m_planeNormal, currentColor, Vec2::ZERO);
		verts.emplace_back(facePrevPoint, face.m_plane.m_planeNormal, currentColor, Vec2::ZERO);
		verts.emplace_back(m_vertexes[firstFaceVert], face.m_plane.m_planeNormal, currentColor, Vec2::ZERO);

	}
}



void ConvexPoly3D::AddVertsForWirePoly(std::vector<Vertex_PCU>& verts, Rgba8 const& color) const
{
	for (int faceIndex = 0; faceIndex < m_faces.size(); faceIndex++) { // Calculating the face middle point can helping with further calculations thus, it's precomputed here
		Face const& face = m_faces[faceIndex];

		Vec3 facePrevPoint = m_vertexes[face.m_faceIndexes[0]];

		for (int faceVertIndex = 1; faceVertIndex < face.m_faceIndexes.size(); faceVertIndex++) {
			int vertInd = face.m_faceIndexes[faceVertIndex];
			Vec3 vertex = m_vertexes[vertInd];

			AddVertsForLineSegment3D(verts, facePrevPoint, vertex, color);

			facePrevPoint = vertex;

		}

		int firstFaceVert = face.m_faceIndexes[0];
		AddVertsForLineSegment3D(verts, facePrevPoint, m_vertexes[firstFaceVert], color);


	}
}



void ConvexPoly3D::Translate(Vec3 const& disp)
{
	for (int vertIndex = 0; vertIndex < m_vertexes.size(); vertIndex++) {
		Vec3& vertex = m_vertexes[vertIndex];
		vertex += disp;
	}
}

bool ConvexPoly3D::IsPointAVertex(Vec3 const& point, float tolerance) const
{
	float toleranceSqr = tolerance * tolerance;

	for (int vertIndex = 0; vertIndex < m_vertexes.size(); vertIndex++) {
		Vec3 const& vertex = m_vertexes[vertIndex];
		float distToPoint = GetDistanceSquared3D(point, vertex);

		if (distToPoint <= toleranceSqr) {
			return true;
		}
	}

	return false;
}


std::vector<Vec3> Face::GetFaceVerts() const
{
	std::vector<Vec3> faceVerts;
	faceVerts.reserve(m_faceIndexes.size());

	if (m_owningPolygon) {
		for (int faceVertIndex = 0; faceVertIndex < m_faceIndexes.size(); faceVertIndex++) {
			int vertInd = m_faceIndexes[faceVertIndex];
			faceVerts.push_back(m_owningPolygon->m_vertexes[vertInd]);
		}
	}

	return faceVerts;
}

void Face::RecalculatePlane()
{
	if (m_owningPolygon && (m_faceIndexes.size() > 2)) {
		Vec3 firstVert = m_owningPolygon->m_vertexes[m_faceIndexes[0]];
		Vec3 secVert = m_owningPolygon->m_vertexes[m_faceIndexes[1]];

		Vec3 firstBasis = (firstVert - m_middlePoint).GetNormalized();
		Vec3 secBasis = (secVert - m_middlePoint).GetNormalized();
		Vec3 normal = CrossProduct3D(firstBasis, secBasis).GetNormalized();

		Vec3 midPolyToMidFace = (m_middlePoint - m_owningPolygon->m_middlePoint);
		float dotFromCenter = DotProduct3D(midPolyToMidFace, normal);

		if (dotFromCenter < 0.0f) normal *= -1.0f;

		m_plane.m_planeNormal = normal;
		m_plane.m_distToPlane = m_middlePoint.GetLength();
	}
}
