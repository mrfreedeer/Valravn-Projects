#include "Engine/Renderer/Material.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/GraphicsCommon.hpp"
#include "Engine/Core/XmlUtils.hpp"

Material::Material(const MaterialConfig& config) :
	m_config(config)
{
	
}

Material::~Material()
{
	DX_SAFE_RELEASE(m_PSO);
	m_PSO = nullptr;

	//for (D3D12_INPUT_ELEMENT_DESC& elementDesc : m_inputLayout) {
	//	// This is because I had to allocate some memory for the semantic names
	//	free((void*)elementDesc.SemanticName);
	//}
}

const std::string& Material::GetName() const
{
	return m_config.m_name;
}

const std::string& Material::GetPath() const
{
	return m_config.m_src;
}

void Material::LoadFromXML(XMLElement const* xmlElement)
{
	
	while (xmlElement) {
		std::string attrName = xmlElement->Name();
		ParseAttribute(attrName, *xmlElement);
		xmlElement = xmlElement->NextSiblingElement();
	}

	// Sibling with same config is this same material
	m_siblings.m_blendModeSiblings[(size_t)m_config.m_blendMode[0]] = this;
	m_siblings.m_cullModeSiblings[(size_t)m_config.m_cullMode] = this;
	m_siblings.m_depthFuncSiblings[(size_t)m_config.m_depthFunc] = this;
	m_siblings.m_depthEnableSiblings[(size_t)m_config.m_depthEnable] = this;
	m_siblings.m_fillModeSiblings[(size_t)m_config.m_fillMode] = this;
	m_siblings.m_topologySiblings[(size_t)m_config.m_topology] = this;
	m_siblings.m_windingOrderSiblings[(size_t)m_config.m_windingOrder] = this;
}

void Material::ParseShader(std::string const& attributeName, XMLElement const& xmlElement)
{
	ShaderType shaderType = ShaderType::InvalidShader;

	if (AreStringsEqualCaseInsensitive(attributeName, "vertexshader")) {
		shaderType = ShaderType::Vertex;
	}
	else if (AreStringsEqualCaseInsensitive(attributeName, "pixelshader")) {
		shaderType = ShaderType::Pixel;
	}
	else if (AreStringsEqualCaseInsensitive(attributeName, "geometryshader")) {
		shaderType = ShaderType::Geometry;
	}
	else if (AreStringsEqualCaseInsensitive(attributeName, "hullshader")) {
		shaderType = ShaderType::Hull;
	}
	else if (AreStringsEqualCaseInsensitive(attributeName, "domainshader")) {
		shaderType = ShaderType::Domain;
	}
	else if (AreStringsEqualCaseInsensitive(attributeName, "meshshader")) {
		shaderType = ShaderType::Mesh;
		m_pipelineType = PipelineType::Mesh;
	}
	else if (AreStringsEqualCaseInsensitive(attributeName, "computeshader")) {
		shaderType = ShaderType::Compute;
		m_pipelineType = PipelineType::Compute;
	}

	ShaderLoadInfo& loadInfo = m_config.m_shaders[shaderType];
	loadInfo.m_shaderEntryPoint = ParseXmlAttribute(xmlElement, "entryPoint", "Unknown");
	bool isEngineMat = ParseXmlAttribute(xmlElement, "EngineShader", false);

	std::string shaderSrc = (isEngineMat) ? ENGINE_MAT_DIR : "";
	shaderSrc += ParseXmlAttribute(xmlElement, "src", "Unknown Shader Src");
	loadInfo.m_shaderName = ParseXmlAttribute(xmlElement, "shaderName", shaderSrc);

	loadInfo.m_shaderSrc = shaderSrc;
	loadInfo.m_shaderType = shaderType;

}

#undef OPAQUE

void Material::ParseBlendMode(XMLElement const& xmlElement)
{
	std::string blendModeStr = ParseXmlAttribute(xmlElement, "value", "Opaque");

	BlendMode defaultBlendMode = ParseFromString(blendModeStr, BlendMode::OPAQUE);

	for (int rtIndex = 0; rtIndex < 8; rtIndex++) {
		BlendMode& rtBlendMode = m_config.m_blendMode[rtIndex];
		rtBlendMode = defaultBlendMode;

	}
}

void Material::ParseWindingOrder(XMLElement const& xmlElement)
{
	std::string windingOrderStr = ParseXmlAttribute(xmlElement, "value", "CCW");
	WindingOrder& windingOrder = m_config.m_windingOrder;

	if (AreStringsEqualCaseInsensitive(windingOrderStr, "ccw")) {
		windingOrder = WindingOrder::COUNTERCLOCKWISE;
	}
	else {
		windingOrder = WindingOrder::CLOCKWISE;
	}
}

void Material::ParseCullMode(XMLElement const& xmlElement)
{
	std::string cullModestr = ParseXmlAttribute(xmlElement, "value", "BackFace");
	CullMode& cullMode = m_config.m_cullMode;

	if (AreStringsEqualCaseInsensitive(cullModestr, "frontface")) {
		cullMode = CullMode::FRONT;
	}
	else if (AreStringsEqualCaseInsensitive(cullModestr, "backface")) {
		cullMode = CullMode::BACK;
	}
	else {
		cullMode = CullMode::NONE;
	}
}

void Material::ParseFillMode(XMLElement const& xmlElement)
{
	std::string fillModeStr = ParseXmlAttribute(xmlElement, "value", "Solid");
	FillMode& fillMode = m_config.m_fillMode;

	if (AreStringsEqualCaseInsensitive(fillModeStr, "solid")) {
		fillMode = FillMode::SOLID;
	}
	else {
		fillMode = FillMode::WIREFRAME;
	}
}

void Material::ParseTopology(XMLElement const& xmlElement)
{
	std::string topologyStr = ParseXmlAttribute(xmlElement, "value", "TriangleList");
	TopologyType& topologyMode = m_config.m_topology;

	if (AreStringsEqualCaseInsensitive(topologyStr, "TriangleList")) {
		topologyMode = TopologyType::TOPOLOGY_TYPE_TRIANGLE;
	}
	else if (AreStringsEqualCaseInsensitive(topologyStr, "LineList")) {
		topologyMode = TopologyType::TOPOLOGY_TYPE_LINE;
	}
	else if (AreStringsEqualCaseInsensitive(topologyStr, "PointList")) {
		topologyMode = TopologyType::TOPOLOGY_TYPE_POINT;
	}
	else if (AreStringsEqualCaseInsensitive(topologyStr, "PatchList")) {
		topologyMode = TopologyType::TOPOLOGY_TYPE_PATCH;
	}
	else {
		ERROR_AND_DIE(Stringf("UNSUPPORTED TOPOLOGY MODE %s", topologyStr.c_str()));
	}
}

void Material::ParseDepthStencil(XMLElement const& xmlElement)
{
	std::string depthFunctionStr = ParseXmlAttribute(xmlElement, "depthFunction", "ALWAYS");
	DepthFunc& depthTest = m_config.m_depthFunc;

	if (AreStringsEqualCaseInsensitive(depthFunctionStr, "Lessequal")) {
		depthTest = DepthFunc::LESSEQUAL;
	}
	else if (AreStringsEqualCaseInsensitive(depthFunctionStr, "Always")) {
		depthTest = DepthFunc::ALWAYS;
	}
	else if (AreStringsEqualCaseInsensitive(depthFunctionStr, "Equal")) {
		depthTest = DepthFunc::EQUAL;
	}
	else if (AreStringsEqualCaseInsensitive(depthFunctionStr, "Greater")) {
		depthTest = DepthFunc::GREATER;
	}
	else if (AreStringsEqualCaseInsensitive(depthFunctionStr, "GREATEREQUAL")) {
		depthTest = DepthFunc::GREATEREQUAL;
	}
	else if (AreStringsEqualCaseInsensitive(depthFunctionStr, "LESS")) {
		depthTest = DepthFunc::LESS;
	}
	else if (AreStringsEqualCaseInsensitive(depthFunctionStr, "NEVER")) {
		depthTest = DepthFunc::NEVER;
	}
	else if (AreStringsEqualCaseInsensitive(depthFunctionStr, "NOTEQUAL")) {
		depthTest = DepthFunc::NOTEQUAL;
	}
	else {
		ERROR_AND_DIE("UNSUPPORTED DEPTH TEST");
	}

	m_config.m_depthEnable = ParseXmlAttribute(xmlElement, "writeDepth", false);
}

void Material::ParseRenderTargets(XMLElement const& xmlElement)
{
	XMLElement const* renderTargetDesc = xmlElement.FirstChildElement();
	m_config.m_numRenderTargets = ParseXmlAttribute(xmlElement, "count", 1);
	if(m_config.m_numRenderTargets == 0) m_config.m_renderTargetFormats[0] = TextureFormat::INVALID;

	while (renderTargetDesc) {
		std::string texFormat = ParseXmlAttribute(*renderTargetDesc, "format", "INVALID");
		unsigned int rtIndex = ParseXmlAttribute(*renderTargetDesc, "index", 0);
		std::string blendModeStr = ParseXmlAttribute(*renderTargetDesc, "blendMode", "Opaque");

		m_config.m_renderTargetFormats[rtIndex] = ParseFromString(texFormat, TextureFormat::INVALID);
		BlendMode customBlendMode = ParseFromString(blendModeStr, BlendMode::OPAQUE);
		m_config.m_blendMode[rtIndex] = customBlendMode;

		renderTargetDesc = renderTargetDesc->NextSiblingElement();
	}
}

char const* Material::GetEntryPoint(ShaderType shaderType) const
{
	return m_config.m_shaders[shaderType].m_shaderEntryPoint.c_str();
}

char const* Material::GetTargetForShader(ShaderType shaderType)
{
	switch (shaderType)
	{
	case Vertex:
		return "vs_6_5";
	case Pixel:
		return "ps_6_5";
	case Geometry:
		return "gs_6_5";
	case Mesh:
		return "ms_6_5";
	case Hull:
		return "hs_6_5";
	case Domain:
		return "ds_6_5";
	case Compute:
		return "cs_6_5";
	default:
		ERROR_AND_DIE("UNSUPPORTED SHADER TYPE");
	}
}

void Material::ParseAttribute(std::string const& attributeName, XMLElement const& xmlElement)
{

	if (ContainsStringCaseInsensitive(attributeName, "shader")) {
		ParseShader(attributeName, xmlElement);
		return;
	}

	if (AreStringsEqualCaseInsensitive(attributeName, "blendmode")) {
		ParseBlendMode(xmlElement);
		return;
	}

	if (AreStringsEqualCaseInsensitive(attributeName, "Windingorder")) {
		ParseWindingOrder(xmlElement);
		return;
	}

	if (AreStringsEqualCaseInsensitive(attributeName, "cullmode")) {
		ParseCullMode(xmlElement);
		return;
	}
	if (AreStringsEqualCaseInsensitive(attributeName, "topology")) {
		ParseTopology(xmlElement);
		return;
	}
	if (AreStringsEqualCaseInsensitive(attributeName, "depthstencil")) {
		ParseDepthStencil(xmlElement);
		return;
	}

	if (AreStringsEqualCaseInsensitive(attributeName, "rendertargets")) {
		ParseRenderTargets(xmlElement);
		return;
	}

}

MaterialConfig::MaterialConfig()
{
	// Default RT0 is R8G8B8A8_UNORM
	m_renderTargetFormats[0] = TextureFormat::R8G8B8A8_UNORM;
	for (unsigned int rtIndex = 1; rtIndex < 8; rtIndex++) {
		m_renderTargetFormats[rtIndex] = TextureFormat::INVALID;
	}
}
