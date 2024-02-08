#include "Game/Gameplay/BlockDefinition.hpp"
#include "Game/Framework/GameCommon.hpp"

std::vector<BlockDefinition*> BlockDefinition::s_blockDefinitions;

BlockDefinition::BlockDefinition(std::string name, unsigned char id, AABB2 const& topUVs, AABB2 const& sideUVs, AABB2 const& bottomUVs, bool isVisible, bool isSolid, bool isOpaque, int outdoorLightInfluence, int indoorLightInfluence) :
	m_name(name),
	m_id(id),
	m_topUVs(topUVs),
	m_sideUVs(sideUVs),
	m_bottomUVs(bottomUVs),
	m_isVisible(isVisible),
	m_isSolid(isSolid),
	m_isOpaque(isOpaque),
	m_outdoorLightInfluence(outdoorLightInfluence),
	m_indoorLightInfluence(indoorLightInfluence)
{
}

void BlockDefinition::InitializeDefinitions()
{
	CreateBlockDef("air", IntVec2(0, 0), IntVec2(0, 0), IntVec2(0, 0), false, false, false, 0, 0);
	CreateBlockDef("grass", IntVec2(32, 33), IntVec2(33, 33), IntVec2(32, 34), true, true, true, 0, 0);
	CreateBlockDef("dirt", IntVec2(32, 34), IntVec2(32, 34), IntVec2(32, 34), true, true, true, 0, 0);
	CreateBlockDef("stone", IntVec2(33, 32), IntVec2(33, 32), IntVec2(33, 32), true, true, true, 0, 0);
	CreateBlockDef("bricks", IntVec2(34, 32), IntVec2(34, 32), IntVec2(34, 32), true, true, true, 0, 0);
	CreateBlockDef("glowstone", IntVec2(46, 34), IntVec2(46, 34), IntVec2(46, 34), true, true, true, 0, 15);
	CreateBlockDef("water", IntVec2(32, 44), IntVec2(32, 44), IntVec2(32, 44), true, false, false, 0, 0);
	CreateBlockDef("coal", IntVec2(63, 34), IntVec2(63, 34), IntVec2(63, 34), true, true, true, 0, 0);
	CreateBlockDef("iron", IntVec2(63, 35), IntVec2(63, 35), IntVec2(63, 35), true, true, true, 0, 0);
	CreateBlockDef("gold", IntVec2(63, 36), IntVec2(63, 36), IntVec2(63, 36), true, true, true, 0, 0);
	CreateBlockDef("diamond", IntVec2(63, 37), IntVec2(63, 37), IntVec2(63, 37), true, true, true, 0, 0);
	CreateBlockDef("sand", IntVec2(34, 34), IntVec2(34, 34), IntVec2(34, 34), true, true, true, 0, 0);
	CreateBlockDef("ice", IntVec2(36, 35), IntVec2(36, 35), IntVec2(36, 35), true, true, true, 0, 0);
	CreateBlockDef("oak_log", IntVec2(38, 33), IntVec2(36, 33), IntVec2(38, 33), true, true, true, 0, 0);
	CreateBlockDef("oak_leaves", IntVec2(32, 35), IntVec2(32, 35), IntVec2(32, 35), true, true, true, 0, 0);
	CreateBlockDef("spruce_log", IntVec2(38, 33), IntVec2(37, 33), IntVec2(38, 33), true, true, true, 0, 0);
	CreateBlockDef("spruce_leaves", IntVec2(62, 41), IntVec2(62, 41), IntVec2(62, 41), true, true, true, 0, 0);
	CreateBlockDef("cactus", IntVec2(39, 36), IntVec2(37, 36), IntVec2(38, 36), true, true, true, 0, 0);
}

void BlockDefinition::DestroyDefinitions()
{
	for (int blockDefIndex = 0; blockDefIndex < s_blockDefinitions.size(); blockDefIndex++) {
		BlockDefinition*& blockDef = s_blockDefinitions[blockDefIndex];
		if (blockDef) {
			delete blockDef;
			blockDef = nullptr;
		}
	}

	s_blockDefinitions.resize(0);
}

BlockDefinition const* BlockDefinition::GetDefByName(std::string const& name)
{
	for (int blockDefIndex = 0; blockDefIndex < s_blockDefinitions.size(); blockDefIndex++) {
		BlockDefinition* blockDef = s_blockDefinitions[blockDefIndex];
		if (blockDef) {
			if (blockDef->m_name == name) {
				return blockDef;
			}
		}
	}

	return nullptr;
}

BlockDefinition const* BlockDefinition::GetDefById(unsigned char id)
{
	if(id >= s_blockDefinitions.size() || id < 0) return nullptr;
	return s_blockDefinitions[id];
}

int BlockDefinition::GetSpriteIndex(IntVec2 const& spriteCoords)
{
	return spriteCoords.x + (spriteCoords.y * 64);
}

void BlockDefinition::CreateBlockDef(std::string name, IntVec2 const& topUVs, IntVec2 const& sideUVs, IntVec2 const& bottomUVs, bool isVisible, bool isSolid, bool isOpaque, int outdoorLightInfluence, int indoorLightInfluence)
{
	SpriteSheet simpleMinerSprites(*g_textures[(int)GAME_TEXTURE::SimpleMinerSprites], IntVec2(64, 64));

	bool useWhiteTexture = g_gameConfigBlackboard.GetValue("DEBUG_FORCE_WHITE_TEXTURE", false);


	AABB2 topSpriteUvs = simpleMinerSprites.GetSpriteDef(GetSpriteIndex(topUVs)).GetUVs();
	AABB2 bottomSpriteUvs = simpleMinerSprites.GetSpriteDef(GetSpriteIndex(bottomUVs)).GetUVs();
	AABB2 sideSpriteUvs = simpleMinerSprites.GetSpriteDef(GetSpriteIndex(sideUVs)).GetUVs();

	if (useWhiteTexture) {
		AABB2 whiteTexture = simpleMinerSprites.GetSpriteDef(GetSpriteIndex(IntVec2(2, 4))).GetUVs();
		topSpriteUvs = whiteTexture;
		bottomSpriteUvs = whiteTexture;
		sideSpriteUvs = whiteTexture;
	}


	BlockDefinition* newBlockDef = new BlockDefinition(name, static_cast<unsigned char>(s_blockDefinitions.size()), topSpriteUvs, sideSpriteUvs, bottomSpriteUvs, isVisible, isSolid, isOpaque, outdoorLightInfluence, indoorLightInfluence);
	s_blockDefinitions.push_back(newBlockDef);
}
