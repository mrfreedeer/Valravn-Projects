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

ImmediateContext::ImmediateContext(ImmediateContext const& copyFrom)
{
	m_drawFlags = copyFrom.m_drawFlags; // 0 defaults for all pipeline defaults
	m_clearRtFlags = copyFrom.m_clearRtFlags;
	m_isDRTCleared = copyFrom.m_isDRTCleared;

	m_vertexType = copyFrom.m_vertexType;
	m_pipelineType = copyFrom.m_pipelineType;
	m_material = copyFrom.m_material;
	m_depthTarget = copyFrom.m_depthTarget;
	m_cameraCBO = copyFrom.m_cameraCBO;
	m_modelCBO = copyFrom.m_modelCBO;
	m_externalVBO = copyFrom.m_externalVBO;
	m_externalIBO = copyFrom.m_externalIBO;

	m_vertexStart = copyFrom.m_vertexStart;
	m_vertexCount = copyFrom.m_vertexCount;
	m_indexStart = copyFrom.m_indexStart;
	m_indexCount = copyFrom.m_indexCount;
	m_srvHandleStart = copyFrom.m_srvHandleStart;
	m_cbvHandleStart = copyFrom.m_cbvHandleStart;
	m_uavHandleStart = copyFrom.m_uavHandleStart;
	m_depthSRVSlot = copyFrom.m_depthSRVSlot;
	m_markerPopCount = copyFrom.m_markerPopCount;
	m_markerPushCount = copyFrom.m_markerPushCount;

	m_dispatchThreads = copyFrom.m_dispatchThreads;
	m_modelConstants = copyFrom.m_modelConstants;

	m_boundTextures.insert(copyFrom.m_boundTextures.begin(), copyFrom.m_boundTextures.end());
	m_boundRWTextures.insert(copyFrom.m_boundRWTextures.begin(), copyFrom.m_boundRWTextures.end());
	m_boundCBuffers.insert(copyFrom.m_boundCBuffers.begin(), copyFrom.m_boundCBuffers.end());
	m_boundBuffers.insert(copyFrom.m_boundBuffers.begin(), copyFrom.m_boundBuffers.end());
	m_boundRWBuffers.insert(copyFrom.m_boundRWBuffers.begin(), copyFrom.m_boundRWBuffers.end());

	for (unsigned int rtIndex = 0; rtIndex < 8; rtIndex++) {
		m_renderTargets[rtIndex] = copyFrom.m_renderTargets[rtIndex];
	}
	for (unsigned int constantInd = 0; constantInd < 16; constantInd++) {
		m_drawConstants[constantInd] = copyFrom.m_drawConstants[constantInd];
	}
}

