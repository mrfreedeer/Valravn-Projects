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
#include <dxcapi.h>
#include <d3dx12.h> // Notice the X. These are the helper structures not the DX12 header
#include <regex>
#include <ThirdParty/ImGUI/imgui.h>
#include <ThirdParty/ImGUI/imgui_impl_win32.h>
#include <ThirdParty/ImGUI/imgui_impl_dx12.h>
#include <DirectXMath.h>
using namespace DirectX;

#pragma message("ENGINE_DIR == " ENGINE_DIR)
#pragma comment( lib, "dxgi.lib" )        // directx graphics interface
#pragma comment( lib, "dxguid.lib" )

MaterialSystem* g_theMaterialSystem = nullptr;

constexpr int g_cameraBufferSlot = 0;
constexpr int g_modelBufferSlot = 1;
constexpr int g_lightBufferSlot = 2;
constexpr int g_engineBufferCount = 3;

struct LightConstants {
	Vec3 DirectionalLight = Vec3::ZERO;
	float PaddingDirectionalLight = 0.0f;
	float DirectionalLightIntensity[4];
	float AmbientIntensity[4];
	Light Lights[MAX_LIGHTS];
	//------------- 16 bytes SSAO Related variables
	float MaxOcclusionPerSample = 0.00000175f;
	float SSAOFalloff = 0.00000001f;
	float SampleRadius = 0.00001f;
	int SampleSize = 64;
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
	m_scissorRect = CD3DX12_RECT(0, 0, LONG(windowDims.x), LONG(windowDims.y));

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

	// This sample does not support full screen transitions.
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
	m_resourcesFence = new Fence(m_device.Get(), m_commandQueue.Get());
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
	ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_defaultRootSignature)), "COULD NOT CREATE ROOT SIGNATURE");
	SetDebugName(m_defaultRootSignature, "DEFAULTROOTSIGNATURE");

	rootSignatureSerialization = D3D12SerializeVersionedRootSignature(&MSRootSignature, MSsignature.GetAddressOf(), error.GetAddressOf());
	ThrowIfFailed(rootSignatureSerialization, "COULD NOT SERIALIZE ROOT SIGNATURE");
	//ThrowIfFailed(m_device->CreateRootSignature(0, MSsignature->GetBufferPointer(), MSsignature->GetBufferSize(), IID_PPV_ARGS(&m_MSRootSignature)), "COULD NOT CREATE ROOT SIGNATURE");
	//SetDebugName(m_MSRootSignature, "DEFAULTROOTSIGNATURE");
}

void Renderer::CreateBackBuffers()
{
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
		backBufferTexInfo.m_handle = new Resource(m_device.Get());
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
	ID3D12GraphicsCommandList6* cmdList = GetCurrentCommandList(CommandListType::RESOURCES);
	Resource*& handle = creationInfo.m_handle;
	ID3D12Resource2* textureUploadHeap = nullptr;

	if (handle) {
		handle->m_resource->AddRef();
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

		handle = new Resource(m_device.Get());
		handle->m_currentState = initialResourceState;

		D3D12_CLEAR_VALUE clearValueTex = {};
		D3D12_CLEAR_VALUE* clearValue = NULL;

		// If it can be bound as RT then it needs the clear colour, otherwise it's null
		if (creationInfo.m_bindFlags & RESOURCE_BIND_RENDER_TARGET_BIT) {
			creationInfo.m_clearColour.GetAsFloats(clearValueTex.Color);
			clearValueTex.Format = LocalToD3D12(creationInfo.m_clearFormat);
			clearValue = &clearValueTex;
		}
		else if (creationInfo.m_bindFlags & RESOURCE_BIND_DEPTH_STENCIL_BIT) {
			creationInfo.m_clearColour.GetAsFloats(clearValueTex.Color);
			clearValueTex.Format = LocalToD3D12(creationInfo.m_clearFormat);
			clearValue = &clearValueTex;
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
			SetDebugName(textureUploadHeap, "Texture Upload Heap");

			D3D12_SUBRESOURCE_DATA imageData = {};
			imageData.pData = creationInfo.m_initialData;
			imageData.RowPitch = creationInfo.m_stride * creationInfo.m_dimensions.x;
			imageData.SlicePitch = creationInfo.m_stride * creationInfo.m_dimensions.y * creationInfo.m_dimensions.x;

			UpdateSubresources(cmdList, handle->m_resource, textureUploadHeap, 0, 0, 1, &imageData);
		}

		std::string const errorMsg = Stringf("COULD NOT CREATE TEXTURE WITH NAME %s", creationInfo.m_name.c_str());
		ThrowIfFailed(textureCreateHR, errorMsg.c_str());
		handle->m_currentState = D3D12_RESOURCE_STATE_COPY_DEST;
	}
	Texture* newTexture = new Texture(creationInfo);
	if (textureUploadHeap) {
		newTexture->m_uploadRsc = new Resource(m_device.Get());
		newTexture->m_uploadRsc->m_resource = textureUploadHeap;
	}
	newTexture->m_handle = handle;
	SetDebugName(newTexture->m_handle->m_resource, creationInfo.m_name.c_str());

	m_loadedTextures.push_back(newTexture);

	return newTexture;
}

void Renderer::DestroyTexture(Texture* textureToDestroy)
{
	if (textureToDestroy) {
		Resource* texResource = textureToDestroy->m_handle;
		Resource* uploadResource = textureToDestroy->m_uploadRsc;

		delete textureToDestroy;
		if (texResource) {
			delete texResource;
		}

		if (uploadResource) {
			delete uploadResource;
		}
	}
}

/// <summary>
/// These are the default textures to render to.
/// They're separate textures than the back buffers,
/// as post processing effects require processing the 
/// image first then copying into the back buffers
/// </summary>
void Renderer::CreateDefaultTextureTargets()
{
	IntVec2 texDimensions = Window::GetWindowContext()->GetClientDimensions();
	TextureCreateInfo defaultRTInfo = {};
	defaultRTInfo.m_bindFlags = ResourceBindFlagBit::RESOURCE_BIND_RENDER_TARGET_BIT | ResourceBindFlagBit::RESOURCE_BIND_SHADER_RESOURCE_BIT;
	defaultRTInfo.m_dimensions = texDimensions;
	defaultRTInfo.m_format = TextureFormat::R8G8B8A8_UNORM;
	defaultRTInfo.m_owner = this;
	defaultRTInfo.m_clearColour = Rgba8(0, 0, 0, 255);

	for (unsigned int rtTargetInd = 0; rtTargetInd < m_config.m_backBuffersCount; rtTargetInd++) {
		defaultRTInfo.m_name = Stringf("DefaultRenderTarget %d", rtTargetInd);
		defaultRTInfo.m_handle = nullptr;
		m_defaultRenderTargets.push_back(CreateTexture(defaultRTInfo));
	}

	TextureCreateInfo defaultDSTInfo = {};
	defaultDSTInfo.m_bindFlags = ResourceBindFlagBit::RESOURCE_BIND_DEPTH_STENCIL_BIT | ResourceBindFlagBit::RESOURCE_BIND_SHADER_RESOURCE_BIT;
	defaultDSTInfo.m_dimensions = texDimensions;
	defaultDSTInfo.m_format = TextureFormat::R24G8_TYPELESS;
	defaultDSTInfo.m_clearFormat = TextureFormat::D24_UNORM_S8_UINT;
	defaultDSTInfo.m_name = "DefaultDepthTarget";
	defaultDSTInfo.m_owner = this;
	defaultDSTInfo.m_clearColour = Rgba8(255, 255, 255, 255);

	m_defaultDepthTarget = CreateTexture(defaultDSTInfo);
}

Texture* Renderer::GetTextureForFileName(char const* imageFilePath)
{
	Texture* textureToGet = nullptr;

	for (Texture*& loadedTexture : m_loadedTextures) {
		if (loadedTexture->GetImageFilePath() == imageFilePath) {
			return loadedTexture;
		}
	}
	return textureToGet;
}

Texture* Renderer::CreateTextureFromImage(Image const& image)
{
	TextureCreateInfo ci{};
	ci.m_owner = this;
	ci.m_name = image.GetImageFilePath();
	ci.m_dimensions = image.GetDimensions();
	ci.m_initialData = image.GetRawData();
	ci.m_stride = sizeof(Rgba8);

	Texture* newTexture = CreateTexture(ci);
	SetDebugName(newTexture->GetResource()->m_resource, newTexture->m_name.c_str());

	return newTexture;
}

Texture* Renderer::CreateTextureFromFile(char const* imageFilePath)
{
	Image loadedImage(imageFilePath);
	Texture* newTexture = CreateTextureFromImage(loadedImage);

	return newTexture;
}

Texture* Renderer::GetActiveRenderTarget() const
{
	return m_defaultRenderTargets[m_currentRenderTarget];
}

Texture* Renderer::GetBackUpRenderTarget() const
{
	int otherInd = (m_currentRenderTarget + 1) % 2;
	return m_defaultRenderTargets[otherInd];
}

Texture* Renderer::GetActiveBackBuffer() const
{
	return m_backBuffers[m_currentBackBuffer];
}

Texture* Renderer::GetBackUpBackBuffer() const
{
	int otherInd = (m_currentRenderTarget + 1) % 2;
	return m_backBuffers[otherInd];
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
	InitializeCBufferArrays();

	size_t totalCmdList = size_t(m_config.m_backBuffersCount * (size_t)CommandListType::NUM_COMMAND_LIST_TYPES);
	m_commandAllocators.resize(totalCmdList);
	m_commandLists.resize(totalCmdList);

	for (unsigned int frameIndex = 0; frameIndex < m_config.m_backBuffersCount; frameIndex++) {
		for (unsigned int cmdListType = 0; cmdListType < (unsigned int)CommandListType::NUM_COMMAND_LIST_TYPES; cmdListType++) {
			size_t accessIndex = (((size_t)frameIndex * 2) + (size_t)cmdListType);
			ID3D12CommandAllocator*& cmdAllocator = m_commandAllocators[accessIndex];
			ID3D12GraphicsCommandList6*& cmdList = m_commandLists[accessIndex];

			ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator)), "FAILED TO CREATE COMMAND ALLOCATOR");
			ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocator, nullptr, IID_PPV_ARGS(&cmdList)), "FAILED TO CREATE COMMAND LIST");

			cmdList->Close();
		}
	}

	CreateBackBuffers();
	CreateDefaultTextureTargets();
	ResetGPUState();


	m_defaultTexture = new Texture();
	Image whiteTexelImg(IntVec2(1, 1), Rgba8::WHITE);
	whiteTexelImg.m_imageFilePath = "DefaultTexture";
	m_defaultTexture = CreateTextureFromImage(whiteTexelImg);

	m_immediateContexts = new ImmediateContext[m_config.m_immediateCtxCount];

	MaterialSystemConfig matSystemConfig = {
	this // Renderer
	};
	g_theMaterialSystem = new MaterialSystem(matSystemConfig);
	g_theMaterialSystem->Startup();

	m_default3DMaterial = GetMaterialForName("Default3DMaterial");
	m_default2DMaterial = GetMaterialForName("Default2DMaterial");
	m_currentBackBuffer = m_swapChain->GetCurrentBackBufferIndex();

	DebugRenderConfig debugSystemConfig;
	debugSystemConfig.m_renderer = this;
	debugSystemConfig.m_startHidden = false;
	debugSystemConfig.m_fontName = "Data/Images/SquirrelFixedFont";

	DebugRenderSystemStartup(debugSystemConfig);
	InitializeImGui();
	CreateDefaultBufferObjects();

}

void Renderer::CreatePSOForMaterial(Material* material)
{
	MaterialConfig& matConfig = material->m_config;
	std::string baseName = material->GetName();
	std::string debugName;
	std::string shaderSource;


	for (ShaderLoadInfo& loadInfo : matConfig.m_shaders) {
		if (loadInfo.m_shaderSrc.empty()) continue;

		ShaderByteCode* shaderByteCode = CompileOrGetShaderBytes(loadInfo);
		material->m_byteCodes[loadInfo.m_shaderType] = shaderByteCode;
	}
	HRESULT psoCreation = {};
	std::vector<std::string> nameStrings;

	if (material->IsMeshShader()) {
		CreateMeshShaderPSO(material);
	}
	else {
		CreateGraphicsPSO(material);
	}

	ThrowIfFailed(psoCreation, "COULD NOT CREATE PSO");

	std::string shaderDebugName = "PSO:";
	shaderDebugName += baseName;
	SetDebugName(material->m_PSO, shaderDebugName.c_str());
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

void Renderer::CreateInputLayoutFromVS(std::vector<uint8_t>& shaderByteCode, std::vector<D3D12_SIGNATURE_PARAMETER_DESC>& elementsDescs, std::vector<std::string>& semanticNames)
{
	ComPtr<IDxcUtils> pUtils;
	DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&pUtils));

	ComPtr<IDxcBlobEncoding> sourceBlob{};
	ComPtr<IDxcBlob> pShader;
	HRESULT blobCreation = pUtils->CreateBlob(shaderByteCode.data(), (UINT32)shaderByteCode.size(), DXC_CP_ACP, &sourceBlob);
	ThrowIfFailed(blobCreation, "COULD NOT CREATE BLOB FOR SHADER REFLECTION");
	ComPtr<IDxcResult> results = {};

	DxcBuffer bufferSource = {};
	bufferSource.Ptr = sourceBlob->GetBufferPointer();
	bufferSource.Size = sourceBlob->GetBufferSize();
	bufferSource.Encoding = DXC_CP_ACP; // Assume BOM says UTF8 or UTF16 or this is ANSI text.

	ComPtr<IDxcResult> pResults;
	ID3D12ShaderReflection* pShaderReflection = NULL;
	HRESULT reflectOp = pUtils->CreateReflection(&bufferSource, IID_PPV_ARGS(&pShaderReflection));
	ThrowIfFailed(reflectOp, "FAILED TO GET REFLECTION OUT OF SHADER");

	//// Get shader info
	D3D12_SHADER_DESC shaderDesc;
	pShaderReflection->GetDesc(&shaderDesc);


	// Read input layout description from shader info
	for (UINT i = 0; i < shaderDesc.InputParameters; i++)
	{
		D3D12_SIGNATURE_PARAMETER_DESC paramDesc;
		pShaderReflection->GetInputParameterDesc(i, &paramDesc);
		semanticNames.push_back(std::string(paramDesc.SemanticName));
		//save element desc
		//
		elementsDescs.emplace_back(D3D12_SIGNATURE_PARAMETER_DESC{
		paramDesc.SemanticName,			// Name of the semantic
		paramDesc.SemanticIndex,		// Index of the semantic
		paramDesc.Register,				// Number of member variables
		paramDesc.SystemValueType,		// A predefined system value, or D3D_NAME_UNDEFINED if not applicable
		paramDesc.ComponentType,		// Scalar type (e.g. uint, float, etc.)
		paramDesc.Mask,					// Mask to indicate which components of the register
										// are used (combination of D3D10_COMPONENT_MASK values)
		paramDesc.ReadWriteMask,		// Mask to indicate whether a given component is 
										// never written (if this is an output signature) or
										// always read (if this is an input signature).
										// (combination of D3D_MASK_* values)
		paramDesc.Stream,			// Stream index
		paramDesc.MinPrecision			// Minimum desired interpolation precision
			});
	}
	DX_SAFE_RELEASE(pShaderReflection);
}

ShaderByteCode* Renderer::CompileOrGetShaderBytes(ShaderLoadInfo const& shaderLoadInfo)
{
	ShaderByteCode* retByteCode = GetByteCodeForShaderSrc(shaderLoadInfo);
	//ShaderByteCode* retByteCode = nullptr;

	if (retByteCode) return retByteCode;

	retByteCode = new ShaderByteCode();
	retByteCode->m_src = shaderLoadInfo.m_shaderSrc;
	retByteCode->m_shaderType = shaderLoadInfo.m_shaderType;
	retByteCode->m_shaderName = shaderLoadInfo.m_shaderName;

	//std::string shaderSource;
	//FileReadToString(shaderSource, shaderLoadInfo.m_shaderSrc);
	CompileShaderToByteCode(retByteCode->m_byteCode, shaderLoadInfo);

	m_shaderByteCodes.push_back(retByteCode);
	return retByteCode;
}

ShaderByteCode* Renderer::GetByteCodeForShaderSrc(ShaderLoadInfo const& shaderLoadInfo)
{
	for (ShaderByteCode*& byteCode : m_shaderByteCodes) {
		if (AreStringsEqualCaseInsensitive(shaderLoadInfo.m_shaderName, byteCode->m_shaderName)) {
			return byteCode;
		}
	}

	return nullptr;
}

void Renderer::CreateGraphicsPSO(Material* material)
{
	std::vector<D3D12_SIGNATURE_PARAMETER_DESC> reflectInputDesc;

	ShaderByteCode* vsByteCode = material->m_byteCodes[ShaderType::Vertex];
	std::vector<std::string> nameStrings;
	CreateInputLayoutFromVS(vsByteCode->m_byteCode, reflectInputDesc, nameStrings);

	// Const Char* copying from D3D12_SIGNATURE_PARAMETER_DESC is quite problematic
	std::vector<D3D12_INPUT_ELEMENT_DESC>& inputLayoutDesc = material->m_inputLayout;
	inputLayoutDesc.resize(reflectInputDesc.size());

	for (int inputIndex = 0; inputIndex < reflectInputDesc.size(); inputIndex++) {
		D3D12_INPUT_ELEMENT_DESC& elementDesc = inputLayoutDesc[inputIndex];
		D3D12_SIGNATURE_PARAMETER_DESC& paramDesc = reflectInputDesc[inputIndex];
		std::string& currentString = nameStrings[inputIndex];

		paramDesc.SemanticName = currentString.c_str();
		elementDesc.Format = GetFormatForComponent(paramDesc.ComponentType, currentString.c_str(), paramDesc.Mask);
		elementDesc.SemanticName = currentString.c_str();
		elementDesc.SemanticIndex = paramDesc.SemanticIndex;
		elementDesc.InputSlot = 0;
		elementDesc.AlignedByteOffset = (inputIndex == 0) ? 0 : D3D12_APPEND_ALIGNED_ELEMENT;
		elementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		elementDesc.InstanceDataStepRate = 0;
	}

	MaterialConfig const& matConfig = material->m_config;

	D3D12_RASTERIZER_DESC rasterizerDesc = CD3DX12_RASTERIZER_DESC(
		LocalToD3D12(matConfig.m_fillMode),			// Fill mode
		LocalToD3D12(matConfig.m_cullMode),			// Cull mode
		LocalToD3D12(matConfig.m_windingOrder),		// Winding order
		D3D12_DEFAULT_DEPTH_BIAS,					// Depth bias
		D3D12_DEFAULT_DEPTH_BIAS_CLAMP,				// Bias clamp
		D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,		// Slope scaled bias
		TRUE,										// Depth Clip enable
		FALSE,										// Multi sample (MSAA)
		FALSE,										// Anti aliased line enable
		0,											// Force sample count
		D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF	// Conservative Rasterization
	);

	D3D12_BLEND_DESC blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	SetBlendModeSpecs(matConfig.m_blendMode, blendDesc);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	ShaderByteCode* psByteCode = material->m_byteCodes[ShaderType::Pixel];
	ShaderByteCode* gsByteCode = material->m_byteCodes[ShaderType::Geometry];
	ShaderByteCode* hsByteCode = material->m_byteCodes[ShaderType::Hull];
	ShaderByteCode* dsByteCode = material->m_byteCodes[ShaderType::Domain];

	psoDesc.InputLayout.NumElements = (UINT)reflectInputDesc.size();
	psoDesc.InputLayout.pInputElementDescs = inputLayoutDesc.data();
	psoDesc.pRootSignature = m_defaultRootSignature.Get();
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(psByteCode->m_byteCode.data(), psByteCode->m_byteCode.size());

	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vsByteCode->m_byteCode.data(), vsByteCode->m_byteCode.size());
	if (gsByteCode) {
		psoDesc.GS = CD3DX12_SHADER_BYTECODE(gsByteCode->m_byteCode.data(), gsByteCode->m_byteCode.size());
	}

	if (hsByteCode) {
		psoDesc.HS = CD3DX12_SHADER_BYTECODE(hsByteCode->m_byteCode.data(), hsByteCode->m_byteCode.size());
	}

	if (dsByteCode) {
		psoDesc.DS = CD3DX12_SHADER_BYTECODE(dsByteCode->m_byteCode.data(), dsByteCode->m_byteCode.size());
	}

	psoDesc.RasterizerState = rasterizerDesc;
	psoDesc.BlendState = blendDesc;
	psoDesc.DepthStencilState.DepthEnable = matConfig.m_depthEnable;
	psoDesc.DepthStencilState.DepthFunc = LocalToD3D12(matConfig.m_depthFunc);
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	psoDesc.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp = // a stencil operation structure, does not really matter since stencil testing is turned off
	{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
	psoDesc.DepthStencilState.FrontFace = defaultStencilOp; // both front and back facing polygons get the same treatment
	psoDesc.DepthStencilState.BackFace = defaultStencilOp;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE(matConfig.m_topology);
	psoDesc.NumRenderTargets = (UINT)matConfig.m_numRenderTargets;

	for (unsigned int rtIndex = 0; rtIndex < 8; rtIndex++) {
		TextureFormat const& format = matConfig.m_renderTargetFormats[rtIndex];
		if (format == TextureFormat::INVALID) continue;
		psoDesc.RTVFormats[rtIndex] = LocalToColourD3D12(format);
	}
	psoDesc.DSVFormat = (matConfig.m_depthEnable) ? DXGI_FORMAT_D24_UNORM_S8_UINT : DXGI_FORMAT_UNKNOWN;
	psoDesc.SampleDesc.Count = 1;


	HRESULT psoCreationResult = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&material->m_PSO));
	ThrowIfFailed(psoCreationResult, Stringf("FAILED TO CREATE PSO FOR MATERIAL %s", material->GetName().c_str()).c_str());
}

void Renderer::CreateMeshShaderPSO(Material* material)
{
	D3DX12_MESH_SHADER_PIPELINE_STATE_DESC psoDesc = {};
	MaterialConfig const& matConfig = material->m_config;

	D3D12_RASTERIZER_DESC rasterizerDesc = CD3DX12_RASTERIZER_DESC(
		LocalToD3D12(matConfig.m_fillMode),			// Fill mode
		LocalToD3D12(matConfig.m_cullMode),			// Cull mode
		LocalToD3D12(matConfig.m_windingOrder),		// Winding order
		D3D12_DEFAULT_DEPTH_BIAS,					// Depth bias
		D3D12_DEFAULT_DEPTH_BIAS_CLAMP,				// Bias clamp
		D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,		// Slope scaled bias
		TRUE,										// Depth Clip enable
		FALSE,										// Multi sample (MSAA)
		FALSE,										// Anti aliased line enable
		0,											// Force sample count
		D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF	// Conservative Rasterization
	);

	D3D12_BLEND_DESC blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	SetBlendModeSpecs(matConfig.m_blendMode, blendDesc);

	ShaderByteCode* psByteCode = material->m_byteCodes[ShaderType::Pixel];
	ShaderByteCode* msByteCode = material->m_byteCodes[ShaderType::Mesh];

	psoDesc.pRootSignature = m_defaultRootSignature.Get();
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(psByteCode->m_byteCode.data(), psByteCode->m_byteCode.size());
	psoDesc.MS = CD3DX12_SHADER_BYTECODE(msByteCode->m_byteCode.data(), msByteCode->m_byteCode.size());

	psoDesc.RasterizerState = rasterizerDesc;
	psoDesc.BlendState = blendDesc;
	psoDesc.DepthStencilState.DepthEnable = matConfig.m_depthEnable;
	psoDesc.DepthStencilState.DepthFunc = LocalToD3D12(matConfig.m_depthFunc);
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	psoDesc.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp = // a stencil operation structure, does not really matter since stencil testing is turned off
	{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
	psoDesc.DepthStencilState.FrontFace = defaultStencilOp; // both front and back facing polygons get the same treatment
	psoDesc.DepthStencilState.BackFace = defaultStencilOp;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE(matConfig.m_topology);
	psoDesc.NumRenderTargets = (UINT)matConfig.m_numRenderTargets;

	for (unsigned int rtIndex = 0; rtIndex < 8; rtIndex++) {
		TextureFormat const& format = matConfig.m_renderTargetFormats[rtIndex];
		if (format == TextureFormat::INVALID) continue;
		psoDesc.RTVFormats[rtIndex] = LocalToColourD3D12(format);
	}
	psoDesc.DSVFormat = (matConfig.m_depthEnable) ? DXGI_FORMAT_D24_UNORM_S8_UINT : DXGI_FORMAT_UNKNOWN;
	psoDesc.SampleDesc.Count = 1;

	auto psoStream = CD3DX12_PIPELINE_MESH_STATE_STREAM(psoDesc);

	D3D12_PIPELINE_STATE_STREAM_DESC streamDesc;
	streamDesc.pPipelineStateSubobjectStream = &psoStream;
	streamDesc.SizeInBytes = sizeof(psoStream);

	HRESULT psoCreationResult = m_device->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&material->m_PSO));
	ThrowIfFailed(psoCreationResult, "FAILED TO CREATE MESH SHADER");
	material->m_isMeshShader = true;
}

void Renderer::SetBlendModeSpecs(BlendMode const* blendModes, D3D12_BLEND_DESC& blendDesc)
{
	/*
		WARNING!!!!!
		IF THE RT IS SINGLE CHANNEL I.E: FLOATR_32,
		PICKING ALPHA WILL GET THE SRC ALPHA (DOES NOT EXIST)
		MAKES ALL WRITES TO RENDER TARGET INVALID!!!!

	*/
	blendDesc.IndependentBlendEnable = TRUE;
	for (int rtIndex = 0; rtIndex < 8; rtIndex++) {
		BlendMode currentRtBlendMode = blendModes[rtIndex];
		D3D12_RENDER_TARGET_BLEND_DESC& rtBlendDesc = blendDesc.RenderTarget[rtIndex];
		rtBlendDesc.BlendEnable = true;
		rtBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		rtBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
		rtBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
		rtBlendDesc.DestBlendAlpha = D3D12_BLEND_ONE;
		rtBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;

		switch (currentRtBlendMode) {
		case BlendMode::ALPHA:
			rtBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
			rtBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
			break;
		case BlendMode::ADDITIVE:
			rtBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
			rtBlendDesc.DestBlend = D3D12_BLEND_ONE;
			break;
		case BlendMode::OPAQUE:
			rtBlendDesc.SrcBlend = D3D12_BLEND_ONE;
			rtBlendDesc.DestBlend = D3D12_BLEND_ZERO;
			break;
		default:
			ERROR_AND_DIE(Stringf("Unknown / unsupported blend mode #%i", currentRtBlendMode));
			break;
		}
	}
}

void Renderer::InitializeCBufferArrays()
{
	// Max amount of cbuffers existing if we used all descriptors
	// and all were a unique cbuffer for a draw pass
	constexpr unsigned int maxCbufferLimit = CBV_DESCRIPTORS_AMOUNT / g_engineBufferCount;

	BufferDesc cameraBufferDesc = {};
	cameraBufferDesc.data = nullptr;
	cameraBufferDesc.memoryUsage = MemoryUsage::Default;
	cameraBufferDesc.owner = this;
	cameraBufferDesc.size = sizeof(CameraConstants);
	cameraBufferDesc.stride = sizeof(CameraConstants);
	cameraBufferDesc.debugName = "CameraCBuffer";

	BufferDesc modelBufferDesc = cameraBufferDesc;
	modelBufferDesc.size = sizeof(ModelConstants);
	modelBufferDesc.stride = sizeof(ModelConstants);
	modelBufferDesc.debugName = "ModelCBuffer";

	BufferDesc lightBufferDesc = cameraBufferDesc;
	lightBufferDesc.size = sizeof(LightConstants);
	lightBufferDesc.stride = sizeof(LightConstants);
	lightBufferDesc.debugName = "LightCBuffer";

	m_cameraCBOArray = std::vector<ConstantBuffer>(maxCbufferLimit, cameraBufferDesc);
	m_modelCBOArray = std::vector<ConstantBuffer>(maxCbufferLimit, modelBufferDesc);
	m_lightCBOArray = std::vector<ConstantBuffer>(maxCbufferLimit, modelBufferDesc);

}

void Renderer::CreateDefaultBufferObjects()
{
	BufferDesc bufferDesc = {};
	bufferDesc.data = nullptr;
	bufferDesc.debugName = "VBO";
	bufferDesc.memoryUsage = MemoryUsage::Default;
	bufferDesc.owner = this;
	bufferDesc.size = sizeof(Vertex_PCU);
	bufferDesc.stride = sizeof(Vertex_PCU);

	m_immediateVBO = new VertexBuffer(bufferDesc);

	bufferDesc.size = sizeof(Vertex_PNCU);
	bufferDesc.stride = sizeof(Vertex_PNCU);
	m_immediateDiffuseVBO = new VertexBuffer(bufferDesc);

	bufferDesc.size = sizeof(unsigned int);
	bufferDesc.stride = sizeof(unsigned int);
	m_immediateIBO = new IndexBuffer(bufferDesc);
	m_immediateDiffuseIBO = new IndexBuffer(bufferDesc);

}

void Renderer::FinishPendingPrePassResourceTasks()
{
	UploadImmediateVertexes();

	ID3D12GraphicsCommandList6* currentCmdList = GetCurrentCommandList(CommandListType::RESOURCES);
	if (m_pendingRscCopy.size() > 0) {

		std::vector<D3D12_RESOURCE_BARRIER> copyRscBarriers;
		copyRscBarriers.reserve(m_pendingRscCopy.size() * 2);
		for (Buffer* buffer : m_pendingRscCopy) {
			buffer->m_buffer->AddResourceBarrierToList(D3D12_RESOURCE_STATE_COPY_DEST, copyRscBarriers);
			buffer->m_uploadBuffer->AddResourceBarrierToList(D3D12_RESOURCE_STATE_COPY_SOURCE, copyRscBarriers);
		}

		//currentCmdList->ResourceBarrier((UINT)m_pendingCopyRscBarriers.size(), m_pendingCopyRscBarriers.data());
		currentCmdList->ResourceBarrier((UINT)copyRscBarriers.size(), copyRscBarriers.data());

		for (Buffer* buffer : m_pendingRscCopy) {
			buffer->ResetCopyState();
			Resource* uploadRsc = buffer->m_uploadBuffer;
			Resource* defaultRsc = buffer->m_buffer;

			currentCmdList->CopyResource(defaultRsc->m_resource, uploadRsc->m_resource);
			AddBufferBindResourceBarrier(buffer, m_pendingRscBarriers);
		}
	}

	Texture* activeRT = GetActiveBackBuffer();
	Resource* rtRsc = activeRT->GetResource();

	Resource* depthRsc = m_defaultDepthTarget->GetResource();
	depthRsc->AddResourceBarrierToList(D3D12_RESOURCE_STATE_DEPTH_WRITE, m_pendingRscBarriers);
	rtRsc->AddResourceBarrierToList(D3D12_RESOURCE_STATE_RENDER_TARGET, m_pendingRscBarriers);

	if (m_pendingRscBarriers.size() > 0) {
		currentCmdList->ResourceBarrier((UINT)m_pendingRscBarriers.size(), m_pendingRscBarriers.data());
	}
}

void Renderer::FinishPendingPreFxPassResourceTasks()
{
	ID3D12GraphicsCommandList6* currentCmdList = GetCurrentCommandList(CommandListType::DEFAULT);
	if (m_pendingRscCopy.size() > 0) {

		std::vector<D3D12_RESOURCE_BARRIER> copyRscBarriers;
		copyRscBarriers.reserve(m_pendingRscCopy.size() * 2);
		for (Buffer* buffer : m_pendingRscCopy) {
			buffer->m_buffer->AddResourceBarrierToList(D3D12_RESOURCE_STATE_COPY_DEST, copyRscBarriers);
			buffer->m_uploadBuffer->AddResourceBarrierToList(D3D12_RESOURCE_STATE_COPY_SOURCE, copyRscBarriers);
		}

		//currentCmdList->ResourceBarrier((UINT)m_pendingCopyRscBarriers.size(), m_pendingCopyRscBarriers.data());
		currentCmdList->ResourceBarrier((UINT)copyRscBarriers.size(), copyRscBarriers.data());

		for (Buffer* buffer : m_pendingRscCopy) {
			buffer->ResetCopyState();
			Resource* uploadRsc = buffer->m_uploadBuffer;
			Resource* defaultRsc = buffer->m_buffer;

			currentCmdList->CopyResource(defaultRsc->m_resource, uploadRsc->m_resource);
			AddBufferBindResourceBarrier(buffer, m_pendingRscBarriers);
		}
	}

	if (m_pendingRscBarriers.size() > 0) {
		currentCmdList->ResourceBarrier((UINT)m_pendingRscBarriers.size(), m_pendingRscBarriers.data());
	}
}

void Renderer::UploadImmediateVertexes()
{
	// Unlit vertexes. This is a giant buffer with all vertexes for better performance
	size_t vertexesSize = sizeof(Vertex_PCU) * m_immediateVertexes.size();
	if (vertexesSize > 0) {
		m_immediateVBO->GuaranteeBufferSize(vertexesSize);
		m_immediateVBO->CopyCPUToGPU(m_immediateVertexes.data(), vertexesSize);
	}

	// Single buffer for any indexed draw calls
	size_t indexSize = sizeof(unsigned) * m_immediateIndices.size();
	if (indexSize > 0) {
		m_immediateIBO->GuaranteeBufferSize(indexSize);
		m_immediateIBO->CopyCPUToGPU(m_immediateIndices.data(), indexSize);
	}

	// Lit vertexes also get their own buffer
	size_t vertexesDiffuseSize = sizeof(Vertex_PNCU) * m_immediateDiffuseVertexes.size();
	if (vertexesDiffuseSize > 0) {
		m_immediateDiffuseVBO->GuaranteeBufferSize(vertexesDiffuseSize);
		m_immediateDiffuseVBO->CopyCPUToGPU(m_immediateDiffuseVertexes.data(), vertexesDiffuseSize);
	}

	// Single buffer for any indexed draw calls
	size_t diffuseIndexSize = sizeof(unsigned) * m_immediateDiffuseIndices.size();
	if (diffuseIndexSize > 0) {
		m_immediateDiffuseIBO->GuaranteeBufferSize(diffuseIndexSize);
		m_immediateDiffuseIBO->CopyCPUToGPU(m_immediateDiffuseIndices.data(), diffuseIndexSize);
	}
}

void Renderer::DrawAllEffects()
{
	for (FxContext& ctx : m_effectsContexts) {
		DrawEffect(ctx);
	}
}

void Renderer::DrawEffect(FxContext& fxCtx)
{
	Texture* activeTarget = GetActiveRenderTarget();
	Texture* backTarget = GetBackUpRenderTarget();

	CopyTextureWithMaterial(backTarget, activeTarget, fxCtx.m_depthTarget, fxCtx.m_fx);

	m_currentRenderTarget = (m_currentRenderTarget + 1) % 2;
}

void Renderer::DrawAllImmediateContexts()
{
	for (unsigned int ctxIndex = 0; ctxIndex < m_currentDrawCtx; ctxIndex++) {
		ImmediateContext& ctx = m_immediateContexts[ctxIndex];
		DrawImmediateContext(ctx);
	}
}

void Renderer::DrawImmediateContext(ImmediateContext& ctx)
{
	std::vector<D3D12_RESOURCE_BARRIER> drawRscBarriers;
	FillResourceBarriers(ctx, drawRscBarriers);

	Material* material = ctx.m_material;
	ID3D12GraphicsCommandList6* cmdList = GetCurrentCommandList(CommandListType::DEFAULT);

	unsigned int rscBarrierCount = (unsigned int)drawRscBarriers.size();
	if (rscBarrierCount > 0) {
		cmdList->ResourceBarrier(rscBarrierCount, drawRscBarriers.data());
	}

	cmdList->SetPipelineState(material->m_PSO);

	// Copy Descriptors into the GPU Descriptor heap
	for (auto& [slot, texture] : ctx.m_boundTextures) {
		Texture const* boundTex = texture;
		if (!boundTex) boundTex = m_defaultTexture;
		CopyTextureToDescriptorHeap(boundTex, ctx.m_srvHandleStart, slot);
	}

	// Copy Descriptors into the GPU Descriptor heap
	for (auto& [slot, cBuffer] : ctx.m_boundCBuffers) {
		CopyBufferToGPUHeap(cBuffer, RESOURCE_BIND_CONSTANT_BUFFER_VIEW_BIT, ctx.m_cbvHandleStart, slot);
	}

	for (auto& [slot, pBuffer] : ctx.m_boundBuffers) {
		CopyBufferToGPUHeap(pBuffer, RESOURCE_BIND_SHADER_RESOURCE_BIT, ctx.m_srvHandleStart, slot);
	}

	CopyEngineCBuffersToGPUHeap(ctx);
	DescriptorHeap* srvUAVCBVHeap = GetGPUDescriptorHeap(DescriptorHeapType::SRV_UAV_CBV);
	DescriptorHeap* samplerHeap = GetGPUDescriptorHeap(DescriptorHeapType::SAMPLER);
	ID3D12DescriptorHeap* allDescriptorHeaps[] = {
		srvUAVCBVHeap->GetHeap(),
		samplerHeap->GetHeap()
	};

	UINT numHeaps = sizeof(allDescriptorHeaps) / sizeof(ID3D12DescriptorHeap*);
	cmdList->SetDescriptorHeaps(numHeaps, allDescriptorHeaps);
	cmdList->SetGraphicsRootSignature(m_defaultRootSignature.Get());
	// 0 -> CBV
	// 1 -> SRV
	// 2 -> UAV
	// 3 -> Samplers
	cmdList->SetGraphicsRootDescriptorTable(0, srvUAVCBVHeap->GetGPUHandleAtOffset(ctx.m_cbvHandleStart));
	cmdList->SetGraphicsRootDescriptorTable(1, srvUAVCBVHeap->GetGPUHandleAtOffset(ctx.m_srvHandleStart));
	cmdList->SetGraphicsRootDescriptorTable(2, srvUAVCBVHeap->GetGPUHandleAtOffset(UAV_HANDLE_START));
	cmdList->SetGraphicsRootDescriptorTable(3, samplerHeap->GetGPUHandleForHeapStart());

	cmdList->RSSetViewports(1, &m_viewport);
	cmdList->RSSetScissorRects(1, &m_scissorRect);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[8] = {};
	D3D12_RENDER_PASS_RENDER_TARGET_DESC renderPassRTDesc[8] = {};

	UINT rtCount = 0;
	for (unsigned int rtIndex = 0; rtIndex < 8; rtIndex++) {
		Texture* rt = ctx.m_renderTargets[rtIndex];
		if (!rt) continue;
		rtCount++;

		D3D12_CPU_DESCRIPTOR_HANDLE& rtHandle = rtvHandles[rtIndex];
		rtHandle = rt->GetOrCreateView(RESOURCE_BIND_RENDER_TARGET_BIT)->GetHandle();
	}

	ResourceView* dsvView = ctx.m_depthTarget->GetOrCreateView(RESOURCE_BIND_DEPTH_STENCIL_BIT);
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvView->GetHandle();

	cmdList->OMSetRenderTargets(rtCount, rtvHandles, FALSE, &dsvHandle);
	if (ctx.UsesMeshShaders()) {
		unsigned int threadX = ctx.m_meshThreads.x;
		unsigned int threadY = ctx.m_meshThreads.y;
		unsigned int threadZ = ctx.m_meshThreads.z;
		cmdList->DispatchMesh(threadX, threadY, threadZ);
	}
	else {
		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		VertexBuffer* usedBuffer = (ctx.m_externalVBO) ? ctx.m_externalVBO : GetImmediateVBO(ctx.GetVertexType());;
		IndexBuffer* usedIndexedBuffer = nullptr;


		D3D12_VERTEX_BUFFER_VIEW D3DbufferView = {};
		BufferView vBufferView = usedBuffer->GetBufferView();
		// Initialize the vertex buffer view.
		D3DbufferView.BufferLocation = vBufferView.m_bufferLocation;
		D3DbufferView.StrideInBytes = (UINT)vBufferView.m_strideInBytes;
		D3DbufferView.SizeInBytes = (UINT)vBufferView.m_sizeInBytes;

		cmdList->IASetVertexBuffers(0, 1, &D3DbufferView);

		if (ctx.IsIndexDraw()) {
			usedIndexedBuffer = (ctx.m_externalIBO) ? ctx.m_externalIBO : m_immediateIBO;
			D3D12_INDEX_BUFFER_VIEW D3DindexedBufferView = {};
			BufferView iBufferView = usedIndexedBuffer->GetBufferView();
			D3DindexedBufferView.BufferLocation = iBufferView.m_bufferLocation;
			D3DindexedBufferView.Format = DXGI_FORMAT_R32_UINT;
			D3DindexedBufferView.SizeInBytes = (UINT)iBufferView.m_sizeInBytes;

			cmdList->IASetIndexBuffer(&D3DindexedBufferView);
			cmdList->DrawIndexedInstanced((UINT)ctx.m_indexCount, 1, (UINT)ctx.m_indexStart, (INT)ctx.m_vertexStart, 0);
		}
		else {
			cmdList->DrawInstanced((UINT)ctx.m_vertexCount, 1, (UINT)ctx.m_vertexStart, 0);
		}
	}
}

ImmediateContext& Renderer::GetCurrentDrawCtx()
{
	return m_immediateContexts[m_currentDrawCtx];
}

ConstantBuffer* Renderer::GetNextCBufferSlot(ConstantBufferType cBufferType)
{
	ConstantBuffer* nextCBuffer = nullptr;
	switch (cBufferType)
	{
	case ConstantBufferType::CAMERA:
		if (m_currentCameraBufferSlot > m_cameraCBOArray.size()) ERROR_AND_DIE("OUT OF BOUNDS CAMERA CBUFFER SLOT");
		nextCBuffer = &m_cameraCBOArray[m_currentCameraBufferSlot++];
		break;
	case ConstantBufferType::MODEL:
		if (m_currentModelBufferSlot > m_modelCBOArray.size()) ERROR_AND_DIE("OUT OF BOUNDS MODEL CBUFFER SLOT");
		nextCBuffer = &m_modelCBOArray[m_currentModelBufferSlot++];
		break;
	case ConstantBufferType::LIGHT:
		if (m_currentLightBufferSlot > m_lightCBOArray.size()) ERROR_AND_DIE("OUT OF BOUNDS LIGHT CBUFFER SLOT");
		nextCBuffer = &m_lightCBOArray[m_currentLightBufferSlot++];
		break;
	case ConstantBufferType::NUM_CONSTANT_BUFFER_TYPES:
	default:
		ERROR_AND_DIE("UNDEFINED TYPE OF ENGINE CONSTANT BUFFER")
			break;
	}

	nextCBuffer->Initialize();
	return nextCBuffer;
}

void Renderer::SetContextDescriptorStarts(ImmediateContext& ctx)
{
	ctx.m_srvHandleStart = m_srvHandleStart;
	ctx.m_cbvHandleStart = m_cbvHandleStart;

	UpdateDescriptorsHandleStarts(ctx);
}

void Renderer::SetModelBufferForCtx(ImmediateContext& ctx)
{
	if (m_isModelBufferDirty) {
		ctx.m_modelCBO = GetNextCBufferSlot(ConstantBufferType::MODEL);
		ctx.m_modelCBO->CopyCPUToGPU(&ctx.m_modelConstants, sizeof(ModelConstants));
	}
	m_isModelBufferDirty = false;
}

void Renderer::SetContextDrawInfo(ImmediateContext& ctx, unsigned int numVertexes, Vertex_PCU const* vertexes)
{
	SetModelBufferForCtx(ctx);

	ctx.m_vertexCount = numVertexes;
	ctx.m_vertexStart = m_immediateVertexes.size();
	std::copy(vertexes, vertexes + numVertexes, std::back_inserter(m_immediateVertexes));
	if (m_isModelBufferDirty) {
		ctx.m_modelCBO->CopyCPUToGPU(&ctx.m_modelConstants, sizeof(ModelConstants));
	}

	ctx.SetDrawCallUsage(true);
}

void Renderer::SetContextDrawInfo(ImmediateContext& ctx, unsigned int numVertexes, Vertex_PNCU const* vertexes)
{
	SetModelBufferForCtx(ctx);

	ctx.m_vertexCount = numVertexes;
	ctx.m_vertexStart = m_immediateDiffuseVertexes.size();
	std::copy(vertexes, vertexes + numVertexes, std::back_inserter(m_immediateDiffuseVertexes));
	if (m_isModelBufferDirty) {
		ctx.m_modelCBO->CopyCPUToGPU(&ctx.m_modelConstants, sizeof(ModelConstants));
	}

	ctx.SetDrawCallUsage(true);
}

void Renderer::SetContextIndexedDrawInfo(ImmediateContext& ctx, unsigned int numVertexes, Vertex_PCU const* vertexes, unsigned int indexCount, unsigned int const* indexes)
{
	SetModelBufferForCtx(ctx);

	ctx.m_vertexCount = numVertexes;
	ctx.m_vertexStart = m_immediateVertexes.size();
	ctx.m_indexCount = indexCount;
	ctx.m_indexStart = m_immediateIndices.size();

	std::copy(vertexes, vertexes + numVertexes, std::back_inserter(m_immediateVertexes));
	std::copy(indexes, indexes + indexCount, std::back_inserter(m_immediateIndices));
	if (m_isModelBufferDirty) {
		ctx.m_modelCBO->CopyCPUToGPU(&ctx.m_modelConstants, sizeof(ModelConstants));
	}

	ctx.SetIndexDrawFlag(true);
	ctx.SetDrawCallUsage(true);
}

void Renderer::SetContextIndexedDrawInfo(ImmediateContext& ctx, unsigned int numVertexes, Vertex_PNCU const* vertexes, unsigned int indexCount, unsigned int const* indexes)
{
	SetModelBufferForCtx(ctx);

	ctx.m_vertexCount = numVertexes;
	ctx.m_vertexStart = m_immediateVertexes.size();
	ctx.m_indexCount = indexCount;
	ctx.m_indexStart = m_immediateIndices.size();

	std::copy(vertexes, vertexes + numVertexes, std::back_inserter(m_immediateDiffuseVertexes));
	std::copy(indexes, indexes + indexCount, std::back_inserter(m_immediateIndices));
	if (m_isModelBufferDirty) {
		ctx.m_modelCBO->CopyCPUToGPU(&ctx.m_modelConstants, sizeof(ModelConstants));
	}

	ctx.SetIndexDrawFlag(true);
	ctx.SetDrawCallUsage(true);
}

DescriptorHeap* Renderer::GetCPUDescriptorHeap(DescriptorHeapType descriptorHeapType) const
{
	return m_CPUDescriptorHeaps[(size_t)descriptorHeapType];
}

DescriptorHeap* Renderer::GetGPUDescriptorHeap(DescriptorHeapType descriptorHeapType) const
{
	size_t typeAsIndex = (size_t)descriptorHeapType;
	if (typeAsIndex > (size_t)DescriptorHeapType::MAX_GPU_VISIBLE) return nullptr;

	return m_GPUDescriptorHeaps[(size_t)descriptorHeapType];
}

/// <summary>
/// Through copying the context onto the next, we can keep resources bound between render passes
/// </summary>
void Renderer::CopyCurrentDrawCtxToNext()
{
	// Copy relevant information into the next context

	if (m_currentDrawCtx <= IMMEDIATE_CTX_AMOUNT - 1) {
		ImmediateContext& nextCtx = m_immediateContexts[m_currentDrawCtx + 1];

		nextCtx = ImmediateContext(m_immediateContexts[m_currentDrawCtx]);
		nextCtx.ResetExternalBuffers();
		nextCtx.SetIndexDrawFlag(false);
		nextCtx.SetPipelineTypeFlag(false);
		nextCtx.SetDefaultDepthTextureSRVFlag(false);
		nextCtx.SetVertexType(VertexType::PCU); // PCU is default
	}
	m_currentDrawCtx++;
}

void Renderer::CopyTextureToDescriptorHeap(Texture const* texture, unsigned int handleStart, unsigned int slot)
{
	Texture* usedTex = const_cast<Texture*>(texture);
	ResourceView* rsv = usedTex->GetOrCreateView(RESOURCE_BIND_SHADER_RESOURCE_BIT);
	CopyResourceToGPUHeap(rsv, handleStart, slot);
}

void Renderer::CopyBufferToGPUHeap(Buffer* bufferToBind, ResourceBindFlagBit bindFlag, unsigned int handleStart, unsigned int slot)
{
	ResourceView* rsv = bufferToBind->GetOrCreateView(bindFlag);
	CopyResourceToGPUHeap(rsv, handleStart, slot);
}

void Renderer::CopyEngineCBuffersToGPUHeap(ImmediateContext& ctx)
{
	ConstantBuffer* cameraCBO = ctx.m_cameraCBO;
	ConstantBuffer* modelCBO = ctx.m_modelCBO;

	CopyBufferToGPUHeap(cameraCBO, RESOURCE_BIND_CONSTANT_BUFFER_VIEW_BIT, ctx.m_cbvHandleStart, g_cameraBufferSlot);
	CopyBufferToGPUHeap(modelCBO, RESOURCE_BIND_CONSTANT_BUFFER_VIEW_BIT, ctx.m_cbvHandleStart, g_modelBufferSlot);
}

void Renderer::CopyResourceToGPUHeap(ResourceView* rsv, unsigned int handleStart, unsigned int slot)
{
	unsigned int accessSlot = handleStart + slot;
	DescriptorHeap* srvHeap = GetGPUDescriptorHeap(DescriptorHeapType::SRV_UAV_CBV);
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = srvHeap->GetHandleAtOffset(accessSlot);
	D3D12_CPU_DESCRIPTOR_HANDLE cBufferHandle = rsv->GetHandle();

	m_device->CopyDescriptorsSimple(1, srvHandle, rsv->GetHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void Renderer::UpdateDescriptorsHandleStarts(ImmediateContext const& ctx)
{
	static auto findHighestValTex = [](const std::pair<unsigned int, Texture const*>& a, const std::pair<unsigned int, Texture const*>& b)->bool { return a.first < b.first; };
	static auto findHighestValCbuffer = [](const std::pair<unsigned int, ConstantBuffer*>& a, const std::pair<unsigned int, ConstantBuffer*>& b)->bool { return a.first < b.first; };
	static auto findHighestValSRVBuffer = [](const std::pair<unsigned int, Buffer*>& a, const std::pair<unsigned int, Buffer*>& b)->bool { return a.first < b.first; };


	// Find the max value to skip that amount of slots
	// Empty slots must be respected even if wasteful
	// Buffers bound as SRV share handle start with textures
	auto texMaxIt = std::max_element(ctx.m_boundTextures.begin(), ctx.m_boundTextures.end(), findHighestValTex);
	auto cBufferMaxIt = std::max_element(ctx.m_boundCBuffers.begin(), ctx.m_boundCBuffers.end(), findHighestValCbuffer);
	auto SRVBufferMaxIt = std::max_element(ctx.m_boundBuffers.begin(), ctx.m_boundBuffers.end(), findHighestValSRVBuffer);

	unsigned int texMax = (texMaxIt != ctx.m_boundTextures.end()) ? texMaxIt->first : 0;
	unsigned int srvBufferMax = (SRVBufferMaxIt != ctx.m_boundBuffers.end()) ? SRVBufferMaxIt->first : 0;
	unsigned int cBufferMax = (cBufferMaxIt != ctx.m_boundCBuffers.end()) ? cBufferMaxIt->first : 0;

	unsigned int SRVMax = (srvBufferMax > texMax) ? srvBufferMax : texMax;

	// This is accounting for the Engine's constant buffers
	// slot 0: Camera
	// slot 1: Model
	// slot 2: Light
	if (cBufferMax < 2) cBufferMax = 2;

	m_srvHandleStart += SRVMax + 1;
	m_cbvHandleStart += cBufferMax + 1;
}

VertexBuffer* Renderer::GetImmediateVBO(VertexType vertexType)
{
	switch (vertexType)
	{
	case VertexType::PCU:
		return m_immediateVBO;
	case VertexType::PNCU:
		return m_immediateDiffuseVBO;
	default:
		ERROR_AND_DIE("UNDEFINED VERTEX TYPE");
		break;
	}
}

void Renderer::ExecuteCommandLists(unsigned int count, ID3D12CommandList** cmdLists)
{
	m_commandQueue->ExecuteCommandLists(count, cmdLists);
	m_currentCmdListIndex = (m_currentCmdListIndex + 1) % 2;
}

void Renderer::AddBufferBindResourceBarrier(Buffer* bufferToBind, std::vector<D3D12_RESOURCE_BARRIER>& barriersArray)
{
	switch (bufferToBind->m_bufferType)
	{
	case BufferType::VertexBuffer:
	case BufferType::ConstantBuffer:
		bufferToBind->m_buffer->AddResourceBarrierToList(D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, barriersArray);
		break;
	case BufferType::IndexBuffer:
		bufferToBind->m_buffer->AddResourceBarrierToList(D3D12_RESOURCE_STATE_INDEX_BUFFER, barriersArray);
		break;
	case BufferType::StructuredBuffer:
		bufferToBind->m_buffer->AddResourceBarrierToList(D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, barriersArray);
		break;
	default:
		ERROR_AND_DIE("UNRECOGNIZED BUFFER TYPE");
		break;
	}
}

void Renderer::ExecuteMainRenderPass()
{
	// Uploads pending resources and inserts resource barriers before drawing
	FinishPendingPrePassResourceTasks();
	DrawAllImmediateContexts();

	//ID3D12GraphicsCommandList6* cmdList = GetCurrentCommandList(CommandListType::DEFAULT);
	//ID3D12GraphicsCommandList6* rscCmdList = GetCurrentCommandList(CommandListType::RESOURCES);
	//rscCmdList->Close();
	//cmdList->Close();

	//ID3D12CommandList* ppCmdLists[] = { rscCmdList, cmdList };
	//ExecuteCommandLists(_countof(ppCmdLists), ppCmdLists);

}

void Renderer::ExecuteEffectsRenderPass()
{
	//FinishPendingPreFxPassResourceTasks();
	DrawAllEffects();
}

void Renderer::FillResourceBarriers(ImmediateContext& ctx, std::vector<D3D12_RESOURCE_BARRIER>& out_rscBarriers)
{
	// Textures
	for (auto& [slot, texture] : ctx.m_boundTextures) {
		Resource* rsc = texture->GetResource();
		rsc->AddResourceBarrierToList(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, out_rscBarriers);
		if (texture == m_defaultDepthTarget) {
			ctx.SetDefaultDepthTextureSRVFlag(true);
		}
	}

	// Constant Buffers
	for (auto& [slot, cBuffer] : ctx.m_boundCBuffers) {
		Resource* rsc = cBuffer->GetResource();
		rsc->AddResourceBarrierToList(D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, out_rscBarriers);
	}

	// All other buffers e.g: UAVs

	for (auto& [slot, buffer] : ctx.m_boundBuffers) {
		Resource* rsc = buffer->GetResource();
		rsc->AddResourceBarrierToList(D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, out_rscBarriers);
	}

	// External VBO, IBO

	if (ctx.m_externalIBO) {
		Resource* externalIBORsc = ctx.m_externalIBO->GetResource();
		externalIBORsc->AddResourceBarrierToList(D3D12_RESOURCE_STATE_INDEX_BUFFER, out_rscBarriers);
	}

	if (ctx.m_externalVBO) {
		Resource* externalVBORsc = ctx.m_externalVBO->GetResource();
		externalVBORsc->AddResourceBarrierToList(D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, out_rscBarriers);
	}
}

void Renderer::BeginFrame()
{
	// In the first frame, it's most likely that textures are being created
	if (m_currentFrame == 0) {
		ID3D12GraphicsCommandList6* cmdList = GetCurrentCommandList(CommandListType::DEFAULT);
		ID3D12GraphicsCommandList6* rscCmdList = GetCurrentCommandList(CommandListType::RESOURCES);

		cmdList->Close();
		rscCmdList->Close();

		// Execute the command list.
		ID3D12CommandList* ppCmdLists[] = { rscCmdList, cmdList };
		ExecuteCommandLists(_countof(ppCmdLists), ppCmdLists);

		m_fence->Signal();
		m_fence->Wait();
	}

	g_theMaterialSystem->BeginFrame();
	DebugRenderBeginFrame();
	BeginFrameImGui();

	ResetGPUState();
}

void Renderer::EndFrame()
{

	ExecuteMainRenderPass();
	ExecuteEffectsRenderPass();
	DebugRenderEndFrame();
	EndFrameImGui();


	g_theMaterialSystem->EndFrame();

	ID3D12GraphicsCommandList6* cmdList = GetCurrentCommandList(CommandListType::DEFAULT);
	ID3D12GraphicsCommandList6* rscCmdList = GetCurrentCommandList(CommandListType::RESOURCES);

	Resource* currentBackBuffer = GetActiveBackBuffer()->GetResource();
	Resource* currentRT = GetActiveRenderTarget()->GetResource();


	currentBackBuffer->TransitionTo(D3D12_RESOURCE_STATE_COPY_DEST, cmdList);
	currentRT->TransitionTo(D3D12_RESOURCE_STATE_COPY_SOURCE, cmdList);

	cmdList->CopyResource(currentBackBuffer->m_resource, currentRT->m_resource);



	currentBackBuffer->TransitionTo(D3D12_RESOURCE_STATE_PRESENT, cmdList);

	cmdList->Close();
	rscCmdList->Close();

	// Execute the command list.
	ID3D12CommandList* ppCmdLists[] = { rscCmdList, cmdList };
	ExecuteCommandLists(_countof(ppCmdLists), ppCmdLists);

	// Present the frame.
	ThrowIfFailed(m_swapChain->Present(1, 0), "FAILED TO PRESENT");
	m_fence->Signal();
	m_fence->Wait();

	m_currentBackBuffer = m_swapChain->GetCurrentBackBufferIndex();
	m_currentFrame++;
}

void Renderer::Shutdown()
{
	m_fence->Signal();
	m_fence->Wait();

	m_resourcesFence->Signal();
	m_resourcesFence->Wait();

	ShutdownImGui();
	DebugRenderSystemShutdown();
	g_theMaterialSystem->Shutdown();

	delete[] m_immediateContexts;
	m_immediateContexts = nullptr;

	for (unsigned int textureInd = 0; textureInd < m_loadedTextures.size(); textureInd++) {
		Texture* tex = m_loadedTextures[textureInd];
		DestroyTexture(tex);
	}
	m_loadedTextures.clear();

	for (unsigned int fontInd = 0; fontInd < m_loadedFonts.size(); fontInd++) {
		BitmapFont* font = m_loadedFonts[fontInd];
		if (font) {
			delete font;
		}
	}

	m_loadedFonts.clear();

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


	m_device.Reset();
	m_DXGIFactory.Reset();
	m_commandQueue.Reset();
	m_swapChain.Reset();
	m_defaultRootSignature.Reset();

	// Clearing cmd lists and allocators
	for (unsigned int index = 0; index < m_commandAllocators.size(); index++) {
		DX_SAFE_RELEASE(m_commandAllocators[index]);
		DX_SAFE_RELEASE(m_commandLists[index]);
	}

	m_commandAllocators.resize(0);
	m_commandLists.resize(0);

	// App resources.
	m_defaultRenderTargets.clear();
	m_backBuffers.clear();
	m_pendingRscBarriers.clear();
	m_pendingRscCopy.clear();

	for (unsigned int byteCodeInd = 0; byteCodeInd < m_shaderByteCodes.size(); byteCodeInd++) {
		ShaderByteCode* byteCode = m_shaderByteCodes[byteCodeInd];
		if (byteCode) delete byteCode;
	}
	m_shaderByteCodes.clear();
	m_immediateVertexes.clear();
	m_immediateDiffuseVertexes.clear();
	m_immediateIndices.clear();
	m_immediateDiffuseIndices.clear();


	if (m_fence) {
		delete m_fence;
		m_fence = nullptr;
	}

	if (m_resourcesFence) {
		delete m_resourcesFence;
		m_resourcesFence = nullptr;
	}


	m_defaultDepthTarget = nullptr;
	m_defaultTexture = nullptr;
	m_default2DMaterial = nullptr;
	m_default3DMaterial = nullptr;

	m_cameraCBOArray.clear();
	m_lightCBOArray.clear();
	m_modelCBOArray.clear();

	m_currentBackBuffer = 0;
	m_currentRenderTarget = 0;
	m_currentDrawCtx = 0;
	m_currentCameraBufferSlot = 0;
	m_currentModelBufferSlot = 0;
	m_currentLightBufferSlot = 0;
	m_srvHandleStart = 0;
	m_cbvHandleStart = 0;
	m_currentFrame = 0;

	m_defaultTexture = nullptr;
	m_defaultDepthTarget = nullptr;

	if (m_immediateVBO) {
		delete m_immediateVBO;
		m_immediateVBO = nullptr;
	}

	if (m_immediateIBO) {
		delete m_immediateIBO;
		m_immediateIBO = nullptr;
	}

	if (m_immediateDiffuseVBO) {
		delete m_immediateDiffuseVBO;
		m_immediateDiffuseVBO = nullptr;
	}

	if (m_immediateDiffuseIBO) {
		delete m_immediateDiffuseIBO;
		m_immediateDiffuseIBO = nullptr;
	}
}

void Renderer::BeginCamera(Camera const& camera)
{
	m_currentCamera = &camera;
	ImmediateContext& ctx = GetCurrentDrawCtx();
	ctx.Reset();

	if (camera.GetCameraMode() == CameraMode::Orthographic) {
		m_is3DDefault = false;
	}

	// Default Bindings
	BindMaterial(GetDefaultMaterial(m_is3DDefault));
	BindTexture(m_defaultTexture);
	SetSamplerMode(SamplerMode::POINTCLAMP);

	ctx.SetRenderTarget(0, GetActiveRenderTarget());
	ctx.SetDepthRenderTarget(m_defaultDepthTarget);

	CameraConstants cameraConstants = {};
	cameraConstants.ProjectionMatrix = camera.GetProjectionMatrix();
	cameraConstants.ViewMatrix = camera.GetViewMatrix();
	cameraConstants.InvertedMatrix = cameraConstants.ProjectionMatrix.GetInverted();

	ConstantBuffer* nextCameraBuffer = GetNextCBufferSlot(ConstantBufferType::CAMERA);
	nextCameraBuffer->CopyCPUToGPU(&cameraConstants, sizeof(CameraConstants));

	ModelConstants modelConstants = {};
	ctx.m_modelConstants = modelConstants;
	m_isModelBufferDirty = true;

	ctx.m_cameraCBO = nextCameraBuffer;
}

void Renderer::EndCamera(Camera const& camera)
{
	if (m_currentCamera != &camera) {
		ERROR_AND_DIE("ENDING WITH A DIFFERENT CAMERA");
	}
	m_currentCamera = nullptr;
}

void Renderer::ClearTexture(Rgba8 const& color, Texture* tex)
{
	float colorAsArray[4];
	color.GetAsFloats(colorAsArray);

	Resource* rtResource = tex->GetResource();
	rtResource->AddResourceBarrierToList(D3D12_RESOURCE_STATE_RENDER_TARGET, m_pendingRscBarriers);

	D3D12_CPU_DESCRIPTOR_HANDLE currentRTVHandle = tex->GetOrCreateView(RESOURCE_BIND_RENDER_TARGET_BIT)->GetHandle();;
	ID3D12GraphicsCommandList6* cmdList = GetCurrentCommandList(CommandListType::DEFAULT);

	cmdList->ClearRenderTargetView(currentRTVHandle, colorAsArray, 0, nullptr);
}


void Renderer::ClearScreen(Rgba8 const& color)
{
	Texture* currentRt = GetActiveRenderTarget();
	ClearTexture(color, currentRt);
	ClearDepth();
}

void Renderer::ClearRenderTarget(unsigned int slot, Rgba8 const& color)
{
	ImmediateContext& currentDrawCtx = GetCurrentDrawCtx();
	Texture* tex = currentDrawCtx.m_renderTargets[slot];
	if (!tex)return;
	Resource* resource = tex->GetResource();
	resource->AddResourceBarrierToList(D3D12_RESOURCE_STATE_RENDER_TARGET, m_pendingRscBarriers);

	currentDrawCtx.SetRenderTargetClear(slot, true);
	ClearTexture(color, tex);
}

void Renderer::ClearDepth(float clearDepth /*= 1.0f*/)
{
	ImmediateContext& currentDrawCtx = GetCurrentDrawCtx();
	Texture* drt = currentDrawCtx.GetDepthRenderTarget();
	ResourceView* dsvView = drt->GetOrCreateView(RESOURCE_BIND_DEPTH_STENCIL_BIT);
	Resource* dsvRsc = drt->GetResource();
	dsvRsc->AddResourceBarrierToList(D3D12_RESOURCE_STATE_DEPTH_WRITE, m_pendingRscBarriers);

	D3D12_CLEAR_FLAGS clearFlags = D3D12_CLEAR_FLAG_DEPTH;
	currentDrawCtx.SetDepthRenderTargetClear(true);
	ID3D12GraphicsCommandList6* defaultCmdList = GetCurrentCommandList(CommandListType::DEFAULT);

	defaultCmdList->ClearDepthStencilView(dsvView->GetHandle(), clearFlags, clearDepth, 0, 0, NULL);
}

Material* Renderer::CreateOrGetMaterial(std::filesystem::path materialPathNoExt)
{
	return g_theMaterialSystem->CreateOrGetMaterial(materialPathNoExt);
}

Texture* Renderer::CreateOrGetTextureFromFile(char const* imageFilePath)
{
	Texture* existingTexture = GetTextureForFileName(imageFilePath);
	if (existingTexture)
	{
		return existingTexture;
	}

	// Never seen this texture before!  Let's load it.
	Texture* newTexture = CreateTextureFromFile(imageFilePath);
	return newTexture;
}


void Renderer::DispatchMesh(unsigned int threadX, unsigned threadY, unsigned int threadZ)
{
	ImmediateContext& ctx = GetCurrentDrawCtx();
	ctx.SetPipelineTypeFlag(true);
	ctx.m_meshThreads = IntVec3((unsigned int)threadX, (unsigned int)threadY, (unsigned int)threadZ); // Cast is fine, it's not possible to dispatch this many threads to worry about
	ctx.m_srvHandleStart = m_srvHandleStart;
	ctx.m_cbvHandleStart = m_cbvHandleStart;

	SetContextDescriptorStarts(ctx);
	ctx.SetDrawCallUsage(true);
	SetModelBufferForCtx(ctx);

	CopyCurrentDrawCtxToNext();
}

void Renderer::DrawVertexBuffer(VertexBuffer* const& vertexBuffer)
{
	ImmediateContext& ctx = GetCurrentDrawCtx();
	BindVertexBuffer(vertexBuffer);
	SetContextDescriptorStarts(ctx);
	ctx.SetDrawCallUsage(true);
	SetModelBufferForCtx(ctx);


	CopyCurrentDrawCtxToNext();
}

void Renderer::DrawIndexedVertexBuffer(VertexBuffer* const& vertexBuffer, IndexBuffer* const& indexBuffer, size_t indexCount)
{
	ImmediateContext& ctx = GetCurrentDrawCtx();
	BindVertexBuffer(vertexBuffer);
	BindIndexBuffer(indexBuffer, indexCount);
	SetContextDescriptorStarts(ctx);
	ctx.SetDrawCallUsage(true);
	SetModelBufferForCtx(ctx);


	CopyCurrentDrawCtxToNext();
}

void Renderer::DrawVertexArray(unsigned int numVertexes, const Vertex_PCU* vertexes)
{
	ImmediateContext& ctx = GetCurrentDrawCtx();

	SetContextDescriptorStarts(ctx);
	SetContextDrawInfo(ctx, numVertexes, vertexes);
	CopyCurrentDrawCtxToNext();
}

void Renderer::DrawVertexArray(std::vector<Vertex_PCU> const& vertexes)
{
	DrawVertexArray((unsigned int)vertexes.size(), vertexes.data());
}

void Renderer::DrawIndexedVertexArray(unsigned int numVertexes, Vertex_PCU const* vertexes, unsigned int numIndices, unsigned int const* indices)
{
	ImmediateContext& ctx = GetCurrentDrawCtx();

	SetContextDescriptorStarts(ctx);
	SetContextIndexedDrawInfo(ctx, numVertexes, vertexes, numIndices, indices);
	CopyCurrentDrawCtxToNext();
}

void Renderer::DrawIndexedVertexArray(std::vector<Vertex_PCU> const& vertexes, std::vector<unsigned int> const& indices)
{
	DrawIndexedVertexArray((unsigned int)vertexes.size(), vertexes.data(), (unsigned int)indices.size(), indices.data());
}

void Renderer::DrawVertexArray(unsigned int numVertexes, const Vertex_PNCU* vertexes)
{
	ImmediateContext& ctx = GetCurrentDrawCtx();
	ctx.SetVertexType(VertexType::PNCU);

	SetContextDescriptorStarts(ctx);
	SetContextDrawInfo(ctx, numVertexes, vertexes);
	CopyCurrentDrawCtxToNext();
}

void Renderer::DrawVertexArray(std::vector<Vertex_PNCU> const& vertexes)
{
	DrawVertexArray((unsigned int)vertexes.size(), vertexes.data());
}

void Renderer::DrawIndexedVertexArray(unsigned int numVertexes, const Vertex_PNCU* vertexes, unsigned int numIndices, unsigned int const* indices)
{
	ImmediateContext& ctx = GetCurrentDrawCtx();

	ctx.SetVertexType(VertexType::PNCU);
	SetContextDescriptorStarts(ctx);
	SetContextIndexedDrawInfo(ctx, numVertexes, vertexes, numIndices, indices);
	CopyCurrentDrawCtxToNext();
}

void Renderer::DrawIndexedVertexArray(std::vector<Vertex_PNCU> const& vertexes, std::vector<unsigned int> const& indices)
{
	DrawIndexedVertexArray((unsigned int)vertexes.size(), vertexes.data(), (unsigned int)indices.size(), indices.data());
}


void Renderer::SetModelMatrix(Mat44 const& modelMat)
{
	ImmediateContext& ctx = GetCurrentDrawCtx();
	ctx.m_modelConstants.ModelMatrix = modelMat;
	m_isModelBufferDirty = true;
}

void Renderer::SetModelColor(Rgba8 const& modelColor)
{
	ImmediateContext& ctx = GetCurrentDrawCtx();
	modelColor.GetAsFloats(ctx.m_modelConstants.ModelColor);
	m_isModelBufferDirty = true;
}

BitmapFont* Renderer::CreateBitmapFont(std::filesystem::path bitmapPath)
{
	std::string filename = bitmapPath.string();
	std::string filePathString = bitmapPath.replace_extension(".png").string();
	Texture* bitmapTexture = CreateOrGetTextureFromFile(filePathString.c_str());
	BitmapFont* newBitmapFont = new BitmapFont(filename.c_str(), *bitmapTexture);

	m_loadedFonts.push_back(newBitmapFont);
	return newBitmapFont;
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
	for (int loadedFontIndex = 0; loadedFontIndex < m_loadedFonts.size(); loadedFontIndex++) {
		BitmapFont*& bitmapFont = m_loadedFonts[loadedFontIndex];
		if (bitmapFont->m_fontFilePathNameWithNoExtension == bitmapPath.string()) {
			return bitmapFont;
		}
	}

	return CreateBitmapFont(bitmapPath);

}

Material* Renderer::GetMaterialForName(char const* materialName)
{
	return g_theMaterialSystem->GetMaterialForName(materialName);

}

Material* Renderer::GetMaterialForPath(std::filesystem::path const& materialPath)
{
	return g_theMaterialSystem->GetMaterialForPath(materialPath);

}

Material* Renderer::GetDefaultMaterial(bool isUsing3D) const
{
	if (isUsing3D) return m_default3DMaterial;
	return m_default2DMaterial;
}

void Renderer::BindConstantBuffer(ConstantBuffer* cBuffer, unsigned int slot /*= 0*/)
{
	ImmediateContext& ctx = GetCurrentDrawCtx();
	ctx.m_boundCBuffers[slot] = cBuffer;
}

void Renderer::BindTexture(Texture const* texture, unsigned int slot /*= 0*/)
{
	ImmediateContext& currentDrawCtx = GetCurrentDrawCtx();

	currentDrawCtx.m_boundTextures[slot] = texture;
	if (!texture) {
		if (slot == 0) {
			currentDrawCtx.m_boundTextures[slot] = m_defaultTexture;
		}
		else {
			currentDrawCtx.m_boundTextures.erase(slot);
		}
	}
}

void Renderer::BindMaterial(Material* mat)
{
	ImmediateContext& currentDrawCtx = GetCurrentDrawCtx();

	if (!mat) mat = GetDefaultMaterial(m_is3DDefault);
	currentDrawCtx.m_material = mat;
}

void Renderer::BindMaterialByName(char const* materialName)
{
	Material* material = g_theMaterialSystem->GetMaterialForName(materialName);
	if (!material) material = GetDefaultMaterial();

	BindMaterial(material);
}

void Renderer::BindMaterialByPath(std::filesystem::path materialPath)
{
	materialPath.replace_extension("xml");
	Material* material = g_theMaterialSystem->GetMaterialForPath(materialPath);
	if (!material) material = GetDefaultMaterial();

	BindMaterial(material);
}

void Renderer::BindVertexBuffer(VertexBuffer* const& vertexBuffer)
{
	ImmediateContext& currentDrawCtx = GetCurrentDrawCtx();

	currentDrawCtx.m_externalVBO = vertexBuffer;
	currentDrawCtx.m_vertexStart = 0;
	currentDrawCtx.m_vertexCount = (vertexBuffer->GetSize()) / vertexBuffer->GetStride();
}

void Renderer::BindIndexBuffer(IndexBuffer* const& indexBuffer, size_t indexCount)
{
	ImmediateContext& currentDrawCtx = GetCurrentDrawCtx();

	currentDrawCtx.m_externalIBO = indexBuffer;
	currentDrawCtx.m_indexStart = 0;
	currentDrawCtx.m_indexCount = indexCount;
	currentDrawCtx.SetIndexDrawFlag(true);
}

void Renderer::BindStructuredBuffer(Buffer* const& buffer, unsigned int slot)
{
	ImmediateContext& currentDrawCtx = GetCurrentDrawCtx();

	currentDrawCtx.m_boundBuffers[slot] = buffer;
}

void Renderer::SetRenderTarget(Texture* dst, unsigned int slot /*= 0*/)
{
	if (slot > 7) {
		ERROR_RECOVERABLE("TRYING TO SET RENDER TARGET INDEX > 8");
		return;
	}
	else if (slot < 0) {
		ERROR_RECOVERABLE("TRYING TO SET RENDER TARGET INDEX < 0");
		return;
	}

	ImmediateContext& currentDrawCtx = GetCurrentDrawCtx();
	currentDrawCtx.m_renderTargets[slot] = dst;

	if (dst) {
		Resource* rtRsc = dst->GetResource();
		rtRsc->AddResourceBarrierToList(D3D12_RESOURCE_STATE_RENDER_TARGET, m_pendingRscBarriers);
	}
}


void Renderer::SetDepthRenderTarget(Texture* dst)
{
	if (!dst) dst = m_defaultDepthTarget;
	bool isDSVCompatible = dst->IsBindCompatible(RESOURCE_BIND_DEPTH_STENCIL_BIT);
	GUARANTEE_OR_DIE(isDSVCompatible, "TEXTURE IS NOT DSV COMPATIBLE");
	ImmediateContext& ctx = GetCurrentDrawCtx();
	ctx.SetDepthRenderTarget(dst);
}

void Renderer::SetBlendMode(BlendMode newBlendMode)
{
	ImmediateContext& currentDrawCtx = GetCurrentDrawCtx();

	Material* currentMaterial = (currentDrawCtx.m_material) ? currentDrawCtx.m_material : GetDefaultMaterial();
	Material* siblingMat = g_theMaterialSystem->GetSiblingMaterial(currentMaterial, SiblingMatTypes::BLEND_MODE_SIBLING, (unsigned int)newBlendMode);
	currentDrawCtx.m_material = siblingMat;
}

void Renderer::SetCullMode(CullMode newCullMode)
{
	ImmediateContext& currentDrawCtx = GetCurrentDrawCtx();

	Material* currentMaterial = (currentDrawCtx.m_material) ? currentDrawCtx.m_material : GetDefaultMaterial();
	Material* siblingMat = g_theMaterialSystem->GetSiblingMaterial(currentMaterial, SiblingMatTypes::CULL_MODE_SIBLING, (unsigned int)newCullMode);
	currentDrawCtx.m_material = siblingMat;
}

void Renderer::SetFillMode(FillMode newFillMode)
{
	ImmediateContext& currentDrawCtx = GetCurrentDrawCtx();

	Material* currentMaterial = (currentDrawCtx.m_material) ? currentDrawCtx.m_material : GetDefaultMaterial();
	Material* siblingMat = g_theMaterialSystem->GetSiblingMaterial(currentMaterial, SiblingMatTypes::FILL_MODE_SIBLING, (unsigned int)newFillMode);
	currentDrawCtx.m_material = siblingMat;
}

void Renderer::SetWindingOrder(WindingOrder newWindingOrder)
{
	ImmediateContext& currentDrawCtx = GetCurrentDrawCtx();

	Material* currentMaterial = (currentDrawCtx.m_material) ? currentDrawCtx.m_material : GetDefaultMaterial();
	Material* siblingMat = g_theMaterialSystem->GetSiblingMaterial(currentMaterial, SiblingMatTypes::WINDING_ORDER_SIBLING, (unsigned int)newWindingOrder);
	currentDrawCtx.m_material = siblingMat;
}

void Renderer::SetRasterizerState(CullMode cullMode, FillMode fillMode, WindingOrder windingOrder)
{
	SetCullMode(cullMode);
	SetFillMode(fillMode);
	SetWindingOrder(windingOrder);
}

void Renderer::SetDepthFunction(DepthFunc newDepthFunc)
{
	ImmediateContext& currentDrawCtx = GetCurrentDrawCtx();

	Material* currentMaterial = (currentDrawCtx.m_material) ? currentDrawCtx.m_material : GetDefaultMaterial();
	Material* siblingMat = g_theMaterialSystem->GetSiblingMaterial(currentMaterial, SiblingMatTypes::DEPTH_FUNC_SIBLING, (unsigned int)newDepthFunc);
	currentDrawCtx.m_material = siblingMat;
}

void Renderer::SetWriteDepth(bool writeDepth)
{
	ImmediateContext& currentDrawCtx = GetCurrentDrawCtx();

	Material* currentMaterial = (currentDrawCtx.m_material) ? currentDrawCtx.m_material : GetDefaultMaterial();
	Material* siblingMat = g_theMaterialSystem->GetSiblingMaterial(currentMaterial, SiblingMatTypes::DEPTH_ENABLING_SIBLING, (unsigned int)writeDepth);
	currentDrawCtx.m_material = siblingMat;
}

void Renderer::SetDepthStencilState(DepthFunc newDepthFunc, bool writeDepth)
{
	SetDepthFunction(newDepthFunc);
	SetWriteDepth(writeDepth);
}

void Renderer::SetTopology(TopologyType newTopologyType)
{
	ImmediateContext& currentDrawCtx = GetCurrentDrawCtx();

	Material* currentMaterial = (currentDrawCtx.m_material) ? currentDrawCtx.m_material : GetDefaultMaterial();
	Material* siblingMat = g_theMaterialSystem->GetSiblingMaterial(currentMaterial, SiblingMatTypes::TOPOLOGY_SIBLING, (unsigned int)newTopologyType);
	currentDrawCtx.m_material = siblingMat;
}

void Renderer::SetSamplerMode(SamplerMode samplerMode)
{
	D3D12_SAMPLER_DESC samplerDesc = {};
	switch (samplerMode)
	{
	case SamplerMode::POINTCLAMP:
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

		break;
	case SamplerMode::POINTWRAP:
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

		break;
	case SamplerMode::BILINEARCLAMP:
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

		break;
	case SamplerMode::BILINEARWRAP:
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		break;

	case SamplerMode::SHADOWMAPS:
		samplerDesc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		samplerDesc.BorderColor[0] = 0.0f;
		samplerDesc.BorderColor[1] = 0.0f;
		samplerDesc.BorderColor[2] = 0.0f;
		samplerDesc.BorderColor[3] = 0.0f;

		break;
	default:
		break;
	}
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	DescriptorHeap* samplerHeap = GetCPUDescriptorHeap(DescriptorHeapType::SAMPLER);

	m_device->CreateSampler(&samplerDesc, samplerHeap->GetHandleAtOffset(0));
	DescriptorHeap* gpuSamplerHeap = GetGPUDescriptorHeap(DescriptorHeapType::SAMPLER);
	m_device->CopyDescriptorsSimple(1, gpuSamplerHeap->GetCPUHandleForHeapStart(), samplerHeap->GetHandleAtOffset(0), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
}

void Renderer::SetDirectionalLight(Vec3 const& direction)
{
	m_directionalLight = direction;
}

void Renderer::SetDirectionalLightIntensity(Rgba8 const& intensity)
{
	m_directionalLightIntensity = intensity;
}

void Renderer::SetAmbientIntensity(Rgba8 const& intensity)
{
	m_ambientIntensity = intensity;
}


bool Renderer::SetLight(Light const& light, int index)
{
	if (index < MAX_LIGHTS) {
		EulerAngles eulerAngle = EulerAngles::CreateEulerAngleFromForward(light.Direction);
		Mat44 viewMatrix = eulerAngle.GetMatrix_XFwd_YLeft_ZUp();
		Vec3 fwd = light.Direction.GetNormalized();
		Vec3 translation = Vec3::ZERO;

		// This is to work with shadow maps, taken of the Real-Time shadows book
		translation.x = -DotProduct3D(light.Position, viewMatrix.GetIBasis3D());
		translation.y = -DotProduct3D(light.Position, viewMatrix.GetJBasis3D());
		translation.z = -DotProduct3D(light.Position, viewMatrix.GetKBasis3D());

		viewMatrix = viewMatrix.GetOrthonormalInverse();
		viewMatrix.SetTranslation3D(translation);

		Mat44 projectionMatrix = Mat44::CreatePerspectiveProjection(light.SpotAngle, 2.0f, 0.01f, 100.0f);
		projectionMatrix.Append(m_lightRenderTransform);

		Light& assignedSlot = m_lights[index];
		assignedSlot = light;
		assignedSlot.ViewMatrix = viewMatrix;
		assignedSlot.ProjectionMatrix = projectionMatrix;

		return true;
	}
	return false;
}

void Renderer::BindLightConstants()
{
	LightConstants lightConstants = {};
	lightConstants.DirectionalLight = m_directionalLight;
	m_directionalLightIntensity.GetAsFloats(lightConstants.DirectionalLightIntensity);
	m_ambientIntensity.GetAsFloats(lightConstants.AmbientIntensity);

	//lightConstants.MaxOcclusionPerSample = m_SSAOMaxOcclusionPerSample;
	//lightConstants.SampleRadius = m_SSAOSampleRadius;
	//lightConstants.SampleSize = m_SSAOSampleSize;
	//lightConstants.SSAOFalloff = m_SSAOFalloff;

	memcpy(&lightConstants.Lights, &m_lights, sizeof(m_lights));

	ConstantBuffer* lightBuffer = GetNextCBufferSlot(ConstantBufferType::LIGHT);

	lightBuffer->CopyCPUToGPU(&lightConstants, sizeof(lightConstants));

	BindConstantBuffer(lightBuffer, g_lightBufferSlot);
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

void Renderer::AddToUpdateQueue(Buffer* bufferToUpdate)
{
	// Add to state tracking vector
	m_pendingRscCopy.insert(bufferToUpdate);
}

Texture* Renderer::GetCurrentRenderTarget() const
{
	if (!m_currentCamera) return GetActiveRenderTarget();
	Texture* cameraColorTarget = m_currentCamera->GetRenderTarget();
	return  (cameraColorTarget) ? cameraColorTarget : GetActiveRenderTarget();
}

Texture* Renderer::GetCurrentDepthTarget() const
{
	if (!m_currentCamera) return m_defaultDepthTarget;
	Texture* cameraDepthTarget = m_currentCamera->GetDepthTarget();
	return  (cameraDepthTarget) ? cameraDepthTarget : m_defaultDepthTarget;
}

void Renderer::ApplyEffect(Material* effect, Camera const* camera /*= nullptr*/, Texture* customDepth /*= nullptr*/)
{
	FxContext newFxCtx = {};

	CameraConstants cameraConstants = {};
	if (camera) {
		cameraConstants.ProjectionMatrix = camera->GetProjectionMatrix();
		cameraConstants.ViewMatrix = camera->GetViewMatrix();
		cameraConstants.InvertedMatrix = camera->GetProjectionMatrix().GetInverted();
	}

	newFxCtx.m_cameraCBO = GetNextCBufferSlot(ConstantBufferType::CAMERA);
	newFxCtx.m_cameraCBO->CopyCPUToGPU(&cameraConstants, sizeof(CameraConstants));

	newFxCtx.m_fx = effect;
	newFxCtx.m_depthTarget = (customDepth) ? customDepth : m_defaultDepthTarget;

	m_effectsContexts.push_back(newFxCtx);
}

void Renderer::CopyTextureWithMaterial(Texture* dst, Texture* src, Texture* depthBuffer, Material* effect)
{
	Resource* srcResource = src->GetResource();
	Resource* dstResource = dst->GetResource();
	Resource* dsvResource = depthBuffer->GetResource();

	std::vector<D3D12_RESOURCE_BARRIER> rscBarriers;
	rscBarriers.reserve(3);
	srcResource->AddResourceBarrierToList(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, rscBarriers);
	dsvResource->AddResourceBarrierToList(D3D12_RESOURCE_STATE_DEPTH_READ, rscBarriers);
	dstResource->AddResourceBarrierToList(D3D12_RESOURCE_STATE_RENDER_TARGET, rscBarriers);

	ID3D12GraphicsCommandList6* defaultCmdList = GetCurrentCommandList(CommandListType::DEFAULT);
	defaultCmdList->ResourceBarrier((unsigned int)rscBarriers.size(), rscBarriers.data());

	ClearTexture(Rgba8(0, 0, 0, 255), dst);

	BindMaterial(effect);

	SetSamplerMode(SamplerMode::BILINEARCLAMP);

	CopyTextureToDescriptorHeap(src, m_srvHandleStart, 0);
	CopyTextureToDescriptorHeap(depthBuffer, m_srvHandleStart, 1);

	DescriptorHeap* srvUAVCBVHeap = GetGPUDescriptorHeap(DescriptorHeapType::SRV_UAV_CBV);
	DescriptorHeap* samplerHeap = GetGPUDescriptorHeap(DescriptorHeapType::SAMPLER);
	ID3D12DescriptorHeap* allDescriptorHeaps[] = {
		srvUAVCBVHeap->GetHeap(),
		samplerHeap->GetHeap()
	};

	UINT numHeaps = sizeof(allDescriptorHeaps) / sizeof(ID3D12DescriptorHeap*);

	defaultCmdList->SetPipelineState(effect->m_PSO);
	defaultCmdList->SetGraphicsRootSignature(m_defaultRootSignature.Get());
	defaultCmdList->SetDescriptorHeaps(numHeaps, allDescriptorHeaps);


	defaultCmdList->SetGraphicsRootDescriptorTable(0, srvUAVCBVHeap->GetGPUHandleAtOffset(m_cbvHandleStart));
	defaultCmdList->SetGraphicsRootDescriptorTable(1, srvUAVCBVHeap->GetGPUHandleAtOffset(m_srvHandleStart));
	defaultCmdList->SetGraphicsRootDescriptorTable(2, srvUAVCBVHeap->GetGPUHandleAtOffset(UAV_HANDLE_START));
	defaultCmdList->SetGraphicsRootDescriptorTable(3, samplerHeap->GetGPUHandleForHeapStart());

	m_cbvHandleStart += 1;
	m_srvHandleStart += 2;


	defaultCmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	D3D12_VIEWPORT viewport = {};
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<float>(m_config.m_window->GetClientDimensions().x);
	viewport.Height = static_cast<float>(m_config.m_window->GetClientDimensions().y);
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1;

	D3D12_RECT scissorRect = {};
	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = static_cast<long>(m_config.m_window->GetClientDimensions().x);
	scissorRect.bottom = static_cast<long>(m_config.m_window->GetClientDimensions().y);

	D3D12_CPU_DESCRIPTOR_HANDLE rtHandle = dst->GetOrCreateView(RESOURCE_BIND_RENDER_TARGET_BIT)->GetHandle();
	D3D12_CPU_DESCRIPTOR_HANDLE dstHandle = depthBuffer->GetOrCreateView(RESOURCE_BIND_DEPTH_STENCIL_BIT)->GetHandle();

	defaultCmdList->RSSetViewports(1, &viewport);
	defaultCmdList->RSSetScissorRects(1, &scissorRect);
	defaultCmdList->OMSetRenderTargets(1, &rtHandle, FALSE, &dstHandle);

	defaultCmdList->DrawInstanced(3, 1, 0, 0);

	Material* defaultMat = GetDefaultMaterial();

	defaultCmdList->ClearState(defaultMat->m_PSO);
}

ID3D12CommandAllocator* Renderer::GetCommandAllocForCmdList(CommandListType cmdListType)
{
	size_t accessIndex = ((size_t)m_currentBackBuffer * 2) + size_t(cmdListType);
	return m_commandAllocators[accessIndex];
}

ID3D12GraphicsCommandList6* Renderer::GetCurrentCommandList(CommandListType cmdListType)
{
	size_t accessIndex = ((size_t)m_currentBackBuffer * 2) + size_t(cmdListType);
	return m_commandLists[accessIndex];
}

void Renderer::ResetGPUState()
{
	/*m_fence->Signal();
	m_fence->Wait();

	m_resourcesFence->Signal();
	m_resourcesFence->Wait();*/

	// Command list allocators can only be reset when the associated 
	// command lists have finished execution on the GPU
	m_currentCameraBufferSlot = 0;
	m_currentModelBufferSlot = 0;
	m_currentLightBufferSlot = 0;

	m_currentDrawCtx = 0;
	m_srvHandleStart = SRV_HANDLE_START;
	m_cbvHandleStart = CBV_HANDLE_START;

	auto cmdAllocator = GetCommandAllocForCmdList(CommandListType::DEFAULT);
	ID3D12GraphicsCommandList6* defaultCmdList = GetCurrentCommandList(CommandListType::DEFAULT);
	ThrowIfFailed(cmdAllocator->Reset(), "FAILED TO RESET COMMAND ALLOCATOR");
	ThrowIfFailed(defaultCmdList->Reset(cmdAllocator, nullptr), "COULD NOT RESET COMMAND LIST");

	auto rscCmdAlloc = GetCommandAllocForCmdList(CommandListType::RESOURCES);
	ID3D12GraphicsCommandList6* rscCmdList = GetCurrentCommandList(CommandListType::RESOURCES);
	ThrowIfFailed(rscCmdAlloc->Reset(), "FAILED TO RESET RESOURCES COMMAND ALLOCATOR");
	ThrowIfFailed(rscCmdList->Reset(rscCmdAlloc, nullptr), "COULD NOT RESET RESOURCES COMMAND LIST");

	m_immediateVertexes.clear();
	m_immediateDiffuseVertexes.clear();
	m_immediateIndices.clear();
	m_immediateDiffuseIndices.clear();
	m_pendingRscBarriers.clear();
	m_pendingRscCopy.clear();
	m_effectsContexts.clear();

	m_isModelBufferDirty = false;
}

Texture* Renderer::GetDefaultRenderTarget() const
{
	return GetActiveRenderTarget();
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
	ID3D12GraphicsCommandList6* defaultCmdList = GetCurrentCommandList(CommandListType::DEFAULT);

	ImGui::Render();
	defaultCmdList->OMSetRenderTargets(1, &currentRTVHandle, FALSE, nullptr);
	defaultCmdList->SetDescriptorHeaps(1, &m_ImGuiSrvDescHeap);

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), defaultCmdList);
#endif
}




