#pragma once
#include "Engine/Renderer/GraphicsCommon.hpp"
#include "Engine/Math/IntVec3.hpp"
#include <map>
/*
	Flags
	Bit:		Meaning
	0			Normal vertex draw(0) / Indexed draw(1)
	1			Uses default depth as texture
	2			Has been used for draw call
	3			Uses diffuse shaders
*/

constexpr unsigned int INDEXED_BIT_SHIFT = 0;
constexpr unsigned int DEPTH_TEXTURE_BIT_SHIFT = 1;
constexpr unsigned int DRAW_CALLED_BIT_SHIFT = 2;
constexpr unsigned int DIFFUSE_BIT_SHIFT = 3;


constexpr unsigned int INDEXED_BIT_MASK = (1 << 0);
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
	bool IsIndexedDraw() const { return m_drawFlags & INDEXED_BIT_MASK; }
	bool IsComputeShader() const { return m_pipelineType == PipelineType::Compute; }
	bool IsDrawTypePipeline() const { return m_pipelineType != PipelineType::Compute; }
	bool UsesMeshShaders() const { return m_pipelineType == PipelineType::Mesh; }
	bool UsesRegularPipeline() const { return m_pipelineType == PipelineType::Graphics; }
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
	void SetPipelineType(PipelineType pipelineType);
	void SetDefaultDepthTextureSRVFlag(bool usesDepthAsTexture);
	void SetDrawCallUsage(bool usedInDrawCall);
	void SetDiffuseShaderUsage(bool usesDiffuseShaders);
	void SetRenderTargetClear(unsigned int slot, bool isCleared);
	void SetDepthRenderTargetClear(bool isCleared);
	void SetRenderTarget(unsigned int index, Texture* renderTarget);
	void SetDepthRenderTarget(Texture* depthRenderTarget);
	void SetVertexType(VertexType vertexType);
	void SetRootConstant(unsigned int constant, unsigned int slot);

	void Reset();
	void ResetExternalBuffers();
private:
	ImmediateContext() = default;

private:
	unsigned int m_drawFlags = 0; // 0 defaults for all pipeline defaults
	unsigned int m_clearRtFlags = 0;
	bool m_isDRTCleared = false;

	VertexType m_vertexType = VertexType::PCU;
	PipelineType m_pipelineType = PipelineType::Graphics;
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
	unsigned int m_uavHandleStart = 0;
	unsigned int m_depthSRVSlot = 0;
	IntVec3 m_dispatchThreads = IntVec3::ZERO;
	ModelConstants m_modelConstants = {};

	std::map<unsigned int, Texture const*> m_boundTextures;
	std::map<unsigned int, Texture const*> m_boundRWTextures;
	std::map<unsigned int, ConstantBuffer*> m_boundCBuffers;
	std::map<unsigned int, Buffer*> m_boundBuffers;
	std::map<unsigned int, Buffer*> m_boundRWBuffers;
	unsigned int m_drawConstants[16] = {};
};