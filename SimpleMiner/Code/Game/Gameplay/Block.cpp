#include "Game/Gameplay/Block.hpp"

constexpr unsigned char SKY_MASK = 0b00000001;
constexpr unsigned char LIGHT_DIRTY_MASK = 0b00000010;
constexpr unsigned char INDOOR_LIGHT_MASK = 0b00001111;
constexpr unsigned char OUTDOOR_LIGHT_MASK = 0b11110000;
constexpr int OUTDOOR_LIGHT_BITSHIFT = 4;
constexpr int INDOOR_LIGHT_BITSHIFT = 0;

unsigned char Block::GetIndoorLightInfluence() const
{
	return (m_lightInfluences & INDOOR_LIGHT_MASK) >> INDOOR_LIGHT_BITSHIFT;
}

unsigned char Block::GetOutdoorLightInfluence() const
{
	return (m_lightInfluences & OUTDOOR_LIGHT_MASK) >> OUTDOOR_LIGHT_BITSHIFT;
}

void Block::SetIndoorLightInfluence(int indoorLightInfluence)
{
	m_lightInfluences &= ~INDOOR_LIGHT_MASK;
	m_lightInfluences |= indoorLightInfluence;
}

void Block::SetOutdoorLightInfluence(int outdoorLightInfluence)
{
	m_lightInfluences &= ~OUTDOOR_LIGHT_MASK;
	m_lightInfluences |= (outdoorLightInfluence << OUTDOOR_LIGHT_BITSHIFT);
}

bool Block::IsSky() const
{
	return (m_bitFlags & SKY_MASK);
}

void Block::SetIsSky(bool isSky)
{
	m_bitFlags &= 0;
	m_bitFlags |= (isSky) ? 1 : 0;
}

bool Block::IsLightDirty() const
{
	return (m_bitFlags & LIGHT_DIRTY_MASK) >> 1;
}

void Block::SetIsLightDirty(bool isLightDirty)
{
	unsigned char newBit = (isLightDirty) ? 1 : 0;
	m_bitFlags &= ~LIGHT_DIRTY_MASK;
	m_bitFlags |= (newBit << 1);
}
