#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Engine/Renderer/D3D12/Resource.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include <d3d12.h>

ConstantBuffer::ConstantBuffer(BufferDesc const& bufferDesc) :
	Buffer(bufferDesc)
{
	size_t newSize = AlignToCBufferStride(m_size);
	m_size = newSize;
	m_stride = m_size;

}

/// <summary>
/// For Cbuffers, it makes sense to choose a different initialization time
/// </summary>
void ConstantBuffer::Initialize()
{
	if(m_initialized) return;
	Buffer::Initialize();
	m_initialized = true;
}


void ConstantBuffer::CopyCPUToGPU(void const* data, size_t sizeInBytes)
{
	sizeInBytes = AlignToCBufferStride(sizeInBytes);
	Buffer::CopyCPUToGPU(data, sizeInBytes);
}

size_t ConstantBuffer::AlignToCBufferStride(size_t size) const
{
	size_t newSize = (size & ~0xFF) + 0x100;
	return newSize;
}

ConstantBuffer::~ConstantBuffer()
{
	if (m_bufferView) {
		delete m_bufferView;
	}
}

ResourceView* ConstantBuffer::GetOrCreateView()
{
	if (m_bufferView) return m_bufferView;

	BufferView bufferV = GetBufferView();

	D3D12_CONSTANT_BUFFER_VIEW_DESC* cBufferView = new D3D12_CONSTANT_BUFFER_VIEW_DESC();
	cBufferView->BufferLocation = bufferV.m_bufferLocation;
	cBufferView->SizeInBytes = (UINT)bufferV.m_sizeInBytes;

	ResourceViewInfo bufferViewInfo = {};
	bufferViewInfo.m_cbvDesc = cBufferView;
	bufferViewInfo.m_viewType = RESOURCE_BIND_CONSTANT_BUFFER_VIEW_BIT;

	m_bufferView = m_owner->CreateResourceView(bufferViewInfo, m_descriptorHeap);

	return m_bufferView;
}
