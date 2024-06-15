#pragma once
#include "Engine/Renderer/GraphicsCommon.hpp"
#include "Engine/Math/IntVec3.hpp"
#include <map>
/*
	Flags
	Bit:		Meaning
	0			Normal vertex draw(0) / Indexed draw(1)
	1			Regular Graphics Pipeline(0) / Mesh Shader Pipeline(1)
	2			Uses default depth as texture
	3			Is render pass begin
	4			Is render pass end
	5			Has been used for draw call
	6			Uses diffuse shaders
	7			Clears Render Target
	8			Clears Render Pass
*/

constexpr unsigned int INDEXED_BIT_SHIFT = 0;
constexpr unsigned int PIPELINE_TYPE_SHIFT = 1;
constexpr unsigned int DEPTH_TEXTURE_BIT_SHIFT = 2;
constexpr unsigned int DRAW_CALLED_BIT_SHIFT = 3;
constexpr unsigned int DIFFUSE_BIT_SHIFT = 4;


constexpr unsigned int INDEXED_BIT_MASK = (1 << 0);
constexpr unsigned int PIPELINE_TYPE_BIT_MASK = (1 << PIPELINE_TYPE_SHIFT);
constexpr unsigned int DEPTH_TEXTURE_BIT_MASK = (1 << DEPTH_TEXTURE_BIT_SHIFT);
constexpr unsigned int DRAW_CALLED_BIT_MASK = (1 << DRAW_CALLED_BIT_SHIFT);
constexpr unsigned int DIFFUSE_BIT_MASK = (1 << DIFFUSE_BIT_SHIFT);

class ConstantBuffer;
class Texture;
class Buffer;
class Material;
class IndexBuffer;
class VertexBuffer;

class ImmediateContext {
	friend class Renderer;
public:
	// Getters
	bool IsIndexDraw() const { return m_drawFlags & INDEXED_BIT_MASK; }
	bool UsesMeshShaders() const { return m_drawFlags & PIPELINE_TYPE_BIT_MASK; }
	bool UsesRegularPipeline() const { return !(m_drawFlags & PIPELINE_TYPE_BIT_MASK); }
	bool UsesDefaultDepthAsTexture() const { return m_drawFlags & DEPTH_TEXTURE_BIT_MASK; }
	bool WasUsedForDrawCall() const { return m_drawFlags & DRAW_CALLED_BIT_MASK; }
	bool UsesDiffuseShaders() const { return m_drawFlags & DIFFUSE_BIT_MASK; }
	bool UsesExternalBuffers() const { return (m_externalVBO || m_externalIBO); }
	Texture const* GetDepthSRV() { return m_boundTextures[m_depthSRVSlot]; }
	bool IsRenderTargetCleared(unsigned int slot) const { return m_clearRtFlags & (1 << slot); }
	bool IsDRTCleared() const { return m_isDRTCleared; }
	VertexType GetVertexType() const { return m_vertexType; }
	Texture* GetDepthRenderTarget() { return m_depthTarget; }

	// Setters
	void SetIndexDrawFlag(bool isIndexedDraw);
	void SetPipelineTypeFlag(bool usesMeshShader);
	void SetDefaultDepthTextureSRVFlag(bool usesDepthAsTexture);
	void SetDrawCallUsage(bool usedInDrawCall);
	void SetDiffuseShaderUsage(bool usesDiffuseShaders);
	void SetRenderTargetClear(unsigned int slot, bool isCleared);
	void SetDepthRenderTargetClear(bool isCleared);
	void SetRenderTarget(unsigned int index, Texture* renderTarget);
	void SetDepthRenderTarget(Texture* depthRenderTarget);
	void SetVertexType(VertexType vertexType);

	void Reset();
	void ResetExternalBuffers();
private:
	ImmediateContext() = default;

private:
	unsigned int m_drawFlags = 0; // 0 defaults for all pipeline defaults
	unsigned int m_clearRtFlags = 0;
	bool m_isDRTCleared = false;

	VertexType m_vertexType = VertexType::PCU;
	Material* m_material = nullptr;
	Texture* m_renderTargets[8] = {};
	Texture* m_depthTarget = nullptr;
	ConstantBuffer* m_cameraCBO = nullptr;
	ConstantBuffer* m_modelCBO = nullptr;
	VertexBuffer* m_externalVBO = nullptr;
	IndexBuffer* m_externalIBO = nullptr;

	size_t m_vertexStart = 0;
	size_t m_vertexCount = 0;
	size_t m_indexStart = 0;
	size_t m_indexCount = 0;
	unsigned int m_srvHandleStart = 0;
	unsigned int m_cbvHandleStart = 0;
	unsigned int m_depthSRVSlot = 0;
	IntVec3 m_meshThreads = IntVec3::ZERO;
	ModelConstants m_modelConstants = {};

	std::map<unsigned int, Texture const*> m_boundTextures;
	std::map<unsigned int, ConstantBuffer*> m_boundCBuffers;
	std::map<unsigned int, Buffer*> m_boundBuffers;
};