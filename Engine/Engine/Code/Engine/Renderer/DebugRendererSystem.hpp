#pragma once
#include <string>

class Renderer;
class Clock;
class Camera;

struct Vec3;
struct Vec2;
struct Rgba8;
struct AABB3;
struct Mat44;

//------------------------------------------------------------------------
enum class DebugRenderMode
{
	ALWAYS = 0,
	USEDEPTH,
	XRAY,
	NUM_DEBUG_RENDER_MODES
};

//------------------------------------------------------------------------
struct DebugRenderConfig
{
	Renderer* m_renderer = nullptr;
	bool m_startHidden = false;
	std::string m_fontName = "";
};

// Setup
void DebugRenderSystemStartup(const DebugRenderConfig& config);
void DebugRenderSystemShutdown();

// Control
void DebugRenderSetVisible();
void DebugRenderSetHidden();
void DebugRenderClear();
void DebugRenderSetParentClock(Clock& parent);
void DebugRenderSetTimeDilation(double timeDilation);
Clock const& DebugRenderGetClock();

// Output
void DebugRenderBeginFrame();
void DebugRenderWorld(const Camera& camera);
void DebugRenderScreen(const Camera& camera);
void DebugRenderEndFrame();

// Geometry
void DebugAddWorldPoint(const Vec3& pos, float radius, float duration, const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode, int stacks = 16, int slices = 32);
void DebugAddWorldLine(const Vec3& start, const Vec3& end, float radius, float duration, const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode);
void DebugAddWorldWireCylinder(const Vec3& base, const Vec3& top, float radius, float duration, const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode);
void DebugAddWorldWireSphere(const Vec3& center, float radius, float duration, const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode, int stacks = 16, int slices = 32);
void DebugAddWorldArrow(const Vec3& start, const Vec3& end, float radius, float duration, const Rgba8& baseColor, const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode);
void DebugAddWorldBox(const AABB3& bounds, float duration, const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode);
void DebugAddWorldBasis(const Mat44& basis, float duration, const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode);
void DebugAddWorldText(const std::string& text, const Mat44& transform, float textHeight, const Vec2& alignment, float duration, const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode);
void DebugAddWorldBillboardText(const std::string& text, const Vec3& origin, float textHeight, const Vec2& alignment, float duration, const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode);
void DebugAddScreenText(const std::string& text, const Vec2& position, float duration, const Vec2& alignment, float size, const Rgba8& startColor, const Rgba8& endColor);
void DebugAddMessage(const std::string& text, float duration, const Rgba8& startColor, const Rgba8& endColor);
