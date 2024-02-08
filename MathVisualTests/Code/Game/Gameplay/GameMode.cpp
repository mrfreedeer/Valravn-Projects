#include "Game/Gameplay/GameMode.hpp"
#include "Game/Gameplay/Game.hpp"

GameMode::GameMode(Game* pointerToGame, Vec2 const& cameraSize, Vec2 const& UICameraSize):
	m_game(pointerToGame),
	m_cameraSize(cameraSize),
	m_UICameraSize(UICameraSize)
{
	m_UICameraCenter = m_UICameraSize * 0.5f;
	m_clock.SetParent(pointerToGame->GetClock());
}

GameMode::~GameMode()
{
}

void GameMode::Update(float deltaSeconds)
{
	m_timeAlive += deltaSeconds;
	UpdateCameras(deltaSeconds);
}

void GameMode::RenderUI() const
{
	g_theRenderer->BeginCamera(m_UICamera);
	//g_theRenderer->ClearScreen(Rgba8::BLACK);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetRasterizerState(CullMode::NONE, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
	g_theRenderer->SetSamplerMode(SamplerMode::POINTCLAMP);

	std::string gameModeAsText = GetCurrentGameStateAsText();
	std::string gameModeHelperText = m_helperText;

	std::vector<Vertex_PCU> titleVerts;
	Vec2 titlePos(m_UICameraSize.x * 0.01f, m_UICameraSize.y * 0.98f);
	Vec2 helpTextPos(m_UICameraSize.x * 0.01f, titlePos.y * 0.97f);

	g_squirrelFont->AddVertsForText2D(titleVerts, titlePos, m_textCellHeight, gameModeAsText, Rgba8::YELLOW, 0.6f);
	g_squirrelFont->AddVertsForText2D(titleVerts, helpTextPos, m_textCellHeight, gameModeHelperText, Rgba8::GREEN, 0.6f);
	g_theRenderer->BindTexture(&g_squirrelFont->GetTexture());
	g_theRenderer->DrawVertexArray(titleVerts);

	if (m_useTextAnimation) {

		RenderTextAnimation();
	}
	AABB2 devConsoleBounds(m_UICamera.GetOrthoBottomLeft(), m_UICamera.GetOrthoTopRight());

	g_theConsole->Render(devConsoleBounds);

	g_theRenderer->EndCamera(m_UICamera);

}

void GameMode::RenderTextAnimation() const
{
	if (m_textAnimTriangles.size() > 0) {
		g_theRenderer->BindTexture(&g_squirrelFont->GetTexture());
		g_theRenderer->DrawVertexArray(m_textAnimTriangles);
	}
}

Rgba8 const GameMode::GetRandomColor() const
{
	unsigned char randomR = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	unsigned char randomG = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	unsigned char randomB = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	unsigned char randomA = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	return Rgba8(randomR, randomG, randomB, randomA);
}

void GameMode::UpdateTextAnimation(float deltaTime, std::string text, Vec2 const& textLocation, float textCellHeight)
{
		m_timeElapsedTextAnimation += deltaTime;
		m_textAnimTriangles.clear();

		Rgba8 usedTextColor = m_textAnimationColor;
		if (m_transitionTextColor) {

			float timeAsFraction = GetFractionWithin(m_timeElapsedTextAnimation, 0, m_textAnimationDuration);

			usedTextColor.r = static_cast<unsigned char>(Interpolate(m_startTextColor.r, m_endTextColor.r, timeAsFraction));
			usedTextColor.g = static_cast<unsigned char>(Interpolate(m_startTextColor.g, m_endTextColor.g, timeAsFraction));
			usedTextColor.b = static_cast<unsigned char>(Interpolate(m_startTextColor.b, m_endTextColor.b, timeAsFraction));
			usedTextColor.a = static_cast<unsigned char>(Interpolate(m_startTextColor.a, m_endTextColor.a, timeAsFraction));

		}

		g_squirrelFont->AddVertsForText2D(m_textAnimTriangles, Vec2(), textCellHeight, text, usedTextColor);

		float fullTextWidth = g_squirrelFont->GetTextWidth(textCellHeight, text);
		float textWidth = GetTextWidthForAnim(fullTextWidth);

		Vec2 iBasis = GetIBasisForTextAnim();
		Vec2 jBasis = GetJBasisForTextAnim();

		TransformText(iBasis, jBasis, Vec2(textLocation.x - textWidth * 0.5f, textLocation.y));

		if (m_timeElapsedTextAnimation >= m_textAnimationDuration) {
			m_useTextAnimation = false;
			m_timeElapsedTextAnimation = 0.0f;
		}

}

void GameMode::TransformText(Vec2 const& iBasis, Vec2 const& jBasis, Vec2 const& translation)
{
	for (int vertIndex = 0; vertIndex < int(m_textAnimTriangles.size()); vertIndex++) {
		Vec3& vertPosition = m_textAnimTriangles[vertIndex].m_position;
		TransformPositionXY3D(vertPosition, iBasis, jBasis, translation);
	}
}

float GameMode::GetTextWidthForAnim(float fullTextWidth) {

	float timeAsFraction = GetFractionWithin(m_timeElapsedTextAnimation, 0.0f, m_textAnimationDuration);
	float textWidth = 0.0f;
	if (timeAsFraction < m_textMovementPhaseTimePercentage) {
		if (timeAsFraction < m_textMovementPhaseTimePercentage * 0.5f) {
			textWidth = RangeMapClamped(timeAsFraction, 0.0f, m_textMovementPhaseTimePercentage * 0.5f, 0.0f, fullTextWidth);
		}
		else {
			return fullTextWidth;
		}
	}
	else if (timeAsFraction > m_textMovementPhaseTimePercentage && timeAsFraction < 1 - m_textMovementPhaseTimePercentage) {
		return fullTextWidth;
	}
	else {
		if (timeAsFraction < 1.0f - m_textMovementPhaseTimePercentage * 0.5f) {
			return fullTextWidth;
		}
		else {
			textWidth = RangeMapClamped(timeAsFraction, 1.0f - m_textMovementPhaseTimePercentage * 0.5f, 1.0f, fullTextWidth, 0.0f);
		}
	}
	return textWidth;

}

Vec2 const GameMode::GetIBasisForTextAnim()
{
	float timeAsFraction = GetFractionWithin(m_timeElapsedTextAnimation, 0.0f, m_textAnimationDuration);
	float iScale = 0.0f;
	if (timeAsFraction < m_textMovementPhaseTimePercentage) {
		if (timeAsFraction < m_textMovementPhaseTimePercentage * 0.5f) {
			iScale = RangeMapClamped(timeAsFraction, 0.0f, m_textMovementPhaseTimePercentage * 0.5f, 0.0f, 1.0f);
		}
		else {
			iScale = 1.0f;
		}
	}
	else if (timeAsFraction > m_textMovementPhaseTimePercentage && timeAsFraction < 1 - m_textMovementPhaseTimePercentage) {
		iScale = 1.0f;
	}
	else {
		if (timeAsFraction < 1.0f - m_textMovementPhaseTimePercentage * 0.5f) {
			iScale = 1.0f;
		}
		else {
			iScale = RangeMapClamped(timeAsFraction, 1.0f - m_textMovementPhaseTimePercentage * 0.5f, 1.0f, 1.0f, 0.0f);
		}
	}

	return Vec2(iScale, 0.0f);
}

Vec2 const GameMode::GetJBasisForTextAnim()
{
	float timeAsFraction = GetFractionWithin(m_timeElapsedTextAnimation, 0.0f, m_textAnimationDuration);
	float jScale = 0.0f;
	if (timeAsFraction < m_textMovementPhaseTimePercentage) {
		if (timeAsFraction < m_textMovementPhaseTimePercentage * 0.5f) {
			jScale = 0.05f;
		}
		else {
			jScale = RangeMapClamped(timeAsFraction, m_textMovementPhaseTimePercentage * 0.5f, m_textMovementPhaseTimePercentage, 0.05f, 1.0f);

		}
	}
	else if (timeAsFraction > m_textMovementPhaseTimePercentage && timeAsFraction < 1 - m_textMovementPhaseTimePercentage) {
		jScale = 1.0f;
	}
	else {
		if (timeAsFraction < 1.0f - m_textMovementPhaseTimePercentage * 0.5f) {
			jScale = RangeMapClamped(timeAsFraction, 1.0f - m_textMovementPhaseTimePercentage, 1.0f - m_textMovementPhaseTimePercentage * 0.5f, 1.0f, 0.05f);
		}
		else {
			jScale = 0.05f;
		}
	}

	return Vec2(0.0f, jScale);
}

void GameMode::UpdateCameras(float deltaSeconds)
{
	UpdateWorldCamera(deltaSeconds);
	UpdateUICamera(deltaSeconds);
}

void GameMode::UpdateWorldCamera(float deltaSeconds)
{
	m_worldCamera.SetOrthoView(Vec2(), m_cameraSize);
	if (m_screenShake) {
		float easingInScreenShake = m_timeShakingScreen / m_screenShakeDuration;
		easingInScreenShake *= (m_screenShakeTranslation * 0.95f);

		float randAmountX = rng.GetRandomFloatInRange(-1.0f, 1.0f) * (m_screenShakeTranslation - easingInScreenShake);
		float randAmountY = rng.GetRandomFloatInRange(-1.0f, 1.0f) * (m_screenShakeTranslation - easingInScreenShake);

		m_worldCamera.TranslateCamera(randAmountX, randAmountY);

		m_timeShakingScreen += deltaSeconds;

		if (m_timeShakingScreen > m_screenShakeDuration) {
			m_screenShake = false;
			m_timeShakingScreen = 0.0f;;
		}
	}
}

void GameMode::UpdateUICamera(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	m_UICamera.SetOrthoView(Vec2(), m_UICameraSize);
}

std::string GameMode::GetCurrentGameStateAsText() const
{
	std::string fullTitleText("Mode (F6/F7 for prev/next): ");
	fullTitleText += m_gameModeName;
	return fullTitleText;
}

Vec2 const GameMode::GetClampedRandomPositionInWorld() const
{
	float randX = rng.GetRandomFloatInRange(m_worldSize.x * 0.1f, m_worldSize.x * 0.9f);
	float randY = rng.GetRandomFloatInRange(m_worldSize.y * 0.1f, m_worldSize.y * 0.9f);

	return Vec2(randX, randY);
}

Vec2 const GameMode::GetClampedRandomPositionInWorld(float shapeRadius) const
{
	float randX = rng.GetRandomFloatInRange(shapeRadius, m_worldSize.x - shapeRadius);
	float randY = rng.GetRandomFloatInRange(shapeRadius, m_worldSize.y - shapeRadius);

	return Vec2(randX, randY);

}

Vec2 const GameMode::GetRandomPositionInWorld() const
{
	float randX = rng.GetRandomFloatInRange(0.0f, m_worldSize.x);
	float randY = rng.GetRandomFloatInRange(0.0f, m_worldSize.y);

	return Vec2(randX, randY);
}
