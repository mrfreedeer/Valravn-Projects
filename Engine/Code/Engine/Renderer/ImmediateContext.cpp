#include "Engine/Renderer/ImmediateContext.hpp"

void ImmediateContext::SetIndexDrawFlag(bool isIndexedDraw)
{
	m_drawFlags &= ~INDEXED_BIT_MASK;
	m_drawFlags |= (isIndexedDraw << INDEXED_BIT_SHIFT);
}

void ImmediateContext::SetPipelineTypeFlag(bool usesMeshShader)
{
	m_drawFlags &= ~PIPELINE_TYPE_BIT_MASK;
	m_drawFlags |= (usesMeshShader << PIPELINE_TYPE_SHIFT);
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

void ImmediateContext::Reset()
{
	m_vertexType = VertexType::PCU;
	m_drawFlags = 0;
	m_clearRtFlags = 0;
	m_isDRTCleared = false;
	m_cameraCBO = nullptr;
	m_modelCBO = nullptr;
	m_boundTextures.clear();
	m_boundCBuffers.clear();
	m_boundBuffers.clear();
	m_vertexStart = 0;
	m_vertexCount = 0;
	m_indexStart = 0;
	m_indexCount = 0;
	m_meshThreads = IntVec3::ZERO;
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
}

void ImmediateContext::ResetExternalBuffers()
{
	m_externalIBO = nullptr;
	m_externalVBO = nullptr;
}

