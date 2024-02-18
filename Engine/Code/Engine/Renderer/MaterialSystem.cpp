#include "MaterialSystem.hpp"
#include "Engine/Renderer/Material.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Core/XmlUtils.hpp"
#include <string>

MaterialSystem::MaterialSystem(MaterialSystemConfig const& config) :
	m_config(config)
{

}

void MaterialSystem::Startup()
{
	LoadEngineMaterials();
}

void MaterialSystem::Shutdown()
{
	for (Material*& mat : m_loadedMaterials) {
		delete mat;
		mat = nullptr;
	}
}

void MaterialSystem::BeginFrame()
{

}

void MaterialSystem::EndFrame()
{

}

Material* MaterialSystem::CreateOrGetMaterial(std::filesystem::path& materialPathNoExt)
{
	std::string materialXMLPath = materialPathNoExt.replace_extension(".xml").string();
	std::string materialName = materialPathNoExt.filename().replace_extension("").string();
	Material* material = g_theMaterialSystem->GetMaterialForPathOrName(materialXMLPath.c_str(), materialName);

	if (material) {
		return material;
	}

	return CreateMaterial(materialXMLPath);
}



Material* MaterialSystem::GetMaterialForName(std::string const& materialNameNoExt)
{
	for (int matIndex = 0; matIndex < m_loadedMaterials.size(); matIndex++) {
		Material* material = m_loadedMaterials[matIndex];
		if (material->GetName() == materialNameNoExt) {
			return material;
		}
	}
	return nullptr;
}


Material* MaterialSystem::GetMaterialForPath(std::filesystem::path const& materialPath)
{
	for (int matIndex = 0; matIndex < m_loadedMaterials.size(); matIndex++) {
		Material* material = m_loadedMaterials[matIndex];
		if (material->GetPath() == materialPath.string()) {
			return material;
		}
	}
	return nullptr;
}


Material* MaterialSystem::GetMaterialForPathOrName(std::filesystem::path const& materialPath, std::string const& materialName)
{
	for (int matIndex = 0; matIndex < m_loadedMaterials.size(); matIndex++) {
		Material* material = m_loadedMaterials[matIndex];
		bool foundMat = material->GetPath() == materialPath.string();
		foundMat = foundMat || AreStringsEqualCaseInsensitive(material->GetName(), materialName);
		if (foundMat) {
			return material;
		}
	}

	return nullptr;
}

Material* MaterialSystem::CreateMaterial(std::string const& materialXMLFile)
{
	XMLDoc materialDoc;
	XMLError loadStatus = materialDoc.LoadFile(materialXMLFile.c_str());
	GUARANTEE_OR_DIE(loadStatus == tinyxml2::XML_SUCCESS, Stringf("COULD NOT LOAD MATERIAL XML FILE %s", materialXMLFile.c_str()));

	XMLElement const* firstElem = materialDoc.FirstChildElement("Material");

	std::string matName = ParseXmlAttribute(*firstElem, "name", "Unnamed Material");

	// Material properties
	XMLElement const* matProperty = firstElem->FirstChildElement();

	Material* newMat = new Material();
	newMat->LoadFromXML(matProperty);
	newMat->m_config.m_name = matName;
	newMat->m_config.m_src = materialXMLFile;
	m_config.m_renderer->CreatePSOForMaterial(newMat);

	m_loadedMaterials.push_back(newMat);

	return newMat;
}



Material* MaterialSystem::GetSiblingMaterial(Material* material, SiblingMatTypes siblingType, unsigned int newSiblingAccessor)
{
	Material* siblingMat = nullptr;
	MaterialConfig newConfig = material->m_config;
	switch (siblingType)
	{
	case SiblingMatTypes::BLEND_MODE_SIBLING:
		siblingMat = material->m_siblings.m_blendModeSiblings[newSiblingAccessor];
		newConfig.m_blendMode = (BlendMode)newSiblingAccessor;
		newConfig.m_name += Stringf("RunTSibBlend(%s)", EnumToString((BlendMode)newSiblingAccessor));
		break;
	case SiblingMatTypes::DEPTH_FUNC_SIBLING:
		siblingMat = material->m_siblings.m_depthFuncSiblings[newSiblingAccessor];
		newConfig.m_depthFunc = (DepthFunc)newSiblingAccessor;
		newConfig.m_name += Stringf("RunTSibDepth(%s)", EnumToString((DepthFunc)newSiblingAccessor));
		break;
	case SiblingMatTypes::DEPTH_ENABLING_SIBLING:
		siblingMat = material->m_siblings.m_depthEnableSiblings[newSiblingAccessor];
		newConfig.m_depthEnable = (bool)newSiblingAccessor;
		newConfig.m_name += Stringf("RunTSibDepthEnable(%d)", newSiblingAccessor);
		break;
	case SiblingMatTypes::FILL_MODE_SIBLING:
		siblingMat = material->m_siblings.m_fillModeSiblings[newSiblingAccessor];
		newConfig.m_fillMode = (FillMode)newSiblingAccessor;
		newConfig.m_name += Stringf("RunTSibFill(%s)", EnumToString((FillMode)newSiblingAccessor));
		break;
	case SiblingMatTypes::CULL_MODE_SIBLING:
		siblingMat = material->m_siblings.m_cullModeSiblings[newSiblingAccessor];
		newConfig.m_cullMode = (CullMode)newSiblingAccessor;
		newConfig.m_name += Stringf("RunTSibCull(%s)", EnumToString((CullMode)newSiblingAccessor));
		break;
	case SiblingMatTypes::WINDING_ORDER_SIBLING:
		siblingMat = material->m_siblings.m_windingOrderSiblings[newSiblingAccessor];
		newConfig.m_windingOrder = (WindingOrder)newSiblingAccessor;
		newConfig.m_name += Stringf("RunTSibWinding(%s)", EnumToString((WindingOrder)newSiblingAccessor));
		break;
	case SiblingMatTypes::TOPOLOGY_SIBLING:
		siblingMat = material->m_siblings.m_topologySiblings[newSiblingAccessor];
		newConfig.m_topology = (TopologyType)newSiblingAccessor;
		newConfig.m_name += Stringf("RunTSibTopology(%s)", EnumToString((TopologyType)newSiblingAccessor));
		break;
	default:
		ERROR_AND_DIE(Stringf("UNRECOGNIZED SIBLING MATERIAL TYPE: %d", siblingType).c_str())
		break;
	}

	if (!siblingMat) {
		siblingMat = new Material(newConfig);
		m_config.m_renderer->CreatePSOForMaterial(siblingMat);

		m_loadedMaterials.push_back(siblingMat);
		SetSibling(material, siblingMat, siblingType, newSiblingAccessor);
	}

	return siblingMat;
}

void MaterialSystem::LoadEngineMaterials()
{
	std::string materialsPath = ENGINE_MAT_DIR;
	for (auto const& matPath : std::filesystem::directory_iterator(materialsPath)) {
		if (matPath.is_directory()) continue;

		CreateMaterial(matPath.path().string());
	}
}

void MaterialSystem::SetSibling(Material* material, Material* siblingMaterial, SiblingMatTypes siblingType, unsigned int newSiblingAccessor)
{
	switch (siblingType)
	{
	case SiblingMatTypes::BLEND_MODE_SIBLING:
		material->m_siblings.m_blendModeSiblings[newSiblingAccessor] = siblingMaterial;
		siblingMaterial->m_siblings.m_blendModeSiblings[(size_t)material->m_config.m_blendMode] = material;
		break;
	case SiblingMatTypes::DEPTH_FUNC_SIBLING:
		material->m_siblings.m_depthFuncSiblings[newSiblingAccessor] = siblingMaterial;
		siblingMaterial->m_siblings.m_depthFuncSiblings[(size_t)material->m_config.m_depthFunc] = material;
		break;
	case SiblingMatTypes::DEPTH_ENABLING_SIBLING:
		material->m_siblings.m_depthEnableSiblings[newSiblingAccessor] = siblingMaterial;
		siblingMaterial->m_siblings.m_depthEnableSiblings[(size_t)material->m_config.m_depthEnable] = material;
		break;
	case SiblingMatTypes::FILL_MODE_SIBLING:
		material->m_siblings.m_fillModeSiblings[newSiblingAccessor] = siblingMaterial;
		siblingMaterial->m_siblings.m_fillModeSiblings[(size_t)material->m_config.m_fillMode] = material;
		break;
	case SiblingMatTypes::CULL_MODE_SIBLING:
		material->m_siblings.m_cullModeSiblings[newSiblingAccessor] = siblingMaterial;
		siblingMaterial->m_siblings.m_cullModeSiblings[(size_t)material->m_config.m_cullMode] = material;
		break;
	case SiblingMatTypes::WINDING_ORDER_SIBLING:
		material->m_siblings.m_windingOrderSiblings[newSiblingAccessor] = siblingMaterial;
		siblingMaterial->m_siblings.m_windingOrderSiblings[(size_t)material->m_config.m_windingOrder] = material;
		break;
	case SiblingMatTypes::TOPOLOGY_SIBLING:
		material->m_siblings.m_topologySiblings[newSiblingAccessor] = siblingMaterial;
		siblingMaterial->m_siblings.m_topologySiblings[(size_t)material->m_config.m_topology] = material;
		break;
	default:
		ERROR_AND_DIE(Stringf("UNRECOGNIZED SIBLING MATERIAL TYPE: %d", siblingType).c_str())
			break;
	}
}
