#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include <d3d11.h> 

IndexBuffer::IndexBuffer(ID3D11Device* device, size_t sizeBytes) :
	m_size(sizeBytes),
	m_device(device)
{
	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = static_cast<UINT>(sizeBytes);
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;

	HRESULT bufferCreationResult = m_device->CreateBuffer(&bufferDesc, NULL, &m_buffer);
	if (!SUCCEEDED(bufferCreationResult)) {
		ERROR_AND_DIE("INDEX BUFFER CREATION RESULT ERROR");
	}
}

IndexBuffer::~IndexBuffer()
{
	DX_SAFE_RELEASE(m_buffer);
}



unsigned int IndexBuffer::GetStride() const
{
	return sizeof(unsigned int);
}

bool IndexBuffer::GuaranteeBufferSize(size_t newSizeBytes)
{
	if (m_size >= newSizeBytes) return false;

	DX_SAFE_RELEASE(m_buffer);

	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = static_cast<UINT>(newSizeBytes);
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;

	HRESULT bufferCreationResult = m_device->CreateBuffer(&bufferDesc, NULL, &m_buffer);
	if (!SUCCEEDED(bufferCreationResult)) {
		ERROR_AND_DIE("INDEX BUFFER RESIZE RESULT ERROR");
	}

	m_size = newSizeBytes;
	return true;
}
