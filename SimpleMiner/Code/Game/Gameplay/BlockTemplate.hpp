#pragma once
#include <vector>
#include <string>
#include "Engine/Math/IntVec3.hpp"


struct BlockTemplateEntry {
public:
	BlockTemplateEntry(uint8_t blockType, IntVec3 const& offset): m_blockType(blockType), m_offset(offset){};

	uint8_t m_blockType;
	IntVec3 m_offset;
};

class BlockTemplate {
	public:
	BlockTemplate(char const* name, std::vector<BlockTemplateEntry>& blockTemplateEntries);

	static void InitializeDefinitions();
	static void DestroyDefinitions();


	static BlockTemplate const* GetByName(char const* name);
	std::vector<BlockTemplateEntry> m_blockTemplateEntries;

	private:
	std::string m_name;
	static std::vector<BlockTemplate*> s_blockTemplates;
};