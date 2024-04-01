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
	m_descriptorHeap(bufferDesc.descriptorHeap),
	m_data(bufferDesc.data)
{

}


Buffer::~Buffer()
{
	delete m_buffer;
	m_buffer = nullptr;

	delete m_uploadResource;
	m_uploadResource = nullptr;
}

void Buffer::Initialize()
{
	m_buffer = new Resource();
	m_uploadResource = new Resource();
	switch (m_memoryUsage)
	{
	case MemoryUsage::Default:
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
		ERROR_AND_DIE("HAVENT DONE UAV TEXTURE VIEW");
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
	GuaranteeBufferSize(sizeInBytes);
	//ComPtr<ID3D12Device2>& device = m_owner->m_device;

	//m_buffer->TransitionTo(D3D12_RESOURCE_STATE_GENERIC_READ, m_ow);
	ID3D12Resource2*& resource = m_uploadResource->m_resource;
	ID3D12GraphicsCommandList6* rscCommandList = m_owner->m_ResourcesCommandList.Get();

	auto cmdAlloc = m_owner->GetCommandAllocForCmdList(CommandListType::ResourcesCommandList);
	cmdAlloc->Reset();
	rscCommandList->Reset(cmdAlloc, nullptr);

	m_uploadResource->TransitionTo(D3D12_RESOURCE_STATE_GENERIC_READ, rscCommandList);
	m_buffer->TransitionTo(D3D12_RESOURCE_STATE_COPY_DEST, rscCommandList);

	UINT8* dataBegin;
	CD3DX12_RANGE readRange(0, sizeInBytes);        // We do not intend to read from this resource on the CPU.
	ThrowIfFailed(resource->Map(0, &readRange, reinterpret_cast<void**>(&dataBegin)), "COULD NOT MAP BUFFER");
	memcpy(dataBegin, data, sizeInBytes);
	resource->Unmap(0, nullptr);



	rscCommandList->CopyResource(m_buffer->m_resource, m_uploadResource->m_resource);
	m_buffer->TransitionTo(D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, rscCommandList);
	rscCommandList->Close();

	ID3D12CommandList* ppCommandLists[] = { rscCommandList };
	m_owner->ExecuteCommandLists(ppCommandLists, 1);

	ComPtr<ID3D12Fence1> fence;
	m_owner->CreateFence(fence, "Rsc Fence");
	m_owner->SignalFence(fence, 1);
	if (fence->GetCompletedValue() != 1)
	{
		HANDLE event = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
		fence->SetEventOnCompletion(1, event);

		WaitForSingleObjectEx(event, INFINITE, false);
		CloseHandle(event);
	}
}

void Buffer::CreateDynamicBuffer(void const* data)
{
	DX_SAFE_RELEASE(m_buffer->m_resource);
	DX_SAFE_RELEASE(m_uploadResource->m_resource);
	CreateBuffer(m_buffer);
	CreateBuffer(m_uploadResource, true);
	if (data) {
		CopyCPUToGPU(data, m_size);
	}

}

void Buffer::CreateBuffer(Resource*& buffer, bool isUpload)
{
	ID3D12Resource2*& resource = buffer->m_resource;

	CD3DX12_HEAP_PROPERTIES heapProperties = (isUpload) ? CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD) : CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_RESOURCE_STATES initialState = (isUpload) ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COMMON;
	buffer->m_currentState = initialState;
	CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m_size);
	ThrowIfFailed(m_owner->m_device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		initialState,
		nullptr,
		IID_PPV_ARGS(&resource)), "COULD NOT CREATE GPU BUFFER");
	char const* debugName = (isUpload) ? "UPLOAD BUFFER" : "GPU BUFFER";
	m_owner->SetDebugName(resource, debugName);
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

ResourceView* Buffer::CreateShaderResourceView()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = new D3D12_SHADER_RESOURCE_VIEW_DESC();
	/*
	* SHADER COMPONENT MAPPING RGBA = 0,1,2,3
	* 4 Force a value of 0
	* 5 Force a value of 1
	*/
	srvDesc->Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(0, 1, 2, 3);
	srvDesc->Format = LocalToD3D12(TextureFormat::UNKNOWN);
	srvDesc->ViewDimension = D3D12_SRV_DIMENSION_BUFFER;

	ResourceViewInfo viewInfo = {};
	viewInfo.m_srvDesc = srvDesc;
	viewInfo.m_viewType = RESOURCE_BIND_SHADER_RESOURCE_BIT;
	viewInfo.m_source = m_buffer;


	srvDesc->Buffer.FirstElement = 0;
	srvDesc->Buffer.NumElements = UINT(m_size / m_stride);
	srvDesc->Buffer.StructureByteStride = UINT(m_stride);
	srvDesc->Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;


	ResourceView* newView = m_owner->CreateResourceView(viewInfo);
	m_views.push_back(newView);

	return newView;
}

StructuredBuffer::StructuredBuffer(BufferDesc const& bufDesc) :
	Buffer(bufDesc)
{
	Buffer::Initialize();
}

StructuredBuffer::~StructuredBuffer()
{

}
