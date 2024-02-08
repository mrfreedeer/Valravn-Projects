#include "Engine/Math/MathUtils.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Framework/App.hpp"

bool g_drawDebug = false;
BitmapFont* g_squirrelFont = nullptr;

void DebugDrawRing(Vec2 const& ringCenter, float radius, float thickness, Rgba8 const& color)
{
	Vertex_PCU vertexes[360];

	float halfThickness = thickness * 0.5f;
	float angle = 0;
	float deltaAngle = 6.0f;

	float innerRadius = radius - halfThickness;
	float outerRadius = radius + halfThickness;

	for (int j = 0; angle < 360; j += 6, angle += deltaAngle) {
		float angleCos = CosDegrees(angle);
		float angleSin = SinDegrees(angle);
		Vec2 localBottom(angleCos, angleSin);
		Vec2 localTop(angleCos, angleSin);
		localBottom *= innerRadius;
		localTop *= outerRadius;

		Vec2 localLeft(CosDegrees(angle + deltaAngle), SinDegrees(angle + deltaAngle));
		Vec2 localLeftTop(localLeft);

		localLeft *= innerRadius;
		localLeftTop *= outerRadius;

		Vec3 worldBottom(localBottom.x + ringCenter.x, localBottom.y + ringCenter.y, 0);
		Vec3 worldTop(localTop.x + ringCenter.x, localTop.y + ringCenter.y, 0);
		Vec3 worldLeft(localLeft.x + ringCenter.x, localLeft.y + ringCenter.y, 0);
		Vec3 worldLeftTop(localLeftTop.x + ringCenter.x, localLeftTop.y + ringCenter.y, 0);

		vertexes[j] = Vertex_PCU(worldBottom, color, Vec2());
		vertexes[j + 1] = Vertex_PCU(worldLeft, color, Vec2());
		vertexes[j + 2] = Vertex_PCU(worldTop, color, Vec2());

		vertexes[j + 3] = Vertex_PCU(worldLeftTop, color, Vec2());
		vertexes[j + 4] = Vertex_PCU(worldLeft, color, Vec2());
		vertexes[j + 5] = Vertex_PCU(worldTop, color, Vec2());
	}

	g_theRenderer->DrawVertexArray(360, vertexes);

}


void DebugDrawLine(Vec2 const& lineStart, Vec2 const& lineEnd, float thickness, Rgba8 const& color) {

	Vec2 fwd = (lineEnd - lineStart);
	fwd = fwd.GetNormalized();
	Vec2 left = fwd.GetRotated90Degrees();
	left *= thickness * .5f;
	fwd *= thickness * .5f;

	Vertex_PCU vertexes[6];

	Vec2 startLeft = lineStart + left - fwd;
	Vec2 startRight = lineStart - left - fwd;

	Vec2 endLeft = lineEnd + left + fwd;
	Vec2 endRight = lineEnd - left + fwd;

	vertexes[0] = Vertex_PCU(Vec3(startLeft.x, startLeft.y, 0.f), color, Vec2());
	vertexes[1] = Vertex_PCU(Vec3(startRight.x, startRight.y, 0.f), color, Vec2());
	vertexes[2] = Vertex_PCU(Vec3(endLeft.x, endLeft.y, 0.f), color, Vec2());

	vertexes[3] = Vertex_PCU(Vec3(endLeft.x, endLeft.y, 0.f), color, Vec2());
	vertexes[4] = Vertex_PCU(Vec3(startRight.x, startRight.y, 0.f), color, Vec2());
	vertexes[5] = Vertex_PCU(Vec3(endRight.x, endRight.y, 0.f), color, Vec2());

	g_theRenderer->DrawVertexArray(6, vertexes);
}

void DrawCircle(Vec2 const& circleCenter, float radius, Rgba8 const& color)
{

	Vertex_PCU vertexes[360];
	float angle = 0;

	float deltaAngle = 6.0f;

	for (int j = 0; angle < 360; j += 6, angle += deltaAngle) {

		float angleCos = CosDegrees(angle);
		float angleSin = SinDegrees(angle);
		Vec2 bottom;
		Vec2 localTop(angleCos, angleSin);

		localTop *= radius;

		Vec2 localLeftTop(CosDegrees(angle + deltaAngle), SinDegrees(angle + deltaAngle));

		localLeftTop *= radius;

		Vec3 worldBottom(bottom.x + circleCenter.x, bottom.y + circleCenter.y, 0);
		Vec3 worldTop(localTop.x + circleCenter.x, localTop.y + circleCenter.y, 0);
		Vec3 worldLeftTop(localLeftTop.x + circleCenter.x, localLeftTop.y + circleCenter.y, 0);


		vertexes[j] = Vertex_PCU(worldBottom, color, Vec2());
		vertexes[j + 1] = Vertex_PCU(worldBottom, color, Vec2());
		vertexes[j + 2] = Vertex_PCU(worldTop, color, Vec2());

		vertexes[j + 3] = Vertex_PCU(worldLeftTop, color, Vec2());
		vertexes[j + 4] = Vertex_PCU(worldBottom, color, Vec2());
		vertexes[j + 5] = Vertex_PCU(worldTop, color, Vec2());
	}

	g_theRenderer->DrawVertexArray(360, vertexes);
}
