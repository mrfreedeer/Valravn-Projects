#pragma once
#include <string>
#include <cstdint>
#include <vector>
#include <map>
#include <d3d12.h>
#include "Engine/Renderer/GraphicsCommon.hpp"

enum ShaderType: unsigned int {
	InvalidShader = UINT_MAX,
	Vertex = 0,
	Pixel,
	Geometry,
	Hull,
	Domain,
	Compute,
	NUM_SHADER_TYPES
};

struct ShaderLoadInfo {
	std::string m_shaderSrc = "";
	std::string m_shaderName = "";
	std::string m_shaderEntryPoint = "";
	ShaderType m_shaderType = ShaderType::InvalidShader;
	bool m_antialiasing = false;
};

struct MaterialConfig
{
	std::string m_name = "Unnamed";
	std::string m_src = "";
	ShaderLoadInfo m_shaders[ShaderType::NUM_SHADER_TYPES] = {};

	BlendMode m_blendMode = BlendMode::OPAQUE;
	DepthFunc m_depthFunc = DepthFunc::LESSEQUAL;
	FillMode m_fillMode = FillMode::SOLID;
	CullMode m_cullMode = CullMode::BACK;
	WindingOrder m_windingOrder = WindingOrder::COUNTERCLOCKWISE;
	TopologyType m_topology = TopologyType::TOPOLOGY_TYPE_TRIANGLE;
	bool m_depthEnable = false;
	bool m_stencilEnable = false;
};


struct ShaderByteCode {
	std::string m_src = "";
	std::string m_shaderName = "";
	std::vector<uint8_t> m_byteCode;
	ShaderType m_shaderType = ShaderType::InvalidShader;
};

class Material;

struct SiblingMaterials {
	Material* m_blendModeSiblings[(size_t)BlendMode::NUM_BLEND_MODES] = {};
	Material* m_depthFuncSiblings[(size_t)DepthFunc::NUM_DEPTH_TESTS] = {};
	Material* m_depthEnableSiblings[2] = {}; // Only one with hard coded two. It's either on or off
	Material* m_fillModeSiblings[(size_t)FillMode::NUM_FILL_MODES] = {};
	Material* m_cullModeSiblings[(size_t)CullMode::NUM_CULL_MODES] = {};
	Material* m_windingOrderSiblings[(size_t)WindingOrder::NUM_WINDING_ORDERS] = {};
	Material* m_topologySiblings[(size_t)TopologyType::NUM_TOPOLOGIES] = {};
};

namespace tinyxml2 {
	class XMLElement;
}

typedef tinyxml2::XMLElement XMLElement;
class Material
{
	friend class Renderer;
	friend class MaterialSystem;

public:
	Material(const Material& copy) = delete;
	const std::string& GetName() const;
	const std::string& GetPath() const;
private:
	Material(const MaterialConfig& config);
	Material() = default;
	~Material();

	void LoadFromXML(XMLElement const* xmlElement);
	void ParseAttribute(std::string const& attributeName, XMLElement const& xmlElement);
	void ParseShader(std::string const& attributeName, XMLElement const& xmlElement);
	void ParseBlendMode(XMLElement const& xmlElement);
	void ParseWindingOrder(XMLElement const& xmlElement);
	void ParseCullMode(XMLElement const& xmlElement);
	void ParseFillMode(XMLElement const& xmlElement);
	void ParseTopology(XMLElement const& xmlElement);
	void ParseDepthStencil(XMLElement const& xmlElement);
	
	char const* GetEntryPoint(ShaderType shaderType) const;
	static char const* GetTargetForShader(ShaderType shaderType);
private:
	MaterialConfig	m_config = {};
	SiblingMaterials m_siblings = {};
	ShaderByteCode* m_byteCodes[ShaderType::NUM_SHADER_TYPES]= {} ;
	std::vector<uint8_t> m_cachedPSO;
	std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayout;

	ID3D12PipelineState* m_PSO = nullptr;
};
