#include "Engine/Core/Buffer.hpp"
#include "Engine/Renderer/D3D12/Resource.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/GraphicsCommon.hpp"
#include <d3d12.h>
#include <d3dx12.h> // Notice the X. These are the helper structures not the DX12 header

Buffer::Buffer(BufferDesc const& bufferDesc) :
	m_owner(bufferDesc.owner),
	m_size(bufferDesc.size),
	m_stride(bufferDesc.stride),
	m_memoryUsage(bufferDesc.memoryUsage),
	m_descriptorHeap(bufferDesc.descriptorHeap),
	m_data(bufferDesc.data)
{
	
}

void Buffer::Initialize()
{
	m_buffer = new Resource();
	switch (m_memoryUsage)
	{
	case MemoryUsage::Default:
		CreateDefaultBuffer(m_data);
		break;
	case MemoryUsage::Dynamic:
		CreateDynamicBuffer(m_data);
		break;
	default:
		break;
	}
}

bool Buffer::GuaranteeBufferSize(size_t newsize)
{
	if (m_size >= newsize) return false;

	DX_SAFE_RELEASE(m_buffer->m_resource);
	m_size = newsize;
	CreateDynamicBuffer(nullptr);

	return true;
}

BufferView Buffer::GetBufferView() const
{
	BufferView bView = {
		m_buffer->m_resource->GetGPUVirtualAddress(),
		m_size,
		m_stride
	};

	return bView;
}

Buffer::~Buffer()
{
	delete m_buffer;
	m_buffer = nullptr;
}

void Buffer::CopyCPUToGPU(void const* data, size_t sizeInBytes)
{
	GuaranteeBufferSize(sizeInBytes);
	//ComPtr<ID3D12Device2>& device = m_owner->m_device;

	//m_buffer->TransitionTo(D3D12_RESOURCE_STATE_GENERIC_READ, m_ow);
	ID3D12Resource2*& resource = m_buffer->m_resource;



	UINT8* dataBegin;
	CD3DX12_RANGE readRange(0, sizeInBytes);        // We do not intend to read from this resource on the CPU.
	ThrowIfFailed(resource->Map(0, &readRange, reinterpret_cast<void**>(&dataBegin)), "COULD NOT MAP BUFFER");
	memcpy(dataBegin, data, sizeInBytes);
	resource->Unmap(0, nullptr);
}

void Buffer::CreateDynamicBuffer(void const* data)
{
	DX_SAFE_RELEASE(m_buffer->m_resource);
	CreateAndCopyToUploadBuffer(m_buffer->m_resource, data);
}

void Buffer::CreateDefaultBuffer(void const* data)
{
	ID3D12Resource2*& resource = m_buffer->m_resource;

	CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m_size);
	ThrowIfFailed(m_owner->m_device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&resource)), "COULD NOT CREATE GPU BUFFER");
	m_owner->SetDebugName(resource, "BUFFER");

	ID3D12Resource2* intermediateBuffer = nullptr;
	CreateAndCopyToUploadBuffer(intermediateBuffer, data);
	ComPtr<ID3D12GraphicsCommandList2> bufferCommList = m_owner->GetBufferCommandList();
	ID3D12CommandList* ppCommandList[] = { bufferCommList.Get() };
	// Copy to final buffer destination
	bufferCommList->CopyBufferRegion(resource, 0, intermediateBuffer, 0, m_size);
	m_owner->ExecuteCommandLists(ppCommandList, 1);
	DX_SAFE_RELEASE(intermediateBuffer);
}


void Buffer::CreateAndCopyToUploadBuffer(ID3D12Resource2*& uploadBuffer, void const* data)
{
	ComPtr<ID3D12Device2>& device = m_owner->m_device;

	CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m_size);
	ThrowIfFailed(device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&uploadBuffer)), "COULD NOT CREATE COMMITED VERTEX BUFFER RESOURCE");

	if (data) {
		// Copy the data
		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(0, m_size);        // We do not intend to read from this resource on the CPU.
		ThrowIfFailed(uploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)), "COULD NOT MAP VERTEX BUFFER");
		memcpy(pVertexDataBegin, data, m_size);
		uploadBuffer->Unmap(0, nullptr);

		m_owner->WaitForGPU();
	}

	m_buffer->m_currentState = (int)D3D12_RESOURCE_STATE_GENERIC_READ;
}
