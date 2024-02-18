#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Engine/Renderer/DebugRendererSystem.hpp"
#include "Engine/Core/EngineCommon.hpp"

SpriteAnimDefinition::SpriteAnimDefinition(const SpriteSheet& sheet, int startSpriteIndex, int endSpriteIndex, float durationSeconds, SpriteAnimPlaybackType playbackType, Vec3 const& direction, Material* shader) :
	m_spriteSheet(sheet),
	m_startSpriteIndex(startSpriteIndex),
	m_endSpriteIndex(endSpriteIndex),
	m_durationSeconds(durationSeconds),
	m_playbackType(playbackType),
	m_direction(direction),
	m_shader (shader)
{
}

float SpriteAnimGroupDefinition::GetTotalLengthTime() const
{
	return m_animationsDefs[0].GetDuration();
}

const SpriteDefinition& SpriteAnimDefinition::GetSpriteDefAtTime(float seconds) const
{
	switch (m_playbackType)
	{
	case SpriteAnimPlaybackType::ONCE:
		return GetSpritDefAtTimePlayOnce(seconds);
		break;
	case SpriteAnimPlaybackType::LOOP:
		return GetSpritDefAtTimeLoop(seconds);
		break;
	case SpriteAnimPlaybackType::PINGPONG:
		return GetSpritDefAtTimePingPong(seconds);
		break;
	default:
		ERROR_AND_DIE(Stringf("INVALID PLAYBACK TYPE: %d", (int)m_playbackType));
		break;
	}
}

float SpriteAnimDefinition::GetDuration() const
{
	return m_durationSeconds;
}

const SpriteDefinition& SpriteAnimDefinition::GetSpritDefAtTimePlayOnce(float seconds) const
{
	if (seconds >= m_durationSeconds) return m_spriteSheet.GetSpriteDef(m_endSpriteIndex);

	float timePerSprite = (m_durationSeconds) / static_cast<float>(m_endSpriteIndex - m_startSpriteIndex + 1);

	float estimatedSpriteIndex = seconds / timePerSprite;

	int spritesAlreadyPlayed = RoundDownToInt(estimatedSpriteIndex);

	return m_spriteSheet.GetSpriteDef(m_startSpriteIndex + spritesAlreadyPlayed);
}

const SpriteDefinition& SpriteAnimDefinition::GetSpritDefAtTimeLoop(float seconds) const
{
	float timePerSprite = (m_durationSeconds) / static_cast<float>(m_endSpriteIndex - m_startSpriteIndex + 1);

	float absoluteTimeInAnimation = seconds;

	while (absoluteTimeInAnimation >= m_durationSeconds) {
		absoluteTimeInAnimation -= m_durationSeconds;
	}

	float estimatedSpriteIndex = absoluteTimeInAnimation / timePerSprite;

	int spritesAlreadyPlayed = RoundDownToInt(estimatedSpriteIndex);

	return m_spriteSheet.GetSpriteDef(m_startSpriteIndex + spritesAlreadyPlayed);
}

const SpriteDefinition& SpriteAnimDefinition::GetSpritDefAtTimePingPong(float seconds) const
{
	int numSprites = m_endSpriteIndex - m_startSpriteIndex + 1;
	float timePerSprite = (m_durationSeconds) / static_cast<float>(numSprites);

	float absoluteTimeInAnimation = seconds;
	float pingpongAnimationTotalTime = static_cast<float>(((m_endSpriteIndex - m_startSpriteIndex) * 2)) * timePerSprite;

	while (absoluteTimeInAnimation > pingpongAnimationTotalTime) {
		absoluteTimeInAnimation -= pingpongAnimationTotalTime;
	}

	float estimatedSpriteIndex = absoluteTimeInAnimation / timePerSprite;

	int spritesAlreadyPlayed = RoundDownToInt(estimatedSpriteIndex);

	int indexForSpriteToPlay = m_startSpriteIndex + spritesAlreadyPlayed;
	if (indexForSpriteToPlay > m_endSpriteIndex) {
		indexForSpriteToPlay = m_endSpriteIndex - (indexForSpriteToPlay % m_endSpriteIndex);
	}

	return m_spriteSheet.GetSpriteDef(indexForSpriteToPlay);
}

SpriteAnimGroupDefinition::SpriteAnimGroupDefinition(Texture const* pointerToTexture) :
	m_texture(pointerToTexture)
{
}

SpriteAnimGroupDefinition::~SpriteAnimGroupDefinition()
{
	delete m_spriteSheet;
	m_spriteSheet = nullptr;
}

void SpriteAnimGroupDefinition::LoadAnimationsFromXML(XMLElement const& element)
{
	m_name = ParseXmlAttribute(element, "name", "Unnamed");
	m_material = ParseXmlAttribute(element, "material", "Default2DMaterial");
	IntVec2 spriteSheetDims = ParseXmlAttribute(element, "cellCount", IntVec2::ZERO);
	std::string playbackType = ParseXmlAttribute(element, "playbackMode", "Loop");
	m_playbackType = SpriteAnimPlaybackType::LOOP;
	m_playbackType = (playbackType == "Once") ? SpriteAnimPlaybackType::ONCE : m_playbackType;
	m_playbackType = (playbackType == "PingPong") ? SpriteAnimPlaybackType::PINGPONG : m_playbackType;
	m_secondsPerFrame = ParseXmlAttribute(element, "secondsPerFrame", 1.0f);
	m_scaleBySpeed = ParseXmlAttribute(element, "scaleBySpeed", false);

	m_spriteSheet = new SpriteSheet(*m_texture, spriteSheetDims);


	XMLElement const* currentSpriteAnim = element.FirstChildElement("Direction");

	while (currentSpriteAnim) {
		XMLElement const* animeSpriteInfo = currentSpriteAnim->FirstChildElement("Animation");

		Vec3 direction = ParseXmlAttribute(*currentSpriteAnim, "vector", Vec3::ZERO);
		direction.Normalize();
		int startFrame = ParseXmlAttribute(*animeSpriteInfo, "startFrame", 0);
		int endFrame = ParseXmlAttribute(*animeSpriteInfo, "endFrame", 0);
		int durationFrames = abs(endFrame - startFrame) + 1;
		m_animationsDefs.emplace_back(*m_spriteSheet, startFrame, endFrame, durationFrames * m_secondsPerFrame, m_playbackType, direction);

		currentSpriteAnim = currentSpriteAnim->NextSiblingElement();
	}
}

SpriteDefinition const* SpriteAnimGroupDefinition::GetSpriteAtDefTime(float seconds, Vec3 const& direction) const
{
	float biggestDotProduct = -1.0f;
	SpriteAnimDefinition const* bestSuitedSpriteAnim = nullptr;

	for (int spriteAnimGroupIndex = 0; spriteAnimGroupIndex < m_animationsDefs.size(); spriteAnimGroupIndex++) {
		SpriteAnimDefinition const& spriteAnimDef = m_animationsDefs[spriteAnimGroupIndex];
		float dotProduct = spriteAnimDef.GetDotProduct(direction);
		dotProduct = Clamp(dotProduct, -1.0f, 1.0f);
		if (dotProduct >= biggestDotProduct) {
			biggestDotProduct = dotProduct;
			bestSuitedSpriteAnim = &spriteAnimDef;
		}
	}

	if (bestSuitedSpriteAnim) {
		return &bestSuitedSpriteAnim->GetSpriteDefAtTime(seconds);
	}

	return nullptr;
}
