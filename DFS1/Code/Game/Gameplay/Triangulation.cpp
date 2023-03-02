#include "Engine/Math/LineSegment2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Math/ConvexPoly2D.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Game/Gameplay/Triangulation.hpp"
#include "Game/Framework/GameCommon.hpp"
#include <algorithm>

bool IsComposedOfInterestPoints(DelaunayTriangle const& triangle, DelaunayConvexPoly2D const& poly, std::vector<Vec2> const& artificialPoints);
DelaunayTriangle const GetASuperTriangleFromPoly(DelaunayConvexPoly2D const& convexPoly) {
	Vec2 const& center = convexPoly.m_middlePoint;

	Vec2 furthestPointFromCenter = Vec2::ZERO;
	float furthestSqrDist = FLT_MIN;

	for (int pointIndex = 0; pointIndex < convexPoly.m_vertexes.size(); pointIndex++) {
		Vec2 const& point = convexPoly.m_vertexes[pointIndex];
		float distToPoint = GetDistanceSquared2D(point, center);
		if (distToPoint > furthestSqrDist) {
			furthestSqrDist = distToPoint;
			furthestPointFromCenter = point;
		}
	}

	float furthestDist = sqrtf(furthestSqrDist);

	Vec2 dispToFurthestPoint = (furthestPointFromCenter - center).GetNormalized() * furthestDist * 2.0f;

	Vec2 pointA = center + dispToFurthestPoint;
	Vec2 pointB = center + dispToFurthestPoint.GetRotatedDegrees(120.0f);
	Vec2 pointC = center + dispToFurthestPoint.GetRotatedDegrees(240.0f);

	return DelaunayTriangle(pointA, pointB, pointC);
}

DelaunayTriangle const GetASuperTriangleFromPointSet2D(std::vector<Vec2> const& pointSet2D) {
	Vec2 center = Vec2::ZERO;

	for (int pointIndex = 0; pointIndex < pointSet2D.size(); pointIndex++) {
		Vec2 const& point = pointSet2D[pointIndex];
		center += point;
	}

	center /= (float)pointSet2D.size();

	Vec2 furthestPointFromCenter = Vec2::ZERO;
	float furthestSqrDist = FLT_MIN;

	for (int pointIndex = 0; pointIndex < pointSet2D.size(); pointIndex++) {
		Vec2 const& point = pointSet2D[pointIndex];
		float distToPoint = GetDistanceSquared2D(point, center);
		if (distToPoint > furthestSqrDist) {
			furthestSqrDist = distToPoint;
			furthestPointFromCenter = point;
		}
	}

	float furthestDist = sqrtf(furthestSqrDist);

	Vec2 dispToFurthestPoint = (furthestPointFromCenter - center).GetNormalized() * furthestDist * 2;

	Vec2 pointA = center + dispToFurthestPoint;
	Vec2 pointB = center + dispToFurthestPoint.GetRotatedDegrees(120.0f);
	Vec2 pointC = center + dispToFurthestPoint.GetRotatedDegrees(240.0f);

	return DelaunayTriangle(pointA, pointB, pointC);
}

DelaunayTriangle::DelaunayTriangle(std::vector<Vec2> const& vertexes) :
	DelaunayTriangle(vertexes[0], vertexes[1], vertexes[2])
{
}

DelaunayTriangle::DelaunayTriangle(Vec2 const& pointA, Vec2 const& pointB, Vec2 const& pointC)
{
	m_vertexes[0] = pointA;
	m_vertexes[1] = pointB;
	m_vertexes[2] = pointC;

	CalculateCircumcenter();
}

bool DelaunayTriangle::IsPointInsideCircumcenter(Vec2 const& point) const
{
	return IsPointInsideDisc2D(point, m_circumcenter, m_circumcenterRadius);
}

bool DelaunayTriangle::DoTrianglesShareEdge(DelaunayTriangle const& otherTriangle) const
{
	DelaunayEdge otherTriangleEdges[3];
	otherTriangle.GetEdges(otherTriangleEdges);

	DelaunayEdge thisTriangleEdges[3];
	GetEdges(thisTriangleEdges);

	for (int edgeIndex = 0; edgeIndex < 3; edgeIndex++) {
		for (int otherEdgeIndex = 0; otherEdgeIndex < 3; otherEdgeIndex++) {
			if (otherTriangleEdges[otherEdgeIndex] == thisTriangleEdges[edgeIndex]) return true;
		}
	}

	return false;
}

bool DelaunayTriangle::IsPointAVertex(Vec2 const& point, float tolerance = 0.025f) const
{
	float toleranceSqr = tolerance * tolerance;
	for (int vertexIndex = 0; vertexIndex < 3; vertexIndex++) {
		float distToPoint = GetDistanceSquared2D(point, m_vertexes[vertexIndex]);
		if (distToPoint < toleranceSqr) return true;
	}

	return false;
}

void DelaunayTriangle::SetVertex(int vertexIndex, Vec2 const& vertex)
{
	m_vertexes[vertexIndex] = vertex;
}

void DelaunayTriangle::Translate(Vec2 const& translation)
{
	for (int vertexIndex = 0; vertexIndex < 3; vertexIndex++) {
		m_vertexes[vertexIndex] += translation;
	}
}

bool DelaunayTriangle::ReplaceVertex(Vec2 const& oldVertex, Vec2 const& newVertex)
{
	for (int vertexIndex = 0; vertexIndex < 3; vertexIndex++) {
		Vec2& vertex = m_vertexes[vertexIndex];
		if (vertex == oldVertex) {
			vertex = newVertex;
			CalculateCircumcenter();
			return true;
		}
	}
	return false;
}

void DelaunayTriangle::GetEdges(DelaunayEdge* edges) const
{
	edges[0] = DelaunayEdge(m_vertexes[0], m_vertexes[1]);
	edges[1] = DelaunayEdge(m_vertexes[0], m_vertexes[2]);
	edges[2] = DelaunayEdge(m_vertexes[1], m_vertexes[2]);
}

DelaunayEdge const DelaunayTriangle::GetClosestEdgeToPoint(Vec2 const& refPoint) const
{
	DelaunayEdge triangleEdges[3];
	GetEdges(triangleEdges);

	float closestSqrDist = FLT_MAX;
	DelaunayEdge closestEdge;


	for (int edgeIndex = 0; edgeIndex < 3; edgeIndex++) {
		DelaunayEdge const& edge = triangleEdges[edgeIndex];
		Vec2 edgeCenter = edge.GetCenter();
		float distToPoint = GetDistanceSquared2D(edgeCenter, refPoint);
		if (distToPoint < closestSqrDist) {
			closestSqrDist = distToPoint;
			closestEdge = edge;
		}
	}
	return closestEdge;
}

Vec2 const DelaunayTriangle::GetRemainingVertex(Vec2 const& vertexToExclude1, Vec2 const& vertexToExclude2) const
{
	for (int vertexIndex = 0; vertexIndex < 3; vertexIndex++) {
		Vec2 const& vertex = m_vertexes[vertexIndex];
		if ((vertex != vertexToExclude2) && (vertex != vertexToExclude1)) {
			return vertex;
		}
	}

	return Vec2::ZERO;
}

bool DelaunayTriangle::ContainsAnyTriangleVertex(DelaunayTriangle const& otherTriangle) const
{
	for (int vertexIndex = 0; vertexIndex < 3; vertexIndex++) {
		Vec2 const& vertex = m_vertexes[vertexIndex];
		for (int otherVertexIndex = 0; otherVertexIndex < 3; otherVertexIndex++) {
			Vec2 const& otherVertex = otherTriangle.m_vertexes[otherVertexIndex];
			if (vertex == otherVertex) return true;
		}

	}
	return false;
}

bool DelaunayTriangle::ContainsEdge(DelaunayEdge const& edge) const
{
	DelaunayEdge triangleEdges[3];
	GetEdges(triangleEdges);
	for (int edgeIndex = 0; edgeIndex < 3; edgeIndex++) {
		if (triangleEdges[edgeIndex] == edge) return true;
	}
	return false;
}

void DelaunayTriangle::AddVertsForTriangleMesh(std::vector<Vertex_PCU>& verts, Rgba8 const& color, float lineThickness) const
{
	LineSegment2 edgeA(m_vertexes[0], m_vertexes[1]);
	LineSegment2 edgeB(m_vertexes[0], m_vertexes[2]);
	LineSegment2 edgeC(m_vertexes[1], m_vertexes[2]);
	AddVertsForLineSegment2D(verts, edgeA, color, lineThickness, false);
	AddVertsForLineSegment2D(verts, edgeB, color, lineThickness, false);
	AddVertsForLineSegment2D(verts, edgeC, color, lineThickness, false);
}

bool DelaunayTriangle::operator==(DelaunayTriangle const& otherTriangle)
{
	for (int vertIndex = 0; vertIndex < 3; vertIndex++) {
		Vec2 const& vertex = m_vertexes[vertIndex];
		bool containsVert = otherTriangle.IsPointAVertex(vertex);

		if (!containsVert) return false;
	}

	for (int vertIndex = 0; vertIndex < 3; vertIndex++) {
		Vec2 const& vertex = otherTriangle.m_vertexes[vertIndex];
		bool containsVert = IsPointAVertex(vertex);

		if (!containsVert) return false;
	}


	return true;
}

bool DelaunayTriangle::IsTriangleCollapsed() const
{
	float minScore = 0.999f;
	Vec2 dirToA = (m_vertexes[1] - m_vertexes[0]).GetNormalized();
	Vec2 dirToB = (m_vertexes[2] - m_vertexes[0]).GetNormalized();

	bool collapsedSideOne = DotProduct2D(dirToA, dirToB) > minScore;

	dirToA = (m_vertexes[0] - m_vertexes[1]).GetNormalized();
	dirToB = (m_vertexes[2] - m_vertexes[1]).GetNormalized();

	bool collapsedSideTwo = DotProduct2D(dirToA, dirToB) > minScore;

	dirToA = (m_vertexes[0] - m_vertexes[2]).GetNormalized();
	dirToB = (m_vertexes[1] - m_vertexes[2]).GetNormalized();

	bool collapsedSideThree = DotProduct2D(dirToA, dirToB) > minScore;

	int scoreOne = (collapsedSideOne) ? 1 : 0;
	int scoreTwo = (collapsedSideTwo) ? 1 : 0;
	int scoreThree = (collapsedSideThree) ? 1 : 0;

	int totalScore = scoreOne + scoreTwo + scoreThree;

	return totalScore > 1;
}

void DelaunayTriangle::CalculateCircumcenter()
{
	Vec2 dispEdgeA = m_vertexes[1] - m_vertexes[0];
	Vec2 centerEdgeA = m_vertexes[0] + (dispEdgeA * 0.5f);
	Vec2 rayEdgeA = (dispEdgeA.GetRotated90Degrees()).GetNormalized(); // Getting the perpendicular vector to one side

	Vec2 EdgeB = m_vertexes[2] - m_vertexes[0];
	Vec2 centerEdgeB = m_vertexes[0] + (EdgeB * 0.5f);
	Vec2 rayEdgeB = (EdgeB.GetRotated90Degrees()).GetNormalized();

	Vec2 dispBetweenCenters = centerEdgeB - centerEdgeA;
	Vec2 negDispCenters = -dispBetweenCenters; // The sign is important for calculating the intersection

	float dispFromEdgeAtoBinI = DotProduct2D(dispBetweenCenters, rayEdgeA); // displacement along I axis
	Vec2 projShadowRay = ((rayEdgeA * dispFromEdgeAtoBinI) - rayEdgeA).GetNormalized();

	float dispFromEdgeBtoAinI = DotProduct2D(negDispCenters, rayEdgeB);
	Vec2 endLineSegment = centerEdgeB + (rayEdgeB * dispFromEdgeBtoAinI); // RayEdgeB is treated as lineSegment to use RaycastVsLineSegment
	LineSegment2 lineSegment(centerEdgeB, endLineSegment);

	// This is treated as line segment vs Raycast, except there is no end to the raycast, therefore, no early outs (there will always be a hit)

	Vec2 const& rayForward = rayEdgeA;
	Vec2 const& rayStart = centerEdgeA;

	Vec2 jFwd = rayForward.GetRotated90Degrees();
	Vec2 dispRayToLineSegmentStart = (lineSegment.m_start - rayStart);
	Vec2 dispRayToLineSegmentEnd = (lineSegment.m_end - rayStart);
	Vec2 dispRaySegment = lineSegment.m_end - lineSegment.m_start;

	float dispRayLineStartJ = DotProduct2D(dispRayToLineSegmentStart, jFwd);
	float dispRayLineEndJ = DotProduct2D(dispRayToLineSegmentEnd, jFwd);

	float hitFraction = (dispRayLineStartJ) / (dispRayLineStartJ - dispRayLineEndJ);
	Vec2 impactPos = lineSegment.m_start + (hitFraction * dispRaySegment);

	m_circumcenter = impactPos;
	m_circumcenterRadius = (m_vertexes[1] - m_circumcenter).GetLength();
}



std::vector<DelaunayTriangle> TriangulateConvexPoly2D(DelaunayConvexPoly2D const& convexPoly, std::vector<Vec2> const& artificialPoints, bool includeSuperTriangle, float toleranceVertexDistance, int maxSteps)
{

	std::vector<DelaunayTriangle> calculatedTriangles;
	calculatedTriangles.reserve(convexPoly.m_vertexes.size() * 5);


	DelaunayTriangle superTriangle = GetASuperTriangleFromPoly(convexPoly);
	calculatedTriangles.push_back(DelaunayTriangle(superTriangle));

	if (convexPoly.m_vertexes.size() < 3) {
		return calculatedTriangles;
	}

	std::vector<Vec2> pointsToInsert;
	pointsToInsert.insert(pointsToInsert.end(), convexPoly.m_vertexes.begin(), convexPoly.m_vertexes.end());
	pointsToInsert.insert(pointsToInsert.end(), artificialPoints.begin(), artificialPoints.end());

	int maxLoops = (maxSteps == -1) ? (int)pointsToInsert.size() : maxSteps;

	std::vector<DelaunayTriangle> badTriangles;
	for (int vertexIndex = 0; vertexIndex < maxLoops; vertexIndex++) {
		std::vector<DelaunayEdge> allEdges;
		std::vector<DelaunayEdge> badEdges;
		Vec2 const& vertex = pointsToInsert[vertexIndex];

		for (int triangleIndex = 0; triangleIndex < calculatedTriangles.size(); triangleIndex++) {
			DelaunayTriangle& triangle = calculatedTriangles[triangleIndex];
			if (triangle.IsPointAVertex(vertex)) continue;
			if (triangle.IsPointInsideCircumcenter(vertex)) {
				badTriangles.push_back(triangle);
				DelaunayEdge triangleEdges[3];
				triangle.GetEdges(triangleEdges);

				for (int edgeIndex = 0; edgeIndex < 3; edgeIndex++) {
					if (std::find(allEdges.begin(), allEdges.end(), triangleEdges[edgeIndex]) != allEdges.end()) {
						badEdges.push_back(triangleEdges[edgeIndex]);
					}
					else {
						allEdges.push_back(triangleEdges[edgeIndex]);
					}
				}
				calculatedTriangles.erase(calculatedTriangles.begin() + triangleIndex);
				triangleIndex--;
			}

		}

		for (int edgeIndex = 0; edgeIndex < allEdges.size(); edgeIndex++) {
			DelaunayEdge const& edge = allEdges[edgeIndex];
			if (std::find(badEdges.begin(), badEdges.end(), allEdges[edgeIndex]) == badEdges.end()) {
				bool areSomeVertexEqual = (edge.m_pointA - edge.m_pointB).GetLengthSquared() < toleranceVertexDistance;
				areSomeVertexEqual = areSomeVertexEqual || ((edge.m_pointA - vertex).GetLengthSquared() < toleranceVertexDistance);
				areSomeVertexEqual = areSomeVertexEqual || ((edge.m_pointB - vertex).GetLengthSquared() < toleranceVertexDistance);
				if (!areSomeVertexEqual) {
					DelaunayTriangle newTriangle(edge.m_pointA, edge.m_pointB, vertex);
					if (newTriangle.IsTriangleCollapsed()) continue; // Warning. Collapsed triangles are common in this algorithm. It's a question of how collapsed I allow it to be
					auto it = std::find(badTriangles.begin(), badTriangles.end(), newTriangle);

					if (it == badTriangles.end()) {
						calculatedTriangles.emplace_back(edge.m_pointA, edge.m_pointB, vertex);
					}

				}
			}
		}
	}

	if (includeSuperTriangle) return calculatedTriangles;

	std::vector<DelaunayTriangle> resultingTriangles;
	resultingTriangles.reserve(convexPoly.m_vertexes.size() - 2); // A convex poly should have this a mount of triangles if triangulated correctly

	for (int triangleIndex = 0; triangleIndex < calculatedTriangles.size(); triangleIndex++) {
		DelaunayTriangle const& triangle = calculatedTriangles[triangleIndex];
		if (triangle.ContainsAnyTriangleVertex(superTriangle)) continue;
		resultingTriangles.push_back(triangle);
	}



	return resultingTriangles;
}

std::vector<DelaunayTriangle> TriangulatePointSet2D(std::vector<Vec2> const& pointSet, bool includeSuperTriangle)
{
	std::vector<DelaunayTriangle> calculatedTriangles;

	if (pointSet.size() < 3) {
		return calculatedTriangles;
	}
	DelaunayTriangle superTriangle = GetASuperTriangleFromPointSet2D(pointSet);
	calculatedTriangles.push_back(DelaunayTriangle(superTriangle));

	for (int vertexIndex = 0; vertexIndex < pointSet.size(); vertexIndex++) {
		//for (int vertexIndex = 0; vertexIndex < 2; vertexIndex++) {
		std::vector<DelaunayTriangle> badTriangles;
		std::vector<DelaunayEdge> allEdges;
		std::vector<DelaunayEdge> badEdges;
		Vec2 const& vertex = pointSet[vertexIndex];

		for (int triangleIndex = 0; triangleIndex < calculatedTriangles.size(); triangleIndex++) {
			DelaunayTriangle& triangle = calculatedTriangles[triangleIndex];
			if (triangle.IsPointAVertex(vertex)) continue;
			if (triangle.IsPointInsideCircumcenter(vertex)) {
				badTriangles.push_back(triangle);
				DelaunayEdge triangleEdges[3];
				triangle.GetEdges(triangleEdges);

				for (int edgeIndex = 0; edgeIndex < 3; edgeIndex++) {
					if (std::find(allEdges.begin(), allEdges.end(), triangleEdges[edgeIndex]) != allEdges.end()) {
						badEdges.push_back(triangleEdges[edgeIndex]);
					}
					else {
						allEdges.push_back(triangleEdges[edgeIndex]);
					}
				}
				calculatedTriangles.erase(calculatedTriangles.begin() + triangleIndex);
				triangleIndex--;
			}

		}

		for (int edgeIndex = 0; edgeIndex < allEdges.size(); edgeIndex++) {
			DelaunayEdge const& edge = allEdges[edgeIndex];
			if (std::find(badEdges.begin(), badEdges.end(), allEdges[edgeIndex]) == badEdges.end()) {
				calculatedTriangles.emplace_back(edge.m_pointA, edge.m_pointB, vertex);
			}
		}
	}



	if (includeSuperTriangle) return calculatedTriangles;

	std::vector<DelaunayTriangle> resultingTriangles;
	resultingTriangles.reserve(pointSet.size() - 2); // A convex poly should have this a mount of triangles if triangulated correctly

	for (int triangleIndex = 0; triangleIndex < calculatedTriangles.size(); triangleIndex++) {
		DelaunayTriangle const& triangle = calculatedTriangles[triangleIndex];
		if (triangle.ContainsAnyTriangleVertex(superTriangle)) continue;
		resultingTriangles.push_back(triangle);
	}

	return resultingTriangles;
}

std::vector<DelaunayEdge> GetVoronoiDiagram(std::vector<DelaunayTriangle> const& triangleMesh)
{
	std::vector<DelaunayEdge> voronoiDiagram;

	for (int triangleIndex = 0; triangleIndex < triangleMesh.size(); triangleIndex++) {
		DelaunayTriangle const& triangleA = triangleMesh[triangleIndex];
		for (int otherTriangleIndex = triangleIndex + 1; otherTriangleIndex < triangleMesh.size(); otherTriangleIndex++) {
			DelaunayTriangle const& triangleB = triangleMesh[otherTriangleIndex];
			if (triangleA.DoTrianglesShareEdge(triangleB)) {
				voronoiDiagram.emplace_back(triangleA.GetCircumcenter(), triangleB.GetCircumcenter());
			}
		}
	}

	return voronoiDiagram;
}

std::vector<DelaunayEdge> GetVoronoiDiagramFromConvexPoly(std::vector<DelaunayTriangle> const& triangleMesh, DelaunayConvexPoly2D const& convexPoly, std::vector<Vec2> const& artificialPoints)
{
	std::vector<DelaunayEdge> voronoiDiagram;

	for (int triangleIndex = 0; triangleIndex < triangleMesh.size(); triangleIndex++) {
		DelaunayTriangle const& triangleA = triangleMesh[triangleIndex];

		for (int otherTriangleIndex = triangleIndex + 1; otherTriangleIndex < triangleMesh.size(); otherTriangleIndex++) {
			DelaunayTriangle const& triangleB = triangleMesh[otherTriangleIndex];

			if (!IsComposedOfInterestPoints(triangleA, convexPoly, artificialPoints) && !IsComposedOfInterestPoints(triangleB, convexPoly, artificialPoints)) continue;

			if (triangleA.DoTrianglesShareEdge(triangleB)) {
				DelaunayEdge newEdge(triangleA.GetCircumcenter(), triangleB.GetCircumcenter());
				DelaunayRaycast2D clippingResult = newEdge.ClipEdge(convexPoly);

				voronoiDiagram.push_back(newEdge);

				if (clippingResult.m_didImpact) {
					voronoiDiagram.emplace_back(clippingResult.m_impactedEdgeVertexA, clippingResult.m_impactPos);
					voronoiDiagram.emplace_back(clippingResult.m_impactedEdgeVertexB, clippingResult.m_impactPos);
				}
			}
		}
	}


	for (std::vector<DelaunayEdge>::iterator edgeIt = voronoiDiagram.begin(); edgeIt != voronoiDiagram.end();) {
		DelaunayEdge const& edge = *edgeIt;

		float distanceBetVerts = GetDistanceSquared2D(edge.m_pointA, edge.m_pointB);

		if (distanceBetVerts < (0.000025f)) {
			edgeIt = voronoiDiagram.erase(edgeIt);
		}
		else {
			edgeIt++;
		}
	}


	return voronoiDiagram;
}


std::vector<DelaunayEdge> GetEdgesThanContainVertex(std::vector<DelaunayEdge> const& allEdges, Vec2 const& vertex) {
	std::vector<DelaunayEdge> edges;

	for (int edgeIndex = 0; edgeIndex < allEdges.size(); edgeIndex++) {
		DelaunayEdge const& edge = allEdges[edgeIndex];
		if (edge.ContainsPoint(vertex)) {
			edges.push_back(edge);
		}
	}

	return edges;
}


DelaunayEdge const GetNextPointFromEdgeContainingVertex(std::vector<DelaunayEdge>& availableEdges, Vec2 const& startingPoint, DelaunayEdge const& currentEdge, int insideDirection, std::vector<Vec2>& currentPoints) {

	std::vector<DelaunayEdge> candidateEdges;

	if (currentPoints.size() == 0) {
		candidateEdges = GetEdgesThanContainVertex(availableEdges, currentEdge.m_pointA);
		std::vector<DelaunayEdge> otherCandidates = GetEdgesThanContainVertex(availableEdges, currentEdge.m_pointB);
		candidateEdges.insert(candidateEdges.end(), otherCandidates.begin(), otherCandidates.end());
	}
	else {
		candidateEdges = GetEdgesThanContainVertex(availableEdges, currentPoints[currentPoints.size() - 1]);
	}

	DelaunayEdge bestCandidate(Vec2(-1.0f, -1.0f), Vec2(-1.0f, -1.0f));
	float bestCandidateDotproduct = FLT_MIN;
	Vec2 prevDir;

	bool candidateContainsStartPoint = false;
	for (int edgeIndex = 0; edgeIndex < candidateEdges.size(); edgeIndex++) {
		if (candidateContainsStartPoint) break;

		DelaunayEdge const& candidateEdge = candidateEdges[edgeIndex];
		Vec2 otherPoint = Vec2::ZERO;
		Vec2 startPoint = Vec2::ZERO;
		Vec2 middlePoint = Vec2::ZERO;

		if (currentPoints.size() > 0) {
			if (candidateEdge.ContainsPoint(startingPoint)) {
				candidateContainsStartPoint = true;
				bestCandidate = candidateEdge;
			}
		}

		if (candidateEdge.ContainsPoint(currentEdge.m_pointA)) {
			float diffToA = GetDistanceSquared2D(candidateEdge.m_pointA, currentEdge.m_pointA);
			otherPoint = (diffToA < 0.0025f) ? candidateEdge.m_pointB : candidateEdge.m_pointA; // We want the other point, not in currentEdge
			startPoint = currentEdge.m_pointB;
			middlePoint = currentEdge.m_pointA;
		}
		else {
			float diffToB = GetDistanceSquared2D(candidateEdge.m_pointB, currentEdge.m_pointB);

			otherPoint = (diffToB < 0.0025f) ? candidateEdge.m_pointA : candidateEdge.m_pointB; // We want the other point, not in currentEdge
			startPoint = currentEdge.m_pointA;
			middlePoint = currentEdge.m_pointB;
		}

		std::vector<Vec2> possiblePoints = currentPoints;
		if (currentPoints.size() == 0) {
			possiblePoints = { startPoint, middlePoint, otherPoint };
			prevDir = (middlePoint - startPoint).GetNormalized();
		}
		else {
			prevDir = currentPoints[currentPoints.size() - 1] - currentPoints[currentPoints.size() - 2];
			prevDir.Normalize();

			possiblePoints.push_back(otherPoint);
		}

		DelaunayConvexPoly2D resultingPoly(possiblePoints);
		Vec2 dispToCandidateEdge = (otherPoint - startPoint).GetNormalized();

		Vec2 prevDirJ = prevDir.GetRotatedMinus90Degrees();

		float dotBetweenEdges = Clamp(DotProduct2D(prevDirJ, dispToCandidateEdge), -1.0f, 1.0f);
		int dirToNewPoint = (dotBetweenEdges > 0.0f) ? 1 : -1; // If it's above, it's to the left, otherwise, is to the right
		int candidatePolyDir = resultingPoly.GetInsideDirection();


		bool isDispToOtherPointInInsideDir = (dirToNewPoint * insideDirection) > 0;

		if (candidatePolyDir == insideDirection && isDispToOtherPointInInsideDir) {
			if (dotBetweenEdges > bestCandidateDotproduct) {
				bestCandidate = candidateEdge;
				bestCandidateDotproduct = dotBetweenEdges;
			}
		}

	}

	std::vector<DelaunayEdge>::iterator itToCandidate = std::find(availableEdges.begin(), availableEdges.end(), bestCandidate);
	if (itToCandidate != availableEdges.end()) {
		availableEdges.erase(itToCandidate);
	}

	return bestCandidate;

}


std::vector<DelaunayConvexPoly2D> SplitConvexPoly(std::vector<DelaunayEdge> const& convexPolyVoronoiEdges, DelaunayConvexPoly2D const& convexPoly)
{
	std::vector<DelaunayConvexPoly2D> resultingPolygons;

	int insideDirection = convexPoly.GetInsideDirection();

	for (int edgeIndex = 0; edgeIndex < convexPolyVoronoiEdges.size(); edgeIndex++) {
		DelaunayEdge const& currentEdge = convexPolyVoronoiEdges[edgeIndex];

		std::vector<Vec2> newPolyVertexes;
		newPolyVertexes.reserve(6);

		std::vector<DelaunayEdge> availableEdges = convexPolyVoronoiEdges;
		std::vector<DelaunayEdge>::iterator itToCurrent = std::find(availableEdges.begin(), availableEdges.end(), currentEdge);
		availableEdges.erase(itToCurrent);

		DelaunayEdge prevEdge = currentEdge;
		DelaunayEdge nextEdge = GetNextPointFromEdgeContainingVertex(availableEdges, Vec2(-1.0f, -1.0f), currentEdge, insideDirection, newPolyVertexes);

		Vec2 const& pointNotSharedWithNext = (nextEdge.ContainsPoint(currentEdge.m_pointA)) ? currentEdge.m_pointB : currentEdge.m_pointA; // We want the point not shared with previous edge
		Vec2 prevDir = Vec2::ZERO;

		if (pointNotSharedWithNext == currentEdge.m_pointA) {
			newPolyVertexes.push_back(currentEdge.m_pointA);
			newPolyVertexes.push_back(currentEdge.m_pointB);

			if (nextEdge.m_pointA == currentEdge.m_pointB) {
				newPolyVertexes.push_back(nextEdge.m_pointB);
			}
			else {
				newPolyVertexes.push_back(nextEdge.m_pointA);

			}

		}
		else {
			newPolyVertexes.push_back(currentEdge.m_pointB);
			newPolyVertexes.push_back(currentEdge.m_pointA);

			if (nextEdge.m_pointA == currentEdge.m_pointA) {
				newPolyVertexes.push_back(nextEdge.m_pointB);
			}
			else {
				newPolyVertexes.push_back(nextEdge.m_pointA);

			}
		}



		bool firstPass = true; // The first time, the next edge will contain one of the vertexes of the currentEdge
		prevEdge = nextEdge;
		DelaunayEdge invalidEdge(Vec2(-1.0f, -1.0f), Vec2(-1.0f, -1.0f));
		bool foundInvalidEdge = false;

		Vec2 startPoint = newPolyVertexes[0];

		while (!foundInvalidEdge && (!nextEdge.ContainsPoint(currentEdge.m_pointA) && !nextEdge.ContainsPoint(currentEdge.m_pointB) || firstPass)) { // If the edge contains one vertex from currentEdge, there is a closed loop

			Vec2 const& currentPoint = newPolyVertexes[newPolyVertexes.size() - 1];
			prevDir = (currentPoint - newPolyVertexes[newPolyVertexes.size() - 2]).GetNormalized();
			nextEdge = GetNextPointFromEdgeContainingVertex(availableEdges, startPoint, nextEdge, insideDirection, newPolyVertexes);

			if (nextEdge == invalidEdge) {
				foundInvalidEdge = true;
			}
			else {
				Vec2 const& pointNotShared = (prevEdge.ContainsPoint(nextEdge.m_pointA)) ? nextEdge.m_pointB : nextEdge.m_pointA; // We want the point not shared with previous edge

				if (pointNotShared != newPolyVertexes[0]) {
					newPolyVertexes.push_back(pointNotShared);
				}
				firstPass = false;
				prevEdge = nextEdge;
			}

		}

		if (foundInvalidEdge) continue;

		DelaunayConvexPoly2D newPoly(newPolyVertexes);
		newPoly.PurgeIdenticalVertexes();
		if (newPoly.m_vertexes.size() < 3) continue;

		auto polygameSameIter = std::find(resultingPolygons.begin(), resultingPolygons.end(), newPoly);
		if (polygameSameIter == resultingPolygons.end()) {
			resultingPolygons.push_back(newPoly);
		}
	}

	return resultingPolygons;
}

bool IsComposedOfInterestPoints(DelaunayTriangle const& triangle, DelaunayConvexPoly2D const& poly, std::vector<Vec2> const& artificialPoints) {
	for (int triangleVertexIndex = 0; triangleVertexIndex < 3; triangleVertexIndex++) {
		Vec2 const& vertex = triangle.m_vertexes[triangleVertexIndex];
		bool foundVertex = poly.IsPointVertex(vertex);

		if (!foundVertex) {
			for (int artIndex = 0; artIndex < artificialPoints.size() && !foundVertex; artIndex++) {
				Vec2 const& artificalPoint = artificialPoints[artIndex];

				float distToVertex = GetDistanceSquared2D(artificalPoint, vertex);
				foundVertex = distToVertex < 0.00025f;
			}
		}

		if (!foundVertex) return false;
	}

	return true;
}

DelaunayRaycast2D const DelaunayEdge::ClipEdge(DelaunayConvexPoly2D const& convexPoly)
{
	std::vector<Vec2> const& polyVertexes = convexPoly.m_vertexes;

	int polyDirection = convexPoly.GetInsideDirection();

	Vec2 prevVertex = polyVertexes[0];
	for (int polyVertexInd = 1; polyVertexInd < polyVertexes.size() + 1; polyVertexInd++) {
		Vec2 const& currentVertex = (polyVertexInd == polyVertexes.size()) ? polyVertexes[0] : polyVertexes[polyVertexInd];
		Vec2 midpoint = (m_pointA + m_pointB) * 0.5f;
		// Dot product between edge 1 and 2. If they agree, then the direction they agree is correct. If they disagree, if edge one points left, direction is left, is edge points right, direction is right
		Vec2 dispToWorldPos = m_pointA - midpoint;
		Vec2 dispEdge = (currentVertex - prevVertex).GetNormalized();
		float projInJ = DotProduct2D(dispEdge.GetRotated90Degrees(), dispToWorldPos);

		LineSegment2 lineSegment(prevVertex, currentVertex);
		int directionToPoint = (projInJ > 0) ? -1 : 1; // if ProjJ is positive, point is to the left of edge, else to the right

		Vec2 chosenPoint = (directionToPoint * polyDirection > 0) ? m_pointA : m_pointB;
		Vec2 rayDisp = (directionToPoint * polyDirection > 0) ? (m_pointB - m_pointA) : (m_pointA - m_pointB);

		RaycastResult2D raycastVsSide;
		raycastVsSide = RaycastVsLineSegment2D(chosenPoint, rayDisp.GetNormalized(), rayDisp.GetLength(), lineSegment);
		if (raycastVsSide.m_didImpact) {
			m_pointA = chosenPoint;
			m_pointB = raycastVsSide.m_impactPos;
		}

		if (raycastVsSide.m_didImpact) {
			DelaunayRaycast2D raycastResult = {
			true,
			raycastVsSide.m_impactPos,
			prevVertex,
			currentVertex
			};

			return raycastResult;
		}

		prevVertex = currentVertex;
	}

	return DelaunayRaycast2D();
}
