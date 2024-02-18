#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/Vertex_PNCU.hpp"
#include "Engine/Math/AABB2.hpp"	
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/LineSegment2.hpp"
#include "Engine/Math/Capsule2.hpp"
#include <vector>

struct AABB2;
struct OBB2;
struct Mat44;
struct DelaunayConvexPoly2D;
class ConvexPoly2D;
class ConvexHull2D;

// 2D 
void TransformVertexArrayXY3D(int numVerts, Vertex_PCU* verts, float uniformScaleXY, float rotationDegreesAboutZ, Vec2 const& translationXY);
void TransformVertexArrayXY3D(int numVerts, Vertex_PCU* verts, Vec2 const& iBasis, Vec2 const& jBasis, Vec2 const& translationXY);
void AddVertsForAABB2D(std::vector<Vertex_PCU>& verts, AABB2 const& bounds, Rgba8 const& tint, AABB2 UVs = AABB2::ZERO_TO_ONE);
void AddVertsForAABB2D(std::vector<Vertex_PCU>& verts, AABB2 const& bounds, Rgba8 const& tint, const Vec2& uvAtMins, const Vec2& uvAtMaxs);
void AddVertsForHollowAABB2D(std::vector<Vertex_PCU>& verts, AABB2 const& bounds, float radius, Rgba8 const& tint);
void AddVertsForOBB2D(std::vector<Vertex_PCU>& verts, OBB2 const& bounds, Rgba8 const& tint, const Vec2& uvAtMins = Vec2::ZERO, const Vec2& uvAtMaxs = Vec2::ONE);
void AddVertsForOBB2D(std::vector<Vertex_PCU>& verts, OBB2 const& bounds, Rgba8 const& tint, AABB2 UVs);
void AddVertsForDisc2D(std::vector<Vertex_PCU>& verts, Vec2 const& discCenter, float radius, Rgba8 tint, int sectionsAmount = 60);
void AddVertsForLineSegment2D(std::vector<Vertex_PCU>& verts, LineSegment2 const& lineSegment, Rgba8 tint, float lineWidth = 0.2f, bool overhead = true);
void AddVertsForCapsule2D(std::vector<Vertex_PCU>& verts, Capsule2 const& capsule, Rgba8 tint, int amountSectorsCapsEnds = 32);
void AddVertsForArrow2D(std::vector<Vertex_PCU>& verts, Vec2 const& arrowStart, Vec2 const& arrowEnd, Rgba8 color = Rgba8::WHITE, float arrowBodySize = 0.2f, float arrowHeadSize = 2.0f);
void AddVertsForDelaunayConvexPoly2D(std::vector<Vertex_PCU>& verts, DelaunayConvexPoly2D const& convexPoly, Rgba8 const& color, AABB2 const& UVs = AABB2::ZERO_TO_ONE);
void AddVertsForWireDelaunayConvexPoly2D(std::vector<Vertex_PCU>& verts, DelaunayConvexPoly2D const& convexPoly, Rgba8 const& color, float lineThickness = 0.5f);
void AddVertsForConvexPoly2D(std::vector<Vertex_PCU>& verts, ConvexPoly2D const& convexPoly, Rgba8 const& color);
void AddVertsForWireConvexPoly2D(std::vector<Vertex_PCU>& verts, ConvexPoly2D const& convexPoly, Rgba8 const& borderColor, float lineThickness = 0.5f);
void AddVertsForConvexHull2D(std::vector<Vertex_PCU>& verts, ConvexHull2D const& convexHull, Rgba8 const& color, float planeDrawDistance, float lineThickness = 0.5f);

// 3D 
void TransformVertexArray3D(int numVerts, Vertex_PCU* verts, Mat44 const& model);
void AddVertsForLineSegment3D(std::vector<Vertex_PCU>& verts, Vec3 const& start, Vec3 const& end, Rgba8 const& color = Rgba8::WHITE,float thickness = 0.0125f);
void AddVertsForAABB3D(std::vector<Vertex_PCU>& verts, const AABB3& bounds, const Rgba8& color = Rgba8::WHITE, const AABB2& UVs = AABB2::ZERO_TO_ONE);
void AddVertsForAABB3D(std::vector<Vertex_PNCU>& verts, const AABB3& bounds, const Rgba8& color = Rgba8::WHITE, const AABB2& UVs = AABB2::ZERO_TO_ONE);
void AddVertsForIndexedAABB3D(std::vector<Vertex_PCU>& verts, std::vector<unsigned int>& indices, const AABB3& bounds, const Rgba8& color = Rgba8::WHITE, const AABB2& UVs = AABB2::ZERO_TO_ONE);
void AddVertsForWireAABB3D(std::vector<Vertex_PCU>& verts, const AABB3& bounds, const Rgba8& color = Rgba8::WHITE);
void AddVertsForSphere(std::vector<Vertex_PCU>& verts, float radius, int stacks = 16, int slices = 32, Rgba8 const& color = Rgba8::WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE);
void AddVertsForSphere(std::vector<Vertex_PNCU>& verts, float radius, int stacks = 16, int slices = 32, Rgba8 const& color = Rgba8::WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE);
void AddVertsForWireSphere(std::vector<Vertex_PCU>& verts, float radius, int stacks = 16, int slices = 32, Rgba8 const& color = Rgba8::WHITE);
void AddVertsForCylinder(std::vector<Vertex_PCU>& verts, Vec3 const& start, Vec3 const& end, float radius, int slices = 16, Rgba8 const& color = Rgba8::WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE);
void AddVertsForWireCylinder(std::vector<Vertex_PCU>& verts, Vec3 const& start, Vec3 const& end, float radius, int slices = 16, Rgba8 const& color = Rgba8::WHITE);
void AddVertsForCone3D(std::vector<Vertex_PCU>& verts, Vec3 const& start, Vec3 const& end, float radius, int slices = 16, Rgba8 const& color = Rgba8::WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE);
void AddVertsForWireCone3D(std::vector<Vertex_PCU>& verts, Vec3 const& start, Vec3 const& end, float radius, int slices = 16, Rgba8 const& color = Rgba8::WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE);
void AddVertsForArrow3D(std::vector<Vertex_PCU>& verts, Vec3 const& start, Vec3 const& end, float radius, int slices = 16, Rgba8 const& color = Rgba8::WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE);
void AddVertsForBasis3D(std::vector<Vertex_PCU>& verts, Mat44 const& model, float basisSize, float lineThickness);
void AddVertsForQuad3D(std::vector<Vertex_PCU>& verts, Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft, const Rgba8& color = Rgba8::WHITE, const AABB2& UVs = AABB2::ZERO_TO_ONE);
void AddVertsForQuad3D(std::vector<Vertex_PNCU>& verts, Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft, const Rgba8& color = Rgba8::WHITE, const AABB2& UVs = AABB2::ZERO_TO_ONE);
void AddVertsForRoundedQuad3D(std::vector<Vertex_PNCU>& vertexes, Vec3 const& topLeft, Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, const Rgba8& color = Rgba8::WHITE, const AABB2& UVs = AABB2::ZERO_TO_ONE);
void AddVertsForIndexedQuad3D(std::vector<Vertex_PCU>& verts, std::vector<unsigned int>& indices, Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft, const Rgba8& color = Rgba8::WHITE, const AABB2& UVs = AABB2::ZERO_TO_ONE);
void AddVertsForIndexedQuad3D(std::vector<Vertex_PNCU>& verts, std::vector<unsigned int>& indices, Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft, const Rgba8& color = Rgba8::WHITE, const AABB2& UVs = AABB2::ZERO_TO_ONE);
