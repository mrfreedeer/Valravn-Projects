#pragma  once

#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/Stopwatch.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Renderer/DebugRendererSystem.hpp"

#include <vector>
enum class ScrenTextType {
	ScreenMessage,
	FreeText,
	NUM_SCREEN_TEXT_TYPES
};

class Renderer;
class Camera;
class Texture;
struct Mat44;

struct DebugShape {

public:
	DebugShape(Mat44 const& modelMatrix, DebugRenderMode debugRenderMode, float duration, Clock const& parentClock, Rgba8 startColor = Rgba8::WHITE, Rgba8 endColor = Rgba8::WHITE, Texture const* texture = nullptr);
	DebugShape(std::string const& text, ScrenTextType screenTextType, float duration, Clock const& parentClock, Rgba8 startColor = Rgba8::WHITE, Rgba8 endColor = Rgba8::WHITE, Texture const* texture = nullptr);

	bool CanShapeBeDeleted();
	void Render(Renderer* renderer) const;
	Mat44 const& GetModelMatrix() const { return m_modelMatrix; }
	Mat44 const GetBillboardModelMatrix(Camera const& camera) const;
	Rgba8 const GetModelColor() const;
public:
	std::vector<Vertex_PCU> m_verts;
	Texture const* m_texture = nullptr;
	Stopwatch m_stopwach;
	Mat44 m_modelMatrix;
	DebugRenderMode m_debugRenderMode = DebugRenderMode::USEDEPTH;
	ScrenTextType m_screenTextType = ScrenTextType::FreeText;
	Rgba8 m_startColor;
	Rgba8 m_endColor;
	bool m_isBillboard = false;
	bool m_isWorldShape = true;
	bool  m_isWorldText = false;
	std::string m_text = "";
};