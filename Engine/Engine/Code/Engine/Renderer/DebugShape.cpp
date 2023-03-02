#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/DebugShape.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/Billboard.hpp"
#include "Engine/Math/Mat44.hpp"


DebugShape::DebugShape(Mat44 const& modelMatrix, DebugRenderMode debugRenderMode, float duration, Clock const& parentClock, Rgba8 startColor, Rgba8 endColor, Texture const* texture):
	m_modelMatrix(modelMatrix),
	m_debugRenderMode(debugRenderMode),
	m_startColor(startColor),
	m_endColor(endColor),
	m_texture(texture)
{
	m_stopwach.Start(&parentClock, duration);
}

DebugShape::DebugShape(std::string const& text , ScrenTextType screenTextType, float duration, Clock const& parentClock, Rgba8 startColor, Rgba8 endColor, Texture const* texture) :
	m_text(text),
	m_screenTextType(screenTextType),
	m_startColor(startColor),
	m_endColor(endColor),
	m_texture(texture)
{
	m_stopwach.Start(&parentClock, duration);
}

bool DebugShape::CanShapeBeDeleted()
{
	return m_stopwach.HasDurationElapsed();
}

void DebugShape::Render(Renderer* renderer) const
{
	renderer->BindTexture(m_texture);
	renderer->DrawVertexArray(m_verts);
}

Mat44 const DebugShape::GetBillboardModelMatrix(Camera const& camera) const
{
	Vec3 position = m_modelMatrix.GetTranslation3D();
	Mat44 billboardMat = Billboard::GetModelMatrixForBillboard(position, camera, BillboardType::CameraFacingXYZ);
	return billboardMat;
}

Rgba8 const DebugShape::GetModelColor() const
{
	if(m_stopwach.m_duration == 0.0) return m_startColor;
	return Rgba8::InterpolateColors(m_startColor, m_endColor, m_stopwach.GetElapsedFraction());
}
