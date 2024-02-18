#pragma once
#include "Engine/Math/Vec2.hpp"
#include  <vector>

struct Vertex_PCU;
struct Rgba8;

class CubicHermitCurve;

class BezierCurve {
public:
	BezierCurve(std::vector<Vec2> const& points);
	BezierCurve(CubicHermitCurve const& hermitCurve);
	Vec2 const EvaluateAtParametric(float parametricZeroToOne) const;
	void AddVertsForControlPoints(std::vector<Vertex_PCU>& verts, Rgba8 const& color, float radius = 0.6f, int sectionsAmount = 60) const;
	float GetApproximateLength(int numSubdivisions) const;
	Vec2 const EvaluateAtApproximateDistance(float distanceAlongCurve, int numSubdivisions = 64) const;
private:
	std::vector<Vec2> m_points;
};

class CubicHermitCurve {
	friend class BezierCurve;
	friend class Spline2D;
public:
	CubicHermitCurve(Vec2 const& startPoint, Vec2 const startingVelocity, Vec2 const& endPoint, Vec2 const& endVelocity);
	Vec2 const EvaluateAtParametric(float parametricZeroToOne) const;
	void AddVertsForControlPoints(std::vector<Vertex_PCU>& verts, Rgba8 const& color, float radius = 0.6f, int sectionsAmount = 60) const;
	float GetApproximateLength(int numSubdivisions) const;
	Vec2 const EvaluateAtApproximateDistance(float distanceAlongCurve, int numSubdivisions = 64) const;
private:
	std::vector<Vec2> m_points;
};


class Spline2D {
public:
	Spline2D(std::vector<Vec2> const& points);

	Vec2 const EvaluateAtParametric(float parametricZeroToOne) const;
	void AddVertsForControlPoints(std::vector<Vertex_PCU>& verts, Rgba8 const& color, float radius = 0.6f, int sectionsAmount = 60) const;
	float GetApproximateLength(int numSubdivisions) const;
	Vec2 const EvaluateAtApproximateDistance(float distanceAlongCurve, int numSubdivisions = 64) const;
	int GetAmountOfCurves() const { return (int)m_curves.size(); }

private:
	std::vector<CubicHermitCurve> m_curves;
};


float ComputeCubicBezier1D(float a, float b, float c, float d, float t);
Vec2 const ComputeCubicBezier2D(Vec2 const& a, Vec2 const& b, Vec2 const& c, Vec2 const& d, float t);

float ComputeQuinticBezier1D(float a, float b, float c, float d,float e, float f, float t);
Vec2 const ComputeQuinticBezier2D(Vec2 const& a, Vec2 const& b, Vec2 const& c, Vec2 const& d, Vec2 const& e, Vec2 const& f,float t);
