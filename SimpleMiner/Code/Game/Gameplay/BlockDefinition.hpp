#pragma once
#include "Engine/Math/AABB2.hpp"
#include <vector>
#include <string>

class BlockDefinition {
public:
	BlockDefinition(std::string name, unsigned char id, AABB2 const& topUVs, AABB2 const& sideUVs, AABB2 const& bottomUVs, bool isVisible = false, bool isSolid = false, bool isOpaque = false, int outdoorLightInfluence = 0, int indoorLightInfluence = 0);
	static void InitializeDefinitions();
	static void DestroyDefinitions();
	static BlockDefinition const* GetDefByName(std::string const& name);
	static BlockDefinition const* GetDefById(unsigned char id);

public:
	std::string m_name = "";
	bool m_isVisible = false;
	bool m_isSolid = false;
	bool m_isOpaque = false;
	AABB2 m_topUVs = AABB2::ZERO_TO_ONE;
	AABB2 m_sideUVs = AABB2::ZERO_TO_ONE;
	AABB2 m_bottomUVs = AABB2::ZERO_TO_ONE;
	unsigned char m_id = 0;
	int m_outdoorLightInfluence = 0;
	int m_indoorLightInfluence = 0;


	static std::vector<BlockDefinition*> s_blockDefinitions;

private:
	static int GetSpriteIndex(IntVec2 const& spriteCoords);
	static void CreateBlockDef(std::string name, IntVec2 const& topUVs, IntVec2 const& sideUVs, IntVec2 const& bottomUVs, bool isVisible, bool isSolid, bool isOpaque, int outdoorLightInfluence, int indoorLightInfluence);
};