#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/Triangulation3D.hpp"
#include "Game/Gameplay/ConvexPoly3D.hpp"

DelaunayTetrahedron const GetASuperTetrahedronPoly(ConvexPoly3D const& convexPoly) {
	Vec3 const& center = convexPoly.m_middlePoint;

	Vec3 furthestPointFromCenter = Vec3::ZERO;
	float furthestSqrDist = FLT_MIN;

	for (int pointIndex = 0; pointIndex < convexPoly.m_vertexes.size(); pointIndex++) {
		Vec3 const& point = convexPoly.m_vertexes[pointIndex];
		float distToPoint = GetDistanceSquared3D(point, center);
		if (distToPoint > furthestSqrDist) {
			furthestSqrDist = distToPoint;
			furthestPointFromCenter = point;
		}
	}

	float furthestDist = sqrtf(furthestSqrDist);

	float edgeLength = furthestDist * 4.0f;

	Vec3 dispToFurthestPoint = (furthestPointFromCenter - center).GetNormalized() * edgeLength;

	// This is just some arbitrary directions that work best for finding the super tetrahedron

	Vec3 pointA = center + Vec3(0.0f, 0.0f, 1.0f) * edgeLength;
	Vec3 pointB = center + Vec3(-1.0f, -1.0f, -1.0f) * edgeLength;
	Vec3 pointC = center + Vec3(1.0f, -1.0f, -1.0f) * edgeLength;
	Vec3 pointD = center + Vec3(0.0f, 1.5f, -1.0f) * edgeLength;

	return DelaunayTetrahedron(pointA, pointB, pointC, pointD);
}

DelaunayTetrahedron::DelaunayTetrahedron(std::vector<Vec3> const& vertexes)
{
	m_vertexes[0] = vertexes[0];
	m_vertexes[1] = vertexes[1];
	m_vertexes[2] = vertexes[2];
	m_vertexes[3] = vertexes[3];

	CalculateCircumcenter();
}

DelaunayTetrahedron::DelaunayTetrahedron(Vec3 const& pointA, Vec3 const& pointB, Vec3 const& pointC, Vec3 const& pointD)
{
	m_vertexes[0] = pointA;
	m_vertexes[1] = pointB;
	m_vertexes[2] = pointC;
	m_vertexes[3] = pointD;

	CalculateCircumcenter();
}

void DelaunayTetrahedron::AddVertsForFaces(std::vector<Vertex_PCU>& verts, Rgba8 const& faceColor) const
{
	AddVertsForFace(verts, faceColor, m_vertexes[0], m_vertexes[1], m_vertexes[2]);
	AddVertsForFace(verts, faceColor, m_vertexes[0], m_vertexes[1], m_vertexes[3]);
	AddVertsForFace(verts, faceColor, m_vertexes[0], m_vertexes[2], m_vertexes[3]);
	AddVertsForFace(verts, faceColor, m_vertexes[1], m_vertexes[2], m_vertexes[3]);

}

void DelaunayTetrahedron::AddVertsForWireframe(std::vector<Vertex_PCU>& verts, Rgba8 const& wireColor, float thickness) const
{

	Vec3 const& pointA = m_vertexes[0];
	Vec3 const& pointB = m_vertexes[1];
	Vec3 const& pointC = m_vertexes[2];
	Vec3 const& pointD = m_vertexes[3];

	AddVertsForLineSegment3D(verts, pointA, pointB, wireColor, thickness);
	AddVertsForLineSegment3D(verts, pointA, pointC, wireColor, thickness);
	AddVertsForLineSegment3D(verts, pointA, pointD, wireColor, thickness);
	AddVertsForLineSegment3D(verts, pointA, pointB, wireColor, thickness);

	AddVertsForLineSegment3D(verts, pointB, pointC, wireColor, thickness);
	AddVertsForLineSegment3D(verts, pointB, pointD, wireColor, thickness);

	AddVertsForLineSegment3D(verts, pointC, pointD, wireColor, thickness);

}

bool DelaunayTetrahedron::IsPointAVertex(Vec3 const& vertex, float tolerance) const
{
	for (int vertexInd = 0; vertexInd < 4; vertexInd++) {
		Vec3 const& tetraVertex = m_vertexes[vertexInd];
		Vec3 dispBetVerts = (vertex - tetraVertex);

		if (dispBetVerts.GetLengthSquared() <= tolerance) return true;
	}

	return false;
}

bool DelaunayTetrahedron::IsPointInsideCircumcenter(Vec3 const& vertex, float tolerance) const
{
	float distToPointSqr = GetDistanceSquared3D(m_circumcenter, vertex);

	float radiusSqr = m_circumcenterRadius * m_circumcenterRadius;
	float radiusMinusDist = (radiusSqr + (tolerance * tolerance)) - distToPointSqr;

	if (radiusMinusDist >= 0.0f) return true;

	return false;
}

bool DelaunayTetrahedron::DoTetrahedronsShareFaces(DelaunayTetrahedron const& otherTetrahedron) const
{
	DelaunayFace3D thisFaces[4];
	DelaunayFace3D otherFaces[4];

	GetFaces(thisFaces);
	otherTetrahedron.GetFaces(otherFaces);

	for (int thisFaceIndex = 0; thisFaceIndex < 4; thisFaceIndex++) {
		DelaunayFace3D const& thisFace = thisFaces[thisFaceIndex];
		for (int otherFaceIndex = 0; otherFaceIndex < 4; otherFaceIndex++) {
			DelaunayFace3D const& otherFace = otherFaces[otherFaceIndex];

			// Done this way because of floating point error
			bool shareVertA = otherFace.ContainsPoint(thisFace.m_pointA);
			bool shareVertB = otherFace.ContainsPoint(thisFace.m_pointB);
			bool shareVertC = otherFace.ContainsPoint(thisFace.m_pointC);

			if (shareVertA && shareVertB && shareVertC) return true;
		}
	}

	return false;
}

bool DelaunayTetrahedron::ContainsAnyTetrahedronVertex(DelaunayTetrahedron const& otherTetrahedron) const
{
	for (int vertIndex = 0; vertIndex < 4; vertIndex++) {
		Vec3 const& vertex = otherTetrahedron.m_vertexes[vertIndex];
		if (IsPointAVertex(vertex)) return true;
	}

	return false;
}

void DelaunayTetrahedron::GetEdges(DelaunayEdge3D* edges) const
{
	Vec3 const& pointA = m_vertexes[0];
	Vec3 const& pointB = m_vertexes[1];
	Vec3 const& pointC = m_vertexes[2];
	Vec3 const& pointD = m_vertexes[3];

	DelaunayEdge3D edgeOne(pointA, pointB);
	DelaunayEdge3D edgeTwo(pointA, pointC);
	DelaunayEdge3D edgeThree(pointA, pointD);

	DelaunayEdge3D edgeFour(pointB, pointC);
	DelaunayEdge3D edgeFive(pointB, pointD);

	DelaunayEdge3D edgeSix(pointC, pointD);

	edges[0] = edgeOne;
	edges[1] = edgeTwo;
	edges[2] = edgeThree;
	edges[3] = edgeFour;
	edges[4] = edgeFive;
	edges[5] = edgeSix;

}

void DelaunayTetrahedron::GetFaces(DelaunayFace3D* faces) const
{

	Vec3 const& pointA = m_vertexes[0];
	Vec3 const& pointB = m_vertexes[1];
	Vec3 const& pointC = m_vertexes[2];
	Vec3 const& pointD = m_vertexes[3];


	DelaunayFace3D faceOne(pointA, pointB, pointC);
	DelaunayFace3D faceTwo(pointA, pointB, pointD);
	DelaunayFace3D faceThree(pointA, pointC, pointD);
	DelaunayFace3D faceFour(pointB, pointC, pointD);

	faces[0] = faceOne;
	faces[1] = faceTwo;
	faces[2] = faceThree;
	faces[3] = faceFour;

}

void DelaunayTetrahedron::CalculateCircumcenter()
{
	// This is an implementation of the linear algebra equation for calculating the circumcenter of a Tetrahedron

	Vec3 const& A = m_vertexes[0];
	Vec3 const& B = m_vertexes[1];
	Vec3 const& C = m_vertexes[2];
	Vec3 const& D = m_vertexes[3];

	Vec3 dispBA = B - A;	// u1
	Vec3 dispCA = C - A;	// u2
	Vec3 dispDA = D - A;	// u3

	float lenBA = dispBA.GetLengthSquared();	// u1sqr
	float lenCA = dispCA.GetLengthSquared();	// u2sqr
	float lenDA = dispDA.GetLengthSquared();	// u3sqr

	Vec3 crossBC = CrossProduct3D(dispBA, dispCA);	// u1 X u2
	Vec3 crossDB = CrossProduct3D(dispDA, dispBA);	// u3 X u1
	Vec3 crossCD = CrossProduct3D(dispCA, dispDA);	// u2 X u3

	float denom = 2 * (DotProduct3D(dispBA, crossCD));
	denom = 1.0f / denom;

	m_circumcenter = A + denom * ((lenBA * crossCD) + (lenCA * crossDB) + (lenDA * crossBC));
	//m_circumcenter =  A + denom * ((lenDA * crossBC) + (lenCA * crossDB) + (lenBA * crossCD));

	//m_circumcenter.x = ((lenBA * crossCD.x) + (lenCA * crossDB.x) + (lenDA * crossBC.x)) * denom;
	//m_circumcenter.y = ((lenBA * crossCD.y) + (lenCA * crossDB.y) + (lenDA * crossBC.y)) * denom;
	//m_circumcenter.z = ((lenBA * crossCD.z) + (lenCA * crossDB.z) + (lenDA * crossBC.z)) * denom;

	m_circumcenterRadius = (A - m_circumcenter).GetLength();

}

void DelaunayTetrahedron::AddVertsForFace(std::vector<Vertex_PCU>& verts, Rgba8 const& faceColor, Vec3 const& pointA, Vec3 const& pointB, Vec3 const& pointC) const
{
	Vec3 midPoint = pointA + pointB + pointC;

	midPoint /= 3.0f;


	verts.emplace_back(midPoint, faceColor, Vec2::ZERO);
	verts.emplace_back(pointA, faceColor, Vec2::ZERO);
	verts.emplace_back(pointB, faceColor, Vec2::ZERO);


	verts.emplace_back(midPoint, faceColor, Vec2::ZERO);
	verts.emplace_back(pointB, faceColor, Vec2::ZERO);
	verts.emplace_back(pointC, faceColor, Vec2::ZERO);


	verts.emplace_back(midPoint, faceColor, Vec2::ZERO);
	verts.emplace_back(pointC, faceColor, Vec2::ZERO);
	verts.emplace_back(pointA, faceColor, Vec2::ZERO);

}

std::vector<DelaunayTetrahedron> TriangulateConvexPoly3D(ConvexPoly3D const& convexPoly, bool includeSuperTriangle, float toleranceVertexDistance, int maxStep)
{

	int maxLoops = (maxStep == -1) ? (int)convexPoly.m_vertexes.size() : maxStep;

	std::vector<DelaunayTetrahedron> calculatedTetrahedrons;
	calculatedTetrahedrons.reserve(convexPoly.m_vertexes.size() * 5);

	DelaunayTetrahedron superTetrahedron = GetASuperTetrahedronPoly(convexPoly);
	calculatedTetrahedrons.push_back(DelaunayTetrahedron(superTetrahedron));

	if (convexPoly.m_vertexes.size() < 3) {
		return calculatedTetrahedrons;
	}

	for (int vertexIndex = 0; vertexIndex < maxLoops; vertexIndex++) {
		std::vector<DelaunayTetrahedron> badTriangles;
		std::vector<DelaunayFace3D> allFaces;
		std::vector<DelaunayFace3D> badFaces;
		Vec3 const& vertex = convexPoly.m_vertexes[vertexIndex];

		for (int tetrahedronIndex = 0; tetrahedronIndex < calculatedTetrahedrons.size(); tetrahedronIndex++) {
			DelaunayTetrahedron& tetrahedron = calculatedTetrahedrons[tetrahedronIndex];
			if (tetrahedron.IsPointAVertex(vertex)) continue;
			if (tetrahedron.IsPointInsideCircumcenter(vertex)) {
				badTriangles.push_back(tetrahedron);
				DelaunayFace3D tetrahedronFaces[4];
				tetrahedron.GetFaces(tetrahedronFaces);

				for (int edgeIndex = 0; edgeIndex < 4; edgeIndex++) {
					if (std::find(allFaces.begin(), allFaces.end(), tetrahedronFaces[edgeIndex]) != allFaces.end()) {
						badFaces.push_back(tetrahedronFaces[edgeIndex]);
					}
					else {
						allFaces.push_back(tetrahedronFaces[edgeIndex]);
					}
				}
				calculatedTetrahedrons.erase(calculatedTetrahedrons.begin() + tetrahedronIndex);
				tetrahedronIndex--;
			}

		}

		if (g_drawDebug && vertexIndex == maxLoops - 1) {
			for (int badFaceInd = 0; badFaceInd < badFaces.size(); badFaceInd++) {
				DelaunayFace3D const& face = badFaces[badFaceInd];

				DebugAddWorldPoint(face.GetCenter(), 0.15f, 0.0f, Rgba8::YELLOW, Rgba8::YELLOW, DebugRenderMode::USEDEPTH);
			}
		}


		for (int faceIndex = 0; faceIndex < allFaces.size(); faceIndex++) {
			DelaunayFace3D const& face = allFaces[faceIndex];
			if (std::find(badFaces.begin(), badFaces.end(), allFaces[faceIndex]) == badFaces.end()) {

				bool areSomeVertexEqual = face.AreAnyPointsCollapsed(toleranceVertexDistance);
				areSomeVertexEqual = areSomeVertexEqual || face.ContainsPoint(vertex);

				if (!areSomeVertexEqual) {
					calculatedTetrahedrons.emplace_back(face.m_pointA, face.m_pointB, face.m_pointC, vertex);
				}
			}
		}
	}

	if (includeSuperTriangle) return calculatedTetrahedrons;

	std::vector<DelaunayTetrahedron> resultingTriangles;
	resultingTriangles.reserve(convexPoly.m_vertexes.size() - 2); // A convex poly should have this a mount of triangles if triangulated correctly

	for (int tetrahedronIndex = 0; tetrahedronIndex < calculatedTetrahedrons.size(); tetrahedronIndex++) {
		DelaunayTetrahedron const& tetrahedron = calculatedTetrahedrons[tetrahedronIndex];
		if (tetrahedron.ContainsAnyTetrahedronVertex(superTetrahedron)) continue;
		resultingTriangles.push_back(tetrahedron);
	}



	return resultingTriangles;
}

bool IsComposedOfPolygonVertexes(DelaunayTetrahedron const& tetrahedron, ConvexPoly3D const& poly) {
	for (int tetrahedronVertIndex = 0; tetrahedronVertIndex < 4; tetrahedronVertIndex++) {
		Vec3 const& vertex = tetrahedron.m_vertexes[tetrahedronVertIndex];
		if (!poly.IsPointAVertex(vertex, 0.25f)) return false;
	}

	return true;
}


std::vector<DelaunayEdge3D> GetAllowedClippingDirections(std::vector<DelaunayEdge3D> const& allEdges, DelaunayEdge3D const& currentEdge) {

	std::vector<DelaunayEdge3D> allowedClippingFwds;

	Vec3 rayDisp = currentEdge.m_pointB - currentEdge.m_pointA;
	Vec3 rayFwd = rayDisp.GetNormalized();
	float rayMaxDist = rayDisp.GetLength();

	for (int edgeIndex = 0; edgeIndex < allEdges.size(); edgeIndex++) {
		DelaunayEdge3D const& checkedEdge = allEdges[edgeIndex];
		if (checkedEdge == currentEdge) continue;

		RaycastResult3D rayVsEdge = RaycastVsLineSegment3D(currentEdge.m_pointA, rayFwd, rayMaxDist, checkedEdge.m_pointA, checkedEdge.m_pointB);

		if (rayVsEdge.m_didImpact) {
			allowedClippingFwds.push_back(checkedEdge);
		}
	}

	return allowedClippingFwds;
}

bool IsEdgeAlreadyInDiagram(std::vector<DelaunayEdge3D> const& allEdges, DelaunayEdge3D const& edge) {
	for (int edgeIndex = 0; edgeIndex < allEdges.size(); edgeIndex++) {
		DelaunayEdge3D const& diagramEdge = allEdges[edgeIndex];

		bool includesA = diagramEdge.ContainsPoint(edge.m_pointA, 0.025f);
		bool includesB = diagramEdge.ContainsPoint(edge.m_pointB, 0.025f);

		if (includesA && includesB) return true;

	}


	return false;
}

std::vector<Face> GetFacesImpactedByEdge(DelaunayEdge3D const& edge, ConvexPoly3D const& convexPoly, std::vector<Vec3>& impactPoints, float tolerance = 0.025f) {

	std::vector<Face> impactedFaces;

	Vec3 rayStart = edge.m_pointA;
	Vec3 rayDisp = edge.m_pointB - edge.m_pointA;
	Vec3 rayFwd = rayDisp.GetNormalized();
	float maxDist = rayDisp.GetLength();

	float toleranceSqr = tolerance * tolerance;

	for (int faceIndex = 0; faceIndex < convexPoly.m_faces.size(); faceIndex++) {
		Face const& face = convexPoly.m_faces[faceIndex];


		Vec3 const& planeRefPoint = convexPoly.m_vertexes[face.m_faceIndexes[0]];

		RaycastResult3D rayVsPlane = RaycastVsPlane(rayStart, rayFwd, maxDist, planeRefPoint, face.m_normal);

		if (rayVsPlane.m_didImpact) {

			// Lines should be considered perpendicular to the plane
			Vec3 crossWithPlaneNormal = CrossProduct3D(rayFwd, face.m_normal);

			if (crossWithPlaneNormal.GetLengthSquared() <= (toleranceSqr)) {
				impactedFaces.push_back(face);
				impactPoints.push_back(rayVsPlane.m_impactPos);
			}

		}

	}

	return impactedFaces;

}


void GetResultingProjectedEdges(std::vector<DelaunayEdge3D>const& allEdges, std::vector<DelaunayEdge3D> const& otherImpactedEdges, Face const& face, Vec3 faceImpactPoint, std::vector<DelaunayEdge3D>& resultingEdges) {


	std::vector<ConvexPoly3DEdge> faceEdges = face.GetFaceEdges();
	Vec3 const& faceNormal = face.m_normal;
	Vec3 const& planeRefPoint = face.m_middlePoint;

	for (int otherEdgeIndex = 0; otherEdgeIndex < otherImpactedEdges.size(); otherEdgeIndex++) {
		DelaunayEdge3D const& impactedEdge = otherImpactedEdges[otherEdgeIndex];
		bool hitAnyPolyEdge = false;


		Vec3 projectedPointA = ProjectPointOntoPlane(impactedEdge.m_pointA, planeRefPoint, faceNormal);
		Vec3 projectedPointB = ProjectPointOntoPlane(impactedEdge.m_pointB, planeRefPoint, faceNormal);

		float distToFaceImpactPoint = (projectedPointA - faceImpactPoint).GetLengthSquared();
		Vec3 pointNotSharedWithProjEdge = (distToFaceImpactPoint < (0.025f * 0.025f)) ? projectedPointB : projectedPointA;
		Vec3 extendedImpactPoint = Vec3::ZERO;
		ConvexPoly3DEdge extendedImpactEdge;

		for (int faceEdgeIndex = 0; faceEdgeIndex < faceEdges.size(); faceEdgeIndex++) {
			ConvexPoly3DEdge const& currentFaceEdge = faceEdges[faceEdgeIndex];

			Vec3 const& rayStart = currentFaceEdge.m_pointOne;
			Vec3 rayDisp = currentFaceEdge.m_pointTwo - currentFaceEdge.m_pointOne;
			Vec3 rayFwd = rayDisp.GetNormalized();
			float maxRayLength = rayDisp.GetLength();



			RaycastResult3D faceEdgeVsVoronoiEdge = RaycastVsLineSegment3D(rayStart, rayFwd, maxRayLength, projectedPointA, projectedPointB);

			if (faceEdgeVsVoronoiEdge.m_didImpact) {

				Vec3 const& rayImpactPoint = faceEdgeVsVoronoiEdge.m_impactPos;
				DelaunayEdge3D newEdge = DelaunayEdge3D(faceImpactPoint, rayImpactPoint);
				hitAnyPolyEdge = true;
				resultingEdges.push_back(newEdge);
				//DebugAddWorldPoint(rayImpactPoint, 0.05f, 0.0f, Rgba8::GREEN, Rgba8::GREEN, DebugRenderMode::USEDEPTH, 2, 4);


				DelaunayEdge3D edgeFromImpactA = DelaunayEdge3D(rayImpactPoint, currentFaceEdge.m_pointOne);
				DelaunayEdge3D edgeFromImpactB = DelaunayEdge3D(rayImpactPoint, currentFaceEdge.m_pointTwo);


				resultingEdges.push_back(edgeFromImpactA);
				resultingEdges.push_back(edgeFromImpactB);

			}
			else {
				// Paid for raycast here, in case the edge needs to be extended

				Vec3 rayDir = pointNotSharedWithProjEdge - faceImpactPoint;
				RaycastResult3D extendedRaycast = RaycastVsLineSegment3D(faceImpactPoint, rayDir, 10 * maxRayLength, currentFaceEdge.m_pointOne, currentFaceEdge.m_pointTwo);

				if (extendedRaycast.m_didImpact) {
					extendedImpactPoint = extendedRaycast.m_impactPos;
					extendedImpactEdge = currentFaceEdge;
				}
			}

		}

		// If the projected edge only hits one other segment, this means it has a disconnected end, and must be fixed
		if (!hitAnyPolyEdge) {

			DelaunayEdge3D newEdge = DelaunayEdge3D(faceImpactPoint, pointNotSharedWithProjEdge);
			std::vector<DelaunayEdge3D> clippingDirections = GetAllowedClippingDirections(allEdges, newEdge);

			if (clippingDirections.size() == 1) {
				newEdge = DelaunayEdge3D(faceImpactPoint, extendedImpactPoint);

				DelaunayEdge3D edgeFromImpactA = DelaunayEdge3D(extendedImpactPoint, extendedImpactEdge.m_pointOne);
				DelaunayEdge3D edgeFromImpactB = DelaunayEdge3D(extendedImpactPoint, extendedImpactEdge.m_pointTwo);


				resultingEdges.push_back(edgeFromImpactA);
				resultingEdges.push_back(edgeFromImpactB);

			}

			resultingEdges.push_back(newEdge);


		}
	}

}

std::vector<DelaunayEdge3D> GetVoronoiDiagramFromConvexPoly3D(std::vector<DelaunayTetrahedron> const& tetrahedronMesh, ConvexPoly3D const& convexPoly, bool includeEdgeProjections)
{
	std::vector<DelaunayEdge3D> voronoiDiagram;

	for (int tetrahedronIndex = 0; tetrahedronIndex < tetrahedronMesh.size(); tetrahedronIndex++) {
		DelaunayTetrahedron const& tetrahedronA = tetrahedronMesh[tetrahedronIndex];

		for (int otherTetrahedronIndex = tetrahedronIndex + 1; otherTetrahedronIndex < tetrahedronMesh.size(); otherTetrahedronIndex++) {
			DelaunayTetrahedron const& tetrahedronB = tetrahedronMesh[otherTetrahedronIndex];

			if (!IsComposedOfPolygonVertexes(tetrahedronA, convexPoly) && !IsComposedOfPolygonVertexes(tetrahedronB, convexPoly)) continue;

			if (tetrahedronA.DoTetrahedronsShareFaces(tetrahedronB)) {
				DelaunayEdge3D newEdge(tetrahedronA.m_circumcenter, tetrahedronB.m_circumcenter);

				bool isAlreadyInDiagram = IsEdgeAlreadyInDiagram(voronoiDiagram, newEdge);

				if (!isAlreadyInDiagram && !newEdge.IsCollapsed()) {
					voronoiDiagram.push_back(newEdge);
				}

			}
		}
	}

	std::vector<DelaunayEdge3D> addedEdges;
	addedEdges.reserve(voronoiDiagram.size() * 2);

	for (int edgeIndex = 0; edgeIndex < voronoiDiagram.size(); edgeIndex++) {
		DelaunayEdge3D& currentEdge = voronoiDiagram[edgeIndex];

		std::vector<DelaunayEdge3D> interesectedEdges = GetAllowedClippingDirections(voronoiDiagram, currentEdge);

		std::vector<Vec3> impactPoints;
		std::vector<Face> impactedFaces = GetFacesImpactedByEdge(currentEdge, convexPoly, impactPoints);

		/*DebugAddWorldPoint(currentEdge.m_pointA, 0.05f, 0.0f, Rgba8::RED, Rgba8::RED, DebugRenderMode::USEDEPTH, 2, 4);
		DebugAddWorldPoint(currentEdge.m_pointB, 0.05f, 0.0f, Rgba8::RED, Rgba8::RED, DebugRenderMode::USEDEPTH, 2, 4);*/


		for (int impactedInd = 0; impactedInd < impactedFaces.size(); impactedInd++) {
			Face const& impactedFace = impactedFaces[impactedInd];

			float distSqrPointAToCenter = GetDistanceSquared3D(currentEdge.m_pointA, convexPoly.m_middlePoint);
			float distSqrPointBToCenter = GetDistanceSquared3D(currentEdge.m_pointB, convexPoly.m_middlePoint);

			Vec3 closestToPolyCenter = (distSqrPointAToCenter < distSqrPointBToCenter) ? currentEdge.m_pointA : currentEdge.m_pointB;

			currentEdge = DelaunayEdge3D(closestToPolyCenter, impactPoints[impactedInd]);
			/*	DebugAddWorldPoint(impactPoints[impactedInd], 0.05f, 0.0f, Rgba8::CYAN, Rgba8::CYAN, DebugRenderMode::USEDEPTH, 2, 4);
				DebugAddWorldPoint(closestToPolyCenter, 0.05f, 0.0f, Rgba8::CYAN, Rgba8::CYAN, DebugRenderMode::USEDEPTH, 2, 4);*/

			if (includeEdgeProjections) {
				GetResultingProjectedEdges(voronoiDiagram, interesectedEdges, impactedFace, impactPoints[impactedInd], addedEdges);
			}
		}

	}


	voronoiDiagram.insert(voronoiDiagram.end(), addedEdges.begin(), addedEdges.end());


	// Some duplicated edges are created, due to having to add edges using raycasts with every single edge. Therefore, duplicates must be purged

	for (std::vector<DelaunayEdge3D>::iterator edgeIt = voronoiDiagram.begin(); edgeIt != voronoiDiagram.end(); edgeIt++) {
		DelaunayEdge3D const& currentEdge = *edgeIt;

		for (std::vector<DelaunayEdge3D>::iterator otherIt = edgeIt + 1; otherIt != voronoiDiagram.end();) {
			DelaunayEdge3D const& otherEdge = *otherIt;

			bool containsA = otherEdge.ContainsPoint(currentEdge.m_pointA, 0.0025f);
			bool containsB = otherEdge.ContainsPoint(currentEdge.m_pointB, 0.0025f);

			if (containsA && containsB) {
				otherIt = voronoiDiagram.erase(otherIt);
			}
			else {
				otherIt++;
			}


		}
	}


	return voronoiDiagram;
}

