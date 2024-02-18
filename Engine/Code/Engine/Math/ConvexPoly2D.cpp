#include "Engine/Math/ConvexPoly2D.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/ConvexHull2D.hpp"
#include "Engine/Math/Plane2D.hpp"
#include "Engine/Core/BufferUtils.hpp"
#include <algorithm>

DelaunayConvexPoly2D::DelaunayConvexPoly2D(std::vector<Vec2> vertexes) :
	m_vertexes(vertexes)
{
	CalculateMiddlePoint();
}

void DelaunayConvexPoly2D::Translate(Vec2 const& translation)
{
	for (int vertexIndex = 0; vertexIndex < m_vertexes.size(); vertexIndex++) {
		Vec2& vertex = m_vertexes[vertexIndex];
		vertex += translation;
	}

	m_middlePoint += translation;
}

void DelaunayConvexPoly2D::Rotate(float degrees)
{
	for (int vertexIndex = 0; vertexIndex < m_vertexes.size(); vertexIndex++) {
		Vec2& vertex = m_vertexes[vertexIndex];

		Vec2 dispFromMiddle = vertex - m_middlePoint;
		dispFromMiddle.RotateDegrees(degrees);
		vertex = m_middlePoint + dispFromMiddle;
	}
}

DelaunayConvexPoly2D const DelaunayConvexPoly2D::GetRotated(float degrees)
{
	DelaunayConvexPoly2D rotatedPoly(*this);
	rotatedPoly.Rotate(degrees);
	return rotatedPoly;
}

AABB2 const DelaunayConvexPoly2D::GetEnclosingAABB2() const
{
	AABB2 enclosingQuad;
	for (int vertexIndex = 0; vertexIndex < m_vertexes.size(); vertexIndex++) {
		Vec2 const& vertex = m_vertexes[vertexIndex];
		enclosingQuad.StretchToIncludePoint(vertex);
	}
	return enclosingQuad;
}

bool operator<(Vec2 const& pointA, Vec2 const& pointB)
{
	if (pointA.x < pointB.x) {
		return true;
	}
	else if (pointA.x > pointB.x) {
		return false;
	}
	else if (pointA.y < pointB.y) {
		return true;
	}
	return false;
}

void DelaunayConvexPoly2D::CalculateMiddlePoint()
{
	for (int vertexIndex = 0; vertexIndex < m_vertexes.size(); vertexIndex++) {
		Vec2 const& vertex = m_vertexes[vertexIndex];
		m_middlePoint += vertex;
	}
	m_middlePoint /= static_cast<float>(m_vertexes.size());
}

std::vector<Vec2> const DelaunayConvexPoly2D::GetSortedVertexes() const
{
	std::vector<Vec2> sortedVertexes = m_vertexes;
	std::sort(sortedVertexes.begin(), sortedVertexes.end());
	return sortedVertexes;
}

bool DelaunayConvexPoly2D::IsPointVertex(Vec2 const& point, float tolerance) const
{
	for (int vertexIndex = 0; vertexIndex < m_vertexes.size(); vertexIndex++) {
		Vec2 const& vertex = m_vertexes[vertexIndex];
		if ((vertex - point).GetLengthSquared() < tolerance) return true;
	}
	return false;
}
int DelaunayConvexPoly2D::GetInsideDirection() const
{
	Vec2 edgeI = (m_vertexes[1] - m_vertexes[0]).GetNormalized();
	Vec2 edgeJ = edgeI.GetRotated90Degrees();

	Vec2 edgeMidPoint = (m_vertexes[1] + m_vertexes[0]) * 0.5f;
	Vec2 dirToEdgeMidPoint = (edgeMidPoint - m_middlePoint).GetNormalized();

	float dotProdWithJ = DotProduct2D(dirToEdgeMidPoint, edgeJ);
	int direction = (dotProdWithJ > 0) ? 1 : -1;

	return direction;
}

bool DelaunayConvexPoly2D::operator==(DelaunayConvexPoly2D const& compareTo) const {
	if (m_vertexes.size() != compareTo.m_vertexes.size()) return false;

	for (int vertexPointInd = 0; vertexPointInd < m_vertexes.size(); vertexPointInd++) {
		Vec2 const& vertex = m_vertexes[vertexPointInd];
		if (!compareTo.IsPointVertex(vertex, 0.25f)) return false;
	}

	return true;
}

FloatRange const DelaunayConvexPoly2D::ProjectOntoAxis(Vec2 const& normalizedAxis, Vec2 const& origin) const
{
	float min = FLT_MAX;
	float max = FLT_MIN;

	for (int vertexPointInd = 0; vertexPointInd < m_vertexes.size(); vertexPointInd++) {
		Vec2 const& vertex = m_vertexes[vertexPointInd];
		Vec2 dispOriginToVertex = vertex - origin;

		float vertDotProd = DotProduct2D(normalizedAxis, dispOriginToVertex);

		if (vertDotProd < min) {
			min = vertDotProd;
		}

		if (vertDotProd > max) {
			max = vertDotProd;
		}
	}

	return FloatRange(min, max);
}

std::vector<ConvexPoly2DEdge> const DelaunayConvexPoly2D::GetEdges() const
{
	std::vector<ConvexPoly2DEdge> edges;
	for (int vertexInd = 0; vertexInd < m_vertexes.size() - 1; vertexInd++) {
		ConvexPoly2DEdge newEdge = {
			m_vertexes[vertexInd],
			m_vertexes[vertexInd + 1]
		};
		edges.push_back(newEdge);
	}

	ConvexPoly2DEdge lastEdge = {
		m_vertexes[m_vertexes.size() - 1],
		m_vertexes[0]
	};

	edges.push_back(lastEdge);

	return edges;
}

void DelaunayConvexPoly2D::PurgeIdenticalVertexes(float distTolerance)
{

	float toleranceSqr = distTolerance * distTolerance;
	for (std::vector<Vec2>::iterator firstIt = m_vertexes.begin(); firstIt != m_vertexes.end(); firstIt++) {
		Vec2 const& vertex = *firstIt;

		for (std::vector<Vec2>::iterator otherIt = firstIt + 1; otherIt != m_vertexes.end();) {
			Vec2 const& otherVert = *otherIt;

			if (GetDistanceSquared2D(vertex, otherVert) < toleranceSqr) {
				otherIt = m_vertexes.erase(otherIt);
			}
			else {
				otherIt++;
			}
		}
	}
}

/*
	OTHER IMPLEMENTATION FOR CONVEXPOLY2D
*/

ConvexPoly2D::ConvexPoly2D(std::vector<Vec2> const& ccwPoints)
{
	m_ccwPoints.insert(m_ccwPoints.begin(),ccwPoints.begin(), ccwPoints.end());
}

Vec2 const ConvexPoly2D::GetCenter() const
{
	Vec2 middlePoint = Vec2::ZERO;
	for (int vertexIndex = 0; vertexIndex < m_ccwPoints.size(); vertexIndex++) {
		Vec2 const& vertex = m_ccwPoints[vertexIndex];
		middlePoint += vertex;
	}

	middlePoint /= (float) m_ccwPoints.size();
	return middlePoint;
}

float ConvexPoly2D::GetBoudingDisc(Vec2& discCenter)
{
	discCenter = GetCenter();
	float greatestDistToCenter = FLT_MIN;
	for (Vec2 const& point : m_ccwPoints) {
		float distToCenter = GetDistance2D(point, discCenter);
		if (distToCenter > greatestDistToCenter) {
			greatestDistToCenter = distToCenter;
		}
	}
	

	return greatestDistToCenter;
}

void ConvexPoly2D::RotateAroundPoint(Vec2 const& refPoint, float deltaAngle) {
	
	for (Vec2& point : m_ccwPoints) {
		Vec2 dispToPoint = point - refPoint;
		dispToPoint.RotateDegrees(deltaAngle);
		point = refPoint + dispToPoint;
	}

}

void ConvexPoly2D::ScaleAroundPoint(Vec2 const& refPoint, float scale) {

	for (Vec2& point : m_ccwPoints) {
		Vec2 dispToPoint = point - refPoint;
		dispToPoint *= scale;
		point = refPoint + dispToPoint;
	}

}

void ConvexPoly2D::Translate(Vec2 const& displacement)
{
	for (Vec2& point : m_ccwPoints) {
		point += displacement;
	}
}

AABB2 ConvexPoly2D::GetBoundingBox() const
{
	Vec2 mins = Vec2(FLT_MAX, FLT_MAX);
	Vec2 maxs = Vec2(FLT_MIN, FLT_MIN);

	for (Vec2 const& point : m_ccwPoints) {
		if (point.x < mins.x) {
			mins.x = point.x;
		}

		if (point.y < mins.y) {
			mins.y = point.y;
		}

		if (point.x > maxs.x) {
			maxs.x = point.x;
		}

		if (point.y > maxs.y) {
			maxs.y = point.y;
		}
	}
	return AABB2(mins, maxs);
}

void ConvexPoly2D::WritePolyToBuffer(std::vector<unsigned char>& buffer) const
{
	BufferWriter bufWriter(buffer);

	bufWriter.AppendeByte((unsigned char)m_ccwPoints.size());
	for (Vec2 const& vertex : m_ccwPoints) {
		bufWriter.AppendVec2(vertex);
	}

}

ConvexHull2D ConvexPoly2D::GetConvexHull() const
{
	ConvexHull2D asConvexHull;

	for (int pointIndex = 0; pointIndex < (m_ccwPoints.size() - 1); pointIndex++) {
		Vec2 pointOne = m_ccwPoints[pointIndex];
		int nextPoint = pointIndex+1;
		Vec2 pointTwo = m_ccwPoints[nextPoint];

		Vec2 dispBetPoints = pointTwo - pointOne;
		Vec2 normal = dispBetPoints.GetRotatedMinus90Degrees().GetNormalized();

		Plane2D newPlane;
		newPlane.m_planeNormal = normal;
		newPlane.m_distToPlane = DotProduct2D(pointOne, normal);

		asConvexHull.m_planes.push_back(newPlane);

	}

	Vec2 firstPoint = m_ccwPoints[0];
	Vec2 lastPoint = m_ccwPoints[m_ccwPoints.size() - 1];

	Vec2 dispBetPoints = firstPoint - lastPoint;
	Vec2 normal = dispBetPoints.GetRotatedMinus90Degrees().GetNormalized();
	Plane2D lastPlane;

	lastPlane.m_planeNormal = normal;
	lastPlane.m_distToPlane = DotProduct2D(firstPoint, normal);
	asConvexHull.m_planes.push_back(lastPlane);

	return asConvexHull;
}

