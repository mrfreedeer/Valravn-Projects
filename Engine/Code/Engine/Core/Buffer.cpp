#include "Engine/Core/Buffer.hpp"
#include "Engine/Renderer/D3D12/Resource.hpp"
#include "Engine/Renderer/D3D12/D3D12TypeConversions.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/GraphicsCommon.hpp"
#include <d3d12.h>
#include <d3dx12.h> // Notice the X. These are the helper structures not the DX12 header

Buffer::Buffer(BufferDesc const& bufferDesc) :
	m_owner(bufferDesc.owner),
	m_size(bufferDesc.size),
	m_stride(bufferDesc.stride),
	m_memoryUsage(bufferDesc.memoryUsage),
	m_bufferDesc(bufferDesc),
	m_name(bufferDesc.debugName),
	m_device(m_owner->m_device.Get())
{

}


Buffer::~Buffer()
{
	for (ResourceView*& view : m_views) {
		if (view) {
			delete view;
			view = nullptr;
		}
	}

	m_views.clear();

	if (m_buffer) {
		delete m_buffer;
		m_buffer = nullptr;
	}

	if (m_uploadBuffer) {
		delete m_uploadBuffer;
		m_uploadBuffer = nullptr;
	}

}

void Buffer::Initialize()
{
	m_buffer = new Resource(m_device);

	switch (m_memoryUsage)
	{
	case MemoryUsage::Default:
		CreateBuffer(m_buffer);
		break;
	case MemoryUsage::Upload:
		CreateBuffer(m_buffer, true);
		break;
	default:
		break;
	}

	if (m_bufferDesc.data) {
		CopyCPUToGPU(m_bufferDesc.data, m_bufferDesc.size);
	}
}

bool Buffer::GuaranteeBufferSize(size_t newsize)
{
	if (m_size >= newsize) return false;

	if (m_buffer) {
		DX_SAFE_RELEASE(m_buffer->m_resource);
	}

	m_size = newsize;
	if (m_memoryUsage == MemoryUsage::Default) {
		CreateBuffer(m_buffer);
	}

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

ResourceView* Buffer::GetOrCreateView(ResourceBindFlagBit viewType)
{
	for (ResourceView*& currentView : m_views) {
		if (currentView->m_viewInfo.m_viewType == viewType) {
			return currentView;
		}
	}

	switch (viewType)
	{
	case RESOURCE_BIND_SHADER_RESOURCE_BIT:
		return CreateShaderResourceView();
	case RESOURCE_BIND_UNORDERED_ACCESS_VIEW_BIT:
		return CreateUnorederedAccessView();
	case RESOURCE_BIND_CONSTANT_BUFFER_VIEW_BIT:
		return CreateConstantBufferView();
	case RESOURCE_BIND_DEPTH_STENCIL_BIT:
	case RESOURCE_BIND_RENDER_TARGET_BIT:
	case RESOURCE_BIND_NONE:
	default:
		ERROR_AND_DIE(Stringf("UNSUPPORTED VIEW %d", viewType));
		break;
	}
}


void Buffer::CopyCPUToGPU(void const* data, size_t sizeInBytes)
{
	void* mappedData = nullptr;
	Resource* usedBuffer = nullptr;

	if (m_memoryUsage == MemoryUsage::Default) {
		if (m_uploadBuffer) {
			DX_SAFE_RELEASE(m_uploadBuffer->m_resource);
		}
		else {
			m_uploadBuffer = new Resource(m_device);
		}

		CreateBuffer(m_uploadBuffer, true);

		if (!m_isPendingCopy) {
			m_owner->AddToUpdateQueue(this);
		}
		m_isPendingCopy = true;
		usedBuffer = m_uploadBuffer;

	}
	else {
		usedBuffer = m_buffer;
	}

	usedBuffer->Map(mappedData);
	memcpy(mappedData, data, sizeInBytes);
	usedBuffer->Unmap();


}

void Buffer::ClearPendingCopies()
{
	if (m_uploadBuffer) delete m_uploadBuffer;
	m_isPendingCopy = false;
}


Resource* Buffer::GetResource()
{
	if (m_memoryUsage == MemoryUsage::Upload) return m_uploadBuffer;

	return m_buffer;
}

void Buffer::ResetCopyState()
{
	m_isPendingCopy = false;
}

void Buffer::CreateBuffer(Resource* const& buffer, bool isUpload)
{
	ID3D12Resource2*& resource = buffer->m_resource;

	CD3DX12_HEAP_PROPERTIES heapProperties = (isUpload) ? CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD) : CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_RESOURCE_STATES initialState = (isUpload) ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COMMON;
	buffer->m_currentState = initialState;

	CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m_size);
	if (!isUpload && (m_bufferType == BufferType::StructuredBuffer)) {
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}


	ThrowIfFailed(m_owner->m_device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		initialState,
		nullptr,
		IID_PPV_ARGS(&resource)), "COULD NOT CREATE GPU BUFFER");
	char const* debugName = (isUpload) ? " UPLOAD BUFFER" : " GPU BUFFER";
	std::string fullDebugName = m_name + debugName;
	
	m_owner->SetDebugName(resource, fullDebugName.c_str());
}


ResourceView* Buffer::CreateShaderResourceView()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = new D3D12_SHADER_RESOURCE_VIEW_DESC();
	memset(srvDesc, 0, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));
	/*
	* SHADER COMPONENT MAPPING RGBA = 0,1,2,3
	* 4 Force a value of 0
	* 5 Force a value of 1
	*/
	srvDesc->Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(0, 1, 2, 3);
	srvDesc->Format = LocalToD3D12(TextureFormat::UNKNOWN);
	srvDesc->ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc->Buffer.FirstElement = 0;
	srvDesc->Buffer.NumElements = UINT(m_size / m_stride);
	srvDesc->Buffer.StructureByteStride = UINT(m_stride);
	srvDesc->Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	ResourceViewInfo viewInfo = {};
	viewInfo.m_srvDesc = srvDesc;
	viewInfo.m_viewType = RESOURCE_BIND_SHADER_RESOURCE_BIT;
	viewInfo.m_source = m_buffer;

	ResourceView* newView = m_owner->CreateResourceView(viewInfo);
	m_views.push_back(newView);

	return newView;
}

ResourceView* Buffer::CreateUnorederedAccessView()
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC* uaView = new D3D12_UNORDERED_ACCESS_VIEW_DESC();
	uaView->Buffer.FirstElement = 0;
	uaView->Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	uaView->Buffer.NumElements = UINT(m_size / m_stride);
	uaView->Buffer.StructureByteStride = UINT(m_stride);

	uaView->ViewDimension = D3D12_UAV_DIMENSION_BUFFER;

	ResourceViewInfo bufferViewInfo = {};
	bufferViewInfo.m_uavDesc = uaView;
	bufferViewInfo.m_viewType = RESOURCE_BIND_UNORDERED_ACCESS_VIEW_BIT;
	bufferViewInfo.m_source = m_buffer;

	ResourceView* newView = m_owner->CreateResourceView(bufferViewInfo);
	m_views.push_back(newView);

	return newView;
}

ResourceView* Buffer::CreateConstantBufferView()
{
	BufferView bufferV = GetBufferView();

	D3D12_CONSTANT_BUFFER_VIEW_DESC* cBufferView = new D3D12_CONSTANT_BUFFER_VIEW_DESC();
	cBufferView->BufferLocation = bufferV.m_bufferLocation;
	cBufferView->SizeInBytes = (UINT)bufferV.m_sizeInBytes;

	ResourceViewInfo bufferViewInfo = {};
	bufferViewInfo.m_cbvDesc = cBufferView;
	bufferViewInfo.m_viewType = RESOURCE_BIND_CONSTANT_BUFFER_VIEW_BIT;

	ResourceView* newView = m_owner->CreateResourceView(bufferViewInfo);
	m_views.push_back(newView);

	return newView;
}

StructuredBuffer::StructuredBuffer(BufferDesc const& bufDesc) :
	Buffer(bufDesc)
{
	m_bufferType = BufferType::StructuredBuffer;
	Buffer::Initialize();
}

StructuredBuffer::~StructuredBuffer()
{

}
