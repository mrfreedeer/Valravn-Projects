#pragma once
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "ThirdParty/TinyXML2/tinyxml2.h"


class Material;
typedef tinyxml2::XMLElement XMLElement;
enum class SpriteAnimPlaybackType
{
	ONCE,
	LOOP,
	PINGPONG // Back and forth and back....
};

class SpriteAnimDefinition {
public:
	SpriteAnimDefinition(const SpriteSheet& sheet, int startSpriteIndex, int endSpriteIndex,
		float durationSeconds, SpriteAnimPlaybackType playbackType = SpriteAnimPlaybackType::LOOP, Vec3 const& direction = Vec3::ZERO, Material* shader = nullptr);

	SpriteDefinition const& GetSpriteDefAtTime(float seconds) const;
	float GetDuration() const;
	float GetDotProduct(Vec3 const& direction) const { return DotProduct3D(m_direction, direction); }
	Vec3 GetDirection() const { return m_direction; }
	Material* GetShader() const { return m_shader; }

private:
	SpriteDefinition const& GetSpritDefAtTimePlayOnce(float seconds) const;
	SpriteDefinition const& GetSpritDefAtTimeLoop(float seconds) const;
	SpriteDefinition const& GetSpritDefAtTimePingPong(float seconds) const;


private:
	const SpriteSheet& m_spriteSheet;
	int m_startSpriteIndex = -1;
	int	m_endSpriteIndex = -1;
	float m_durationSeconds = 1.f;
	SpriteAnimPlaybackType	m_playbackType = SpriteAnimPlaybackType::LOOP;
	Vec3 m_direction = Vec3::ZERO;
	Material* m_shader = nullptr;
};

class SpriteAnimGroupDefinition {
public:
	explicit SpriteAnimGroupDefinition(Texture const* pointerToTexture);
	~SpriteAnimGroupDefinition();
	void LoadAnimationsFromXML(XMLElement const& element);
	SpriteDefinition const* GetSpriteAtDefTime(float seconds, Vec3 const& direction) const;
	float GetTotalLengthTime() const;
	std::string GetName() const { return m_name; }
	bool IsScaledBySpeed() const { return m_scaleBySpeed; }
	std::string GetMatName() const { return m_material; }

private:
	SpriteSheet const* m_spriteSheet = nullptr;
	SpriteAnimPlaybackType m_playbackType = SpriteAnimPlaybackType::LOOP;
	float m_secondsPerFrame = 1.0f;
	std::string m_name = "";
	std::string m_material = "";
	Texture const* m_texture = nullptr;
	std::vector<SpriteAnimDefinition> m_animationsDefs;
	bool m_scaleBySpeed = false;
};