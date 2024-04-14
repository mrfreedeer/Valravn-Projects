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
}
 