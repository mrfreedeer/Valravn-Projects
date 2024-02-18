#include "Engine/Renderer/DebugRendererSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Renderer/DebugShape.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Renderer/Billboard.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include <vector>

class DebugRenderSystem {
public:
	DebugRenderSystem(DebugRenderConfig debugSystemConfig);
	~DebugRenderSystem();
	void Startup();
	void Shutdown();
	void Clear();
	void CheckAllShapes();

	void RenderWorldShapes(Camera const& camera) const;
	void RenderScreenShapes(Camera const& camera) const;
	void AddShape(DebugShape* newShape);
	void AddWireShape(DebugShape* newShape);
	void AddBillboard(DebugShape* newShape);
	void SetRenderModes(DebugRenderMode renderMode) const;
	BitmapFont* GetBitmapFont() const;

	Clock const& GetClock() const { return m_clock; }
	void SetTimeDilation(double timeDilation) { m_clock.SetTimeDilation(timeDilation); }
public:
	bool m_isVisible = false;
	Clock m_clock;

private:
	void RenderWireframeShapes(Renderer* renderer) const;
	void RenderBillboardShapes(Renderer* renderer, Camera const& camera) const;
	void RenderDebugShapes(Renderer* renderer) const;
	void RenderTextDebugShapes(Renderer* renderer) const;
	void RenderFreeScreenText(Renderer* renderer) const;
	void RenderTextMessages(Renderer* renderer, Camera const& camera) const;
private:
	mutable std::mutex m_debugShapesMutex[(int)DebugRenderMode::NUM_DEBUG_RENDER_MODES];
	std::vector<DebugShape*> m_debugShapes[(int)DebugRenderMode::NUM_DEBUG_RENDER_MODES];

	mutable std::mutex m_debugTextShapesMutex[(int)DebugRenderMode::NUM_DEBUG_RENDER_MODES];
	std::vector<DebugShape*> m_debugTextShapes[(int)DebugRenderMode::NUM_DEBUG_RENDER_MODES];

	mutable std::mutex m_wireFrameShapesMutex;
	std::vector<DebugShape*> m_wireframeShapes;

	mutable std::mutex m_billboardsMutex;
	std::vector<DebugShape*> m_billboards;

	mutable std::mutex m_screenTextShapesMutex[(int)ScrenTextType::NUM_SCREEN_TEXT_TYPES];
	std::vector<DebugShape*> m_screnTextShapes[(int)ScrenTextType::NUM_SCREEN_TEXT_TYPES];

	DebugRenderConfig m_config;

	Material* m_materials[(int)DebugRenderMode::NUM_DEBUG_RENDER_MODES] = {};
};


DebugRenderSystem::DebugRenderSystem(DebugRenderConfig debugSystemConfig) :
	m_config(debugSystemConfig),
	m_isVisible(!debugSystemConfig.m_startHidden)
{
}

DebugRenderSystem::~DebugRenderSystem()
{

}

void DebugRenderSystem::Startup()
{
	int reserveSize = 100;

	for (int debugRenderTypeIndex = 0; debugRenderTypeIndex < (int)DebugRenderMode::NUM_DEBUG_RENDER_MODES; debugRenderTypeIndex++) {
		std::mutex& listMutex = m_debugShapesMutex[debugRenderTypeIndex];

		listMutex.lock();

		std::vector<DebugShape*>& debugShapeList = m_debugShapes[debugRenderTypeIndex];
		debugShapeList.reserve(reserveSize);

		listMutex.unlock();
	}

	for (int debugRenderTypeIndex = 0; debugRenderTypeIndex < (int)DebugRenderMode::NUM_DEBUG_RENDER_MODES; debugRenderTypeIndex++) {
		std::mutex& listMutex = m_debugTextShapesMutex[debugRenderTypeIndex];

		listMutex.lock();

		std::vector<DebugShape*>& debugShapeList = m_debugTextShapes[debugRenderTypeIndex];
		debugShapeList.reserve(reserveSize);

		listMutex.unlock();
	}


	m_wireFrameShapesMutex.lock();

	m_wireframeShapes.reserve(reserveSize);

	m_wireFrameShapesMutex.unlock();


	m_billboardsMutex.lock();

	m_billboards.reserve(reserveSize);

	m_billboards.clear();

	m_billboardsMutex.unlock();

	for (int screenShapeType = 0; screenShapeType < (int)ScrenTextType::NUM_SCREEN_TEXT_TYPES; screenShapeType++) {
		std::mutex& listMutex = m_screenTextShapesMutex[screenShapeType];
		listMutex.lock();

		std::vector<DebugShape*>& debugShapeList = m_screnTextShapes[screenShapeType];
		debugShapeList.reserve(reserveSize);

		listMutex.unlock();
	}


	std::string enginePath = ENGINE_MAT_DIR;
	m_materials[(int)DebugRenderMode::ALWAYS] = m_config.m_renderer->CreateOrGetMaterial(enginePath + "DebugAlwaysMaterial");
	m_materials[(int)DebugRenderMode::USEDEPTH] = m_config.m_renderer->CreateOrGetMaterial(enginePath + "DebugDepthMaterial");
	m_materials[(int)DebugRenderMode::XRAY] = m_config.m_renderer->CreateOrGetMaterial(enginePath + "DebugXRayMaterial");
}

void DebugRenderSystem::Shutdown()
{
	Clear();
}

void DebugRenderSystem::Clear()
{

	for (int debugRenderTypeIndex = 0; debugRenderTypeIndex < (int)DebugRenderMode::NUM_DEBUG_RENDER_MODES; debugRenderTypeIndex++) {
		std::mutex& listMutex = m_debugShapesMutex[debugRenderTypeIndex];

		listMutex.lock();

		std::vector<DebugShape*>& debugShapeList = m_debugShapes[debugRenderTypeIndex];
		for (int shapeIndex = 0; shapeIndex < debugShapeList.size(); shapeIndex++) {
			DebugShape*& shape = debugShapeList[shapeIndex];
			if (shape) {
				delete shape;
				shape = nullptr;
			}
		}
		debugShapeList.clear();

		listMutex.unlock();
	}

	for (int debugRenderTypeIndex = 0; debugRenderTypeIndex < (int)DebugRenderMode::NUM_DEBUG_RENDER_MODES; debugRenderTypeIndex++) {
		std::mutex& listMutex = m_debugTextShapesMutex[debugRenderTypeIndex];

		listMutex.lock();

		std::vector<DebugShape*>& debugShapeList = m_debugTextShapes[debugRenderTypeIndex];
		for (int shapeIndex = 0; shapeIndex < debugShapeList.size(); shapeIndex++) {
			DebugShape*& shape = debugShapeList[shapeIndex];
			if (shape) {
				delete shape;
				shape = nullptr;
			}
		}
		debugShapeList.clear();

		listMutex.unlock();
	}


	m_wireFrameShapesMutex.lock();

	for (int shapeIndex = 0; shapeIndex < m_wireframeShapes.size(); shapeIndex++) {

		DebugShape*& shape = m_wireframeShapes[shapeIndex];
		if (shape) {
			delete shape;
			shape = nullptr;
		}
	}
	m_wireframeShapes.clear();

	m_wireFrameShapesMutex.unlock();


	m_billboardsMutex.lock();

	for (int shapeIndex = 0; shapeIndex < m_billboards.size(); shapeIndex++) {
		DebugShape*& shape = m_billboards[shapeIndex];
		if (shape) {
			delete shape;
			shape = nullptr;
		}
	}
	m_billboards.clear();

	m_billboardsMutex.unlock();

	for (int screenShapeType = 0; screenShapeType < (int)ScrenTextType::NUM_SCREEN_TEXT_TYPES; screenShapeType++) {
		std::mutex& listMutex = m_screenTextShapesMutex[screenShapeType];
		listMutex.lock();

		std::vector<DebugShape*>& debugShapeList = m_screnTextShapes[screenShapeType];
		for (int shapeIndex = 0; shapeIndex < debugShapeList.size(); shapeIndex++) {
			DebugShape*& shape = debugShapeList[shapeIndex];
			if (shape) {
				delete shape;
				shape = nullptr;
			}
		}
		debugShapeList.clear();

		listMutex.unlock();
	}

}

void DebugRenderSystem::CheckAllShapes()
{

	for (int debugRenderTypeIndex = 0; debugRenderTypeIndex < (int)DebugRenderMode::NUM_DEBUG_RENDER_MODES; debugRenderTypeIndex++) {
		std::mutex& listMutex = m_debugShapesMutex[debugRenderTypeIndex];

		std::vector<DebugShape*>& debugShapeList = m_debugShapes[debugRenderTypeIndex];

		listMutex.lock();
		for (int shapeIndex = 0; shapeIndex < debugShapeList.size(); shapeIndex++) {
			DebugShape*& shape = debugShapeList[shapeIndex];
			if (shape) {
				if (shape->CanShapeBeDeleted()) {
					delete shape;
					shape = nullptr;
				}
			}
		}
		listMutex.unlock();

	}

	for (int debugRenderTypeIndex = 0; debugRenderTypeIndex < (int)DebugRenderMode::NUM_DEBUG_RENDER_MODES; debugRenderTypeIndex++) {

		std::mutex& listMutex = m_debugTextShapesMutex[debugRenderTypeIndex];

		listMutex.lock();
		std::vector<DebugShape*>& debugShapeList = m_debugTextShapes[debugRenderTypeIndex];

		for (int shapeIndex = 0; shapeIndex < debugShapeList.size(); shapeIndex++) {
			DebugShape*& shape = debugShapeList[shapeIndex];
			if (shape) {
				if (shape->CanShapeBeDeleted()) {
					delete shape;
					shape = nullptr;
				}
			}
		}

		listMutex.unlock();
	}

	m_wireFrameShapesMutex.lock();

	for (int shapeIndex = 0; shapeIndex < m_wireframeShapes.size(); shapeIndex++) {
		DebugShape*& shape = m_wireframeShapes[shapeIndex];
		if (shape) {
			if (shape->CanShapeBeDeleted()) {
				delete shape;
				shape = nullptr;
			}
		}
	}

	m_wireFrameShapesMutex.unlock();

	m_billboardsMutex.lock();

	for (int shapeIndex = 0; shapeIndex < m_billboards.size(); shapeIndex++) {
		DebugShape*& shape = m_billboards[shapeIndex];
		if (shape) {
			if (shape->CanShapeBeDeleted()) {
				delete shape;
				shape = nullptr;
			}
		}
	}

	m_billboardsMutex.unlock();

	for (int screenTextTypeIndex = 0; screenTextTypeIndex < (int)ScrenTextType::NUM_SCREEN_TEXT_TYPES; screenTextTypeIndex++) {

		std::mutex& listMutex = m_screenTextShapesMutex[screenTextTypeIndex];
		listMutex.lock();

		std::vector<DebugShape*>& debugShapeList = m_screnTextShapes[screenTextTypeIndex];
		for (int shapeIndex = 0; shapeIndex < debugShapeList.size(); shapeIndex++) {
			DebugShape*& shape = debugShapeList[shapeIndex];
			if (shape) {
				if (shape->CanShapeBeDeleted()) {
					delete shape;
					shape = nullptr;
				}
			}
		}

		listMutex.unlock();
	}

}

void DebugRenderSystem::RenderWorldShapes(Camera const& camera) const
{
	if (!m_isVisible) return;
	//#TODO DX12 FIXTHIS
	Renderer* renderer = m_config.m_renderer;
	renderer->BeginCamera(camera); {
		renderer->BindMaterial(nullptr);
		renderer->BindTexture(nullptr);

		RenderWireframeShapes(renderer);
		RenderBillboardShapes(renderer, camera);
		RenderDebugShapes(renderer);
		RenderTextDebugShapes(renderer);
	}
	renderer->EndCamera(camera);

}

void DebugRenderSystem::RenderWireframeShapes(Renderer* renderer) const
{
	SetRenderModes(DebugRenderMode::USEDEPTH);

	m_wireFrameShapesMutex.lock();

	//#TODO DX12 FIXTHIS

	for (int shapeIndex = 0; shapeIndex < m_wireframeShapes.size(); shapeIndex++) {
		DebugShape* const shape = m_wireframeShapes[shapeIndex];
		if (shape) {
			renderer->SetModelMatrix(shape->GetModelMatrix());
			renderer->SetModelColor(shape->GetModelColor());
			shape->Render(renderer);
		}
	}

	m_wireFrameShapesMutex.unlock();
}

void DebugRenderSystem::RenderBillboardShapes(Renderer* renderer, Camera const& camera) const
{
	//#TODO DX12 FIXTHIS

	renderer->BindMaterial(m_materials[(int)DebugRenderMode::ALWAYS]);
	BitmapFont const* font = GetBitmapFont();

	renderer->BindTexture(&font->GetTexture());

	m_billboardsMutex.lock();

	for (int shapeIndex = 0; shapeIndex < m_billboards.size(); shapeIndex++) {
		DebugShape* const shape = m_billboards[shapeIndex];
		if (!shape) continue;
		Mat44 modelMat = shape->GetBillboardModelMatrix(camera);
		modelMat.SetTranslation3D(shape->GetModelMatrix().GetTranslation3D());
		renderer->SetModelMatrix(modelMat);
		renderer->SetModelColor(shape->GetModelColor());
		shape->Render(renderer);
	}

	m_billboardsMutex.unlock();

}

void DebugRenderSystem::RenderDebugShapes(Renderer* renderer) const
{
	for (int debugRenderTypeIndex = 0; debugRenderTypeIndex < (int)DebugRenderMode::NUM_DEBUG_RENDER_MODES; debugRenderTypeIndex++) {

		std::mutex& listMutex = m_debugShapesMutex[debugRenderTypeIndex];

		listMutex.lock();

		std::vector<DebugShape*>const& debugShapeList = m_debugShapes[debugRenderTypeIndex];
		SetRenderModes((DebugRenderMode)debugRenderTypeIndex);

		for (int shapeIndex = 0; shapeIndex < (int)debugShapeList.size(); shapeIndex++) {
			DebugShape* const shape = debugShapeList[shapeIndex];
			if (shape) {
				renderer->SetModelMatrix(shape->GetModelMatrix());
				renderer->SetModelColor(shape->GetModelColor());
				shape->Render(renderer);
			}
		}

		listMutex.unlock();
	}
}

void DebugRenderSystem::RenderTextDebugShapes(Renderer* renderer) const
{
	for (int debugRenderTypeIndex = 0; debugRenderTypeIndex < (int)DebugRenderMode::NUM_DEBUG_RENDER_MODES; debugRenderTypeIndex++) {

		std::mutex& listMutex = m_debugTextShapesMutex[debugRenderTypeIndex];
		listMutex.lock();

		std::vector<DebugShape*>const& debugShapeList = m_debugTextShapes[debugRenderTypeIndex];
		SetRenderModes((DebugRenderMode)debugRenderTypeIndex);
		//#TODO DX12 FIXTHIS

		for (int shapeIndex = 0; shapeIndex < (int)debugShapeList.size(); shapeIndex++) {
			DebugShape* const shape = debugShapeList[shapeIndex];
			if (shape) {
				renderer->SetModelMatrix(shape->GetModelMatrix());
				renderer->SetModelColor(shape->GetModelColor());
				shape->Render(renderer);
			}
		}

		listMutex.unlock();
	}
}


void DebugRenderSystem::RenderScreenShapes(Camera const& camera) const
{
	if (!m_isVisible) return;

	Renderer* renderer = m_config.m_renderer;
	//#TODO DX12 FIXTHIS

	renderer->BeginCamera(camera);
	Material* default2DMat = renderer->GetMaterialForName("Default2DMaterial");
	renderer->BindMaterial(default2DMat);

	RenderFreeScreenText(renderer);
	RenderTextMessages(renderer, camera);

	renderer->EndCamera(camera);

}

void DebugRenderSystem::RenderFreeScreenText(Renderer* renderer) const
{
	std::mutex& listMutex = m_screenTextShapesMutex[(int)ScrenTextType::FreeText];
	listMutex.lock();


	std::vector<DebugShape*>const& debugShapeList = m_screnTextShapes[(int)ScrenTextType::FreeText];
	for (int shapeIndex = 0; shapeIndex < debugShapeList.size(); shapeIndex++) {
		DebugShape const* shape = debugShapeList[shapeIndex];
		if (shape) {
			//#TODO DX12 FIXTHIS

			renderer->SetModelColor(shape->GetModelColor());
			shape->Render(renderer);
		}
	}

	listMutex.unlock();
}

void DebugRenderSystem::RenderTextMessages(Renderer* renderer, Camera const& camera) const
{
	float cellHeight = camera.GetOrthoTopRight().y * 0.02f;

	AABB2 bounds(camera.GetOrthoBottomLeft(), camera.GetOrthoTopRight());
	AABB2 inputBounds(camera.GetOrthoBottomLeft(), camera.GetOrthoTopRight());
	BitmapFont* font = GetBitmapFont();

	float minGroupTextHeight = bounds.m_maxs.y;
	float maxLinesShown = 21.5f;

	std::mutex& listMutex = m_screenTextShapesMutex[(int)ScrenTextType::ScreenMessage];
	listMutex.lock();

	std::vector<DebugShape*> const& textShapes = m_screnTextShapes[(int)ScrenTextType::ScreenMessage];

	std::vector<Vertex_PCU> textVerts;
	textVerts.reserve(textShapes.size() * 40);

	int roundedUpMaxLinesShown = RoundDownToInt(maxLinesShown) + 2;
	float lineWidth = 0;
	int renderLineIndex = 1;
	for (int lineIndex = 0; lineIndex < textShapes.size(); lineIndex++ && roundedUpMaxLinesShown >= 0, roundedUpMaxLinesShown--) {
		DebugShape const* shape = textShapes[lineIndex];
		if (!shape) continue;

		lineWidth = font->GetTextWidth(cellHeight, shape->m_text);

		AABB2 lineAABB2(Vec2::ZERO, Vec2(lineWidth, cellHeight));
		bounds.AlignABB2WithinBounds(lineAABB2, Vec2(0.0f, 1.0f));

		lineAABB2.m_mins.y = minGroupTextHeight - ((renderLineIndex) * (cellHeight * 2.0f));
		Rgba8 textColor = Rgba8::InterpolateColors(shape->m_startColor, shape->m_endColor, shape->m_stopwach.GetElapsedFraction());
		font->AddVertsForTextInBox2D(textVerts, lineAABB2, cellHeight, shape->m_text, textColor);
		renderLineIndex++;
	}

	listMutex.unlock();
	//#TODO DX12 FIXTHIS

	renderer->BindTexture(&font->GetTexture());
	if (textVerts.size() > 0) {
		renderer->DrawVertexArray(textVerts);
	}
}

void DebugRenderSystem::AddShape(DebugShape* newShape)
{

	if (newShape->m_stopwach.m_duration == -1.0f) {
		newShape->m_stopwach.m_duration = FLT_MAX;
	}

	if (newShape->m_debugRenderMode == DebugRenderMode::XRAY) {
		DebugShape* solidShape = new DebugShape(*newShape);
		solidShape->m_debugRenderMode = DebugRenderMode::USEDEPTH;
		AddShape(solidShape);
		newShape->m_startColor.a = 120;
		newShape->m_endColor.a = 120;
	}


	std::mutex& debugShapeListMutex = m_debugShapesMutex[(int)newShape->m_debugRenderMode];
	debugShapeListMutex.lock();																// lock

	std::vector<DebugShape*>& debugShapeList = m_debugShapes[(int)newShape->m_debugRenderMode];

	std::mutex& textDebugMutex = m_debugTextShapesMutex[(int)newShape->m_debugRenderMode];
	textDebugMutex.lock();																	// lock

	std::vector<DebugShape*>& textDebugShapeList = m_debugTextShapes[(int)newShape->m_debugRenderMode];

	std::vector<DebugShape*>& usedDebugShapeList = (newShape->m_isWorldText) ? textDebugShapeList : debugShapeList;


	std::mutex& screenTextMutex = m_screenTextShapesMutex[(int)newShape->m_screenTextType];
	screenTextMutex.lock();																	// lock

	std::vector<DebugShape*>& screnTextShapeList = m_screnTextShapes[(int)newShape->m_screenTextType];

	std::vector<DebugShape*>& usedShapeList = (newShape->m_isWorldShape) ? usedDebugShapeList : screnTextShapeList;

	for (int shapeIndex = 0; shapeIndex < (int)usedShapeList.size(); shapeIndex++) {
		DebugShape*& shape = usedShapeList[shapeIndex];
		if (!shape) {
			shape = newShape;

			debugShapeListMutex.unlock();
			textDebugMutex.unlock();
			screenTextMutex.unlock();


			return;
		}
	}
	usedShapeList.push_back(newShape);


	debugShapeListMutex.unlock();
	textDebugMutex.unlock();
	screenTextMutex.unlock();
}

void DebugRenderSystem::AddWireShape(DebugShape* newShape)
{
	if (newShape->m_stopwach.m_duration == -1.0f) {
		newShape->m_stopwach.m_duration = FLT_MAX;
	}

	m_wireFrameShapesMutex.lock();

	for (int shapeIndex = 0; shapeIndex < m_wireframeShapes.size(); shapeIndex++) {
		DebugShape*& shape = m_wireframeShapes[shapeIndex];
		if (!shape) {
			shape = newShape;
			m_wireFrameShapesMutex.unlock();
			return;
		}
	}
	m_wireframeShapes.push_back(newShape);
	m_wireFrameShapesMutex.unlock();

	if (newShape->m_debugRenderMode == DebugRenderMode::XRAY) {
		DebugShape* solidShape = new DebugShape(*newShape);
		solidShape->m_debugRenderMode = DebugRenderMode::USEDEPTH;
		AddWireShape(solidShape);
	}
}

void DebugRenderSystem::AddBillboard(DebugShape* newShape)
{
	if (newShape->m_stopwach.m_duration == -1.0f) {
		newShape->m_stopwach.m_duration = FLT_MAX;
	}

	m_billboardsMutex.lock();

	for (int shapeIndex = 0; shapeIndex < m_billboards.size(); shapeIndex++) {
		DebugShape*& shape = m_billboards[shapeIndex];
		if (!shape) {
			shape = newShape;
			m_billboardsMutex.unlock();
			return;
		}
	}
	m_billboards.push_back(newShape);

	m_billboardsMutex.unlock();
}

void DebugRenderSystem::SetRenderModes(DebugRenderMode renderMode) const
{
	Renderer* renderer = m_config.m_renderer;
	//#TODO DX12 FIXTHIS

	switch (renderMode)
	{
	case DebugRenderMode::ALWAYS:
		renderer->BindMaterial(m_materials[(int)DebugRenderMode::ALWAYS]);
		break;
	case DebugRenderMode::USEDEPTH:
		renderer->BindMaterial(m_materials[(int)DebugRenderMode::USEDEPTH]);
		break;
	case DebugRenderMode::XRAY:
		renderer->BindMaterial(m_materials[(int)DebugRenderMode::XRAY]);
		break;
	}
}

BitmapFont* DebugRenderSystem::GetBitmapFont() const
{
	return m_config.m_renderer->CreateOrGetBitmapFont(m_config.m_fontName.c_str());
}

static DebugRenderSystem* debugRenderSystem = nullptr;

void DebugRenderSystemStartup(const DebugRenderConfig& config)
{
	debugRenderSystem = new DebugRenderSystem(config);
	debugRenderSystem->Startup();
}

void DebugRenderSystemShutdown()
{
	debugRenderSystem->Shutdown();
	delete debugRenderSystem;
	debugRenderSystem = nullptr;
}

void DebugRenderSetVisible()
{
	debugRenderSystem->m_isVisible = true;
}

void DebugRenderSetHidden()
{
	debugRenderSystem->m_isVisible = false;
}

void DebugRenderClear()
{
	debugRenderSystem->Clear();
}

void DebugRenderSetParentClock(Clock& parent)
{
	debugRenderSystem->m_clock.SetParent(parent);
}

void DebugRenderSetTimeDilation(double timeDilation)
{
	debugRenderSystem->SetTimeDilation(timeDilation);
}

Clock const& DebugRenderGetClock()
{
	return debugRenderSystem->GetClock();
}

void DebugRenderBeginFrame()
{
}

void DebugRenderWorld(const Camera& camera)
{
	debugRenderSystem->RenderWorldShapes(camera);
}

void DebugRenderScreen(const Camera& camera)
{
	debugRenderSystem->RenderScreenShapes(camera);
}

void DebugRenderEndFrame()
{
	debugRenderSystem->CheckAllShapes();
}

void DebugAddWorldPoint(const Vec3& pos, float radius, float duration, const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode, int stacks, int slices)
{
	Mat44 modelMatrix;
	modelMatrix.AppendTranslation3D(pos);

	DebugShape* newShape = new DebugShape(modelMatrix, mode, duration, debugRenderSystem->GetClock(), startColor, endColor);
	AddVertsForSphere(newShape->m_verts, radius, stacks, slices);
	debugRenderSystem->AddShape(newShape);
}

void DebugAddWorldLine(const Vec3& start, const Vec3& end, float radius, float duration, const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode)
{
	Mat44 modelMatrix;
	DebugShape* newShape = new DebugShape(modelMatrix, mode, duration, debugRenderSystem->GetClock(), startColor, endColor);
	AddVertsForCylinder(newShape->m_verts, start, end, radius);
	debugRenderSystem->AddShape(newShape);
}

void DebugAddWorldWireCylinder(const Vec3& base, const Vec3& top, float radius, float duration, const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode)
{
	Mat44 modelMatrix;

	DebugShape* newShape = new DebugShape(modelMatrix, mode, duration, debugRenderSystem->GetClock(), startColor, endColor);
	AddVertsForCylinder(newShape->m_verts, base, top, radius);
	debugRenderSystem->AddWireShape(newShape);
}

void DebugAddWorldWireSphere(const Vec3& center, float radius, float duration, const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode, int stacks, int slices)
{
	Mat44 modelMatrix;
	modelMatrix.AppendTranslation3D(center);

	DebugShape* newShape = new DebugShape(modelMatrix, mode, duration, debugRenderSystem->GetClock(), startColor, endColor);
	AddVertsForWireSphere(newShape->m_verts, radius, stacks, slices);
	debugRenderSystem->AddWireShape(newShape);
}

void DebugAddWorldArrow(const Vec3& start, const Vec3& end, float radius, float duration, const Rgba8& baseColor, const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode)
{
	Mat44 modelMatrix;

	DebugShape* newShape = new DebugShape(modelMatrix, mode, duration, debugRenderSystem->GetClock(), startColor, endColor);
	AddVertsForArrow3D(newShape->m_verts, start, end, radius, 16, baseColor);
	debugRenderSystem->AddShape(newShape);
}

void DebugAddWorldBox(const AABB3& bounds, float duration, const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode)
{
	Mat44 modelMatrix;

	DebugShape* newShape = new DebugShape(modelMatrix, mode, duration, debugRenderSystem->GetClock(), startColor, endColor);
	AddVertsForAABB3D(newShape->m_verts, bounds);
	debugRenderSystem->AddShape(newShape);
}

void DebugAddWorldBasis(const Mat44& basis, float duration, const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode)
{
	Mat44 modelMatrix;
	modelMatrix.AppendTranslation3D(basis.GetTranslation3D());

	DebugShape* newShape = new DebugShape(modelMatrix, mode, duration, debugRenderSystem->GetClock(), startColor, endColor);
	float basisRadius = 0.075f;

	AddVertsForArrow3D(newShape->m_verts, Vec3::ZERO, basis.GetIBasis3D(), basisRadius, 16, Rgba8::RED);
	AddVertsForArrow3D(newShape->m_verts, Vec3::ZERO, basis.GetJBasis3D(), basisRadius, 16, Rgba8::GREEN);
	AddVertsForArrow3D(newShape->m_verts, Vec3::ZERO, basis.GetKBasis3D(), basisRadius, 16, Rgba8::BLUE);
	debugRenderSystem->AddShape(newShape);

}

void DebugAddWorldText(const std::string& text, const Mat44& transform, float textHeight, const Vec2& alignment, float duration, const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode)
{
	BitmapFont* font = debugRenderSystem->GetBitmapFont();
	AABB2 textBox;
	textBox.SetDimensions(Vec2(font->GetTextWidth(textHeight, text), textHeight));

	DebugShape* newShape = new DebugShape(transform, mode, duration, debugRenderSystem->GetClock(), startColor, endColor, &font->GetTexture());
	newShape->m_isWorldText = true;
	font->AddVertsForTextInBox2D(newShape->m_verts, textBox, textHeight, text, Rgba8::WHITE, 1.0f, alignment);
	debugRenderSystem->AddShape(newShape);
}

void DebugAddWorldBillboardText(const std::string& text, const Vec3& origin, float textHeight, const Vec2& alignment, float duration, const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode)
{
	BitmapFont* font = debugRenderSystem->GetBitmapFont();
	AABB2 textBox;
	textBox.SetDimensions(Vec2(font->GetTextWidth(textHeight, text), textHeight));

	Mat44 modelMatrix;
	modelMatrix.SetTranslation3D(origin);

	DebugShape* newShape = new DebugShape(modelMatrix, mode, duration, debugRenderSystem->GetClock(), startColor, endColor, &font->GetTexture());
	font->AddVertsForTextInBox2D(newShape->m_verts, textBox, textHeight, text, Rgba8::WHITE, 1.0f, alignment);
	debugRenderSystem->AddBillboard(newShape);

}

void DebugAddScreenText(const std::string& text, const Vec2& position, float duration, const Vec2& alignment, float size, const Rgba8& startColor, const Rgba8& endColor)
{
	BitmapFont* font = debugRenderSystem->GetBitmapFont();
	AABB2 textBox;
	textBox.m_mins = position;
	textBox.m_maxs = Vec2(font->GetTextWidth(size, text), size) + position;

	DebugShape* newShape = new DebugShape(text, ScrenTextType::FreeText, duration, debugRenderSystem->GetClock(), startColor, endColor, &font->GetTexture());
	newShape->m_isWorldShape = false;
	font->AddVertsForTextInBox2D(newShape->m_verts, textBox, size, text, Rgba8::WHITE, 1.0f, alignment);
	debugRenderSystem->AddShape(newShape);
}

void DebugAddMessage(const std::string& text, float duration, const Rgba8& startColor, const Rgba8& endColor)
{
	BitmapFont* font = debugRenderSystem->GetBitmapFont();
	AABB2 textBox;

	DebugShape* newShape = new DebugShape(text, ScrenTextType::ScreenMessage, duration, debugRenderSystem->GetClock(), startColor, endColor, &font->GetTexture());
	newShape->m_isWorldShape = false;
	debugRenderSystem->AddShape(newShape);
}


