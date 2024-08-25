#include "Engine/Renderer/ImmediateContext.hpp"

void ImmediateContext::SetIndexDrawFlag(bool isIndexedDraw)
{
	m_drawFlags &= ~INDEXED_BIT_MASK;
	m_drawFlags |= (isIndexedDraw << INDEXED_BIT_SHIFT);
}

void ImmediateContext::SetPipelineType(PipelineType pipelineType)
{
	m_pipelineType = pipelineType;
}

void ImmediateContext::SetDefaultDepthTextureSRVFlag(bool usesDepthAsTexture)
{
	m_drawFlags &= ~DEPTH_TEXTURE_BIT_MASK;
	m_drawFlags |= (usesDepthAsTexture << DEPTH_TEXTURE_BIT_SHIFT);
}


void ImmediateContext::SetDrawCallUsage(bool usedInDrawCall)
{
	m_drawFlags &= ~DRAW_CALLED_BIT_MASK;
	m_drawFlags |= (usedInDrawCall << DRAW_CALLED_BIT_SHIFT);
}

void ImmediateContext::SetDiffuseShaderUsage(bool usesDiffuseShaders)
{
	m_drawFlags &= ~DIFFUSE_BIT_MASK;
	m_drawFlags |= (usesDiffuseShaders << DIFFUSE_BIT_SHIFT);
}

void ImmediateContext::SetRenderTargetClear(unsigned int slot, bool isCleared)
{
	m_clearRtFlags &= (0 << slot);
	m_clearRtFlags |= (isCleared << slot);
}

void ImmediateContext::SetDepthRenderTargetClear(bool isCleared)
{
	m_isDRTCleared = isCleared;
}

void ImmediateContext::SetRenderTarget(unsigned int index, Texture* renderTarget)
{
	m_renderTargets[index] = renderTarget;
}

void ImmediateContext::SetDepthRenderTarget(Texture* depthRenderTarget)
{
	m_depthTarget = depthRenderTarget;
}

void ImmediateContext::SetVertexType(VertexType vertexType)
{
	m_vertexType = vertexType;
}

void ImmediateContext::SetRootConstant(unsigned int constant, unsigned int slot)
{
	m_drawConstants[slot] = constant;
}


void ImmediateContext::Reset()
{
	ResetCopy();
	ResetExternalBuffers();
}

void ImmediateContext::ResetCopy()
{
	m_vertexType = VertexType::PCU;
	m_pipelineType = PipelineType::Graphics;
	m_drawFlags = 0; // Keep the markers push or pop status
	m_clearRtFlags = 0;
	m_isDRTCleared = false;
	m_cameraCBO = nullptr;
	m_modelCBO = nullptr;
	m_boundTextures.clear();
	m_boundCBuffers.clear();
	m_boundBuffers.clear();
	m_boundRWBuffers.clear();
	m_boundRWTextures.clear();
	m_vertexStart = 0;
	m_vertexCount = 0;
	m_indexStart = 0;
	m_indexCount = 0;
	m_dispatchThreads = IntVec3::ZERO;
	//VertexBuffer* m_immediateBuffer = nullptr;
	m_material = nullptr;
	for (int rtIndex = 0; rtIndex < 8; rtIndex++) {
		m_renderTargets[rtIndex] = nullptr;
	}
	m_depthTarget = nullptr;
	m_srvHandleStart = 0;
	m_cbvHandleStart = 0;
	m_externalIBO = nullptr;
	m_externalVBO = nullptr;

	memset(m_drawConstants, 0, sizeof(unsigned int) * 16);
}

void ImmediateContext::ResetExternalBuffers()
{
	m_externalIBO = nullptr;
	m_externalVBO = nullptr;
}

