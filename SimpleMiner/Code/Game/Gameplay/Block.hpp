#pragma once

class Block {
public:
	unsigned char m_typeIndex = 0;
	unsigned char m_lightInfluences = 0;
	unsigned char m_bitFlags = 0;

public:
	unsigned char GetIndoorLightInfluence() const;
	unsigned char GetOutdoorLightInfluence() const;
	void SetIndoorLightInfluence(int indoorLightInfluence);
	void SetOutdoorLightInfluence(int outdoorLightInfluence);
	bool IsSky() const;
	void SetIsSky(bool isSky);

	bool IsLightDirty() const;
	void SetIsLightDirty(bool isLightDirty);
};