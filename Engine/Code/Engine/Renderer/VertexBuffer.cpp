#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include <d3d11.h> 

VertexBuffer::VertexBuffer(ID3D11Device* device, size_t sizeBytes, size_t strideSize, MemoryUsage memoryUsage, void const* data):
	m_size(sizeBytes),
	m_device(device),
	m_stride(strideSize)
{
	if (memoryUsage == MemoryUsage::Dynamic) {
		D3D11_BUFFER_DESC bufferDesc = {};
		bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufferDesc.ByteWidth = static_cast<UINT>(sizeBytes);
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		HRESULT bufferCreationResult = m_device->CreateBuffer(&bufferDesc, NULL, &m_buffer);
		if (!SUCCEEDED(bufferCreationResult)) {
			ERROR_AND_DIE("VERTEX BUFFER CREATION RESULT ERROR");
		}
	}
	else {
		D3D11_BUFFER_DESC bufferDesc = {};
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.ByteWidth = static_cast<UINT>(sizeBytes);
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA initialData;
		initialData.pSysMem = data;
		initialData.SysMemPitch = 0;
		initialData.SysMemSlicePitch = 0;

		HRESULT bufferCreationResult = m_device->CreateBuffer(&bufferDesc, &initialData, &m_buffer);
		if (!SUCCEEDED(bufferCreationResult)) {
			ERROR_AND_DIE("VERTEX BUFFER CREATION RESULT ERROR");
		}
	}
	
}

VertexBuffer::~VertexBuffer()
{
	DX_SAFE_RELEASE(m_buffer);
}



unsigned int VertexBuffer::GetStride() const
{
	return static_cast<unsigned int>(m_stride);
}

bool VertexBuffer::GuaranteeBufferSize(size_t newSizeBytes)
{
	if (m_size >= newSizeBytes) return false;
	
	DX_SAFE_RELEASE(m_buffer);

	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = static_cast<UINT>(newSizeBytes);
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HRESULT bufferCreationResult = m_device->CreateBuffer(&bufferDesc, NULL, &m_buffer);
	if (!SUCCEEDED(bufferCreationResult)) {
		ERROR_AND_DIE("VERTEX BUFFER RESIZE RESULT ERROR");
	}

	m_size = newSizeBytes;
	return true;
}
