#include "Engine/Math/AABB3.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Game/Gameplay/Prop.hpp"
#include "Game/Gameplay/Game.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Engine/Renderer/Renderer.hpp"

Prop::Prop(Game* pointerToGame, Vec3 const& startingWorldPosition, PropRenderType renderType, IntVec2 gridSize) :
	Entity(pointerToGame, startingWorldPosition),
	m_type(renderType),
	m_gridSize(gridSize)
{
	InitializeLocalVerts();
}

Prop::Prop(Game* pointerToGame, Vec3 const& startingWorldPosition, float radius, PropRenderType renderType) :
	Entity(pointerToGame, startingWorldPosition),
	m_type(renderType),
	m_sphereRadius(radius)
{
	InitializeLocalVerts();
}

Prop::~Prop()
{
}


void Prop::InitializeLocalVerts()
{
	switch (m_type)
	{
	case PropRenderType::CUBE:
		InitiliazeLocalVertsCube();
		break;
	case PropRenderType::GRID:
		InitiliazeLocalVertsGrid();
		break;
	case PropRenderType::SPHERE:
		InitializeLocalVertsSphere();
		break;
	}
}

void Prop::InitiliazeLocalVertsCube()
{
	AABB3 bounds;
	bounds.SetDimensions(Vec3::ONE);
	bounds.SetCenter(Vec3::ZERO);
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

	AddVertsForQuad3D(m_verts, leftFrontBottom, rightFrontBottom, rightFrontTop, leftFrontTop, Rgba8::CYAN); // Front
	AddVertsForQuad3D(m_verts, rightFrontBottom, rightBackBottom, rightBackTop, rightFrontTop, Rgba8(255, 0, 255)); // Right
	AddVertsForQuad3D(m_verts, rightBackBottom, leftBackBottom, leftBackTop, rightBackTop, Rgba8::RED); // Back
	AddVertsForQuad3D(m_verts, leftBackBottom, leftFrontBottom, leftFrontTop, leftBackTop, Rgba8::GREEN); // Left
	AddVertsForQuad3D(m_verts, leftFrontTop, rightFrontTop, rightBackTop, leftBackTop, Rgba8::BLUE); // Top
	AddVertsForQuad3D(m_verts, leftBackBottom, rightBackBottom, rightFrontBottom, leftFrontBottom, Rgba8::YELLOW); // Bottom;
}

void Prop::InitiliazeLocalVertsGrid() {
	float halfMaxXUnit = static_cast<float>(m_gridSize.x / 2);
	float halfMaxYUnit = static_cast<float>(m_gridSize.y / 2);

	float lineWidth = 0.01f;
	float fivelineWidth = lineWidth * 4.0f;

	AABB3 originXAxis(Vec3(-halfMaxXUnit, -fivelineWidth, -fivelineWidth), Vec3(halfMaxXUnit, fivelineWidth, fivelineWidth));
	AABB3 originYAxis(Vec3(-fivelineWidth, -halfMaxYUnit, -fivelineWidth), Vec3(fivelineWidth, halfMaxYUnit, fivelineWidth));

	AddVertsForAABB3D(m_verts, originXAxis);
	AddVertsForAABB3D(m_verts, originYAxis);
	// CHANGE CODE
	for (int axisLineIndex = 1; axisLineIndex <= (int)halfMaxXUnit; axisLineIndex++) {

		bool divBy5 = (axisLineIndex % 5) == 0;
		float usedLineWidth = (divBy5) ? fivelineWidth : lineWidth;

		AABB3 xAxisLineAABB3(Vec3(-halfMaxXUnit, (float)axisLineIndex - usedLineWidth, -usedLineWidth), Vec3(halfMaxXUnit, (float)axisLineIndex + usedLineWidth, usedLineWidth));
		AABB3 xNegAxisLineAABB3(Vec3(-halfMaxXUnit, -(float)axisLineIndex - usedLineWidth, -usedLineWidth), Vec3(halfMaxXUnit, -(float)axisLineIndex + usedLineWidth, usedLineWidth));

		AABB3 yAxisLineAABB3(Vec3((float)axisLineIndex - usedLineWidth, -halfMaxYUnit, -usedLineWidth), Vec3((float)axisLineIndex + usedLineWidth, halfMaxYUnit, usedLineWidth));
		AABB3 yNegAxisLineAABB3(Vec3(-(float)axisLineIndex - usedLineWidth, -halfMaxYUnit, -usedLineWidth), Vec3(-(float)axisLineIndex + usedLineWidth, halfMaxYUnit, usedLineWidth));

		Rgba8 xAxisColor = (divBy5) ? Rgba8::RED : Rgba8::SILVER;
		Rgba8 yAxisColor = (divBy5) ? Rgba8::GREEN : Rgba8::SILVER;

		AddVertsForAABB3D(m_verts, xAxisLineAABB3, xAxisColor);
		AddVertsForAABB3D(m_verts, xNegAxisLineAABB3, xAxisColor);

		AddVertsForAABB3D(m_verts, yAxisLineAABB3, yAxisColor);
		AddVertsForAABB3D(m_verts, yNegAxisLineAABB3, yAxisColor);
	}
}

void Prop::InitializeLocalVertsSphere() {
	AddVertsForSphere(m_verts, m_sphereRadius, 8, 16);
	//AddVertsForWireCylinder(m_verts, Vec3::ZERO, Vec3(0,0,10), 2);

}


void Prop::Update(float deltaSeconds)
{
	Entity::Update(deltaSeconds);

	if (m_type == PropRenderType::CUBE) {
		float interpolationColor = RangeMapClamped(SinDegrees((float)Clock::GetSystemClock().GetTotalTime() * 100.0f), -1.0f, 1.0f, 0.0f, 1.0f);
		interpolationColor = ClampZeroToOne(interpolationColor);
		m_modelColor.r = static_cast<unsigned char>(Interpolate(0, 255, interpolationColor));
		m_modelColor.g = static_cast<unsigned char>(Interpolate(0, 255, interpolationColor));
		m_modelColor.b = 255;
	}
}

void Prop::Render() const
{
	g_theRenderer->SetModelColor(m_modelColor);
	Mat44 modelMatrix = GetModelMatrix();
	g_theRenderer->SetModelMatrix(modelMatrix);

	switch (m_type) {
	case PropRenderType::CUBE:
		RenderMultiColoredCube();
		break;
	case PropRenderType::GRID:
		RenderGrid();
		break;
	case PropRenderType::SPHERE:
		RenderSphere();
		break;
	}
}

void Prop::RenderMultiColoredCube() const
{
	g_theRenderer->BindTexture(g_textures[(int)GAME_TEXTURE::CompanionCube]);
	g_theRenderer->DrawVertexArray(m_verts);
}

void Prop::RenderGrid() const
{
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray(m_verts);

}

void Prop::RenderSphere() const
{
	g_theRenderer->BindTexture(g_textures[(int)GAME_TEXTURE::TestUV]);
	//g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray(m_verts);
}
