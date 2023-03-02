#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Renderer/Renderer.hpp"

#include <d3d11.h> 

ConstantBuffer::ConstantBuffer(ID3D11Device* device, size_t sizeBytes):
m_size(sizeBytes),
m_device(device)
{
	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = static_cast<UINT>(sizeBytes);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HRESULT bufferCreationResult = m_device->CreateBuffer(&bufferDesc, NULL, &m_buffer);
	if (!SUCCEEDED(bufferCreationResult)) {
		ERROR_AND_DIE("BUFFER CREATION RESULT ERROR");
	}
}

ConstantBuffer::~ConstantBuffer()
{
	DX_SAFE_RELEASE(m_buffer);
}

bool ConstantBuffer::GuaranteeBufferSize(size_t newSizeBytes)
{
	if (m_size >= newSizeBytes) return false;

	DX_SAFE_RELEASE(m_buffer);

	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = static_cast<UINT>(newSizeBytes);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HRESULT bufferCreationResult = m_device->CreateBuffer(&bufferDesc, NULL, &m_buffer);
	if (!SUCCEEDED(bufferCreationResult)) {
		ERROR_AND_DIE("CONSTANT BUFFER RESIZE RESULT ERROR");
	}

	m_size = newSizeBytes;
	return true;
}
