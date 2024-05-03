#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Engine/Renderer/DefaultShader.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Renderer/Material.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Core/Image.hpp"
#include "Engine/Renderer/D3D12/Fence.hpp"
#include "Engine/Renderer/D3D12/Resource.hpp"
#include "Engine/Renderer/D3D12/D3D12TypeConversions.hpp"
#include "Engine/Renderer/GraphicsCommon.hpp"
#include "Engine/Window/Window.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/Vec4.hpp"
#include "Engine/Renderer/DebugRendererSystem.hpp"
#include "Engine/Renderer/ImmediateContext.hpp"
#include <dxgidebug.h>
#include <d3dx12.h> // Notice the X. These are the helper structures not the DX12 header
#include <regex>
#include <ThirdParty/ImGUI/imgui.h>
#include <ThirdParty/ImGUI/imgui_impl_win32.h>
#include <ThirdParty/ImGUI/imgui_impl_dx12.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
using namespace DirectX;

#pragma message("ENGINE_DIR == " ENGINE_DIR)
#pragma comment( lib, "dxgi.lib" )        // directx graphics interface
#pragma comment( lib, "d3dcompiler.lib" ) // shader compiler
#pragma comment( lib, "dxguid.lib" )

MaterialSystem* g_theMaterialSystem = nullptr;
struct VertexTest
{
	XMFLOAT3 position;
	XMFLOAT4 color;
};

std::wstring GetAssetFullPath(LPCWSTR assetName)
{
	std::string engineStr = ENGINE_DIR;
	engineStr += "Renderer/Materials/Shaders/";
	std::wstring returnString(engineStr.begin(), engineStr.end());

	return returnString + assetName;
}

LiveObjectReporter::~LiveObjectReporter()
{
#if defined(_DEBUG) 
	ID3D12Debug3* debugController = nullptr;

	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {

		IDXGIDebug1* debug;
		DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug));
		debugController->Release();
		debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
		debug->Release();
	}
	else {
		ERROR_AND_DIE("COULD NOT ENABLE DX12 LIVE REPORTING");
	}

#endif
}

DXGI_FORMAT GetFormatForComponent(D3D_REGISTER_COMPONENT_TYPE componentType, char const* semanticnName, BYTE mask) {
	// determine DXGI format
	if (mask == 1)
	{
		if (componentType == D3D_REGISTER_COMPONENT_UINT32) return DXGI_FORMAT_R32_UINT;
		else if (componentType == D3D_REGISTER_COMPONENT_SINT32) return DXGI_FORMAT_R32_SINT;
		else if (componentType == D3D_REGISTER_COMPONENT_FLOAT32) return DXGI_FORMAT_R32_FLOAT;
	}
	else if (mask <= 3)
	{
		if (componentType == D3D_REGISTER_COMPONENT_UINT32) return DXGI_FORMAT_R32G32_UINT;
		else if (componentType == D3D_REGISTER_COMPONENT_SINT32) return DXGI_FORMAT_R32G32_SINT;
		else if (componentType == D3D_REGISTER_COMPONENT_FLOAT32) return DXGI_FORMAT_R32G32_FLOAT;
	}
	else if (mask <= 7)
	{
		if (componentType == D3D_REGISTER_COMPONENT_UINT32) return DXGI_FORMAT_R32G32B32_UINT;
		else if (componentType == D3D_REGISTER_COMPONENT_SINT32) return DXGI_FORMAT_R32G32B32_SINT;
		else if (componentType == D3D_REGISTER_COMPONENT_FLOAT32) return DXGI_FORMAT_R32G32B32_FLOAT;
	}
	else if (mask <= 15)
	{
		if (AreStringsEqualCaseInsensitive(semanticnName, "COLOR")) {
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		}
		else {
			if (componentType == D3D_REGISTER_COMPONENT_UINT32) return DXGI_FORMAT_R32G32B32A32_UINT;
			else if (componentType == D3D_REGISTER_COMPONENT_SINT32) return DXGI_FORMAT_R32G32B32A32_SINT;
			else if (componentType == D3D_REGISTER_COMPONENT_FLOAT32) return DXGI_FORMAT_R32G32B32A32_FLOAT;
		}


	}

	return DXGI_FORMAT_UNKNOWN;
}



Renderer::Renderer(RendererConfig const& config) :
	m_config(config)
{

}


Renderer::~Renderer()
{

}

void GetHardwareAdapter(
	IDXGIFactory1* pFactory,
	IDXGIAdapter1** ppAdapter,
	bool requestHighPerformanceAdapter)
{
	*ppAdapter = nullptr;

	ComPtr<IDXGIAdapter1> adapter;

	ComPtr<IDXGIFactory6> factory6;
	if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
	{
		for (
			UINT adapterIndex = 0;
			SUCCEEDED(factory6->EnumAdapterByGpuPreference(
				adapterIndex,
				requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
				IID_PPV_ARGS(&adapter)));
			++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// Don't select the Basic Render Driver adapter.
				// If you want a software adapter, pass in "/warp" on the command line.
				continue;
			}

			// Check to see whether the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			{
				break;
			}
		}
	}

	if (adapter.Get() == nullptr)
	{
		for (UINT adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// Don't select the Basic Render Driver adapter.
				// If you want a software adapter, pass in "/warp" on the command line.
				continue;
			}

			// Check to see whether the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			{
				break;
			}
		}
	}

	*ppAdapter = adapter.Detach();
}


void Renderer::CreateDevice()
{
	UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
	// Enable additional debug layers.
	dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_DXGIFactory)), "FAILED TO CREATE DXGI FACTORY");


	ComPtr<IDXGIAdapter1> hardwareAdapter;
	GetHardwareAdapter(m_DXGIFactory.Get(), &hardwareAdapter, false);

	ThrowIfFailed(D3D12CreateDevice(
		hardwareAdapter.Get(),
		D3D_FEATURE_LEVEL_12_2,
		IID_PPV_ARGS(&m_device)
	), "FAILED TO CREATE DEVICE");

	SetDebugName(m_DXGIFactory, "DXGI FACTORY");
	SetDebugName(m_device, "DEVICE");

#if defined(_DEBUG)
	ComPtr<ID3D12InfoQueue> d3dInfoQueue;
	if (SUCCEEDED(m_device.As(&d3dInfoQueue)))
	{

		D3D12_MESSAGE_ID hide[] =
		{
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
		};
		D3D12_INFO_QUEUE_FILTER filter = {};
		filter.DenyList.NumIDs = _countof(hide);
		filter.DenyList.pIDList = hide;
		d3dInfoQueue->AddStorageFilterEntries(&filter);
	}
#endif
}



void Renderer::EnableDebugLayer()
{
#if defined(_DEBUG)
	// Enable the debug layer (requires the Graphics Tools "optional feature").
	// NOTE: Enabling the debug layer after device creation will invalidate the active device.
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();


		}


	}
#endif
}


void Renderer::CreateCommandQueue()
{
	// Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)), "FAILED TO CREATE COMMANDQUEUE");

	SetDebugName(m_commandQueue, "COMMAND QUEUE");
}

void Renderer::CreateSwapChain()
{
	Window* window = Window::GetWindowContext();
	IntVec2 windowDims = window->GetClientDimensions();

	m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, float(windowDims.x), float(windowDims.y));
	m_scissorRect = CD3DX12_RECT(0.0f, 0.0f, LONG(windowDims.x), LONG(windowDims.y));

	HWND windowHandle = (HWND)window->m_osWindowHandle;
	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = 2;
	swapChainDesc.Width = windowDims.x;
	swapChainDesc.Height = windowDims.y;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> swapChain;
	ThrowIfFailed(m_DXGIFactory->CreateSwapChainForHwnd(
		m_commandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
		windowHandle,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain
	), "Failed to create swap chain");

	// This sample does not support fullscreen transitions.
	ThrowIfFailed(m_DXGIFactory->MakeWindowAssociation(windowHandle, DXGI_MWA_NO_ALT_ENTER), "Failed to make window association");

	ThrowIfFailed(swapChain.As(&m_swapChain), "Failed to get Swap Chain");
	m_currentBackBuffer = m_swapChain->GetCurrentBackBufferIndex();
}

void Renderer::CreateDescriptorHeaps()
{
	m_GPUDescriptorHeaps[(unsigned int)DescriptorHeapType::SRV_UAV_CBV] = new DescriptorHeap(this, DescriptorHeapType::SRV_UAV_CBV, SRV_UAV_CBV_DEFAULT_SIZE, true);
	m_CPUDescriptorHeaps[(unsigned int)DescriptorHeapType::SRV_UAV_CBV] = new DescriptorHeap(this, DescriptorHeapType::SRV_UAV_CBV, SRV_UAV_CBV_DEFAULT_SIZE);

	m_GPUDescriptorHeaps[(unsigned int)DescriptorHeapType::SAMPLER] = new DescriptorHeap(this, DescriptorHeapType::SAMPLER, 2, true);
	m_CPUDescriptorHeaps[(unsigned int)DescriptorHeapType::SAMPLER] = new DescriptorHeap(this, DescriptorHeapType::SAMPLER, 2);

	m_CPUDescriptorHeaps[(unsigned int)DescriptorHeapType::RTV] = new DescriptorHeap(this, DescriptorHeapType::RTV, 64);

	m_CPUDescriptorHeaps[(unsigned int)DescriptorHeapType::DSV] = new DescriptorHeap(this, DescriptorHeapType::DSV, 64);
}

void Renderer::CreateFences()
{
	m_fence = new Fence(m_device.Get(), m_commandQueue.Get());
}

void Renderer::CreateDefaultRootSignature()
{
	/**
		 * Usual layout is 3 Constant Buffers
		 * Textures 0-8
		 * Sampler
		 * #TODO programatically define more complex root signatures. Perhaps just really on the HLSL definition?
		 */


	D3D12_DESCRIPTOR_RANGE1 descriptorRanges[4] = {};
	ZeroMemory(descriptorRanges, sizeof(D3D12_DESCRIPTOR_RANGE1) * 4);
	descriptorRanges[0] = { D3D12_DESCRIPTOR_RANGE_TYPE_CBV, CBV_DESCRIPTORS_AMOUNT, 0,0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE, 0 };
	descriptorRanges[1] = { D3D12_DESCRIPTOR_RANGE_TYPE_SRV, SRV_DESCRIPTORS_AMOUNT,0,0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND };
	descriptorRanges[2] = { D3D12_DESCRIPTOR_RANGE_TYPE_UAV, UAV_DESCRIPTORS_AMOUNT,0,0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND };
	descriptorRanges[3] = { D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 2, 0,0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND };


	// Base parameters, initial at the root table. Could be a descriptor, or a pointer to descriptor 
	// In this case, one descriptor table per slot in the first 3

	CD3DX12_ROOT_PARAMETER1 rootParameters[4] = {};

	rootParameters[0].InitAsDescriptorTable(1, &descriptorRanges[0], D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[1].InitAsDescriptorTable(1, &descriptorRanges[1], D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[2].InitAsDescriptorTable(1, &descriptorRanges[2], D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[3].InitAsDescriptorTable(1, &descriptorRanges[3], D3D12_SHADER_VISIBILITY_ALL);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignature(_countof(descriptorRanges), rootParameters);
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC MSRootSignature(_countof(descriptorRanges), rootParameters);
	rootSignature.Desc_1_2.Flags |= D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;
	rootSignature.Desc_1_2.Flags |= D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;
	MSRootSignature.Desc_1_2.Flags |= D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;
	MSRootSignature.Desc_1_2.Flags |= D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;

	rootSignature.Desc_1_2.Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	//rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	//ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error), "COULD NOT SERIALIZE ROOT SIGNATURE");
	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> MSsignature;
	ComPtr<ID3DBlob> error;

	HRESULT rootSignatureSerialization = D3D12SerializeVersionedRootSignature(&rootSignature, signature.GetAddressOf(), error.GetAddressOf());
	ThrowIfFailed(rootSignatureSerialization, "COULD NOT SERIALIZE ROOT SIGNATURE");
	ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)), "COULD NOT CREATE ROOT SIGNATURE");
	SetDebugName(m_rootSignature, "DEFAULTROOTSIGNATURE");

	rootSignatureSerialization = D3D12SerializeVersionedRootSignature(&MSRootSignature, MSsignature.GetAddressOf(), error.GetAddressOf());
	ThrowIfFailed(rootSignatureSerialization, "COULD NOT SERIALIZE ROOT SIGNATURE");
	//ThrowIfFailed(m_device->CreateRootSignature(0, MSsignature->GetBufferPointer(), MSsignature->GetBufferSize(), IID_PPV_ARGS(&m_MSRootSignature)), "COULD NOT CREATE ROOT SIGNATURE");
	//SetDebugName(m_MSRootSignature, "DEFAULTROOTSIGNATURE");
}

void Renderer::CreateBackBuffers()
{
	DescriptorHeap* rtvHeap = GetCPUDescriptorHeap(DescriptorHeapType::RTV);

	// Handle size is vendor specific
	unsigned int rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	// Get a helper handle to the descriptor and create the RTVs 

	m_backBuffers.resize(m_config.m_backBuffersCount);
	for (unsigned int frameBufferInd = 0; frameBufferInd < m_config.m_backBuffersCount; frameBufferInd++) {
		ID3D12Resource2* bufferTex = nullptr;
		Texture*& backBuffer = m_backBuffers[frameBufferInd];
		HRESULT fetchBuffer = m_swapChain->GetBuffer(frameBufferInd, IID_PPV_ARGS(&bufferTex));
		if (!SUCCEEDED(fetchBuffer)) {
			ERROR_AND_DIE("COULD NOT GET FRAME BUFFER");
		}

		D3D12_RESOURCE_DESC bufferTexDesc = bufferTex->GetDesc();

		TextureCreateInfo backBufferTexInfo = {};
		backBufferTexInfo.m_bindFlags = ResourceBindFlagBit::RESOURCE_BIND_RENDER_TARGET_BIT;
		backBufferTexInfo.m_dimensions = IntVec2((int)bufferTexDesc.Width, (int)bufferTexDesc.Height);
		backBufferTexInfo.m_format = TextureFormat::R8G8B8A8_UNORM;
		backBufferTexInfo.m_name = Stringf("BackBuffer %d", frameBufferInd);
		backBufferTexInfo.m_owner = this;
		backBufferTexInfo.m_handle = new Resource();
		TrackResource(backBufferTexInfo.m_handle);
		backBufferTexInfo.m_handle->m_resource = bufferTex;

		backBuffer = CreateTexture(backBufferTexInfo);
		backBuffer->GetOrCreateView(RESOURCE_BIND_RENDER_TARGET_BIT);
		DX_SAFE_RELEASE(bufferTex);
	}
}

ResourceView* Renderer::CreateShaderResourceView(ResourceViewInfo const& resourceViewInfo) const
{
	DescriptorHeap* srvDescriptorHeap = GetCPUDescriptorHeap(DescriptorHeapType::SRV_UAV_CBV);

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = srvDescriptorHeap->GetNextCPUHandle();
	m_device->CreateShaderResourceView(resourceViewInfo.m_source->m_resource, resourceViewInfo.m_srvDesc, cpuHandle);

	ResourceView* newResourceView = new ResourceView(resourceViewInfo);
	newResourceView->m_descriptorHandle = cpuHandle;
	newResourceView->m_source = resourceViewInfo.m_source;

	return newResourceView;
}

ResourceView* Renderer::CreateRenderTargetView(ResourceViewInfo const& viewInfo) const
{
	D3D12_RENDER_TARGET_VIEW_DESC* rtvDesc = viewInfo.m_rtvDesc;

	DescriptorHeap* rtvDescriptorHeap = GetCPUDescriptorHeap(DescriptorHeapType::RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = rtvDescriptorHeap->GetNextCPUHandle();

	m_device->CreateRenderTargetView(viewInfo.m_source->m_resource, rtvDesc, cpuHandle);

	ResourceView* newResourceView = new ResourceView(viewInfo);
	newResourceView->m_descriptorHandle = cpuHandle;

	return newResourceView;
}

ResourceView* Renderer::CreateDepthStencilView(ResourceViewInfo const& viewInfo) const
{
	D3D12_DEPTH_STENCIL_VIEW_DESC* dsvDesc = viewInfo.m_dsvDesc;

	DescriptorHeap* dsvDescriptorHeap = GetCPUDescriptorHeap(DescriptorHeapType::DSV);
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = dsvDescriptorHeap->GetNextCPUHandle();

	m_device->CreateDepthStencilView(viewInfo.m_source->m_resource, dsvDesc, cpuHandle);

	ResourceView* newResourceView = new ResourceView(viewInfo);
	newResourceView->m_descriptorHandle = cpuHandle;

	return newResourceView;
}

ResourceView* Renderer::CreateConstantBufferView(ResourceViewInfo const& viewInfo) const
{
	D3D12_CONSTANT_BUFFER_VIEW_DESC* cbvDesc = viewInfo.m_cbvDesc;

	DescriptorHeap* cbvDescriptorHeap = GetCPUDescriptorHeap(DescriptorHeapType::SRV_UAV_CBV);
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = cbvDescriptorHeap->GetNextCPUHandle();

	m_device->CreateConstantBufferView(cbvDesc, cpuHandle);
	HRESULT deviceRemoved = m_device->GetDeviceRemovedReason();
	ThrowIfFailed(deviceRemoved, Stringf("DEVICE REMOVED %#04x", deviceRemoved).c_str());

	ResourceView* newResourceView = new ResourceView(viewInfo);
	newResourceView->m_descriptorHandle = cpuHandle;

	return newResourceView;
}

/// <summary>
/// Only function allowed to create textures
/// </summary>
/// <param name="creationInfo"></param>
/// <returns></returns>
Texture* Renderer::CreateTexture(TextureCreateInfo& creationInfo)
{
	Resource*& handle = creationInfo.m_handle;

	if (handle) {
		handle->m_resource->AddRef();
		//handle->m_resource->
	}
	else {
		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.Width = (UINT64)creationInfo.m_dimensions.x;
		textureDesc.Height = (UINT64)creationInfo.m_dimensions.y;
		textureDesc.MipLevels = 1;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.Format = LocalToD3D12(creationInfo.m_format);
		textureDesc.Flags = LocalToD3D12(creationInfo.m_bindFlags);
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		CD3DX12_HEAP_PROPERTIES heapType(D3D12_HEAP_TYPE_DEFAULT);
		D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_COMMON;
		if (creationInfo.m_initialData) {
			initialResourceState = D3D12_RESOURCE_STATE_COPY_DEST;
		}
		handle = new Resource();
		handle->m_currentState = initialResourceState;

		TrackResource(handle);
		D3D12_CLEAR_VALUE clearValueRT = {};
		D3D12_CLEAR_VALUE clearValueDST = {};
		D3D12_CLEAR_VALUE* clearValue = NULL;

		// If it can be bound as RT then it needs the clear colour, otherwise it's null
		if (creationInfo.m_bindFlags & RESOURCE_BIND_RENDER_TARGET_BIT) {
			creationInfo.m_clearColour.GetAsFloats(clearValueRT.Color);
			clearValueRT.Format = LocalToD3D12(creationInfo.m_clearFormat);
			clearValue = &clearValueRT;
		}

		if (creationInfo.m_bindFlags & RESOURCE_BIND_DEPTH_STENCIL_BIT) {
			creationInfo.m_clearColour.GetAsFloats(clearValueDST.Color);
			clearValueDST.Format = LocalToD3D12(TextureFormat::D24_UNORM_S8_UINT);
			clearValue = &clearValueDST;
		}

		HRESULT textureCreateHR = m_device->CreateCommittedResource(
			&heapType,
			D3D12_HEAP_FLAG_NONE,
			&textureDesc,
			initialResourceState,
			clearValue,
			IID_PPV_ARGS(&handle->m_resource)
		);

		if (creationInfo.m_initialData) {
			ID3D12Resource* textureUploadHeap;
			UINT64  const uploadBufferSize = GetRequiredIntermediateSize(handle->m_resource, 0, 1);
			CD3DX12_RESOURCE_DESC uploadHeapDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
			CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);

			HRESULT createUploadHeap = m_device->CreateCommittedResource(
				&heapProperties,
				D3D12_HEAP_FLAG_NONE,
				&uploadHeapDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&textureUploadHeap));

			ThrowIfFailed(createUploadHeap, "FAILED TO CREATE TEXTURE UPLOAD HEAP");
			SetDebugName(textureUploadHeap, "UplHeap");

			D3D12_SUBRESOURCE_DATA imageData = {};
			imageData.pData = creationInfo.m_initialData;
			imageData.RowPitch = creationInfo.m_stride * creationInfo.m_dimensions.x;
			imageData.SlicePitch = creationInfo.m_stride * creationInfo.m_dimensions.y * creationInfo.m_dimensions.x;
			UpdateSubresources(m_commandList.Get(), handle->m_resource, textureUploadHeap, 0, 0, 1, &imageData);
			handle->TransitionTo(D3D12_RESOURCE_STATE_COMMON, m_commandList.Get());

			/*	m_frameUploadHeaps.push_back(textureUploadHeap);
				if (!m_isCommandListOpen) {
					m_uploadRequested = true;
				}*/
		}

		std::string const errorMsg = Stringf("COULD NOT CREATE TEXTURE WITH NAME %s", creationInfo.m_name.c_str());
		ThrowIfFailed(textureCreateHR, errorMsg.c_str());
	}
	//WaitForGPU();
	Texture* newTexture = new Texture(creationInfo);
	newTexture->m_handle = handle;
	SetDebugName(newTexture->m_handle->m_resource, creationInfo.m_name.c_str());

	m_createdTextures.push_back(newTexture);

	return newTexture;
}

void Renderer::DestroyTexture(Texture* textureToDestroy)
{
	if (textureToDestroy) {
		Resource* texResource = textureToDestroy->m_handle;
		delete textureToDestroy;
		if (texResource) {
			delete texResource;
		}
	}
}

void Renderer::Startup()
{

	EnableDebugLayer();
	CreateDevice();
	CreateCommandQueue();
	CreateSwapChain();
	CreateDescriptorHeaps();
	CreateFences(); // Has to go after creating command queue
	CreateDefaultRootSignature();
	CreateBackBuffers();


	TextureCreateInfo secRtvDesc = {};
	secRtvDesc.m_bindFlags = RESOURCE_BIND_RENDER_TARGET_BIT;
	secRtvDesc.m_clearColour = Rgba8::BLACK;
	secRtvDesc.m_clearFormat = TextureFormat::R32_FLOAT;
	secRtvDesc.m_dimensions = IntVec2(2560, 1280);
	secRtvDesc.m_format = TextureFormat::R32_FLOAT;
	secRtvDesc.m_initialData = nullptr;
	secRtvDesc.m_name = "FLOAT RT";
	secRtvDesc.m_owner = this;
	DescriptorHeap* rtvHeap = GetCPUDescriptorHeap(DescriptorHeapType::RTV);


	ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)), "FAILED TO CREATE COMMAND ALLOCATOR");


	// Create the pipeline state, which includes compiling and loading shaders.
	{
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
		// Enable better shader debugging with the graphics debugging tools.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif

		std::vector<unsigned char> vertexBytecode;
		std::vector<unsigned char> pixelBytecode;

		std::string shaderStr = ENGINE_DIR;
		shaderStr += "Renderer/Materials/Shaders/shaders.hlsl";

		ShaderLoadInfo shaderLoadInfo = {};
		shaderLoadInfo.m_shaderEntryPoint = "VSMain";
		shaderLoadInfo.m_shaderSrc = shaderStr;
		shaderLoadInfo.m_shaderName = "VertexShader";
		shaderLoadInfo.m_shaderType = ShaderType::Vertex;

		CompileShaderToByteCode(vertexBytecode, shaderLoadInfo);

		shaderLoadInfo.m_shaderEntryPoint = "PSMain";
		shaderLoadInfo.m_shaderName = "PixelShader";
		shaderLoadInfo.m_shaderType = ShaderType::Pixel;
		CompileShaderToByteCode(pixelBytecode, shaderLoadInfo);

		/*	ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"shaders.hlsl").c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr), "FAILED TO COMPILE VSHADER");
			ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"shaders.hlsl").c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr), "FAILED TO COMPILE PSHADER");*/

			// Define the vertex input layout.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = m_rootSignature.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexBytecode.data(), vertexBytecode.size());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelBytecode.data(), pixelBytecode.size());
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 2;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.RTVFormats[1] = DXGI_FORMAT_R32_FLOAT;
		psoDesc.SampleDesc.Count = 1;
		ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)), "FAILED TO CREATE PSO");
	}

	// Create the command list.
	ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList)), "FAILED TO CREATE COMMAND LIST");

	// Command lists are created in the recording state, but there is nothing
	// to record yet. The main loop expects it to be closed, so close it now.
	ThrowIfFailed(m_commandList->Close(), "FAILED TO CLOSE COMMAND LIST");

	// Create the vertex buffer.
	{
		// Define the geometry for a triangle.
		VertexTest triangleVertices[] =
		{
			{ { 0.0f, 0.25f * 2.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
			{ { 0.25f, -0.25f * 2.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
			{ { -0.25f, -0.25f * 2.0f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
		};

		const UINT vertexBufferSize = sizeof(triangleVertices);

		// Note: using upload heaps to transfer static data like vert buffers is not 
		// recommended. Every time the GPU needs it, the upload heap will be marshalled 
		// over. Please read up on Default Heap usage. An upload heap is used here for 
		// code simplicity and because there are very few verts to actually transfer.
		auto uploadHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
		ThrowIfFailed(m_device->CreateCommittedResource(
			&uploadHeap,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_vertexBuffer)), "FAILED TO CREATE VERTEX BUFFER");

		// Copy the triangle data to the vertex buffer.
		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
		ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)), "FAILED TO MAP VBUFFER");
		memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
		m_vertexBuffer->Unmap(0, nullptr);

		// Initialize the vertex buffer view.
		m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
		m_vertexBufferView.StrideInBytes = sizeof(VertexTest);
		m_vertexBufferView.SizeInBytes = vertexBufferSize;
	}

	for (UINT n = 0; n < 2; n++)
	{
		secRtvDesc.m_handle = nullptr;
		m_floatRenderTargets[n] = CreateTexture(secRtvDesc);
		m_floatRenderTargets[n]->GetResource()->TransitionTo(D3D12_RESOURCE_STATE_RENDER_TARGET, m_commandList.Get());

		/*m_device->CreateRenderTargetView(m_floatRenderTargets[n]->GetResource()->m_resource, nullptr, rtvHeap->GetNextCPUHandle());*/
		//rtvHandle.Offset(1, m_rtvDescriptorSize);

	}

	// Create synchronization objects and wait until assets have been uploaded to the GPU.
	{
		m_fence->Signal();
		m_fence->Wait();

		m_currentBackBuffer = m_swapChain->GetCurrentBackBufferIndex();
	}
}

void Renderer::CreatePSOForMaterial(Material* material)
{

}

bool Renderer::CompileShaderToByteCode(std::vector<unsigned char>& outByteCode, ShaderLoadInfo const& loadInfo)
{

	char const* sourceName = loadInfo.m_shaderSrc.c_str();
	char const* entryPoint = loadInfo.m_shaderEntryPoint.c_str();
	char const* target = Material::GetTargetForShader(loadInfo.m_shaderType);

	ComPtr<IDxcUtils> pUtils;
	ComPtr<IDxcCompiler3> pCompiler;
	DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&pUtils));
	DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&pCompiler));

	ComPtr<IDxcIncludeHandler> pIncludeHandler;
	pUtils->CreateDefaultIncludeHandler(&pIncludeHandler);


	std::filesystem::path filenamePath = std::filesystem::path(sourceName);
	std::string filenameStr = filenamePath.filename().string();
	std::string filenamePDB = filenamePath.filename().replace_extension(".pdb").string();
	std::wstring wFilename = std::wstring(filenameStr.begin(), filenameStr.end());
	std::wstring wEntryPoint = std::wstring(entryPoint, entryPoint + strlen(entryPoint));
	std::wstring wTarget = std::wstring(target, target + strlen(target));
	std::wstring wFilenamePDB = std::wstring(filenamePDB.begin(), filenamePDB.end());
	std::wstring wSrc = std::wstring(loadInfo.m_shaderSrc.begin(), loadInfo.m_shaderSrc.end());

	ComPtr<IDxcCompilerArgs> compilerArgs;
	LPCWSTR additionalArgs[]{
			DXC_ARG_ALL_RESOURCES_BOUND,
	};
	pUtils->BuildArguments(wSrc.c_str(), wEntryPoint.c_str(), wTarget.c_str(), NULL, 0, NULL, 0, &compilerArgs);
	compilerArgs->AddArguments(additionalArgs, _countof(additionalArgs));

	//LPCWSTR pszArgs[] =
	//{
	//	wFilename.c_str(),            // Optional shader source file name for error reporting
	//								 // and for PIX shader source view.  
	//	L"-E", wEntryPoint.c_str(),              // Entry point.
	//	L"-T", wTarget.c_str(),            // Target.
	//	L"-Zs",                      // Enable debug information (slim format)
	//	//L"-D", L"MYDEFINE=1",        // A single define.
	//	//L"-Fo", L"myshader.bin",     // Optional. Stored in the pdb. 
	//	L"-Fd", wFilenamePDB.c_str(),     // The file name of the pdb. This must either be supplied
	//								 // or the autogenerated file name must be used.
	//	L"-Qstrip_reflect",          // Strip reflection into a separate blob. 
	//};

#if defined(ENGINE_DEBUG_RENDER)
	LPCWSTR debugArg[]{
		DXC_ARG_SKIP_OPTIMIZATIONS,
		L"-Qembed_debug",
		DXC_ARG_DEBUG
	};
	compilerArgs->AddArguments(debugArg, _countof(debugArg));
#else 
	LPCWSTR optimizationArg[]{
			DXC_ARG_OPTIMIZATION_LEVEL3
	};
	compilerArgs->AddArguments(optimizationArg, 1);
#endif

	//LPCWSTR pszArgs[] =
	//{
	//	wFilename.c_str(),            // Optional shader source file name for error reporting
	//								 // and for PIX shader source view.  
	//	L"-E", wEntryPoint.c_str(),              // Entry point.
	//	L"-T", wTarget.c_str(),            // Target.
	//	L"-Zs",                      // Enable debug information (slim format)
	//	//L"-D", L"MYDEFINE=1",        // A single define.
	//	//L"-Fo", L"myshader.bin",     // Optional. Stored in the pdb. 
	//	L"-Fd", wFilenamePDB.c_str(),     // The file name of the pdb. This must either be supplied
	//								 // or the autogenerated file name must be used.
	//	L"-Qstrip_reflect",          // Strip reflection into a separate blob. 
	//};

	DxcBuffer bufferSource = {};
	ComPtr<IDxcBlobEncoding> pSource = nullptr;
	pUtils->LoadFile(wSrc.c_str(), nullptr, &pSource);
	bufferSource.Ptr = pSource->GetBufferPointer();
	bufferSource.Size = pSource->GetBufferSize();
	BOOL encodingKnown = TRUE;
	pSource->GetEncoding(&encodingKnown, &bufferSource.Encoding); // Assume BOM says UTF8 or UTF16 or this is ANSI text.

	ComPtr<IDxcResult> pResults;
	auto stringCompilerArgs = compilerArgs->GetArguments();
	auto optionsCount = compilerArgs->GetCount();
	pCompiler->Compile(&bufferSource, stringCompilerArgs, optionsCount, pIncludeHandler.Get(), IID_PPV_ARGS(&pResults));


	ComPtr<IDxcBlobUtf8> pErrors = nullptr;
	pResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr);
	// Note that d3dcompiler would return null if no errors or warnings are present.
	// IDxcCompiler3::Compile will always return an error buffer, but its length
	// will be zero if there are no warnings or errors.

	HRESULT hrStatus;
	pResults->GetStatus(&hrStatus);

	ThrowIfFailed(hrStatus, Stringf("COULD NOT COMPILE SHADER FILE: %s", pErrors->GetStringPointer()).c_str());
	ComPtr<IDxcBlob> pShader = nullptr;
	ComPtr<IDxcBlobUtf16> pShaderName = nullptr;
	pResults->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pShader), &pShaderName);

	outByteCode.resize(pShader->GetBufferSize());
	memcpy(outByteCode.data(), pShader->GetBufferPointer(), pShader->GetBufferSize());

	return true;
}

void Renderer::BeginFrame()
{
	Vertex_PCU triangleVertices[] =
	{
		Vertex_PCU(Vec3(-0.25f, -0.25f, 0.0f), Rgba8(255, 255, 255, 255), Vec2(0.0f, 0.0f)),
		Vertex_PCU(Vec3(0.25f, -0.25f, 0.0f), Rgba8(255, 255, 255, 255), Vec2(1.0f, 0.0f)),
		Vertex_PCU(Vec3(0.0f, 0.25f, 0.0f), Rgba8(255, 255, 255, 255), Vec2(0.5f, 1.0f))
	};

	// Command list allocators can only be reset when the associated 
   // command lists have finished execution on the GPU; apps should use 
   // fences to determine GPU execution progress.
	ThrowIfFailed(m_commandAllocator->Reset(), "FAILED TO RESET COMMMAND ALLOCATOR");

	// However, when ExecuteCommandList() is called on a particular command 
	// list, that command list can then be reset at any time and must be before 
	// re-recording.
	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()), "FAILED TO RESET COMMAND LIST");

	// Set necessary state.
	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	Texture* currentBackBuffer = m_backBuffers[m_currentBackBuffer];
	Resource* backBuffer = currentBackBuffer->GetResource();
	backBuffer->TransitionTo(D3D12_RESOURCE_STATE_RENDER_TARGET, m_commandList.Get());




	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_backBuffers[m_currentBackBuffer]->GetOrCreateView(RESOURCE_BIND_RENDER_TARGET_BIT)->GetHandle();
	D3D12_CPU_DESCRIPTOR_HANDLE secrtvHandle = m_floatRenderTargets[m_currentBackBuffer]->GetOrCreateView(RESOURCE_BIND_RENDER_TARGET_BIT)->GetHandle();

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[] = { rtvHandle, secrtvHandle };
	m_commandList->OMSetRenderTargets(2, rtvHandles, FALSE, nullptr);

	// Record commands.
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	const float secClearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_commandList->ClearRenderTargetView(secrtvHandle, secClearColor, 0, nullptr);
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	m_commandList->DrawInstanced(3, 1, 0, 0);

	backBuffer->TransitionTo(D3D12_RESOURCE_STATE_PRESENT, m_commandList.Get());

	ThrowIfFailed(m_commandList->Close(), "FAILED TO CLOSE COMMAND LIST");
}

void Renderer::EndFrame()
{
	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Present the frame.
	ThrowIfFailed(m_swapChain->Present(1, 0), "FAILED TO PRESENT");
	m_fence->Signal();
	m_fence->Wait();

	m_currentBackBuffer = m_swapChain->GetCurrentBackBufferIndex();
}

void Renderer::Shutdown()
{
	for (unsigned int textureInd = 0; textureInd < m_createdTextures.size(); textureInd++) {
		Texture* tex = m_createdTextures[textureInd];
		DestroyTexture(tex);
	}

	for (unsigned int heapIndex = 0; heapIndex < (unsigned int)DescriptorHeapType::NUM_DESCRIPTOR_HEAPS; heapIndex++) {
		DescriptorHeap*& cpuHeap = m_CPUDescriptorHeaps[heapIndex];
		if (cpuHeap) {
			delete cpuHeap;
			cpuHeap = nullptr;
		}

		if (heapIndex >= (unsigned int)DescriptorHeapType::MAX_GPU_VISIBLE) continue;

		DescriptorHeap*& gpuHeap = m_GPUDescriptorHeaps[heapIndex];

		if (gpuHeap) {
			delete gpuHeap;
			gpuHeap = nullptr;
		}
	}

	if (m_fence) {
		delete m_fence;
		m_fence = nullptr;
	}


	m_device.Reset();
	m_DXGIFactory.Reset();
	m_commandQueue.Reset();
	m_swapChain.Reset();

	m_commandAllocator.Reset();

	m_rootSignature.Reset();
	m_pipelineState.Reset();

	// App resources.
	m_vertexBuffer.Reset();


}

void Renderer::BeginCamera(Camera const& camera)
{

}

void Renderer::EndCamera(Camera const& camera)
{

}

void Renderer::ClearScreen(Rgba8 const& color)
{

}

void Renderer::ClearRenderTarget(unsigned int slot, Rgba8 const& color)
{

}

void Renderer::ClearDepth(float clearDepth /*= 1.0f*/)
{

}

Material* Renderer::CreateOrGetMaterial(std::filesystem::path materialPathNoExt)
{
	return nullptr;
}

Texture* Renderer::CreateOrGetTextureFromFile(char const* imageFilePath)
{
	return nullptr;
}


void Renderer::DispatchMesh(unsigned int threadX, unsigned threadY, unsigned int threadZ)
{

}

void Renderer::DrawVertexArray(unsigned int numVertexes, const Vertex_PCU* vertexes)
{

}

void Renderer::DrawVertexArray(std::vector<Vertex_PCU> const& vertexes)
{

}

void Renderer::DrawVertexArray(unsigned int numVertexes, const Vertex_PNCU* vertexes)
{

}

void Renderer::DrawIndexedVertexArray(unsigned int numVertexes, const Vertex_PCU* vertexes, unsigned int numIndices, unsigned int const* indices)
{

}

void Renderer::DrawIndexedVertexArray(std::vector<Vertex_PCU> const& vertexes, std::vector<unsigned int> const& indices)
{

}

void Renderer::DrawIndexedVertexArray(unsigned int numVertexes, const Vertex_PNCU* vertexes, unsigned int numIndices, unsigned int const* indices)
{

}

void Renderer::BindDepthAsTexture(Texture* depthTarget /*= nullptr*/, unsigned int slot /*= 0*/)
{

}

void Renderer::SetModelMatrix(Mat44 const& modelMat)
{

}

void Renderer::SetModelColor(Rgba8 const& modelColor)
{

}

void Renderer::ExecuteCommandLists(ID3D12CommandList** commandLists, unsigned int count)
{

}

void Renderer::WaitForGPU()
{

}

DescriptorHeap* Renderer::GetGPUDescriptorHeap(DescriptorHeapType descriptorHeapType) const
{
	return m_GPUDescriptorHeaps[(unsigned int)descriptorHeapType];
}

DescriptorHeap* Renderer::GetCPUDescriptorHeap(DescriptorHeapType descriptorHeapType) const
{
	return m_CPUDescriptorHeaps[(unsigned int)descriptorHeapType];
}

ResourceView* Renderer::CreateResourceView(ResourceViewInfo const& resourceViewInfo) const
{
	switch (resourceViewInfo.m_viewType)
	{
	default:
		ERROR_AND_DIE("UNRECOGNIZED VIEW TYPE");
		break;
	case RESOURCE_BIND_SHADER_RESOURCE_BIT:
		return CreateShaderResourceView(resourceViewInfo);
	case RESOURCE_BIND_RENDER_TARGET_BIT:
		return CreateRenderTargetView(resourceViewInfo);
	case RESOURCE_BIND_DEPTH_STENCIL_BIT:
		return CreateDepthStencilView(resourceViewInfo);
	case RESOURCE_BIND_CONSTANT_BUFFER_VIEW_BIT:
		return CreateConstantBufferView(resourceViewInfo);
	case RESOURCE_BIND_UNORDERED_ACCESS_VIEW_BIT:
		// TODO
		break;
	}

	return nullptr;
}

BitmapFont* Renderer::CreateOrGetBitmapFont(std::filesystem::path bitmapPath)
{
	return nullptr;

}

Material* Renderer::GetMaterialForName(char const* materialName)
{
	return nullptr;

}

Material* Renderer::GetMaterialForPath(std::filesystem::path const& materialPath)
{
	return nullptr;

}

Material* Renderer::GetDefaultMaterial() const
{
	return nullptr;

}

Material* Renderer::GetDefault2DMaterial() const
{
	return nullptr;

}

Material* Renderer::GetDefault3DMaterial() const
{
	return nullptr;

}

void Renderer::BindConstantBuffer(ConstantBuffer* cBuffer, unsigned int slot /*= 0*/)
{

}

void Renderer::BindTexture(Texture const* texture, unsigned int slot /*= 0*/)
{

}

void Renderer::BindMaterial(Material* mat)
{

}

void Renderer::BindMaterialByName(char const* materialName)
{

}

void Renderer::BindMaterialByPath(std::filesystem::path materialPath)
{

}

void Renderer::BindVertexBuffer(VertexBuffer* const& vertexBuffer)
{

}

void Renderer::BindIndexBuffer(IndexBuffer* const& indexBuffer, size_t indexCount)
{

}

void Renderer::BindStructuredBuffer(Buffer* const& buffer, unsigned int slot)
{

}

void Renderer::SetRenderTarget(Texture* dst, unsigned int slot /*= 0*/)
{

}

void Renderer::SetMaterialPSO(Material* mat)
{

}

void Renderer::SetBlendMode(BlendMode newBlendMode)
{

}

void Renderer::SetCullMode(CullMode newCullMode)
{

}

void Renderer::SetFillMode(FillMode newFillMode)
{

}

void Renderer::SetWindingOrder(WindingOrder newWindingOrder)
{

}

void Renderer::SetRasterizerState(CullMode cullMode, FillMode fillMode, WindingOrder windingOrder)
{

}

void Renderer::SetDepthFunction(DepthFunc newDepthFunc)
{

}

void Renderer::SetWriteDepth(bool writeDepth)
{
}

void Renderer::SetDepthStencilState(DepthFunc newDepthFunc, bool writeDepth)
{

}

void Renderer::SetTopology(TopologyType newTopologyType)
{

}

void Renderer::SetDirectionalLight(Vec3 const& direction)
{

}

void Renderer::SetDirectionalLightIntensity(Rgba8 const& intensity)
{

}

void Renderer::SetAmbientIntensity(Rgba8 const& intensity)
{

}

bool Renderer::SetLight(Light const& light, int index)
{
	return false;
}

void Renderer::BindLightConstants()
{

}

void Renderer::DrawVertexBuffer(VertexBuffer* const& vertexBuffer)
{

}

void Renderer::DrawIndexedVertexBuffer(VertexBuffer* const& vertexBuffer, IndexBuffer* const& indexBuffer, size_t indexCount)
{

}

void Renderer::SetDebugName(ID3D12Object* object, char const* name)
{
#if defined(ENGINE_DEBUG_RENDER)
	object->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen(name), name);
#else
	UNUSED(object);
	UNUSED(name);
#endif
}

void Renderer::SetSamplerMode(SamplerMode samplerMode)
{

}

void Renderer::AddToUpdateQueue(Buffer* bufferToUpdate)
{

}

Texture* Renderer::GetCurrentRenderTarget() const
{
	return nullptr;

}

Texture* Renderer::GetCurrentDepthTarget() const
{
	return nullptr;

}

void Renderer::ApplyEffect(Material* effect, Camera const* camera /*= nullptr*/, Texture* customDepth /*= nullptr*/)
{

}

void Renderer::CopyTextureWithMaterial(Texture* dst, Texture* src, Texture* depthBuffer, Material* effect, CameraConstants const& cameraConstants /*= CameraConstants()*/)
{

}

void Renderer::TrackResource(Resource* newResource)
{

}

void Renderer::RemoveResource(Resource* newResource)
{

}

void Renderer::SignalFence(ComPtr<ID3D12Fence1>& fence, unsigned int fenceValue)
{

}

ID3D12CommandAllocator* Renderer::GetCommandAllocForCmdList(CommandListType cmdListType)
{
	return nullptr;

}

void Renderer::FlushPendingWork()
{

}

void Renderer::ResetGPUState()
{

}

void Renderer::InitializeImGui()
{
#if defined(ENGINE_USE_IMGUI)

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	Window* window = Window::GetWindowContext();


	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = 1;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		if (m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_ImGuiSrvDescHeap)) != S_OK)
			ERROR_AND_DIE("COULD NOT CREATE IMGUI SRV HEAP");

	}

	HWND windowHandle = (HWND)window->m_osWindowHandle;
	ImGui_ImplWin32_Init(windowHandle);
	ImGui_ImplDX12_Init(m_device.Get(), m_config.m_backBuffersCount,
		DXGI_FORMAT_R8G8B8A8_UNORM, m_ImGuiSrvDescHeap,
		m_ImGuiSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
		m_ImGuiSrvDescHeap->GetGPUDescriptorHandleForHeapStart());
#endif
}

void Renderer::ShutdownImGui()
{
#if defined(ENGINE_USE_IMGUI)

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	DX_SAFE_RELEASE(m_ImGuiSrvDescHeap);
#endif
}

void Renderer::BeginFrameImGui()
{
#if defined(ENGINE_USE_IMGUI)
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	//ImGui::ShowDemoWindow();
#endif
}

void Renderer::EndFrameImGui()
{
#if defined(ENGINE_USE_IMGUI)
	Texture* currentRt = GetActiveRenderTarget();
	D3D12_CPU_DESCRIPTOR_HANDLE currentRTVHandle = currentRt->GetOrCreateView(RESOURCE_BIND_RENDER_TARGET_BIT)->GetHandle();

	ImGui::Render();
	m_commandList->OMSetRenderTargets(1, &currentRTVHandle, FALSE, nullptr);
	m_commandList->SetDescriptorHeaps(1, &m_ImGuiSrvDescHeap);

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_commandList.Get());
#endif
}




