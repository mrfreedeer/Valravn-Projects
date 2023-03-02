#include "Engine/Math/Curves.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/Vertex_PCU.hpp"


BezierCurve::BezierCurve(std::vector<Vec2> const& points) :
	m_points(points)
{
}

BezierCurve::BezierCurve(CubicHermitCurve const& hermitCurve)
{
	m_points.resize(4);
	float aThird = 1.0f / 3.0f;
	m_points[0] = hermitCurve.m_points[0];
	m_points[1] = hermitCurve.m_points[0] + (hermitCurve.m_points[1] * aThird);
	m_points[2] = hermitCurve.m_points[2] - (hermitCurve.m_points[3] * aThird);
	m_points[3] = hermitCurve.m_points[2];
}

Vec2 const BezierCurve::EvaluateAtParametric(float parametricZeroToOne) const
{
	if ((m_points.size() % 2) == 1) {
		ERROR_RECOVERABLE("BEZIER CURVES REQUIRE AN EVEN AMOUNT OF KEY POINTS");
		return Vec2::ZERO;
	}

	std::vector<Vec2> currentPoints = m_points;
	std::vector<Vec2> interpolatedPoints;
	while (interpolatedPoints.size() != 1) {
		interpolatedPoints.clear();
		for (int pointIndex = 0; pointIndex < currentPoints.size(); pointIndex++) {
			if (pointIndex != currentPoints.size() - 1) {
				Vec2 const& pointA = currentPoints[pointIndex];
				Vec2 const& pointB = currentPoints[(size_t)pointIndex + 1];


				Vec2 interpolatedPoint = Interpolate(pointA, pointB, parametricZeroToOne);
				interpolatedPoints.push_back(interpolatedPoint);
			}
		}
		currentPoints = interpolatedPoints;
	}

	return interpolatedPoints[0];
}

void BezierCurve::AddVertsForControlPoints(std::vector<Vertex_PCU>& verts, Rgba8 const& color, float radius, int sectionsAmount) const
{
	Vec2 previousPoint = m_points[0];
	AddVertsForDisc2D(verts, previousPoint, radius, color, sectionsAmount);

	for (int pointIndex = 1; pointIndex < m_points.size(); pointIndex++) {
		Vec2 const& currentPoint = m_points[pointIndex];
		LineSegment2 currentLine(previousPoint, currentPoint);
		AddVertsForDisc2D(verts, currentPoint, radius, color, sectionsAmount);
		AddVertsForLineSegment2D(verts, currentLine, color, radius);
		previousPoint = currentPoint;
	}

}

float BezierCurve::GetApproximateLength(int numSubdivisions) const
{
	float stepPerDivision = 1.0f / (float)numSubdivisions;
	Vec2 prevLocation = EvaluateAtParametric(0.0f);
	float distance = 0.0f;

	for (int step = 1; step <= numSubdivisions; step++) {
		Vec2 currentLocation = EvaluateAtParametric(step * stepPerDivision);
		distance += GetDistance2D(prevLocation, currentLocation);
		prevLocation = currentLocation;
	}

	return distance;
}

Vec2 const BezierCurve::EvaluateAtApproximateDistance(float distanceAlongCurve, int numSubdivisions) const
{
	float stepPerDivision = 1.0f / (float)numSubdivisions;
	Vec2 prevLocation = m_points[0];
	float distance = 0.0f;

	for (int step = 1; step <= numSubdivisions; step++) {
		Vec2 currentLocation = EvaluateAtParametric(step * stepPerDivision);
		float distanceThisStep = GetDistance2D(prevLocation, currentLocation);
		if ((distance + distanceThisStep) >= distanceAlongCurve) {
			float distanceToInterpolate = (distanceAlongCurve - distance) / distanceThisStep;
			return Interpolate(prevLocation, currentLocation, distanceToInterpolate);
		}
		else {
			prevLocation = currentLocation;
			distance += distanceThisStep;
		}
	}

	return m_points[m_points.size() - 1];
}

float ComputeCubicBezier1D(float a, float b, float c, float d, float t)
{
	float ab = Interpolate(a, b, t);
	float bc = Interpolate(b, c, t);
	float cd = Interpolate(c, d, t);

	float abc = Interpolate(ab, bc, t);
	float bcd = Interpolate(bc, cd, t);

	float result = Interpolate(abc, bcd, t);
	return result;
}

Vec2 const ComputeCubicBezier2D(Vec2 const& a, Vec2 const& b, Vec2 const& c, Vec2 const& d, float t)
{
	Vec2 ab = Interpolate(a, b, t);
	Vec2 bc = Interpolate(b, c, t);
	Vec2 cd = Interpolate(c, d, t);

	Vec2 abc = Interpolate(ab, bc, t);
	Vec2 bcd = Interpolate(bc, cd, t);

	Vec2 result = Interpolate(abc, abc, t);
	return result;
}

float ComputeQuinticBezier1D(float a, float b, float c, float d, float e, float f, float t)
{
	float ab = Interpolate(a, b, t);
	float bc = Interpolate(b, c, t);
	float cd = Interpolate(c, d, t);
	float de = Interpolate(d, e, t);
	float ef = Interpolate(e, f, t);

	float abc = Interpolate(ab, bc, t);
	float bcd = Interpolate(bc, cd, t);
	float cde = Interpolate(cd, de, t);
	float def = Interpolate(de, ef, t);

	float abcd = Interpolate(abc, bcd, t);
	float bcde = Interpolate(bcd, cde, t);
	float cdef = Interpolate(cde, def, t);


	float abcde = Interpolate(abcd, bcde, t);
	float bcdef = Interpolate(bcde, cdef, t);

	float result = Interpolate(abcde, bcdef, t);

	return result;
}

Vec2 const ComputeQuinticBezier2D(Vec2 const& a, Vec2 const& b, Vec2 const& c, Vec2 const& d, Vec2 const& e, Vec2 const& f, float t)
{
	Vec2 ab = Interpolate(a, b, t);
	Vec2 bc = Interpolate(b, c, t);
	Vec2 cd = Interpolate(c, d, t);
	Vec2 de = Interpolate(d, e, t);
	Vec2 ef = Interpolate(e, f, t);

	Vec2 abc = Interpolate(ab, bc, t);
	Vec2 bcd = Interpolate(bc, cd, t);
	Vec2 cde = Interpolate(cd, de, t);
	Vec2 def = Interpolate(de, ef, t);

	Vec2 abcd = Interpolate(abc, bcd, t);
	Vec2 bcde = Interpolate(bcd, cde, t);
	Vec2 cdef = Interpolate(cde, def, t);


	Vec2 abcde = Interpolate(abcd, bcde, t);
	Vec2 bcdef = Interpolate(bcde, cdef, t);

	Vec2 result = Interpolate(abcde, bcdef, t);

	return result;
}

CubicHermitCurve::CubicHermitCurve(Vec2 const& startPoint, Vec2 const startingVelocity, Vec2 const& endPoint, Vec2 const& endVelocity)
{
	m_points.push_back(startPoint);
	m_points.push_back(startingVelocity);
	m_points.push_back(endPoint);
	m_points.push_back(endVelocity);
}

Vec2 const CubicHermitCurve::EvaluateAtParametric(float parametricZeroToOne) const
{
	BezierCurve sketchyCurve(*this);
	return sketchyCurve.EvaluateAtParametric(parametricZeroToOne);
}

void CubicHermitCurve::AddVertsForControlPoints(std::vector<Vertex_PCU>& verts, Rgba8 const& color, float radius, int sectionsAmount) const
{
	BezierCurve sketchyCurve(*this);
	sketchyCurve.AddVertsForControlPoints(verts, color, radius, sectionsAmount);
}

float CubicHermitCurve::GetApproximateLength(int numSubdivisions) const
{
	BezierCurve sketchyCurve(*this);
	return sketchyCurve.GetApproximateLength(numSubdivisions);
}

Vec2 const CubicHermitCurve::EvaluateAtApproximateDistance(float distanceAlongCurve, int numSubdivisions) const
{
	BezierCurve sketchyCurve(*this);
	return sketchyCurve.EvaluateAtApproximateDistance(distanceAlongCurve, numSubdivisions);
}

Spline2D::Spline2D(std::vector<Vec2> const& points)
{
	m_curves.reserve(points.size());

	for (int pointIndex = 0; pointIndex < points.size() - 1; pointIndex++) {
		size_t pointIndexAsSizeT = (size_t)pointIndex;
		Vec2 startVelocity = Vec2::ZERO;
		if (pointIndex - 1 >= 0) {
			startVelocity = (points[pointIndexAsSizeT + 1] - points[pointIndexAsSizeT - 1]) * 0.5f;
		}

		Vec2 endVelocity = Vec2::ZERO;
		if (pointIndexAsSizeT + 2 < points.size()) {
			endVelocity = (points[pointIndexAsSizeT + 2] - points[pointIndexAsSizeT]) * 0.5f;
		}

		m_curves.emplace_back(points[pointIndexAsSizeT], startVelocity, points[pointIndexAsSizeT + 1], endVelocity);

	}
}

Vec2 const Spline2D::EvaluateAtParametric(float parametricZeroToOne) const
{
	int index = RoundDownToInt(parametricZeroToOne);
	float t = parametricZeroToOne - (float)index;
	if (parametricZeroToOne >= (float)m_curves.size()) return m_curves[m_curves.size() - 1].m_points[2];
	return m_curves[index].EvaluateAtParametric(t);
}

void Spline2D::AddVertsForControlPoints(std::vector<Vertex_PCU>& verts, Rgba8 const& color, float radius, int sectionsAmount) const
{
	Vec2 previousPoint = m_curves[0].m_points[0];
	AddVertsForDisc2D(verts, previousPoint, radius, color, sectionsAmount);

	for (int curveIndex = 0; curveIndex < m_curves.size(); curveIndex++) {
		Vec2 const& currentPoint = m_curves[curveIndex].m_points[0];
		Vec2 currentSpeed = m_curves[curveIndex].m_points[1];
		LineSegment2 currentLine(previousPoint, currentPoint);
		AddVertsForDisc2D(verts, currentPoint, radius, color, sectionsAmount);
		if (curveIndex == m_curves.size() - 1) {
			AddVertsForDisc2D(verts, m_curves[curveIndex].m_points[2], radius, color, sectionsAmount);
		}
		AddVertsForLineSegment2D(verts, currentLine, color, radius);
		if (currentSpeed.GetLengthSquared() > 0.0f) {
			AddVertsForArrow2D(verts, currentPoint, currentPoint + currentSpeed, Rgba8::RED, 0.5f);
		}

		previousPoint = currentPoint;
	}
}

float Spline2D::GetApproximateLength(int numSubdivisions) const
{
	float totalLength = 0.0f;
	for (int curveIndex = 0; curveIndex < m_curves.size(); curveIndex++) {
		totalLength += m_curves[curveIndex].GetApproximateLength(numSubdivisions);
	}
	return totalLength;
}

Vec2 const Spline2D::EvaluateAtApproximateDistance(float distanceAlongCurve, int numSubdivisions) const
{
	float totalLength = 0.0f;
	for (int curveIndex = 0; curveIndex < m_curves.size(); curveIndex++) {
		float curveLength = m_curves[curveIndex].GetApproximateLength(numSubdivisions);
		if (totalLength + curveLength >= distanceAlongCurve) {
			float distanceToCheck = distanceAlongCurve - totalLength;
			return m_curves[curveIndex].EvaluateAtApproximateDistance(distanceToCheck, numSubdivisions);
		}
		totalLength += curveLength;
	}
	return Vec2::ZERO;
}
