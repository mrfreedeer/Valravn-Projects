#include "Game/Gameplay/BlockTemplate.hpp"
#include "Game/Gameplay/BlockDefinition.hpp"


std::vector<BlockTemplate*> BlockTemplate::s_blockTemplates;

BlockTemplate::BlockTemplate(char const* name, std::vector<BlockTemplateEntry>& blockTemplateEntries):
    m_name(name),
    m_blockTemplateEntries(blockTemplateEntries)
{
}

void BlockTemplate::InitializeDefinitions()
{
    BlockDefinition const* oak = BlockDefinition::GetDefByName("oak_log");
	BlockDefinition const* oak_leaves = BlockDefinition::GetDefByName("oak_leaves"); 
	BlockDefinition const* spruce = BlockDefinition::GetDefByName("spruce_log");
	BlockDefinition const* spruce_leaves = BlockDefinition::GetDefByName("spruce_leaves");
	BlockDefinition const* cactus = BlockDefinition::GetDefByName("cactus");

    std::vector<BlockTemplateEntry> treeTemplateEntries;
    
    treeTemplateEntries.emplace_back(oak->m_id, IntVec3::ZERO);
    treeTemplateEntries.emplace_back(oak->m_id, IntVec3(0, 0, 1));
    treeTemplateEntries.emplace_back(oak->m_id, IntVec3(0, 0, 2));
    treeTemplateEntries.emplace_back(oak->m_id, IntVec3(0, 0, 3));
    treeTemplateEntries.emplace_back(oak_leaves->m_id, IntVec3(0, 0, 4));
    treeTemplateEntries.emplace_back(oak_leaves->m_id, IntVec3(0, 0, 5));

	treeTemplateEntries.emplace_back(oak_leaves->m_id, IntVec3(1, 0, 4));
	treeTemplateEntries.emplace_back(oak_leaves->m_id, IntVec3(0, 1, 4));
	treeTemplateEntries.emplace_back(oak_leaves->m_id, IntVec3(1, 1, 4));
	treeTemplateEntries.emplace_back(oak_leaves->m_id, IntVec3(-1, 1, 4));
	treeTemplateEntries.emplace_back(oak_leaves->m_id, IntVec3(1, -1, 4));
	treeTemplateEntries.emplace_back(oak_leaves->m_id, IntVec3(-1, 0, 4));
	treeTemplateEntries.emplace_back(oak_leaves->m_id, IntVec3(0, -1, 4));
	treeTemplateEntries.emplace_back(oak_leaves->m_id, IntVec3(-1, -1, 4));

    treeTemplateEntries.emplace_back(oak_leaves->m_id, IntVec3(1, 0, 5));
    treeTemplateEntries.emplace_back(oak_leaves->m_id, IntVec3(0, 1, 5));
    treeTemplateEntries.emplace_back(oak_leaves->m_id, IntVec3(1, 1, 5));
    treeTemplateEntries.emplace_back(oak_leaves->m_id, IntVec3(-1, 1, 5));
    treeTemplateEntries.emplace_back(oak_leaves->m_id, IntVec3(1, -1, 5));
    treeTemplateEntries.emplace_back(oak_leaves->m_id, IntVec3(-1, 0, 5));
    treeTemplateEntries.emplace_back(oak_leaves->m_id, IntVec3(0, -1, 5));
    treeTemplateEntries.emplace_back(oak_leaves->m_id, IntVec3(-1, -1, 5));

	std::vector<BlockTemplateEntry> spruceTemplateEntries;

	spruceTemplateEntries.emplace_back(spruce->m_id, IntVec3::ZERO);
	spruceTemplateEntries.emplace_back(spruce->m_id, IntVec3(0, 0, 1));
	spruceTemplateEntries.emplace_back(spruce->m_id, IntVec3(0, 0, 2));
	spruceTemplateEntries.emplace_back(spruce->m_id, IntVec3(0, 0, 3));
	spruceTemplateEntries.emplace_back(spruce_leaves->m_id, IntVec3(0, 0, 4));
	spruceTemplateEntries.emplace_back(spruce_leaves->m_id, IntVec3(0, 0, 5));

	spruceTemplateEntries.emplace_back(spruce_leaves->m_id, IntVec3(1, 0, 4));
	spruceTemplateEntries.emplace_back(spruce_leaves->m_id, IntVec3(0, 1, 4));
	spruceTemplateEntries.emplace_back(spruce_leaves->m_id, IntVec3(1, 1, 4));
	spruceTemplateEntries.emplace_back(spruce_leaves->m_id, IntVec3(-1, 1, 4));
	spruceTemplateEntries.emplace_back(spruce_leaves->m_id, IntVec3(1, -1, 4));
	spruceTemplateEntries.emplace_back(spruce_leaves->m_id, IntVec3(-1, 0, 4));
	spruceTemplateEntries.emplace_back(spruce_leaves->m_id, IntVec3(0, -1, 4));
	spruceTemplateEntries.emplace_back(spruce_leaves->m_id, IntVec3(-1, -1, 4));

	spruceTemplateEntries.emplace_back(spruce_leaves->m_id, IntVec3(1, 0, 5));
	spruceTemplateEntries.emplace_back(spruce_leaves->m_id, IntVec3(0, 1, 5));
	spruceTemplateEntries.emplace_back(spruce_leaves->m_id, IntVec3(1, 1, 5));
	spruceTemplateEntries.emplace_back(spruce_leaves->m_id, IntVec3(-1, 1, 5));
	spruceTemplateEntries.emplace_back(spruce_leaves->m_id, IntVec3(1, -1, 5));
	spruceTemplateEntries.emplace_back(spruce_leaves->m_id, IntVec3(-1, 0, 5));
	spruceTemplateEntries.emplace_back(spruce_leaves->m_id, IntVec3(0, -1, 5));
	spruceTemplateEntries.emplace_back(spruce_leaves->m_id, IntVec3(-1, -1, 5));

	std::vector<BlockTemplateEntry> cactusTemplateEntries;

	cactusTemplateEntries.emplace_back(cactus->m_id, IntVec3::ZERO);
	cactusTemplateEntries.emplace_back(cactus->m_id, IntVec3(0, 0, 1));
	cactusTemplateEntries.emplace_back(cactus->m_id, IntVec3(0, 0, 2));
	cactusTemplateEntries.emplace_back(cactus->m_id, IntVec3(0, 0, 3));

    BlockTemplate* oakTreeTemplate = new BlockTemplate("oak_tree", treeTemplateEntries);
    BlockTemplate* spruceTreeTemplate = new BlockTemplate("spruce_tree", spruceTemplateEntries);
    BlockTemplate* cactusTemplate = new BlockTemplate("cactus", cactusTemplateEntries);

    s_blockTemplates.push_back(oakTreeTemplate);
    s_blockTemplates.push_back(spruceTreeTemplate);
    s_blockTemplates.push_back(cactusTemplate);
}

void BlockTemplate::DestroyDefinitions()
{
	for (int templateIndex = 0; templateIndex < s_blockTemplates.size(); templateIndex++) {
		BlockTemplate*& blockTemplate = s_blockTemplates[templateIndex];
		if (blockTemplate) {
			delete blockTemplate;
            blockTemplate = nullptr;
		}
	}


    s_blockTemplates.clear();
}

BlockTemplate const* BlockTemplate::GetByName(char const* name)
{
    std::string stringName(name);
    for (int templateIndex = 0; templateIndex < s_blockTemplates.size(); templateIndex++) {
        BlockTemplate const* blockTemplate = s_blockTemplates[templateIndex];
        if (blockTemplate) {
            if (blockTemplate->m_name == stringName) {
                return blockTemplate;
            }
        }
    }
    
    return nullptr;
}
