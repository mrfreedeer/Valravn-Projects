#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/LineSegment2.hpp"
#include "Engine/Math/Capsule2.hpp"
#include "Engine/Math/Plane2D.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/ConvexHull2D.hpp"
#include "Engine/Math/ConvexPoly2D.hpp"

void TransformVertexArrayXY3D(int numVerts, Vertex_PCU* verts, float uniformScaleXY, float rotationDegreesAboutZ, Vec2 const& translationXY)
{
	for (int vertIndex = 0; vertIndex < numVerts; vertIndex++) {
		Vertex_PCU& vertex = verts[vertIndex];
		TransformPositionXY3D(vertex.m_position, uniformScaleXY, rotationDegreesAboutZ, translationXY);
	}
}

void TransformVertexArrayXY3D(int numVerts, Vertex_PCU* verts, Vec2 const& iBasis, Vec2 const& jBasis, Vec2 const& translationXY)
{
	for (int vertIndex = 0; vertIndex < numVerts; vertIndex++) {
		Vertex_PCU& vertex = verts[vertIndex];
		TransformPositionXY3D(vertex.m_position, iBasis, jBasis, translationXY);
	}
}

void AddVertsForAABB2D(std::vector<Vertex_PCU>& verts, AABB2 const& bounds, Rgba8 const& tint, AABB2 UVs)
{
	verts.emplace_back(Vec3(bounds.m_mins.x, bounds.m_mins.y, 0.0f), tint, Vec2(UVs.m_mins.x, UVs.m_mins.y));
	verts.emplace_back(Vec3(bounds.m_maxs.x, bounds.m_mins.y, 0.0f), tint, Vec2(UVs.m_maxs.x, UVs.m_mins.y));
	verts.emplace_back(Vec3(bounds.m_maxs.x, bounds.m_maxs.y, 0.0f), tint, Vec2(UVs.m_maxs.x, UVs.m_maxs.y));

	verts.emplace_back(Vec3(bounds.m_mins.x, bounds.m_mins.y, 0.0f), tint, Vec2(UVs.m_mins.x, UVs.m_mins.y));
	verts.emplace_back(Vec3(bounds.m_maxs.x, bounds.m_maxs.y, 0.0f), tint, Vec2(UVs.m_maxs.x, UVs.m_maxs.y));
	verts.emplace_back(Vec3(bounds.m_mins.x, bounds.m_maxs.y, 0.0f), tint, Vec2(UVs.m_mins.x, UVs.m_maxs.y));
}

//------------------------------------------------------------------------------------------------
void AddVertsForAABB2D(std::vector<Vertex_PCU>& mesh, const AABB2& bounds, const Rgba8& tint, const Vec2& uvAtMins, const Vec2& uvAtMaxs)
{
	Vec3 pos0(bounds.m_mins.x, bounds.m_mins.y, 0.f);
	Vec3 pos1(bounds.m_maxs.x, bounds.m_mins.y, 0.f);
	Vec3 pos2(bounds.m_maxs.x, bounds.m_maxs.y, 0.f);
	Vec3 pos3(bounds.m_mins.x, bounds.m_maxs.y, 0.f);

	mesh.emplace_back(pos0, tint, Vec2(uvAtMins.x, uvAtMins.y));
	mesh.emplace_back(pos1, tint, Vec2(uvAtMaxs.x, uvAtMins.y));
	mesh.emplace_back(pos2, tint, Vec2(uvAtMaxs.x, uvAtMaxs.y));

	mesh.emplace_back(pos0, tint, Vec2(uvAtMins.x, uvAtMins.y));
	mesh.emplace_back(pos2, tint, Vec2(uvAtMaxs.x, uvAtMaxs.y));
	mesh.emplace_back(pos3, tint, Vec2(uvAtMins.x, uvAtMaxs.y));
}


void AddVertsForHollowAABB2D(std::vector<Vertex_PCU>& verts, AABB2 const& bounds, float radius, Rgba8 const& tint)
{
	float halfRadius = radius * 0.5f;

	Vec3 innerLeftBottom(bounds.m_mins.x + halfRadius, bounds.m_mins.y + halfRadius, 0.f);
	Vec3 innerRightBottom(bounds.m_maxs.x - halfRadius, bounds.m_mins.y + halfRadius, 0.f);
	Vec3 innerRightTop(bounds.m_maxs.x - halfRadius, bounds.m_maxs.y - halfRadius, 0.f);
	Vec3 innerLeftTop(bounds.m_mins.x + halfRadius, bounds.m_maxs.y - halfRadius, 0.f);

	Vec3 outerLeftBottom(bounds.m_mins.x - halfRadius, bounds.m_mins.y - halfRadius, 0.f);
	Vec3 outerRightBottom(bounds.m_maxs.x + halfRadius, bounds.m_mins.y - halfRadius, 0.f);
	Vec3 outerRightTop(bounds.m_maxs.x + halfRadius, bounds.m_maxs.y + halfRadius, 0.f);
	Vec3 outerLeftTop(bounds.m_mins.x - halfRadius, bounds.m_maxs.y + halfRadius, 0.f);

	// Left frame Side
	verts.emplace_back(outerLeftBottom, tint, Vec2::ZERO);
	verts.emplace_back(innerLeftBottom, tint, Vec2::ZERO);
	verts.emplace_back(innerLeftTop, tint, Vec2::ZERO);

	verts.emplace_back(outerLeftBottom, tint, Vec2::ZERO);
	verts.emplace_back(innerLeftTop, tint, Vec2::ZERO);
	verts.emplace_back(outerLeftTop, tint, Vec2::ZERO);

	// Right Frame Side
	verts.emplace_back(outerRightBottom, tint, Vec2::ZERO);
	verts.emplace_back(innerRightBottom, tint, Vec2::ZERO);
	verts.emplace_back(innerRightTop, tint, Vec2::ZERO);

	verts.emplace_back(outerRightBottom, tint, Vec2::ZERO);
	verts.emplace_back(innerRightTop, tint, Vec2::ZERO);
	verts.emplace_back(outerRightTop, tint, Vec2::ZERO);

	// Top Frame Side
	verts.emplace_back(innerLeftTop, tint, Vec2::ZERO);
	verts.emplace_back(outerRightTop, tint, Vec2::ZERO);
	verts.emplace_back(outerLeftTop, tint, Vec2::ZERO);

	verts.emplace_back(innerLeftTop, tint, Vec2::ZERO);
	verts.emplace_back(innerRightTop, tint, Vec2::ZERO);
	verts.emplace_back(outerRightTop, tint, Vec2::ZERO);

	// Bottom Frame Side
	verts.emplace_back(innerLeftBottom, tint, Vec2::ZERO);
	verts.emplace_back(outerRightBottom, tint, Vec2::ZERO);
	verts.emplace_back(outerLeftBottom, tint, Vec2::ZERO);

	verts.emplace_back(innerLeftBottom, tint, Vec2::ZERO);
	verts.emplace_back(innerRightBottom, tint, Vec2::ZERO);
	verts.emplace_back(outerRightBottom, tint, Vec2::ZERO);
}


void AddVertsForOBB2D(std::vector<Vertex_PCU>& verts, OBB2 const& bounds, Rgba8 const& tint, const Vec2& uvAtMins, const Vec2& uvAtMaxs)
{
	Vec2* cornerPoints = bounds.GetCornerPoints();

	Vec2& bottomLeftCorner = cornerPoints[0];
	Vec2& bottomRightCorner = cornerPoints[1];
	Vec2& topRightCorner = cornerPoints[2];
	Vec2& topLeftCorner = cornerPoints[3];

	verts.emplace_back(Vec3(bottomLeftCorner.x, bottomLeftCorner.y, 0.0f), tint, uvAtMins);
	verts.emplace_back(Vec3(bottomRightCorner.x, bottomRightCorner.y, 0.0f), tint, Vec2(uvAtMaxs.x, uvAtMins.y));
	verts.emplace_back(Vec3(topRightCorner.x, topRightCorner.y, 0.0f), tint, uvAtMaxs);

	verts.emplace_back(Vec3(bottomLeftCorner.x, bottomLeftCorner.y, 0.0f), tint, uvAtMins);
	verts.emplace_back(Vec3(topRightCorner.x, topRightCorner.y, 0.0f), tint, uvAtMaxs);
	verts.emplace_back(Vec3(topLeftCorner.x, topLeftCorner.y, 0.0f), tint, Vec2(uvAtMins.x, uvAtMaxs.y));

	delete[] cornerPoints;
}

void AddVertsForOBB2D(std::vector<Vertex_PCU>& verts, OBB2 const& bounds, Rgba8 const& tint, AABB2 UVs)
{
	Vec2* cornerPoints = bounds.GetCornerPoints();

	Vec2& bottomLeftCorner = cornerPoints[0];
	Vec2& bottomRightCorner = cornerPoints[1];
	Vec2& topRightCorner = cornerPoints[2];
	Vec2& topLeftCorner = cornerPoints[3];

	verts.emplace_back(Vec3(bottomLeftCorner.x, bottomLeftCorner.y, 0.0f), tint, UVs.m_mins);
	verts.emplace_back(Vec3(bottomRightCorner.x, bottomRightCorner.y, 0.0f), tint, Vec2(UVs.m_maxs.x, UVs.m_mins.y));
	verts.emplace_back(Vec3(topRightCorner.x, topRightCorner.y, 0.0f), tint, UVs.m_maxs);

	verts.emplace_back(Vec3(bottomLeftCorner.x, bottomLeftCorner.y, 0.0f), tint, UVs.m_mins);
	verts.emplace_back(Vec3(topRightCorner.x, topRightCorner.y, 0.0f), tint, UVs.m_maxs);
	verts.emplace_back(Vec3(topLeftCorner.x, topLeftCorner.y, 0.0f), tint, Vec2(UVs.m_mins.x, UVs.m_maxs.y));

	delete[] cornerPoints;
}


void AddVertsForDisc2D(std::vector<Vertex_PCU>& verts, Vec2 const& discCenter, float radius, Rgba8 tint, int sectionsAmount)
{

	float deltaAngle = 360.0f / static_cast<float>(sectionsAmount);

	float prevAngleCos = CosDegrees(0);
	float prevAngleSin = SinDegrees(0);

	for (float angle = deltaAngle; angle <= 360.0f; angle += deltaAngle) {
		float currentAngleCos = CosDegrees(angle);
		float currentAngleSin = SinDegrees(angle);

		Vec2 localTop(prevAngleCos, prevAngleSin);
		Vec2 halfUVs(0.5f, 0.5f);
		localTop *= radius;

		Vec2 localLeftTop(currentAngleCos, currentAngleSin);

		localLeftTop *= radius;

		Vec3 worldBottom(discCenter.x, discCenter.y, 0);
		Vec3 worldTop(localTop.x + discCenter.x, localTop.y + discCenter.y, 0);
		Vec3 worldLeftTop(localLeftTop.x + discCenter.x, localLeftTop.y + discCenter.y, 0);

		verts.emplace_back(worldBottom, tint, halfUVs);
		verts.emplace_back(worldTop, tint, halfUVs);
		verts.emplace_back(worldLeftTop, tint, localTop * 0.5f);

		prevAngleCos = currentAngleCos;
		prevAngleSin = currentAngleSin;
	}
}

void AddVertsForLineSegment2D(std::vector<Vertex_PCU>& verts, LineSegment2 const& lineSegment, Rgba8 tint, float lineWidth, bool overhead)
{
	Vec2 lineDir = (lineSegment.m_end - lineSegment.m_start).GetNormalized();
	lineDir *= lineWidth * 0.5f;


	Vec2 lineLeftDir = lineDir.GetRotated90Degrees();
	lineLeftDir *= lineWidth * 0.5f;

	if (!overhead) lineDir = Vec2::ZERO;

	Vec2 lineBottomLeft = lineSegment.m_start - lineDir + lineLeftDir;
	Vec2 lineBottomRight = lineSegment.m_start - lineDir - lineLeftDir;

	Vec2 lineTopLeft = lineSegment.m_end + lineDir + lineLeftDir;
	Vec2 lineTopRight = lineSegment.m_end + lineDir - lineLeftDir;


	verts.emplace_back(Vec3(lineBottomLeft.x, lineBottomLeft.y, 0.0f), tint, Vec2());
	verts.emplace_back(Vec3(lineTopRight.x, lineTopRight.y, 0.0f), tint, Vec2());
	verts.emplace_back(Vec3(lineTopLeft.x, lineTopLeft.y, 0.0f), tint, Vec2());

	verts.emplace_back(Vec3(lineBottomLeft.x, lineBottomLeft.y, 0.0f), tint, Vec2());
	verts.emplace_back(Vec3(lineBottomRight.x, lineBottomRight.y, 0.0f), tint, Vec2());
	verts.emplace_back(Vec3(lineTopRight.x, lineTopRight.y, 0.0f), tint, Vec2());

}

void AddVertsForCapsule2D(std::vector<Vertex_PCU>& verts, Capsule2 const& capsule, Rgba8 tint, int amountOfTriangles)
{
	Vec2 lineLeftDir = (capsule.m_bone.m_end - capsule.m_bone.m_start).GetRotated90Degrees();
	lineLeftDir.SetLength(capsule.m_radius);

	Vec2 capsBottomLeft = capsule.m_bone.m_start + lineLeftDir;
	Vec2 capsBottomRight = capsule.m_bone.m_start - lineLeftDir;

	Vec2 capsTopLeft = capsule.m_bone.m_end + lineLeftDir;
	Vec2 capsTopRight = capsule.m_bone.m_end - lineLeftDir;

	verts.emplace_back(Vec3(capsBottomLeft), tint, Vec2());
	verts.emplace_back(Vec3(capsBottomRight), tint, Vec2());
	verts.emplace_back(Vec3(capsTopRight), tint, Vec2());

	verts.emplace_back(Vec3(capsBottomLeft), tint, Vec2());
	verts.emplace_back(Vec3(capsTopRight), tint, Vec2());
	verts.emplace_back(Vec3(capsTopLeft), tint, Vec2());

	float deltaDeg = 180.0f / amountOfTriangles;

	float startingOrientation = lineLeftDir.GetOrientationDegrees();
	float endingOrientation = startingOrientation + 180.0f;
	for (int sectorIndex = 0; sectorIndex < amountOfTriangles; sectorIndex++) {
		float cosCurrentSector = CosDegrees(startingOrientation + deltaDeg * static_cast<float>(sectorIndex));
		float sinCurrentSector = SinDegrees(startingOrientation + deltaDeg * static_cast<float>(sectorIndex));

		float cosNextSector = 0.0f;
		float sinNextSector = 0.0f;

		if (sectorIndex == amountOfTriangles - 1) {
			cosNextSector = CosDegrees(endingOrientation);
			sinNextSector = SinDegrees(endingOrientation);
		}
		else {
			cosNextSector = CosDegrees(startingOrientation + deltaDeg * static_cast<float>(sectorIndex + 1));
			sinNextSector = SinDegrees(startingOrientation + deltaDeg * static_cast<float>(sectorIndex + 1));
		}


		Vec2 topSector(cosCurrentSector, sinCurrentSector);
		Vec2 topLeftSector(cosNextSector, sinNextSector);

		topLeftSector *= capsule.m_radius;
		topSector *= capsule.m_radius;

		topLeftSector += capsule.m_bone.m_start;
		topSector += capsule.m_bone.m_start;

		Vec3 worldTopSector(topSector);
		Vec3 worldTopLeftSector(topLeftSector);
		Vec3 capsuleVec3Start(capsule.m_bone.m_start);

		verts.emplace_back(capsuleVec3Start, tint, Vec2());
		verts.emplace_back(worldTopSector, tint, Vec2());
		verts.emplace_back(worldTopLeftSector, tint, Vec2());
	}

	float tempStartOrientation = startingOrientation;
	startingOrientation = endingOrientation;
	endingOrientation = tempStartOrientation;

	for (int sectorIndex = 0; sectorIndex < amountOfTriangles; sectorIndex++) {
		float cosCurrentSector = CosDegrees(startingOrientation + deltaDeg * static_cast<float>(sectorIndex));
		float sinCurrentSector = SinDegrees(startingOrientation + deltaDeg * static_cast<float>(sectorIndex));

		float cosNextSector = 0.0f;
		float sinNextSector = 0.0f;

		if (sectorIndex == amountOfTriangles - 1) {
			cosNextSector = CosDegrees(endingOrientation);
			sinNextSector = SinDegrees(endingOrientation);
		}
		else {
			cosNextSector = CosDegrees(startingOrientation + deltaDeg * static_cast<float>(sectorIndex + 1));
			sinNextSector = SinDegrees(startingOrientation + deltaDeg * static_cast<float>(sectorIndex + 1));
		}


		Vec2 topSector(cosCurrentSector, sinCurrentSector);
		Vec2 topLeftSector(cosNextSector, sinNextSector);

		topLeftSector *= capsule.m_radius;
		topSector *= capsule.m_radius;

		topLeftSector += capsule.m_bone.m_end;
		topSector += capsule.m_bone.m_end;

		Vec3 worldTopSector(topSector);
		Vec3 worldTopLeftSector(topLeftSector);
		Vec3 capsuleVec3End(capsule.m_bone.m_end);

		verts.emplace_back(capsuleVec3End, tint, Vec2());
		verts.emplace_back(worldTopSector, tint, Vec2());
		verts.emplace_back(worldTopLeftSector, tint, Vec2());
	}

}

void AddVertsForArrow2D(std::vector<Vertex_PCU>& verts, Vec2 const& arrowStart, Vec2 const& arrowEnd, Rgba8 color, float arrowBodySize, float arrowHeadSize)
{
	Vec2 arrowFwd = (arrowStart != arrowEnd) ? (arrowEnd - arrowStart).GetNormalized() : Vec2::ZERO;
	LineSegment2 arrowLineSegment(arrowStart, arrowEnd - (arrowFwd * arrowHeadSize));
	Vec2 arrowLeft = arrowFwd.GetRotated90Degrees();

	Vec3 arrowLeftVertexPos(arrowEnd + (arrowLeft * 0.5f * arrowHeadSize) - (arrowFwd * arrowHeadSize));
	Vec3 arrowRightVertexPos(arrowEnd - arrowLeft * 0.5f * arrowHeadSize - (arrowFwd * arrowHeadSize));
	Vec3 arrowTopVertexPos(arrowEnd);

	AddVertsForLineSegment2D(verts, arrowLineSegment, color, arrowBodySize);
	verts.emplace_back(arrowLeftVertexPos, color, Vec2());
	verts.emplace_back(arrowRightVertexPos, color, Vec2());
	verts.emplace_back(arrowTopVertexPos, color, Vec2());

}

void AddVertsForDelaunayConvexPoly2D(std::vector<Vertex_PCU>& verts, DelaunayConvexPoly2D const& convexPoly, Rgba8 const& color, AABB2 const& UVs)
{
	Vec2 const& middle = convexPoly.GetMiddlePoint();
	AABB2 enclosingQuad = convexPoly.GetEnclosingAABB2();
	Vec2 UVsAtMiddle = enclosingQuad.GetUVForPoint(middle);
	Vec2 UVsForMiddleVertex = UVs.GetPointAtUV(UVsAtMiddle);


	Vec2 prevPolyVert = convexPoly.m_vertexes[0];
	Vec2 UvsAtPrevPoly = enclosingQuad.GetUVForPoint(prevPolyVert);
	Vec2 UVsForPrevVertex = UVs.GetPointAtUV(UvsAtPrevPoly);


	Vec2 firstPolyVert = prevPolyVert;
	Vec2 firtPolyVertUVs = UVsForPrevVertex;
	for (int polyVertexIndex = 1; polyVertexIndex < convexPoly.m_vertexes.size(); polyVertexIndex++) {
		Vec2 const& polyVertex = convexPoly.m_vertexes[polyVertexIndex];
		Vec2 UVsAtVertex = enclosingQuad.GetUVForPoint(polyVertex);

		Vec2 UVsForVertex = UVs.GetPointAtUV(UVsAtVertex);

		verts.emplace_back(Vec3(prevPolyVert), color, UVsForPrevVertex);
		verts.emplace_back(Vec3(polyVertex), color, UVsForVertex);
		verts.emplace_back(Vec3(middle), color, UVsForMiddleVertex);

		prevPolyVert = polyVertex;
		UVsForPrevVertex = UVsForVertex;

		if (polyVertexIndex == convexPoly.m_vertexes.size() - 1) {
			verts.emplace_back(Vec3(polyVertex), color, UVsForVertex);
			verts.emplace_back(Vec3(firstPolyVert), color, firtPolyVertUVs);
			verts.emplace_back(Vec3(middle), color, UVsForMiddleVertex);

		}
	}

}

void AddVertsForWireDelaunayConvexPoly2D(std::vector<Vertex_PCU>& verts, DelaunayConvexPoly2D const& convexPoly, Rgba8 const& color, float lineThickness)
{
	LineSegment2 lastLineSegment(convexPoly.m_vertexes[convexPoly.m_vertexes.size() - 1], convexPoly.m_vertexes[0]);
	AddVertsForLineSegment2D(verts, lastLineSegment, color, lineThickness, false);

	Vec2 prevPoint = convexPoly.m_vertexes[0];
	for (int polyVertexIndex = 1; polyVertexIndex < convexPoly.m_vertexes.size(); polyVertexIndex++) {
		Vec2 const& polyVertex = convexPoly.m_vertexes[polyVertexIndex];

		LineSegment2 lineSegment(prevPoint, polyVertex);
		AddVertsForLineSegment2D(verts, lineSegment, color, lineThickness, false);
		prevPoint = polyVertex;
	}

}

void AddVertsForConvexPoly2D(std::vector<Vertex_PCU>& verts, ConvexPoly2D const& convexPoly, Rgba8 const& color)
{
	Vec2 middlePoint = convexPoly.GetCenter();
	Vertex_PCU middleVertex;
	middleVertex.m_color = color;
	middleVertex.m_position = Vec3(middlePoint);

	// For the insides
	for (int pointIndex = 0; pointIndex + 1 < (int)convexPoly.m_ccwPoints.size(); pointIndex++) {
		Vec2 const& lineStart = convexPoly.m_ccwPoints[pointIndex];
		int nextIndex = pointIndex + 1;
		Vec2 const& lineEnd = convexPoly.m_ccwPoints[nextIndex];

		verts.emplace_back(middleVertex);
		verts.emplace_back(Vec3(lineStart), color, Vec2::ZERO);
		verts.emplace_back(Vec3(lineEnd), color, Vec2::ZERO);
	}

	Vec2 const& lastStart = convexPoly.m_ccwPoints[convexPoly.m_ccwPoints.size() - 1];
	Vec2 const& lastEnd = convexPoly.m_ccwPoints[0];

	verts.emplace_back(middleVertex);
	verts.emplace_back(Vec3(lastStart), color, Vec2::ZERO);
	verts.emplace_back(Vec3(lastEnd), color, Vec2::ZERO);
}

void AddVertsForWireConvexPoly2D(std::vector<Vertex_PCU>& verts, ConvexPoly2D const& convexPoly, Rgba8 const& borderColor, float lineThickness)
{
	for (int pointIndex = 0; pointIndex + 1 < (int)convexPoly.m_ccwPoints.size(); pointIndex++) {
		Vec2 const& lineStart = convexPoly.m_ccwPoints[pointIndex];
		int nextIndex = pointIndex + 1;
		Vec2 const& lineEnd = convexPoly.m_ccwPoints[nextIndex];
		AddVertsForLineSegment2D(verts, LineSegment2(lineStart, lineEnd), borderColor, lineThickness, false);
	}
	Vec2 const& lastStart = convexPoly.m_ccwPoints[convexPoly.m_ccwPoints.size() - 1];
	Vec2 const& lastEnd = convexPoly.m_ccwPoints[0];
	AddVertsForLineSegment2D(verts, LineSegment2(lastStart, lastEnd), borderColor, lineThickness,false);
}

void AddVertsForConvexHull2D(std::vector<Vertex_PCU>& verts, ConvexHull2D const& convexHull, Rgba8 const& color, float planeDrawDistance, float lineThickness)
{
	for (Plane2D const& plane : convexHull.m_planes) {
		Vec2 middlePoint = plane.m_planeNormal * plane.m_distToPlane;
		Vec2 lineStart = middlePoint + plane.m_planeNormal.GetRotated90Degrees()* planeDrawDistance;
		Vec2 lineEnd = middlePoint - plane.m_planeNormal.GetRotated90Degrees()* planeDrawDistance;

		AddVertsForLineSegment2D(verts, LineSegment2(lineStart, lineEnd), color, lineThickness);
	}

}

void TransformVertexArray3D(int numVerts, Vertex_PCU* verts, Mat44 const& model)
{
	for (int vertIndex = 0; vertIndex < numVerts; vertIndex++) {
		Vertex_PCU& vertex = verts[vertIndex];
		TransformPosition3D(vertex.m_position, model);
	}
}

void AddVertsForLineSegment3D(std::vector<Vertex_PCU>& verts, Vec3 const& start, Vec3 const& end, Rgba8 const& color, float thickness)
{
	Vec3 kBasis = (end - start).GetNormalized();
	Vec3 worldIBasis = Vec3(1.0, 0.0f, 0.0f);
	Vec3 worldJBasis = Vec3(0.0f, 1.0, 0.0f);

	Vec3 jBasis, iBasis;

	if (fabsf(DotProduct3D(kBasis, worldIBasis)) < 1.0f) {
		jBasis = CrossProduct3D(kBasis, worldIBasis).GetNormalized();
		iBasis = CrossProduct3D(jBasis, kBasis).GetNormalized();
	}
	else {
		jBasis = CrossProduct3D(kBasis, worldJBasis).GetNormalized();
		iBasis = CrossProduct3D(jBasis, kBasis).GetNormalized();
	}

	kBasis *= thickness;
	jBasis *= thickness;
	iBasis *= thickness;

	Vec3 const& rightFrontBottom = start - iBasis - jBasis - kBasis;
	Vec3 const& rightBackBottom = start + iBasis - jBasis - kBasis;
	Vec3 const& rightBackTop = end + iBasis - jBasis + kBasis;
	Vec3 const& rightFrontTop = end - iBasis - jBasis + kBasis;

	Vec3 const& leftFrontBottom = start - iBasis + jBasis - kBasis;
	Vec3 const& leftBackBottom = start + iBasis + jBasis - kBasis;
	Vec3 const& leftBackTop = end + iBasis + jBasis + kBasis;
	Vec3 const& leftFrontTop = end - iBasis + jBasis + kBasis;

	AddVertsForQuad3D(verts, leftFrontBottom, rightFrontBottom, rightFrontTop, leftFrontTop, color); // Front
	AddVertsForQuad3D(verts, rightFrontBottom, rightBackBottom, rightBackTop, rightFrontTop, color); // Right
	AddVertsForQuad3D(verts, rightBackBottom, leftBackBottom, leftBackTop, rightBackTop, color); // Back
	AddVertsForQuad3D(verts, leftBackBottom, leftFrontBottom, leftFrontTop, leftBackTop, color); // Left
	AddVertsForQuad3D(verts, leftFrontTop, rightFrontTop, rightBackTop, leftBackTop, color); // Top
	AddVertsForQuad3D(verts, leftBackBottom, rightBackBottom, rightFrontBottom, leftFrontBottom, color); // Bottom;

}

void AddVertsForIndexedQuad3D(std::vector<Vertex_PNCU>& verts, std::vector<unsigned int>& indices, Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft, const Rgba8& color, const AABB2& UVs)
{
	Vec3 kBasis = (topRight - bottomRight).GetNormalized();
	Vec3 jBasis = (topLeft - topRight).GetNormalized();
	Vec3 quadNormal = CrossProduct3D(kBasis, jBasis);

	int zero = (int)verts.size();
	int one = zero + 1;
	int two = zero + 2;
	int three = zero + 3;

	verts.emplace_back(bottomLeft, quadNormal, color, UVs.m_mins);
	verts.emplace_back(bottomRight, quadNormal, color, Vec2(UVs.m_maxs.x, UVs.m_mins.y));
	verts.emplace_back(topLeft, quadNormal, color, Vec2(UVs.m_mins.x, UVs.m_maxs.y));
	verts.emplace_back(topRight, quadNormal, color, UVs.m_maxs);


	indices.push_back(zero);
	indices.push_back(one);
	indices.push_back(three);

	indices.push_back(zero);
	indices.push_back(three);
	indices.push_back(two);
}

void AddVertsForIndexedQuad3D(std::vector<Vertex_PCU>& verts, std::vector<unsigned int>& indices, Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft, const Rgba8& color, const AABB2& UVs)
{
	int zero = (int)verts.size();
	int one = zero + 1;
	int two = zero + 2;
	int three = zero + 3;

	verts.emplace_back(bottomLeft, color, UVs.m_mins);
	verts.emplace_back(bottomRight, color, Vec2(UVs.m_maxs.x, UVs.m_mins.y));
	verts.emplace_back(topLeft, color, Vec2(UVs.m_mins.x, UVs.m_maxs.y));
	verts.emplace_back(topRight, color, UVs.m_maxs);


	indices.push_back(zero);
	indices.push_back(one);
	indices.push_back(three);

	indices.push_back(zero);
	indices.push_back(three);
	indices.push_back(two);
}

void AddVertsForQuad3D(std::vector<Vertex_PCU>& verts, Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft, const Rgba8& color, const AABB2& UVs)
{
	// Right Quad
	verts.emplace_back(bottomLeft, color, UVs.m_mins);
	verts.emplace_back(bottomRight, color, Vec2(UVs.m_maxs.x, UVs.m_mins.y));
	verts.emplace_back(topRight, color, UVs.m_maxs);

	// Left Quad
	verts.emplace_back(bottomLeft, color, UVs.m_mins);
	verts.emplace_back(topRight, color, UVs.m_maxs);
	verts.emplace_back(topLeft, color, Vec2(UVs.m_mins.x, UVs.m_maxs.y));
}

void AddVertsForQuad3D(std::vector<Vertex_PNCU>& verts, Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft, const Rgba8& color, const AABB2& UVs)
{
	Vec3 kBasis = (topRight - bottomRight).GetNormalized();
	Vec3 jBasis = (topLeft - topRight).GetNormalized();
	Vec3 quadNormal = CrossProduct3D(kBasis, jBasis);

	// Right Quad
	verts.emplace_back(bottomLeft, quadNormal, color, UVs.m_mins);
	verts.emplace_back(bottomRight, quadNormal, color, Vec2(UVs.m_maxs.x, UVs.m_mins.y));
	verts.emplace_back(topRight, quadNormal, color, UVs.m_maxs);

	// Left Quad
	verts.emplace_back(bottomLeft, quadNormal, color, UVs.m_mins);
	verts.emplace_back(topRight, quadNormal, color, UVs.m_maxs);
	verts.emplace_back(topLeft, quadNormal, color, Vec2(UVs.m_mins.x, UVs.m_maxs.y));
}
void AddVertsForRoundedQuad3D(std::vector<Vertex_PNCU>& verts, Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft, const Rgba8& color, const AABB2& UVs)
{
	Vec3 dispBetweenEdges = topRight - topLeft;
	Vec3 kBasis = (topRight - bottomRight).GetNormalized();
	Vec3 jBasis = (-dispBetweenEdges).GetNormalized();
	Vec3 quadNormal = CrossProduct3D(kBasis, jBasis);

	Vec3 halfDisp = dispBetweenEdges * 0.5f;

	Vec3 topMiddle = topLeft + (halfDisp);
	Vec3 bottomMiddle = bottomLeft + (halfDisp);

	Vec2 bottomMiddleUVs = UVs.GetPointAtUV(0.5f, 0.0f);
	Vec2 topMiddleUVs = UVs.GetPointAtUV(0.5f, 1.0f);


	// Left 
	verts.emplace_back(bottomLeft, jBasis, color, UVs.m_mins);
	verts.emplace_back(bottomMiddle, quadNormal, color, bottomMiddleUVs);
	verts.emplace_back(topMiddle, quadNormal, color, topMiddleUVs);

	verts.emplace_back(bottomLeft, jBasis, color, UVs.m_mins);
	verts.emplace_back(topMiddle, quadNormal, color, topMiddleUVs);
	verts.emplace_back(topLeft, jBasis, color, Vec2(UVs.m_mins.x, UVs.m_maxs.y));

	// Right
	verts.emplace_back(bottomMiddle, quadNormal, color, bottomMiddleUVs);
	verts.emplace_back(bottomRight, -jBasis, color, Vec2(UVs.m_maxs.x, UVs.m_mins.y));
	verts.emplace_back(topRight, -jBasis, color, UVs.m_maxs);

	verts.emplace_back(bottomMiddle, quadNormal, color, bottomMiddleUVs);
	verts.emplace_back(topRight, -jBasis, color, UVs.m_maxs);
	verts.emplace_back(topMiddle, quadNormal, color, topMiddleUVs);
}
void AddVertsForIndexedAABB3D(std::vector<Vertex_PCU>& verts, std::vector<unsigned int>& indices, const AABB3& bounds, const Rgba8& color, const AABB2& UVs)
{
	Vec3 corners[8];
	bounds.GetCorners(corners);

	Vec3 const& rightFrontBottom = corners[0];
	Vec3 const& rightBackBottom = corners[1];
	Vec3 const& rightBackTop = corners[2];
	Vec3 const& rightFrontTop = corners[3];
	Vec3 const& leftFrontBottom = corners[4];
	Vec3 const& leftBackBottom = corners[5];
	Vec3 const& leftBackTop = corners[6];
	Vec3 const& leftFrontTop = corners[7];

	AddVertsForIndexedQuad3D(verts, indices, leftFrontBottom, rightFrontBottom, rightFrontTop, leftFrontTop, color, UVs); // Front
	AddVertsForIndexedQuad3D(verts, indices, rightFrontBottom, rightBackBottom, rightBackTop, rightFrontTop, color, UVs); // Right
	AddVertsForIndexedQuad3D(verts, indices, rightBackBottom, leftBackBottom, leftBackTop, rightBackTop, color, UVs); // Back
	AddVertsForIndexedQuad3D(verts, indices, leftBackBottom, leftFrontBottom, leftFrontTop, leftBackTop, color, UVs); // Left
	AddVertsForIndexedQuad3D(verts, indices, rightFrontTop, rightBackTop, leftBackTop, leftFrontTop, color, UVs); // Top
	AddVertsForIndexedQuad3D(verts, indices, leftFrontBottom, leftBackBottom, rightBackBottom, rightFrontBottom, color, UVs); // Bottom;
}

void AddVertsForAABB3D(std::vector<Vertex_PCU>& verts, const AABB3& bounds, const Rgba8& color, const AABB2& UVs)
{
	Vec3 corners[8];
	bounds.GetCorners(corners);

	Vec3 const& rightFrontBottom = corners[0];
	Vec3 const& rightBackBottom = corners[1];
	Vec3 const& rightBackTop = corners[2];
	Vec3 const& rightFrontTop = corners[3];
	Vec3 const& leftFrontBottom = corners[4];
	Vec3 const& leftBackBottom = corners[5];
	Vec3 const& leftBackTop = corners[6];
	Vec3 const& leftFrontTop = corners[7];

	AddVertsForQuad3D(verts, leftFrontBottom, rightFrontBottom, rightFrontTop, leftFrontTop, color, UVs); // Front
	AddVertsForQuad3D(verts, rightFrontBottom, rightBackBottom, rightBackTop, rightFrontTop, color, UVs); // Right
	AddVertsForQuad3D(verts, rightBackBottom, leftBackBottom, leftBackTop, rightBackTop, color, UVs); // Back
	AddVertsForQuad3D(verts, leftBackBottom, leftFrontBottom, leftFrontTop, leftBackTop, color, UVs); // Left
	AddVertsForQuad3D(verts, rightFrontTop, rightBackTop, leftBackTop, leftFrontTop, color, UVs); // Top
	AddVertsForQuad3D(verts, leftFrontBottom, leftBackBottom, rightBackBottom, rightFrontBottom, color, UVs); // Bottom;

}

void AddVertsForAABB3D(std::vector<Vertex_PNCU>& verts, const AABB3& bounds, const Rgba8& color, const AABB2& UVs)
{
	Vec3 corners[8];
	bounds.GetCorners(corners);

	Vec3 const& rightFrontBottom = corners[0];
	Vec3 const& rightBackBottom = corners[1];
	Vec3 const& rightBackTop = corners[2];
	Vec3 const& rightFrontTop = corners[3];
	Vec3 const& leftFrontBottom = corners[4];
	Vec3 const& leftBackBottom = corners[5];
	Vec3 const& leftBackTop = corners[6];
	Vec3 const& leftFrontTop = corners[7];

	AddVertsForQuad3D(verts, leftFrontBottom, rightFrontBottom, rightFrontTop, leftFrontTop, color, UVs); // Front
	AddVertsForQuad3D(verts, rightFrontBottom, rightBackBottom, rightBackTop, rightFrontTop, color, UVs); // Right
	AddVertsForQuad3D(verts, rightBackBottom, leftBackBottom, leftBackTop, rightBackTop, color, UVs); // Back
	AddVertsForQuad3D(verts, leftBackBottom, leftFrontBottom, leftFrontTop, leftBackTop, color, UVs); // Left
	AddVertsForQuad3D(verts, rightFrontTop, rightBackTop, leftBackTop, leftFrontTop, color, UVs); // Top
	AddVertsForQuad3D(verts, leftFrontBottom, leftBackBottom, rightBackBottom, rightFrontBottom, color, UVs); // Bottom;

}

void AddVertsForWireAABB3D(std::vector<Vertex_PCU>& verts, const AABB3& bounds, const Rgba8& color)
{
	Vec3 corners[8];
	bounds.GetCorners(corners);

	Vec3 const& rightFrontBottom = corners[0];
	Vec3 const& rightBackBottom = corners[1];
	Vec3 const& rightBackTop = corners[2];
	Vec3 const& rightFrontTop = corners[3];
	Vec3 const& leftFrontBottom = corners[4];
	Vec3 const& leftBackBottom = corners[5];
	Vec3 const& leftBackTop = corners[6];
	Vec3 const& leftFrontTop = corners[7];

	AddVertsForLineSegment3D(verts, leftFrontBottom, leftFrontTop, color);
	AddVertsForLineSegment3D(verts, leftFrontBottom, rightFrontBottom, color);
	AddVertsForLineSegment3D(verts, rightFrontBottom, rightFrontTop, color);
	AddVertsForLineSegment3D(verts, leftFrontTop, rightFrontTop, color);

	AddVertsForLineSegment3D(verts, leftBackBottom, leftBackTop, color);
	AddVertsForLineSegment3D(verts, leftBackBottom, rightBackBottom, color);
	AddVertsForLineSegment3D(verts, rightBackBottom, rightBackTop, color);
	AddVertsForLineSegment3D(verts, leftBackTop, rightBackTop, color);

	AddVertsForLineSegment3D(verts, leftFrontBottom, leftBackBottom, color);
	AddVertsForLineSegment3D(verts, leftFrontTop, leftBackTop, color);
	AddVertsForLineSegment3D(verts, rightFrontBottom, rightBackBottom, color);
	AddVertsForLineSegment3D(verts, rightFrontTop, rightBackTop, color);
}

void AddVertsForSphere(std::vector<Vertex_PCU>& verts, float radius, int stacks, int slices, Rgba8 const& color, AABB2 const& UVs)
{
	float yawDegDelta = 360.0f / static_cast<float>(slices);
	float pitchDegDelta = 180.0f / static_cast<float>(stacks);

	float prevCosYaw = -1.0f;
	float prevSinYaw = -1.0f;

	float prevCosPitch = -1.0f;
	float prevSinPitch = -1.0f;

	for (float yaw = yawDegDelta; yaw <= 360.0f; yaw += yawDegDelta) {
		float cosYaw = CosDegrees(yaw);
		float sinYaw = SinDegrees(yaw);

		if (yaw == yawDegDelta) {
			prevCosYaw = CosDegrees(yaw - yawDegDelta);
			prevSinYaw = SinDegrees(yaw - yawDegDelta);
		}

		float leftU = RangeMapClamped(yaw - yawDegDelta, 0.0f, 360.0f, UVs.m_mins.x, UVs.m_maxs.x);
		float rightU = RangeMapClamped(yaw, 0.0f, 360.0f, UVs.m_mins.x, UVs.m_maxs.x);
		for (float pitch = -90.0f + pitchDegDelta; pitch <= 90.0f; pitch += pitchDegDelta) {
			float cosPitch = CosDegrees(pitch);
			float sinPitch = SinDegrees(pitch);

			if (pitch == -90.0f + pitchDegDelta) {
				prevCosPitch = CosDegrees(pitch - pitchDegDelta);
				prevSinPitch = SinDegrees(pitch - pitchDegDelta);
			}

			Vec3 rightBottomPos(cosYaw * cosPitch, sinYaw * cosPitch, -sinPitch);
			Vec3 rightTopPos(cosYaw * prevCosPitch, sinYaw * prevCosPitch, -prevSinPitch);
			Vec3 leftTopPos(prevCosYaw * prevCosPitch, prevSinYaw * prevCosPitch, -prevSinPitch);
			Vec3 leftBottomPos(prevCosYaw * cosPitch, prevSinYaw * cosPitch, -sinPitch);

			rightBottomPos *= radius;
			rightTopPos *= radius;
			leftTopPos *= radius;
			leftBottomPos *= radius;


			float topV = RangeMapClamped(pitch - pitchDegDelta, 90.0f, -90.0f, UVs.m_mins.y, UVs.m_maxs.y);
			float bottomV = RangeMapClamped(pitch, 90.0f, -90.0f, UVs.m_mins.y, UVs.m_maxs.y);

			verts.emplace_back(leftBottomPos, color, Vec2(leftU, bottomV));
			verts.emplace_back(rightBottomPos, color, Vec2(rightU, bottomV));
			verts.emplace_back(rightTopPos, color, Vec2(rightU, topV));

			verts.emplace_back(leftBottomPos, color, Vec2(leftU, bottomV));
			verts.emplace_back(rightTopPos, color, Vec2(rightU, topV));
			verts.emplace_back(leftTopPos, color, Vec2(leftU, topV));

			prevCosPitch = cosPitch;
			prevSinPitch = sinPitch;
		}

		prevCosYaw = cosYaw;
		prevSinYaw = sinYaw;
	}
}


void AddVertsForSphere(std::vector<Vertex_PNCU>& verts, float radius, int stacks, int slices, Rgba8 const& color, AABB2 const& UVs)
{
	float yawDegDelta = 360.0f / static_cast<float>(slices);
	float pitchDegDelta = 180.0f / static_cast<float>(stacks);

	float prevCosYaw = -1.0f;
	float prevSinYaw = -1.0f;

	float prevCosPitch = -1.0f;
	float prevSinPitch = -1.0f;

	for (float yaw = yawDegDelta; yaw <= 360.0f; yaw += yawDegDelta) {
		float cosYaw = CosDegrees(yaw);
		float sinYaw = SinDegrees(yaw);

		if (yaw == yawDegDelta) {
			prevCosYaw = CosDegrees(yaw - yawDegDelta);
			prevSinYaw = SinDegrees(yaw - yawDegDelta);
		}

		float leftU = RangeMapClamped(yaw - yawDegDelta, 0.0f, 360.0f, UVs.m_mins.x, UVs.m_maxs.x);
		float rightU = RangeMapClamped(yaw, 0.0f, 360.0f, UVs.m_mins.x, UVs.m_maxs.x);
		for (float pitch = -90.0f + pitchDegDelta; pitch <= 90.0f; pitch += pitchDegDelta) {
			float cosPitch = CosDegrees(pitch);
			float sinPitch = SinDegrees(pitch);

			if (pitch == -90.0f + pitchDegDelta) {
				prevCosPitch = CosDegrees(pitch - pitchDegDelta);
				prevSinPitch = SinDegrees(pitch - pitchDegDelta);
			}

			Vec3 rightBottomPos(cosYaw * cosPitch, sinYaw * cosPitch, -sinPitch);
			Vec3 rightTopPos(cosYaw * prevCosPitch, sinYaw * prevCosPitch, -prevSinPitch);
			Vec3 leftTopPos(prevCosYaw * prevCosPitch, prevSinYaw * prevCosPitch, -prevSinPitch);
			Vec3 leftBottomPos(prevCosYaw * cosPitch, prevSinYaw * cosPitch, -sinPitch);

			Vec3 rightBottomNormal = rightBottomPos;
			Vec3 leftBottomNormal = leftBottomPos;
			Vec3 rightTopNormal = rightTopPos;
			Vec3 leftTopNormal = rightTopNormal;
			rightBottomPos *= radius;
			rightTopPos *= radius;
			leftTopPos *= radius;
			leftBottomPos *= radius;


			float topV = RangeMapClamped(pitch - pitchDegDelta, 90.0f, -90.0f, UVs.m_mins.y, UVs.m_maxs.y);
			float bottomV = RangeMapClamped(pitch, 90.0f, -90.0f, UVs.m_mins.y, UVs.m_maxs.y);

			verts.emplace_back(leftBottomPos, leftBottomNormal, color, Vec2(leftU, bottomV));
			verts.emplace_back(rightBottomPos, rightBottomNormal, color, Vec2(rightU, bottomV));
			verts.emplace_back(rightTopPos, rightTopNormal, color, Vec2(rightU, topV));

			verts.emplace_back(leftBottomPos, leftBottomNormal, color, Vec2(leftU, bottomV));
			verts.emplace_back(rightTopPos, rightTopNormal, color, Vec2(rightU, topV));
			verts.emplace_back(leftTopPos, leftTopNormal, color, Vec2(leftU, topV));

			prevCosPitch = cosPitch;
			prevSinPitch = sinPitch;
		}

		prevCosYaw = cosYaw;
		prevSinYaw = sinYaw;
	}
}
void AddVertsForWireSphere(std::vector<Vertex_PCU>& verts, float radius, int stacks, int slices, Rgba8 const& color)
{
	float yawDegDelta = 360.0f / static_cast<float>(slices);
	float pitchDegDelta = 180.0f / static_cast<float>(stacks);

	float prevCosYaw = 1.0f;
	float prevSinYaw = 0.0f;

	float prevCosPitch = 0.0f;
	float prevSinPitch = -1.0f;

	for (float yaw = yawDegDelta; yaw <= 360.0f; yaw += yawDegDelta) {
		float cosYaw = CosDegrees(yaw);
		float sinYaw = SinDegrees(yaw);

		for (float pitch = -90.0f + pitchDegDelta; pitch <= 90.0f; pitch += pitchDegDelta) {
			float cosPitch = CosDegrees(pitch);
			float sinPitch = SinDegrees(pitch);

			Vec3 rightBottomPos(cosYaw * cosPitch, sinYaw * cosPitch, -sinPitch);
			Vec3 rightTopPos(cosYaw * prevCosPitch, sinYaw * prevCosPitch, -prevSinPitch);
			Vec3 leftTopPos(prevCosYaw * prevCosPitch, prevSinYaw * prevCosPitch, -prevSinPitch);
			Vec3 leftBottomPos(prevCosYaw * cosPitch, prevSinYaw * cosPitch, -sinPitch);

			rightBottomPos *= radius;
			rightTopPos *= radius;
			leftTopPos *= radius;
			leftBottomPos *= radius;

			AddVertsForLineSegment3D(verts, leftBottomPos, rightBottomPos, color);
			AddVertsForLineSegment3D(verts, rightBottomPos, rightTopPos, color);
			AddVertsForLineSegment3D(verts, leftTopPos, rightTopPos, color);

			prevCosPitch = cosPitch;
			prevSinPitch = sinPitch;
		}

		prevCosYaw = cosYaw;
		prevSinYaw = sinYaw;
		prevCosPitch = 0.0f;
		prevSinPitch = -1.0f;
	}
}

void AddVertsForCylinder(std::vector<Vertex_PCU>& verts, Vec3 const& start, Vec3 const& end, float radius, int slices, Rgba8 const& color, AABB2 const& UVs)
{
	verts.reserve(size_t(8) * slices);

	Vec3 kBasis = (end - start).GetNormalized();
	Vec3 worldIBasis = Vec3(1.0, 0.0f, 0.0f);
	Vec3 worldJBasis = Vec3(0.0f, 1.0, 0.0f);

	Vec3 jBasis, iBasis;

	if (fabsf(DotProduct3D(kBasis, worldIBasis)) < 1) {
		jBasis = CrossProduct3D(kBasis, worldIBasis).GetNormalized();
		iBasis = CrossProduct3D(jBasis, kBasis).GetNormalized();
	}
	else {
		jBasis = CrossProduct3D(kBasis, worldJBasis).GetNormalized();
		iBasis = CrossProduct3D(jBasis, kBasis).GetNormalized();
	}
	iBasis *= radius;
	jBasis *= radius;

	float degDelta = 360.0f / static_cast<float>(slices);


	float prevCosYaw = 1.0f;
	float prevSinYaw = 0.0f;
	float leftU = UVs.m_mins.x;
	Vec2 midUVs = (UVs.m_maxs + UVs.m_mins) * 0.5f;

	for (float yaw = degDelta; yaw <= 360.0f; yaw += degDelta) {
		float sinYaw = SinDegrees(yaw);
		float cosYaw = CosDegrees(yaw);

		Vec3 bottomLeft = (iBasis * prevCosYaw) + (jBasis * prevSinYaw) + start;
		Vec3 bottomRight = (iBasis * cosYaw) + (jBasis * sinYaw) + start;

		Vec3 topLeft = (iBasis * prevCosYaw) + (jBasis * prevSinYaw) + end;
		Vec3 topRight = (iBasis * cosYaw) + (jBasis * sinYaw) + end;

		float rightU = RangeMapClamped(yaw, 0.0f, 360.0f, UVs.m_mins.x, UVs.m_maxs.x);

		verts.emplace_back(bottomLeft, color, Vec2(leftU, UVs.m_mins.y));
		verts.emplace_back(bottomRight, color, Vec2(rightU, UVs.m_mins.y));
		verts.emplace_back(topRight, color, Vec2(rightU, UVs.m_maxs.y));

		verts.emplace_back(bottomLeft, color, Vec2(leftU, UVs.m_mins.y));
		verts.emplace_back(topRight, color, Vec2(rightU, UVs.m_maxs.y));
		verts.emplace_back(topLeft, color, Vec2(leftU, UVs.m_maxs.y));

		float leftTopU = midUVs.x + RangeMap(prevCosYaw, 0.0f, 1.0f, 0.0f, midUVs.x);
		float rightTopU = midUVs.x + RangeMap(cosYaw, 0.0f, 1.0f, 0.0f, midUVs.x);

		float leftTopV = midUVs.y + RangeMap(prevSinYaw, 0.0f, 1.0f, 0.0f, midUVs.y);
		float rightTopV = midUVs.y + RangeMap(sinYaw, 0.0f, 1.0f, 0.0f, midUVs.y);


		verts.emplace_back(start, color, midUVs);
		verts.emplace_back(bottomRight, color, Vec2(rightTopU, UVs.m_maxs.y - rightTopV));
		verts.emplace_back(bottomLeft, color, Vec2(leftTopU, UVs.m_maxs.y - leftTopV));

		verts.emplace_back(end, color, midUVs);
		verts.emplace_back(topLeft, color, Vec2(leftTopU, leftTopV));
		verts.emplace_back(topRight, color, Vec2(rightTopU, rightTopV));

		leftU = rightU;
		prevCosYaw = cosYaw;
		prevSinYaw = sinYaw;
	}

}

void AddVertsForWireCylinder(std::vector<Vertex_PCU>& verts, Vec3 const& start, Vec3 const& end, float radius, int slices, Rgba8 const& color)
{
	verts.reserve(size_t(18) * slices);

	Vec3 kBasis = (end - start).GetNormalized();
	Vec3 worldIBasis = Vec3(1.0, 0.0f, 0.0f);
	Vec3 worldJBasis = Vec3(0.0f, 1.0, 0.0f);

	Vec3 jBasis, iBasis;

	if (DotProduct3D(kBasis, worldIBasis) < 1) {
		jBasis = CrossProduct3D(kBasis, worldIBasis).GetNormalized();
		iBasis = CrossProduct3D(jBasis, kBasis).GetNormalized();
	}
	else {
		jBasis = CrossProduct3D(kBasis, worldJBasis).GetNormalized();
		iBasis = CrossProduct3D(jBasis, kBasis).GetNormalized();
	}
	iBasis *= radius;
	jBasis *= radius;

	float degDelta = 360.0f / static_cast<float>(slices);

	float prevCosYaw = 1.0f;
	float prevSinYaw = 0.0f;

	for (float yaw = degDelta; yaw <= 360.0f; yaw += degDelta) {
		float sinYaw = SinDegrees(yaw);
		float cosYaw = CosDegrees(yaw);

		Vec3 bottomLeft = (iBasis * prevCosYaw) + (jBasis * prevSinYaw) + start;
		Vec3 bottomRight = (iBasis * cosYaw) + (jBasis * sinYaw) + start;

		Vec3 topLeft = (iBasis * prevCosYaw) + (jBasis * prevSinYaw) + end;
		Vec3 topRight = (iBasis * cosYaw) + (jBasis * sinYaw) + end;

		AddVertsForLineSegment3D(verts, bottomLeft, bottomRight, color);
		AddVertsForLineSegment3D(verts, bottomRight, topRight, color);
		AddVertsForLineSegment3D(verts, topLeft, topRight, color);

		prevCosYaw = cosYaw;
		prevSinYaw = sinYaw;
	}
}

void AddVertsForCone3D(std::vector<Vertex_PCU>& verts, Vec3 const& start, Vec3 const& end, float radius, int slices, Rgba8 const& color, AABB2 const& UVs)
{
	verts.reserve(size_t(6) * slices);

	Vec3 kBasis = (end - start).GetNormalized();
	Vec3 worldIBasis = Vec3(1.0, 0.0f, 0.0f);
	Vec3 worldJBasis = Vec3(0.0f, 1.0, 0.0f);

	Vec3 jBasis, iBasis;

	if (fabsf(DotProduct3D(kBasis, worldIBasis)) < 1) {
		jBasis = CrossProduct3D(kBasis, worldIBasis).GetNormalized();
		iBasis = CrossProduct3D(jBasis, kBasis).GetNormalized();
	}
	else {
		jBasis = CrossProduct3D(kBasis, worldJBasis).GetNormalized();
		iBasis = CrossProduct3D(jBasis, kBasis).GetNormalized();
	}
	iBasis *= radius;
	jBasis *= radius;

	float degDelta = 360.0f / static_cast<float>(slices);

	float prevCosYaw = 1.0f;
	float prevSinYaw = 0.0f;
	float leftU = UVs.m_mins.x;
	Vec2 midUVs = (UVs.m_maxs + UVs.m_mins) * 0.5f;

	for (float yaw = degDelta; yaw <= 360.0f; yaw += degDelta) {
		float sinYaw = SinDegrees(yaw);
		float cosYaw = CosDegrees(yaw);

		Vec3 bottomLeft = (iBasis * prevCosYaw) + (jBasis * prevSinYaw) + start;
		Vec3 bottomRight = (iBasis * cosYaw) + (jBasis * sinYaw) + start;

		float rightU = RangeMapClamped(yaw, 0.0f, 360.0f, UVs.m_mins.x, UVs.m_maxs.x);

		verts.emplace_back(bottomLeft, color, Vec2(leftU, UVs.m_mins.y));
		verts.emplace_back(bottomRight, color, Vec2(rightU, UVs.m_mins.y));
		verts.emplace_back(end, color, Vec2(rightU, UVs.m_maxs.y));

		float leftTopU = midUVs.x + RangeMap(prevCosYaw, 0.0f, 1.0f, 0.0f, midUVs.x);
		float rightTopU = midUVs.x + RangeMap(cosYaw, 0.0f, 1.0f, 0.0f, midUVs.x);

		float leftTopV = midUVs.y + RangeMap(prevSinYaw, 0.0f, 1.0f, 0.0f, midUVs.y);
		float rightTopV = midUVs.y + RangeMap(sinYaw, 0.0f, 1.0f, 0.0f, midUVs.y);


		verts.emplace_back(start, color, midUVs);
		verts.emplace_back(bottomRight, color, Vec2(rightTopU, UVs.m_maxs.y - rightTopV));
		verts.emplace_back(bottomLeft, color, Vec2(leftTopU, UVs.m_maxs.y - leftTopV));

		leftU = rightU;
		prevCosYaw = cosYaw;
		prevSinYaw = sinYaw;
	}
}

void AddVertsForWireCone3D(std::vector<Vertex_PCU>& verts, Vec3 const& start, Vec3 const& end, float radius, int slices, Rgba8 const& color, AABB2 const& UVs)
{
	verts.reserve(size_t(6) * slices);

	Vec3 kBasis = (end - start).GetNormalized();
	Vec3 worldIBasis = Vec3(1.0, 0.0f, 0.0f);
	Vec3 worldJBasis = Vec3(0.0f, 1.0, 0.0f);

	Vec3 jBasis, iBasis;

	if (fabsf(DotProduct3D(kBasis, worldIBasis)) < 1) {
		jBasis = CrossProduct3D(kBasis, worldIBasis).GetNormalized();
		iBasis = CrossProduct3D(jBasis, kBasis).GetNormalized();
	}
	else {
		jBasis = CrossProduct3D(kBasis, worldJBasis).GetNormalized();
		iBasis = CrossProduct3D(jBasis, kBasis).GetNormalized();
	}
	iBasis *= radius;
	jBasis *= radius;

	float degDelta = 360.0f / static_cast<float>(slices);

	float prevCosYaw = 1.0f;
	float prevSinYaw = 0.0f;
	Vec2 midUVs = (UVs.m_maxs + UVs.m_mins) * 0.5f;

	for (float yaw = degDelta; yaw <= 360.0f; yaw += degDelta) {
		float sinYaw = SinDegrees(yaw);
		float cosYaw = CosDegrees(yaw);

		Vec3 bottomLeft = (iBasis * prevCosYaw) + (jBasis * prevSinYaw) + start;
		Vec3 bottomRight = (iBasis * cosYaw) + (jBasis * sinYaw) + start;

		AddVertsForLineSegment3D(verts, bottomLeft, bottomRight, color, 0.001f);
		AddVertsForLineSegment3D(verts, bottomLeft, end, color, 0.001f);
		AddVertsForLineSegment3D(verts, bottomRight, end, color, 0.001f);

		prevCosYaw = cosYaw;
		prevSinYaw = sinYaw;
	}
}

void AddVertsForArrow3D(std::vector<Vertex_PCU>& verts, Vec3 const& start, Vec3 const& end, float radius, int slices, Rgba8 const& color, AABB2 const& UVs)
{
	Vec3 kBasis = (end - start).GetNormalized();
	Vec3 worldIBasis = Vec3(1.0, 0.0f, 0.0f);
	Vec3 worldJBasis = Vec3(0.0f, 1.0, 0.0f);

	Vec3 jBasis, iBasis;

	if (fabsf(DotProduct3D(kBasis, worldIBasis)) < 1) {
		jBasis = CrossProduct3D(kBasis, worldIBasis).GetNormalized();
		iBasis = CrossProduct3D(jBasis, kBasis).GetNormalized();
	}
	else {
		jBasis = CrossProduct3D(kBasis, worldJBasis).GetNormalized();
		iBasis = CrossProduct3D(jBasis, kBasis).GetNormalized();
	}

	float arrowLength = (end - start).GetLength();
	Vec3 arrowBodyEnd = start + (kBasis * arrowLength * 0.85f);
	AddVertsForCylinder(verts, start, arrowBodyEnd, radius, slices, color, UVs);
	AddVertsForCone3D(verts, arrowBodyEnd, end, radius * 1.5f, slices, color, UVs);

}

void AddVertsForBasis3D(std::vector<Vertex_PCU>& verts, Mat44 const& model, float basisSize, float lineThickness)
{
	Vec3 iBasis = model.GetIBasis3D().GetNormalized();
	Vec3 jBasis = model.GetJBasis3D().GetNormalized();
	Vec3 kBasis = model.GetKBasis3D().GetNormalized();

	iBasis *= basisSize;
	jBasis *= basisSize;
	kBasis *= basisSize;

	Vec3 const translation = model.GetTranslation3D();

	AddVertsForLineSegment3D(verts, translation, translation + iBasis, Rgba8::RED, lineThickness);
	AddVertsForLineSegment3D(verts, translation, translation + jBasis, Rgba8::GREEN, lineThickness);
	AddVertsForLineSegment3D(verts, translation, translation + kBasis, Rgba8::BLUE, lineThickness);
}
