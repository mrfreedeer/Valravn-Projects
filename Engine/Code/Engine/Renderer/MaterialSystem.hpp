#pragma once
#include <vector>
#include <string>
#include <filesystem>

class Material;
class Renderer;

struct MaterialSystemConfig {
	Renderer* m_renderer = nullptr;
};

enum class SiblingMatTypes {
	BLEND_MODE_SIBLING = 0,
	DEPTH_FUNC_SIBLING,
	DEPTH_ENABLING_SIBLING,
	FILL_MODE_SIBLING,
	CULL_MODE_SIBLING,
	WINDING_ORDER_SIBLING,
	TOPOLOGY_SIBLING
};

class MaterialSystem {
public:
	MaterialSystem(MaterialSystemConfig const& config);
	void Startup();
	void Shutdown();
	void BeginFrame();
	void EndFrame();

	Material* CreateOrGetMaterial(std::filesystem::path& materialPathNoExt);
	Material* GetMaterialForName(std::string const& materialNameNoExt);
	Material* GetMaterialForPath(std::filesystem::path const& materialPath);
	/// <summary>
	/// Uses single loop to get materials with path or name for material. Path > Name
	/// </summary>
	/// <param name="materialPath"></param>
	/// <param name="materialName"></param>
	/// <returns></returns>
	Material* GetMaterialForPathOrName(std::filesystem::path const& materialPath, std::string const& materialName);
	Material* CreateMaterial(std::string const& materialXMLFile);
	Material* GetSiblingMaterial(Material* material, SiblingMatTypes siblingType, unsigned int newSiblingAccessor);
private:
	void LoadEngineMaterials();
	void SetSibling(Material* material, Material* siblingMaterial, SiblingMatTypes siblingType, unsigned int newSiblingAccessor);
private:
	MaterialSystemConfig m_config = {};
	std::vector<Material*> m_loadedMaterials;
};