#include "Engine/Renderer/UnorderedAccessBuffer.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Renderer/D3D11/D3D11TypeConversions.hpp"
#include <d3d11.h>

UnorderedAccessBuffer::UnorderedAccessBuffer(ID3D11Device* device, void* initialData, size_t numElements, size_t stride, UAVType type, TextureFormat format) :
	m_device(device),
	m_numElements(numElements),
	m_stride(stride),
	m_type(type)
{

	switch (type)
	{
	case UAVType::RAW_TEXTURE2D:
		CreateRawUAV(format);
		break;
	case UAVType::STRUCTURED:
		CreateStructuredUAV(initialData);
		break;
	default:
		ERROR_AND_DIE("UNKNOWN TYPE OF UAV");
		break;
	}

}

UnorderedAccessBuffer::~UnorderedAccessBuffer()
{
	DX_SAFE_RELEASE(m_UAV);
	DX_SAFE_RELEASE(m_SRV);
	DX_SAFE_RELEASE(m_buffer);
}

void UnorderedAccessBuffer::CreateStructuredUAV(void* data)
{

	D3D11_BUFFER_DESC descBuffer = {};
	descBuffer.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	descBuffer.ByteWidth = UINT(m_numElements * m_stride);
	descBuffer.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	descBuffer.StructureByteStride = (UINT)m_stride;

	D3D11_SUBRESOURCE_DATA* initialData = nullptr;

	if (data) {
		initialData = new D3D11_SUBRESOURCE_DATA();
		initialData->pSysMem = data;
		initialData->SysMemPitch = UINT(m_numElements * m_stride);
	}


	HRESULT bufferCreation = m_device->CreateBuffer(&descBuffer, initialData, &m_buffer);
	if (!SUCCEEDED(bufferCreation)) {
		ERROR_AND_DIE("COULD NOT CREATE UABUFFER");
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
	srvDesc.BufferEx.FirstElement = 0;
	srvDesc.BufferEx.NumElements = (UINT)m_numElements;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;

	HRESULT srvCreation = m_device->CreateShaderResourceView(m_buffer, &srvDesc, &m_SRV);
	if (!SUCCEEDED(srvCreation)) {
		ERROR_AND_DIE("COULD NOT CREATE SHADER RESOURCE VIEW FOR UA BUFFER");
	}

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = (UINT)m_numElements;

	HRESULT uavCreation = m_device->CreateUnorderedAccessView(m_buffer, &uavDesc, &m_UAV);
	if (!SUCCEEDED(uavCreation)) {
		ERROR_AND_DIE("COULD NOT CREATE UAV");
	}
}

void UnorderedAccessBuffer::CreateRawUAV(TextureFormat format)
{

	D3D11_BUFFER_DESC descBuffer = {};
	descBuffer.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	descBuffer.ByteWidth = UINT(m_numElements * m_stride);
	descBuffer.StructureByteStride = (UINT)m_stride;

	HRESULT bufferCreation = m_device->CreateBuffer(&descBuffer, NULL, &m_buffer);
	if (!SUCCEEDED(bufferCreation)) {
		ERROR_AND_DIE("COULD NOT CREATE UABUFFER");
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
	srvDesc.BufferEx.FirstElement = 0;
	srvDesc.BufferEx.NumElements = (UINT)m_numElements;
	srvDesc.Format = LocalToD3D11(format);

	HRESULT srvCreation = m_device->CreateShaderResourceView(m_buffer, &srvDesc, &m_SRV);
	if (!SUCCEEDED(srvCreation)) {
		ERROR_AND_DIE("COULD NOT CREATE SHADER RESOURCE VIEW FOR UA BUFFER");
	}

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Format = LocalToD3D11(format);
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = (UINT)m_numElements;

	HRESULT uavCreation = m_device->CreateUnorderedAccessView(m_buffer, &uavDesc, &m_UAV);
	if (!SUCCEEDED(uavCreation)) {
		ERROR_AND_DIE("COULD NOT CREATE UAV");
	}
}
