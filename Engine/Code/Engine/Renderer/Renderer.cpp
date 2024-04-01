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
#include "Engine/Renderer/D3D12/Resource.hpp"
#include "Engine/Renderer/GraphicsCommon.hpp"
#include "Engine/Renderer/D3D12/D3D12TypeConversions.hpp"
#include "Engine/Window/Window.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/Vec4.hpp"
#include "Engine/Renderer/DebugRendererSystem.hpp"
#include <dxgidebug.h>
#include <d3dx12.h> // Notice the X. These are the helper structures not the DX12 header
#include <regex>
#include <ThirdParty/ImGUI/imgui.h>
#include <ThirdParty/ImGUI/imgui_impl_win32.h>
#include <ThirdParty/ImGUI/imgui_impl_dx12.h>

#pragma message("ENGINE_DIR == " ENGINE_DIR)

bool is3DDefault = true;
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

void Renderer::EnableDebugLayer() const
{
#if defined(_DEBUG)
	ID3D12Debug3* debugController = nullptr;

	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		debugController->EnableDebugLayer();
		debugController->SetEnableGPUBasedValidation(TRUE);

		debugController->Release();
	}
	else {
		ERROR_AND_DIE("COULD NOT ENABLE DX12 DEBUG LAYER");
	}

#endif
}

void Renderer::CreateDXGIFactory()
{
	UINT factoryFlags = 0;
	// Create DXGI Factory to create DXGI Objects

#if defined(_DEBUG)
	factoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif


	HRESULT factoryCreation = CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&m_dxgiFactory));

	SetDebugName(m_dxgiFactory, "DXGIFACTORY");

	if (!SUCCEEDED(factoryCreation)) {
		ERROR_AND_DIE("COULD NOT CREATE DXGI FACTORY");
	}

}

ComPtr<IDXGIAdapter4> Renderer::GetAdapter()
{
	ComPtr<IDXGIAdapter4> adapter = nullptr;
	ComPtr<IDXGIAdapter1> adapter1;

	if (m_useWARP) {

		HRESULT enumWarpAdapter = m_dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&adapter));

		if (!SUCCEEDED(enumWarpAdapter)) {
			ERROR_AND_DIE("COULD NOT GET WARP ADAPTER");
		}
	}
	else {
		ComPtr<IDXGIFactory6> factory6;
		HRESULT transformedToFactory6 = m_dxgiFactory.As(&factory6);

		// Prefer dedicated GPU
		if (SUCCEEDED(transformedToFactory6)) {
			for (int adapterIndex = 0;
				factory6->EnumAdapterByGpuPreference(adapterIndex, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
					IID_PPV_ARGS(adapter1.ReleaseAndGetAddressOf())) != DXGI_ERROR_NOT_FOUND;
				adapterIndex++) {
				DXGI_ADAPTER_DESC1 desc;
				adapter1->GetDesc1(&desc);

				if (!(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) {
					break;
				}

			}

		}
		else {
			for (int adapterIndex = 0;
				m_dxgiFactory->EnumAdapters1(adapterIndex, adapter1.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND;
				adapterIndex++) {
				DXGI_ADAPTER_DESC1 desc;
				adapter1->GetDesc1(&desc);

				if (!(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) {
					break;
				}
			}
		}

		factory6.Reset();
	}

	HRESULT castToAdapter4 = adapter1.As(&adapter);
	if (!SUCCEEDED(castToAdapter4)) {
		ERROR_AND_DIE("COULD NOT CAST ADAPTER1 TO ADAPTER4");
	}

	adapter1.Reset();
	return adapter;
}



void Renderer::CreateDevice(ComPtr<IDXGIAdapter4> adapter)
{
	HRESULT deviceCreationResult = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_device));

	SetDebugName(m_device, "Device");
	if (!SUCCEEDED(deviceCreationResult)) {
		ERROR_AND_DIE("COULD NOT CREATE DIRECTX12 DEVICE");
	}

#if defined(_DEBUG)
	ComPtr<ID3D12InfoQueue> infoQueue;
	if (SUCCEEDED(m_device.As(&infoQueue))) {
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
		infoQueue.Reset();
	}
	else {
		ERROR_AND_DIE("COULD NOT SET MESSAGE SEVERITIES DX12 FOR DEBUG PURPORSES");
	}
#endif
}

void Renderer::CreateCommandQueue(D3D12_COMMAND_LIST_TYPE type)
{
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = type;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;

	HRESULT queueCreation = m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_commandQueue));
	SetDebugName(m_commandQueue, "COMMANDQUEUE");
	if (!SUCCEEDED(queueCreation)) {
		ERROR_AND_DIE("COULD NOT CREATE COMMAND QUEUE");
	}
}

bool Renderer::HasTearingSupport()
{
	// Querying if there is support for Variable refresh rate 
	ComPtr<IDXGIFactory4> dxgiFactory;
	BOOL allowTearing = FALSE;
	if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)))) {
		ComPtr<IDXGIFactory5> dxgiFactory5;
		if (SUCCEEDED(dxgiFactory.As(&dxgiFactory5))) {
			dxgiFactory.Reset();
			if (SUCCEEDED(dxgiFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing)))) {
				dxgiFactory5.Reset();
				return allowTearing == TRUE;
			}
		}
	}
	else {
		ERROR_AND_DIE("COULD NOT CREATE DXGI FACTORY FOR TEARING SUPPORT");
	}

	return false;
}

void Renderer::CreateSwapChain()
{
	Window* window = Window::GetWindowContext();
	IntVec2 windowDimensions = window->GetClientDimensions();
	RECT clientRect = {};
	HWND windowHandle = (HWND)window->m_osWindowHandle;
	::GetClientRect(windowHandle, &clientRect);

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = clientRect.right;
	swapChainDesc.Height = clientRect.bottom;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.SampleDesc = { 1, 0 }; // #ToDo Check for reenabling MSAA
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = (UINT)m_config.m_backBuffersCount;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

	swapChainDesc.Flags = (HasTearingSupport()) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	ComPtr<IDXGISwapChain1> swapChain1;

	HRESULT swapChainCreationResult = m_dxgiFactory->CreateSwapChainForHwnd(m_commandQueue.Get(),
		windowHandle,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain1);

	SetDebugName(swapChain1, "SwapChain1");
	if (!SUCCEEDED(swapChainCreationResult)) {
		ERROR_AND_DIE("COULD NOT CREATE SWAPCHAIN1");
	}

	if (!SUCCEEDED(swapChain1.As(&m_swapChain))) {
		ERROR_AND_DIE("COULD NOT CONVERT SWAPCHAIN1 TO SWAPCHAIN4");
	}

	swapChain1.Reset();

}

void Renderer::CreateDescriptorHeap(ID3D12DescriptorHeap*& descriptorHeap, D3D12_DESCRIPTOR_HEAP_TYPE type, unsigned int numDescriptors, bool visibleFromGPU /*=false*/)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = numDescriptors;
	desc.Type = type;
	if (visibleFromGPU) {
		desc.Flags |= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	}

	HRESULT descHeapCreation = m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap));
	SetDebugName(descriptorHeap, "DescriptorHeap");

	if (!SUCCEEDED(descHeapCreation)) {
		ERROR_AND_DIE("FAILED TO CREATE DESCRIPTOR HEAP");
	}
}

void Renderer::CreateRenderTargetViewsForBackBuffers()
{
	// Handle size is vendor specific
	m_RTVdescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
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
		DX_SAFE_RELEASE(bufferTex);
		//m_device->CreateRenderTargetView(bufferTex.Get(), nullptr, rtvHandle);
		//rtvHandle.Offset(m_RTVdescriptorSize);

	}



}

void Renderer::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE type, ComPtr<ID3D12CommandAllocator>& commandAllocator)
{
	HRESULT commAllocatorCreation = m_device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator));
	SetDebugName(commandAllocator, "CommandAllocator");
	if (!SUCCEEDED(commAllocatorCreation)) {
		ERROR_AND_DIE("COULD NOT CREATE COMMAND ALLOCATOR");
	}
}

void Renderer::CreateCommandList(ComPtr<ID3D12GraphicsCommandList6>& commList, D3D12_COMMAND_LIST_TYPE type, ComPtr<ID3D12CommandAllocator>const& commandAllocator)
{
	HRESULT commListCreation = m_device->CreateCommandList(0, type, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commList));
	SetDebugName(commList, "COMMANDLIST");
	if (!SUCCEEDED(commListCreation)) {
		ERROR_AND_DIE("COULD NOT CREATE COMMAND LIST");
	}
}

void Renderer::CreateFence(ComPtr<ID3D12Fence1>& fence, char const* debugName)
{
	HRESULT fenceCreation = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	SetDebugName(fence, debugName);
	if (!SUCCEEDED(fenceCreation)) {
		ERROR_AND_DIE(Stringf("COULD NOT CREATE FENCE: %s", debugName).c_str());
	}

	//fenceCreation = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_syncFence));
	//SetDebugName(m_syncFence, "END_FRAME_FENCE");
	//if (!SUCCEEDED(fenceCreation)) {
	//	ERROR_AND_DIE("COULD NOT CREATE SYNC_FENCE");
	//}
}

void Renderer::CreateFenceEvent()
{
	m_fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
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
	rootParameters[3].InitAsDescriptorTable(1, &descriptorRanges[3], D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignature(_countof(descriptorRanges), rootParameters);
	rootSignature.Desc_1_2.Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignature.Desc_1_2.Flags |= D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;
	rootSignature.Desc_1_2.Flags |= D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;

	//rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	//ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error), "COULD NOT SERIALIZE ROOT SIGNATURE");
	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;

	HRESULT rootSignatureSerialization = D3D12SerializeVersionedRootSignature(&rootSignature, signature.GetAddressOf(), error.GetAddressOf());
	ThrowIfFailed(rootSignatureSerialization, "COULD NOT SERIALIZE ROOT SIGNATURE");
	ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)), "COULD NOT CREATE ROOT SIGNATURE");
	SetDebugName(m_rootSignature, "DEFAULTROOTSIGNATURE");
}

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

unsigned int Renderer::SignalFence(ComPtr<ID3D12CommandQueue>& commandQueue, ComPtr<ID3D12Fence1> fence, unsigned int& fenceValue)
{
	HRESULT fenceSignal = commandQueue->Signal(fence.Get(), fenceValue);

	if (!SUCCEEDED(fenceSignal)) {
		ERROR_AND_DIE("FENCE SIGNALING FAILED");
	}

	return fenceValue + 1;

}

void Renderer::SignalFence(ComPtr<ID3D12Fence1>& fence, unsigned int fenceValue)
{
	SignalFence(m_commandQueue, fence, fenceValue);
}

void Renderer::WaitForFenceValue(ComPtr<ID3D12Fence1>& fence, unsigned int fenceValue, HANDLE fenceEvent)
{
	UINT64 completedValue = fence->GetCompletedValue();
	if (completedValue < fenceValue) {
		HRESULT eventOnCompletion = fence->SetEventOnCompletion(fenceValue, fenceEvent);
		if (!SUCCEEDED(eventOnCompletion)) {
			ERROR_AND_DIE("FAILED TO SET EVENT ON COMPLETION FOR FENCE");
		}
		::WaitForSingleObjectEx(fenceEvent, INFINITE, FALSE);
	}
}

void Renderer::Flush(ComPtr<ID3D12CommandQueue>& commandQueue, ComPtr<ID3D12Fence1> fence, unsigned int* fenceValues, HANDLE fenceEvent)
{
	unsigned int currentValue = fenceValues[m_currentBackBuffer];
	unsigned int newFenceValue = SignalFence(commandQueue, fence, currentValue);

	m_currentBackBuffer = m_swapChain->GetCurrentBackBufferIndex();
	//unsigned int waitOnValue = m_fenceValues[m_currentBackBuffer];
	WaitForFenceValue(fence, m_fenceValues[m_currentBackBuffer], fenceEvent);

	m_fenceValues[m_currentBackBuffer] = newFenceValue;
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
#if defined(GAME_2D)
	is3DDefault = false;
#endif

	LoadEngineShaderBinaries();

	m_fenceValues.resize(m_config.m_backBuffersCount);
	// Enable Debug Layer before initializing any DX12 object
	EnableDebugLayer();
	CreateViewport();
	CreateDXGIFactory();
	ComPtr<IDXGIAdapter4> adapter = GetAdapter();
	CreateDevice(adapter);
	adapter.Reset();
	CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	CreateSwapChain();
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
	m_defaultGPUDescriptorHeaps.resize(2);

	// These are limited by the root signature
	m_defaultGPUDescriptorHeaps[(size_t)DescriptorHeapType::SRV_UAV_CBV] = new DescriptorHeap(this, DescriptorHeapType::SRV_UAV_CBV, SRV_UAV_CBV_DEFAULT_SIZE, true);
	m_defaultGPUDescriptorHeaps[(size_t)DescriptorHeapType::SAMPLER] = new DescriptorHeap(this, DescriptorHeapType::SAMPLER, 64, true);

	m_defaultDescriptorHeaps.resize((size_t)DescriptorHeapType::NUM_DESCRIPTOR_HEAPS);

	/// <summary>
	/// Recommendation here is to have a large pool of descriptors and use them ring buffer style
	/// </summary>
	m_defaultDescriptorHeaps[(size_t)DescriptorHeapType::SRV_UAV_CBV] = new DescriptorHeap(this, DescriptorHeapType::SRV_UAV_CBV, SRV_UAV_CBV_DEFAULT_SIZE);
	m_defaultDescriptorHeaps[(size_t)DescriptorHeapType::SAMPLER] = new DescriptorHeap(this, DescriptorHeapType::SAMPLER, 64);
	m_defaultDescriptorHeaps[(size_t)DescriptorHeapType::RTV] = new DescriptorHeap(this, DescriptorHeapType::RTV, 1024);
	m_defaultDescriptorHeaps[(size_t)DescriptorHeapType::DSV] = new DescriptorHeap(this, DescriptorHeapType::DSV, 8);


	CreateRenderTargetViewsForBackBuffers();
	CreateDefaultTextureTargets();

	size_t numCommandLists = (size_t)CommandListType::NUM_COMMAND_LISTS;

	m_commandLists.resize(numCommandLists);

	for (unsigned int frameIndex = 0; frameIndex < m_config.m_backBuffersCount; frameIndex++) {
		auto& commandAllocators = m_commandAllocators[frameIndex];
		commandAllocators.resize(numCommandLists);
		// 2 for frames
		// 1 for resources
		for (unsigned int rscIndex = 0; rscIndex < numCommandLists; rscIndex++) {
			CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[rscIndex]);
		}
	}
	auto& currentCommandAllocs = m_commandAllocators[m_currentBackBuffer];

	for (unsigned int rscIndex = 0; rscIndex < m_commandLists.size(); rscIndex++) {
		CreateCommandList(m_commandLists[rscIndex], D3D12_COMMAND_LIST_TYPE_DIRECT, currentCommandAllocs[rscIndex]);
	}

	CreateDefaultRootSignature();

	//std::string default2DMatPath = ENGINE_MAT_DIR;
	//default2DMatPath += "Default2DMaterial.xml";
	//m_default2DMaterial = CreateMaterial(default2DMatPath);

	//std::string default3DMatPath = ENGINE_MAT_DIR;
	//default3DMatPath += "Default3DMaterial.xml";
	//m_default3DMaterial = CreateMaterial(default3DMatPath);	

	m_commandList = m_commandLists[(unsigned int)CommandListType::DefaultCommandList];
	m_ResourcesCommandList = m_commandLists[(unsigned int)CommandListType::ResourcesCommandList];

	ThrowIfFailed(m_commandList->Close(), "COULD NOT CLOSE DEFAULT COMMAND LIST");
	ThrowIfFailed(m_ResourcesCommandList->Close(), "COULT NOT CLOSE INTERNAL BUFFER COMMAND LIST");

	CreateFence(m_fence, "END_FRAME_FENCE");

	m_fenceValues[m_currentBackBuffer]++;
	CreateFenceEvent();
	SetSamplerMode(SamplerMode::POINTCLAMP);

	m_commandList->Reset(GetCommandAllocForCmdList(CommandListType::DefaultCommandList), nullptr);

	m_defaultTexture = new Texture();
	Image whiteTexelImg(IntVec2(1, 1), Rgba8::WHITE);
	whiteTexelImg.m_imageFilePath = "DefaultTexture";
	m_defaultTexture = CreateTextureFromImage(whiteTexelImg);


	BufferDesc vertexBuffDesc = {};
	vertexBuffDesc.data = nullptr;
	vertexBuffDesc.descriptorHeap = nullptr;
	vertexBuffDesc.memoryUsage = MemoryUsage::Dynamic;
	vertexBuffDesc.owner = this;
	vertexBuffDesc.size = sizeof(Vertex_PCU);
	vertexBuffDesc.stride = sizeof(Vertex_PCU);

	m_immediateVBO = new VertexBuffer(vertexBuffDesc);



	BufferDesc diffuseVBuffDesc = {};
	diffuseVBuffDesc.data = nullptr;
	diffuseVBuffDesc.descriptorHeap = nullptr;
	diffuseVBuffDesc.memoryUsage = MemoryUsage::Dynamic;
	diffuseVBuffDesc.owner = this;
	diffuseVBuffDesc.size = sizeof(Vertex_PNCU);
	diffuseVBuffDesc.stride = sizeof(Vertex_PNCU);

	m_immediateDiffuseVBO = new VertexBuffer(diffuseVBuffDesc);

	BufferDesc indexBufferDesc = {};
	indexBufferDesc.data = nullptr;
	indexBufferDesc.descriptorHeap = nullptr;
	indexBufferDesc.memoryUsage = MemoryUsage::Dynamic;
	indexBufferDesc.owner = this;
	indexBufferDesc.size = sizeof(unsigned int);
	indexBufferDesc.stride = sizeof(unsigned int);

	m_immediateIBO = new IndexBuffer(indexBufferDesc);

	// Assuming worst case scenario, which would be all constant buffers are full and are only engine ones

	BufferDesc cameraBufferDesc = {};
	cameraBufferDesc.data = nullptr;
	cameraBufferDesc.descriptorHeap = nullptr;
	cameraBufferDesc.memoryUsage = MemoryUsage::Dynamic;
	cameraBufferDesc.owner = this;
	cameraBufferDesc.size = sizeof(CameraConstants);
	cameraBufferDesc.stride = sizeof(CameraConstants);

	BufferDesc modelBufferDesc = cameraBufferDesc;
	modelBufferDesc.size = sizeof(ModelConstants);
	modelBufferDesc.stride = sizeof(ModelConstants);

	BufferDesc lightBufferDesc = cameraBufferDesc;
	lightBufferDesc.size = sizeof(LightConstants);
	lightBufferDesc.stride = sizeof(LightConstants);

	int engineDescriptorShare = CBV_DESCRIPTORS_AMOUNT / g_engineBufferCount;

	m_cameraCBOArray = std::vector<ConstantBuffer>(engineDescriptorShare, cameraBufferDesc);
	m_modelCBOArray = std::vector<ConstantBuffer>(engineDescriptorShare, modelBufferDesc);
	m_lightCBOArray = std::vector<ConstantBuffer>(engineDescriptorShare, modelBufferDesc);


	//// Allocating the memory so they're ready to use
	//for (int bufferInd = 0; bufferInd < m_cameraCBOArray.size(); bufferInd++) {
	//	ConstantBuffer*& cameraBuffer = m_cameraCBOArray[bufferInd];
	//	ConstantBuffer*& modelBuffer = m_modelCBOArray[bufferInd];

	//	cameraBuffer = new ConstantBuffer(cameraBufferDesc);
	//	modelBuffer = new ConstantBuffer(modelBufferDesc);
	//}

	MaterialSystemConfig matSystemConfig = {
	this // Renderer
	};
	g_theMaterialSystem = new MaterialSystem(matSystemConfig);
	g_theMaterialSystem->Startup();

	m_default3DMaterial = GetMaterialForName("Default3DMaterial");
	m_default2DMaterial = GetMaterialForName("Default2DMaterial");


	DebugRenderConfig debugSystemConfig;
	debugSystemConfig.m_renderer = this;
	debugSystemConfig.m_startHidden = false;
	debugSystemConfig.m_fontName = "Data/Images/SquirrelFixedFont";

	DebugRenderSystemStartup(debugSystemConfig);

	m_immediateCtxs = new ImmediateContext[m_immediateCtxCount]{};


	InitializeImGui();

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
	pUtils->BuildArguments(wSrc.c_str(), wEntryPoint.c_str(), wTarget.c_str(), NULL, 0, NULL, 0, &compilerArgs);

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

	/*IDxcCompiler2::CompileWithDebug()
		HRESULT shaderCompileResult = D3DCompile(source, strlen(source), sourceName, macros, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, target, compilerFlags, 0, &shaderBlob, &shaderErrorBlob);

	if (!SUCCEEDED(shaderCompileResult)) {

		std::string errorString = std::string((char*)shaderErrorBlob->GetBufferPointer());
		DX_SAFE_RELEASE(shaderErrorBlob);
		DX_SAFE_RELEASE(shaderBlob);

		DebuggerPrintf(Stringf("%s NOT COMPILING: %s", sourceName, errorString.c_str()).c_str());
		ERROR_AND_DIE("FAILED TO COMPILE SHADER TO BYTECODE");

	}*/


	outByteCode.resize(pShader->GetBufferSize());
	memcpy(outByteCode.data(), pShader->GetBufferPointer(), pShader->GetBufferSize());

	return true;
	}

void Renderer::LoadEngineShaderBinaries()
{
	std::string binariesPath = ENGINE_MAT_DIR;
	binariesPath += R"(Shaders\Binaries)";

	std::regex vertexRegex(".*_v.cso");
	std::regex pixelRegex(".*_p.cso");

	for (auto const& binEntry : std::filesystem::directory_iterator(binariesPath)) {
		std::filesystem::path binaryPath = binEntry.path();
		ShaderByteCode* newByteCode = new ShaderByteCode();
		bool isVertex = std::regex_match(binaryPath.string(), vertexRegex);
		bool isPixel = std::regex_match(binaryPath.string(), pixelRegex);

		ShaderType shaderType = ShaderType::InvalidShader;
		if (isVertex)shaderType = ShaderType::Vertex;
		if (isPixel)shaderType = ShaderType::Pixel;

		newByteCode->m_shaderType = shaderType;
		newByteCode->m_src = binaryPath.string();
		FileReadToBuffer(newByteCode->m_byteCode, binaryPath.string().c_str());
		newByteCode->m_shaderName = binaryPath.filename().replace_extension("").string();

		m_shaderByteCodes.push_back(newByteCode);
	}

}

Material* Renderer::CreateOrGetMaterial(std::filesystem::path materialPathNoExt)
{
	return g_theMaterialSystem->CreateOrGetMaterial(materialPathNoExt);
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
		D3DX12_MESH_SHADER_PIPELINE_STATE_DESC psoDesc = GetMeshShaderPSO(material);
		CD3DX12_PIPELINE_MESH_STATE_STREAM psoStream = CD3DX12_PIPELINE_MESH_STATE_STREAM(psoDesc);

		D3D12_PIPELINE_STATE_STREAM_DESC streamDesc;
		streamDesc.pPipelineStateSubobjectStream = &psoStream;
		streamDesc.SizeInBytes = sizeof(psoStream);

		psoCreation = m_device->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&material->m_PSO));
	}
	else {
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = GetGraphicsPSO(material, nameStrings);
		psoCreation = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&material->m_PSO));

	}

	ThrowIfFailed(psoCreation, "COULD NOT CREATE PSO");


	std::string shaderDebugName = "PSO:";
	shaderDebugName += baseName;
	SetDebugName(material->m_PSO, shaderDebugName.c_str());
}


bool Renderer::CreateInputLayoutFromVS(std::vector<uint8_t>& shaderByteCode, std::vector<D3D12_SIGNATURE_PARAMETER_DESC>& elementsDescs, std::vector<std::string>& semanticNames)
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
	//// Reflect shader info
	////ID3D12ShaderReflection* pShaderReflection = NULL;
	////IDxcResult::GetOutput
	//	HRESULT reflectOp = D3DReflect((void*)shaderByteCode.data(), shaderByteCode.size(), IID_ID3D12ShaderReflection, (void**)&pShaderReflection);
	//ThrowIfFailed(reflectOp, "FAILED TO GET REFLECTION OUT OF SHADER");

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
	return true;
}


Material* Renderer::GetMaterialForName(char const* materialName)
{
	return g_theMaterialSystem->GetMaterialForName(materialName);
}

Material* Renderer::GetMaterialForPath(std::filesystem::path const& materialPath)
{
	return g_theMaterialSystem->GetMaterialForPath(materialPath);
}

Material* Renderer::GetDefaultMaterial() const
{
	Material* defaultMat = nullptr;
	if (is3DDefault) {
		defaultMat = m_default3DMaterial;
	}
	else {
		defaultMat = m_default2DMaterial;
	}

	return defaultMat;
}

Material* Renderer::GetDefault2DMaterial() const
{
	return m_default2DMaterial;
}

Material* Renderer::GetDefault3DMaterial() const
{
	return m_default3DMaterial;
}

void Renderer::SetWindingOrder(WindingOrder newWindingOrder)
{
	ImmediateContext& currentDrawCtx = m_immediateCtxs[m_currentDrawCtx];

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
	ImmediateContext& currentDrawCtx = m_immediateCtxs[m_currentDrawCtx];

	Material* currentMaterial = (currentDrawCtx.m_material) ? currentDrawCtx.m_material : GetDefaultMaterial();
	Material* siblingMat = g_theMaterialSystem->GetSiblingMaterial(currentMaterial, SiblingMatTypes::DEPTH_FUNC_SIBLING, (unsigned int)newDepthFunc);
	currentDrawCtx.m_material = siblingMat;
}

void Renderer::SetWriteDepth(bool writeDepth)
{
	ImmediateContext& currentDrawCtx = m_immediateCtxs[m_currentDrawCtx];

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
	ImmediateContext& currentDrawCtx = m_immediateCtxs[m_currentDrawCtx];

	Material* currentMaterial = (currentDrawCtx.m_material) ? currentDrawCtx.m_material : GetDefaultMaterial();
	Material* siblingMat = g_theMaterialSystem->GetSiblingMaterial(currentMaterial, SiblingMatTypes::TOPOLOGY_SIBLING, (unsigned int)newTopologyType);
	currentDrawCtx.m_material = siblingMat;
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

	ConstantBuffer& lightBuffer = GetCurrentLightBuffer();
	m_currentLightCBufferSlot++;

	lightBuffer.CopyCPUToGPU(&lightConstants, sizeof(lightConstants));

	BindConstantBuffer(&lightBuffer, g_lightBufferSlot);
}

void Renderer::DrawVertexBuffer(VertexBuffer* const& vertexBuffer)
{
	ImmediateContext& currentDrawCtx = m_immediateCtxs[m_currentDrawCtx];

	BindVertexBuffer(vertexBuffer);
	currentDrawCtx.m_srvHandleStart = m_srvHandleStart;
	currentDrawCtx.m_cbvHandleStart = m_cbvHandleStart;

	UpdateDescriptorsHandleStarts(currentDrawCtx);

	if (!m_hasUsedModelSlot) {
		ConstantBuffer* currentModelCBO = currentDrawCtx.m_modelCBO;
		currentModelCBO->CopyCPUToGPU(&currentDrawCtx.m_modelConstants, sizeof(ModelConstants));
	}
	m_hasUsedModelSlot = true;
	CopyCurrentDrawCtxToNext();
}

void Renderer::DrawIndexedVertexBuffer(VertexBuffer* const& vertexBuffer, IndexBuffer* const& indexBuffer, size_t indexCount)
{
	ImmediateContext& currentDrawCtx = m_immediateCtxs[m_currentDrawCtx];

	BindVertexBuffer(vertexBuffer);
	BindIndexBuffer(indexBuffer, indexCount);

	currentDrawCtx.m_srvHandleStart = m_srvHandleStart;
	currentDrawCtx.m_cbvHandleStart = m_cbvHandleStart;
	currentDrawCtx.m_isIndexedDraw = true;

	UpdateDescriptorsHandleStarts(currentDrawCtx);

	if (!m_hasUsedModelSlot) {
		ConstantBuffer* currentModelCBO = currentDrawCtx.m_modelCBO;
		currentModelCBO->CopyCPUToGPU(&currentDrawCtx.m_modelConstants, sizeof(ModelConstants));
	}
	m_hasUsedModelSlot = true;
	CopyCurrentDrawCtxToNext();
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

void Renderer::DrawImmediateCtx(ImmediateContext& ctx)
{
	Texture* currentRt = ctx.m_renderTargets[0];
	Resource* currentRtResc = currentRt->GetResource();
	currentRtResc->TransitionTo(D3D12_RESOURCE_STATE_RENDER_TARGET, m_commandList.Get());

	Texture* depthTarget = ctx.m_depthTarget;
	Resource* depthTargetRsc = depthTarget->GetResource();
	depthTargetRsc->TransitionTo(D3D12_RESOURCE_STATE_DEPTH_WRITE, m_commandList.Get());

	SetMaterialPSO(ctx.m_material);
	for (auto& [slot, texture] : ctx.m_boundTextures) {
		CopyTextureToHeap(texture, ctx.m_srvHandleStart, slot);
	}

	for (auto& [slot, pBuffer] : ctx.m_boundBuffers) {
		CopyBufferToHeap(pBuffer, ctx.m_srvHandleStart, slot);
	}


	CopyCBufferToHeap(ctx.m_cameraCBO, ctx.m_cbvHandleStart, g_cameraBufferSlot);
	CopyCBufferToHeap(ctx.m_modelCBO, ctx.m_cbvHandleStart, g_modelBufferSlot);

	for (auto& [slot, cbuffer] : ctx.m_boundCBuffers) {
		CopyCBufferToHeap(cbuffer, ctx.m_cbvHandleStart, slot);
	}

	DescriptorHeap* srvUAVCBVHeap = GetGPUDescriptorHeap(DescriptorHeapType::SRV_UAV_CBV);
	DescriptorHeap* samplerHeap = GetGPUDescriptorHeap(DescriptorHeapType::SAMPLER);
	ID3D12DescriptorHeap* allDescriptorHeaps[] = {
		srvUAVCBVHeap->GetHeap(),
		samplerHeap->GetHeap()
	};

	UINT numHeaps = sizeof(allDescriptorHeaps) / sizeof(ID3D12DescriptorHeap*);
	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
	m_commandList->SetDescriptorHeaps(numHeaps, allDescriptorHeaps);


	m_commandList->SetGraphicsRootDescriptorTable(0, srvUAVCBVHeap->GetGPUHandleAtOffset(ctx.m_cbvHandleStart));
	m_commandList->SetGraphicsRootDescriptorTable(1, srvUAVCBVHeap->GetGPUHandleAtOffset(ctx.m_srvHandleStart));
	m_commandList->SetGraphicsRootDescriptorTable(2, srvUAVCBVHeap->GetGPUHandleAtOffset(UAV_HANDLE_START));
	m_commandList->SetGraphicsRootDescriptorTable(3, samplerHeap->GetGPUHandleForHeapStart());

	//for (int heapIndex = 0; heapIndex < allDescriptorHeaps.size(); heapIndex++) {
	//	ID3D12DescriptorHeap* currentDescriptorHeap = allDescriptorHeaps[heapIndex];
	//	m_commandList->SetGraphicsRootDescriptorTable(heapIndex, currentDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	//}

	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	// Later, each texture has its handle
	//ResourceView* rtv =  m_defaultRenderTarget->GetOrCreateView(RESOURCE_BIND_RENDER_TARGET_BIT);
	D3D12_CPU_DESCRIPTOR_HANDLE currentRTVHandle = currentRt->GetOrCreateView(RESOURCE_BIND_RENDER_TARGET_BIT)->GetHandle();
	ResourceView* dsvView = ctx.m_depthTarget->GetOrCreateView(RESOURCE_BIND_DEPTH_STENCIL_BIT);
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvView->GetHandle();

	//CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_currentBackBuffer, m_RTVdescriptorSize);
	m_commandList->OMSetRenderTargets(1, &currentRTVHandle, FALSE, &dsvHandle);

	if (ctx.m_isMeshDraw) {
		unsigned int threadX = ctx.m_meshThreads.x;
		unsigned int threadY = ctx.m_meshThreads.y;
		unsigned int threadZ = ctx.m_meshThreads.z;
		m_commandList->DispatchMesh(threadX, threadY, threadZ);
	}
	else {
		m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		VertexBuffer* usedBuffer = (ctx.m_immediateVBO) ? *ctx.m_immediateVBO : m_immediateVBO;
		IndexBuffer* usedIndexedBuffer = nullptr;


		D3D12_VERTEX_BUFFER_VIEW D3DbufferView = {};
		BufferView vBufferView = usedBuffer->GetBufferView();
		// Initialize the vertex buffer view.
		D3DbufferView.BufferLocation = vBufferView.m_bufferLocation;
		D3DbufferView.StrideInBytes = (UINT)vBufferView.m_strideInBytes;
		D3DbufferView.SizeInBytes = (UINT)vBufferView.m_sizeInBytes;

		m_commandList->IASetVertexBuffers(0, 1, &D3DbufferView);
		if (ctx.m_isIndexedDraw) {
			usedIndexedBuffer = (ctx.m_immediateIBO) ? *ctx.m_immediateIBO : m_immediateIBO;
			D3D12_INDEX_BUFFER_VIEW D3DindexedBufferView = {};
			BufferView iBufferView = usedIndexedBuffer->GetBufferView();
			D3DindexedBufferView.BufferLocation = iBufferView.m_bufferLocation;
			D3DindexedBufferView.Format = DXGI_FORMAT_R32_UINT;
			D3DindexedBufferView.SizeInBytes = (UINT)iBufferView.m_sizeInBytes;

			m_commandList->IASetIndexBuffer(&D3DindexedBufferView);
			m_commandList->DrawIndexedInstanced((UINT)ctx.m_indexCount, 1, (UINT)ctx.m_indexStart, (INT)ctx.m_vertexStart, 0);
		}
		else {
			m_commandList->DrawInstanced((UINT)ctx.m_vertexCount, 1, (UINT)ctx.m_vertexStart, 0);
		}
	}
}

void Renderer::CopyCurrentDrawCtxToNext()
{
	if (m_currentDrawCtx <= IMMEDIATE_CTX_AMOUNT - 1) {
		ImmediateContext& nextCtx = m_immediateCtxs[m_currentDrawCtx + 1];

		nextCtx = ImmediateContext(m_immediateCtxs[m_currentDrawCtx]);
		nextCtx.m_immediateIBO = nullptr;
		nextCtx.m_immediateVBO = nullptr;
		nextCtx.m_isIndexedDraw = false;
		nextCtx.m_isMeshDraw = false;
	}
	m_currentDrawCtx++;
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

ComPtr<ID3D12GraphicsCommandList6> Renderer::GetBufferCommandList()
{
	return m_ResourcesCommandList;
}

/// <summary>
/// Create PSO for usual graphics pipeline. Note: the nameStrings MUST live beyond this function's scope. Hence it's a parameter
/// </summary>
/// <param name="material"></param>
/// <param name="nameStrings"></param>
/// <returns></returns>
D3D12_GRAPHICS_PIPELINE_STATE_DESC Renderer::GetGraphicsPSO(Material* material, std::vector<std::string>& nameStrings)
{
	std::vector<D3D12_SIGNATURE_PARAMETER_DESC> reflectInputDesc;

	ShaderByteCode* vsByteCode = material->m_byteCodes[ShaderType::Vertex];
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
	psoDesc.pRootSignature = m_rootSignature.Get();
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
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = (matConfig.m_depthEnable) ? DXGI_FORMAT_D24_UNORM_S8_UINT : DXGI_FORMAT_UNKNOWN;
	psoDesc.SampleDesc.Count = 1;

	return psoDesc;
}

D3DX12_MESH_SHADER_PIPELINE_STATE_DESC Renderer::GetMeshShaderPSO(Material* material)
{
	std::vector<D3D12_SIGNATURE_PARAMETER_DESC> reflectInputDesc;
	std::vector<std::string> nameStrings;


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

	D3DX12_MESH_SHADER_PIPELINE_STATE_DESC psoDesc = {};
	ShaderByteCode* psByteCode = material->m_byteCodes[ShaderType::Pixel];
	ShaderByteCode* msByteCode = material->m_byteCodes[ShaderType::Mesh];

	psoDesc.pRootSignature = m_rootSignature.Get();
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
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = (matConfig.m_depthEnable) ? DXGI_FORMAT_D24_UNORM_S8_UINT : DXGI_FORMAT_UNKNOWN;
	psoDesc.SampleDesc.Count = 1;

	return psoDesc;
}

void Renderer::ResetResourcesState()
{
	for (Resource* currentResource : m_resources) {
		if (!currentResource) {
			ERROR_RECOVERABLE("RESORCE WAS DELETED SOMEWHERE");
			continue;
		}
		currentResource->m_currentState = D3D12_RESOURCE_STATE_COMMON;
	}
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
	ImmediateContext& currentDrawCtx = m_immediateCtxs[m_currentDrawCtx];
	currentDrawCtx.m_isMeshDraw = true;
	currentDrawCtx.m_meshThreads = IntVec3((unsigned int)threadX, (unsigned int)threadY, (unsigned int)threadZ); // Cast is fine, it's not possible to dispatch this many threads to worry about
	currentDrawCtx.m_srvHandleStart = m_srvHandleStart;
	currentDrawCtx.m_cbvHandleStart = m_cbvHandleStart;

	UpdateDescriptorsHandleStarts(currentDrawCtx);

	if (!m_hasUsedModelSlot) {
		ConstantBuffer* currentModelCBO = currentDrawCtx.m_modelCBO;
		currentModelCBO->CopyCPUToGPU(&currentDrawCtx.m_modelConstants, sizeof(ModelConstants));
	}
	CopyCurrentDrawCtxToNext();

	m_hasUsedModelSlot = true;

}

void Renderer::DrawVertexArray(std::vector<Vertex_PCU> const& vertexes)
{
	DrawVertexArray((unsigned int)vertexes.size(), vertexes.data());
}

void Renderer::DrawVertexArray(unsigned int numVertexes, const Vertex_PCU* vertexes)
{
	ImmediateContext& currentDrawCtx = m_immediateCtxs[m_currentDrawCtx];

	currentDrawCtx.m_srvHandleStart = m_srvHandleStart;
	currentDrawCtx.m_cbvHandleStart = m_cbvHandleStart;

	UpdateDescriptorsHandleStarts(currentDrawCtx);

	currentDrawCtx.m_vertexCount = (size_t)numVertexes;
	currentDrawCtx.m_vertexStart = m_immediateVertexes.size();
	std::copy(vertexes, vertexes + numVertexes, std::back_inserter(m_immediateVertexes));

	if (!m_hasUsedModelSlot) {
		ConstantBuffer* currentModelCBO = currentDrawCtx.m_modelCBO;
		currentModelCBO->CopyCPUToGPU(&currentDrawCtx.m_modelConstants, sizeof(ModelConstants));
	}
	CopyCurrentDrawCtxToNext();
	m_hasUsedModelSlot = true;
}

void Renderer::DrawVertexArray(unsigned int numVertexes, const Vertex_PNCU* vertexes)
{
	ImmediateContext& currentDrawCtx = m_immediateCtxs[m_currentDrawCtx];

	currentDrawCtx.m_srvHandleStart = m_srvHandleStart;
	currentDrawCtx.m_cbvHandleStart = m_cbvHandleStart;

	auto findHighestValTex = [](const std::pair<unsigned int, Texture const*>& a, const std::pair<unsigned int, Texture const*>& b)->bool { return a.first < b.first; };
	auto findHighestValCbuffer = [](const std::pair<unsigned int, ConstantBuffer*>& a, const std::pair<unsigned int, ConstantBuffer*>& b)->bool { return a.first < b.first; };

	auto texMaxIt = std::max_element(currentDrawCtx.m_boundTextures.begin(), currentDrawCtx.m_boundTextures.end(), findHighestValTex);
	auto cBufferMaxIt = std::max_element(currentDrawCtx.m_boundCBuffers.begin(), currentDrawCtx.m_boundCBuffers.end(), findHighestValCbuffer);

	unsigned int texMax = (texMaxIt != currentDrawCtx.m_boundTextures.end()) ? texMaxIt->first : 0;
	unsigned int cBufferMax = (cBufferMaxIt != currentDrawCtx.m_boundCBuffers.end()) ? cBufferMaxIt->first : 0;

	m_srvHandleStart += texMax + 1;
	m_cbvHandleStart += cBufferMax + 1;

	m_cbvHandleStart += 2;

	currentDrawCtx.m_vertexCount = (size_t)numVertexes;
	currentDrawCtx.m_vertexStart = m_immediateDiffuseVertexes.size();
	std::copy(vertexes, vertexes + numVertexes, std::back_inserter(m_immediateDiffuseVertexes));

	if (!m_hasUsedModelSlot) {
		ConstantBuffer* currentModelCBO = currentDrawCtx.m_modelCBO;
		currentModelCBO->CopyCPUToGPU(&currentDrawCtx.m_modelConstants, sizeof(ModelConstants));
	}

	currentDrawCtx.m_immediateVBO = &m_immediateDiffuseVBO;

	CopyCurrentDrawCtxToNext();
	m_hasUsedModelSlot = true;


}

void Renderer::DrawVertexArray(std::vector<Vertex_PNCU> const& vertexes)
{
	DrawVertexArray((unsigned int)vertexes.size(), vertexes.data());
}

void Renderer::DrawIndexedVertexArray(unsigned int numVertexes, const Vertex_PCU* vertexes, unsigned int numIndices, unsigned int const* indices)
{
	ImmediateContext& currentDrawCtx = m_immediateCtxs[m_currentDrawCtx];

	currentDrawCtx.m_cbvHandleStart = m_cbvHandleStart;
	currentDrawCtx.m_isIndexedDraw = true;

	UpdateDescriptorsHandleStarts(currentDrawCtx);

	currentDrawCtx.m_vertexCount = (size_t)numVertexes;
	currentDrawCtx.m_vertexStart = m_immediateVertexes.size();
	std::copy(vertexes, vertexes + numVertexes, std::back_inserter(m_immediateVertexes));

	currentDrawCtx.m_indexCount = (size_t)numIndices;
	currentDrawCtx.m_indexStart = m_immediateIndices.size();
	std::copy(indices, indices + numIndices, std::back_inserter(m_immediateIndices));

	if (!m_hasUsedModelSlot) {
		ConstantBuffer* currentModelCBO = currentDrawCtx.m_modelCBO;
		currentModelCBO->CopyCPUToGPU(&currentDrawCtx.m_modelConstants, sizeof(ModelConstants));
	}
	m_hasUsedModelSlot = true;
	CopyCurrentDrawCtxToNext();

}

void Renderer::DrawIndexedVertexArray(std::vector<Vertex_PCU> const& vertexes, std::vector<unsigned int> const& indices)
{
	DrawIndexedVertexArray((unsigned int)vertexes.size(), vertexes.data(), (unsigned int)indices.size(), indices.data());
}

void Renderer::DrawIndexedVertexArray(unsigned int numVertexes, const Vertex_PNCU* vertexes, unsigned int numIndices, unsigned int const* indices)
{
	ImmediateContext& currentDrawCtx = m_immediateCtxs[m_currentDrawCtx];

	currentDrawCtx.m_cbvHandleStart = m_cbvHandleStart;
	currentDrawCtx.m_isIndexedDraw = true;

	UpdateDescriptorsHandleStarts(currentDrawCtx);

	currentDrawCtx.m_vertexCount = (size_t)numVertexes;
	currentDrawCtx.m_vertexStart = m_immediateDiffuseVertexes.size();
	std::copy(vertexes, vertexes + numVertexes, std::back_inserter(m_immediateDiffuseVertexes));

	currentDrawCtx.m_indexCount = (size_t)numIndices;
	currentDrawCtx.m_indexStart = m_immediateIndices.size();
	std::copy(indices, indices + numIndices, std::back_inserter(m_immediateIndices));

	if (!m_hasUsedModelSlot) {
		ConstantBuffer* currentModelCBO = currentDrawCtx.m_modelCBO;
		currentModelCBO->CopyCPUToGPU(&currentDrawCtx.m_modelConstants, sizeof(ModelConstants));
	}
	currentDrawCtx.m_immediateVBO = &m_immediateDiffuseVBO;
	m_hasUsedModelSlot = true;

	CopyCurrentDrawCtxToNext();
}

void Renderer::DrawIndexedVertexArray(std::vector<Vertex_PNCU> const& vertexes, std::vector<unsigned int> const& indices)
{
	DrawIndexedVertexArray((unsigned int)vertexes.size(), vertexes.data(), (unsigned int)indices.size(), indices.data());
}

void Renderer::SetModelMatrix(Mat44 const& modelMat)
{
	ImmediateContext& currentDrawCtx = m_immediateCtxs[m_currentDrawCtx];

	if (m_hasUsedModelSlot) {
		currentDrawCtx.m_modelCBO = &GetNextModelBuffer();
	}
	currentDrawCtx.m_modelConstants.ModelMatrix = modelMat;
	m_hasUsedModelSlot = false;
}

void Renderer::SetModelColor(Rgba8 const& modelColor)
{
	ImmediateContext& currentDrawCtx = m_immediateCtxs[m_currentDrawCtx];

	if (m_hasUsedModelSlot) {
		currentDrawCtx.m_modelCBO = &GetNextModelBuffer();
	}
	modelColor.GetAsFloats(currentDrawCtx.m_modelConstants.ModelColor);
	m_hasUsedModelSlot = false;
}

void Renderer::ExecuteCommandLists(ID3D12CommandList** commandLists, unsigned int count)
{
	m_commandQueue->ExecuteCommandLists(count, commandLists);
}

void Renderer::WaitForGPU()
{
	unsigned int currentValue = m_fenceValues[m_currentBackBuffer];
	int newFenceValue = SignalFence(m_commandQueue, m_fence, currentValue);

	HRESULT fenceCompletion = m_fence->SetEventOnCompletion(currentValue, m_fenceEvent);
	ThrowIfFailed(fenceCompletion, "ERROR ON SETTING EVENT ON COMPLETION FOR FENCE");
	::WaitForSingleObjectEx(m_fenceEvent, INFINITE, false);

	m_fenceValues[m_currentBackBuffer] = newFenceValue;

}

DescriptorHeap* Renderer::GetDescriptorHeap(DescriptorHeapType descriptorHeapType) const
{
	return m_defaultDescriptorHeaps[(size_t)descriptorHeapType];
}

DescriptorHeap* Renderer::GetGPUDescriptorHeap(DescriptorHeapType descriptorHeapType) const
{
	if ((size_t)descriptorHeapType > m_defaultGPUDescriptorHeaps.size()) return nullptr;
	return m_defaultGPUDescriptorHeaps[(size_t)descriptorHeapType];
}

void Renderer::CreateViewport()
{

	m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_config.m_window->GetClientDimensions().x), static_cast<float>(m_config.m_window->GetClientDimensions().y), 0, 1);
	m_scissorRect = CD3DX12_RECT(0, 0, static_cast<long>(m_config.m_window->GetClientDimensions().x), static_cast<long>(m_config.m_window->GetClientDimensions().y));

}

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

			m_frameUploadHeaps.push_back(textureUploadHeap);
			if (!m_isCommandListOpen) {
				m_uploadRequested = true;
			}
		}

		std::string const errorMsg = Stringf("COULD NOT CREATE TEXTURE WITH NAME %s", creationInfo.m_name.c_str());
		ThrowIfFailed(textureCreateHR, errorMsg.c_str());
	}
	//WaitForGPU();
	Texture* newTexture = new Texture(creationInfo);
	newTexture->m_handle = handle;
	SetDebugName(newTexture->m_handle->m_resource, creationInfo.m_name.c_str());

	m_loadedTextures.push_back(newTexture);

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

Texture* Renderer::CreateTextureFromFile(char const* imageFilePath)
{
	Image loadedImage(imageFilePath);
	Texture* newTexture = CreateTextureFromImage(loadedImage);

	return newTexture;
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

	//m_loadedTextures.push_back(newTexture);

	return newTexture;
}

ResourceView* Renderer::CreateShaderResourceView(ResourceViewInfo const& viewInfo) const
{
	DescriptorHeap* srvDescriptorHeap = GetDescriptorHeap(DescriptorHeapType::SRV_UAV_CBV);

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = srvDescriptorHeap->GetNextCPUHandle();
	m_device->CreateShaderResourceView(viewInfo.m_source->m_resource, viewInfo.m_srvDesc, cpuHandle);

	ResourceView* newResourceView = new ResourceView(viewInfo);
	newResourceView->m_descriptorHandle = cpuHandle;

	return newResourceView;
}

ResourceView* Renderer::CreateRenderTargetView(ResourceViewInfo const& viewInfo) const
{
	D3D12_RENDER_TARGET_VIEW_DESC* rtvDesc = viewInfo.m_rtvDesc;

	DescriptorHeap* rtvDescriptorHeap = GetDescriptorHeap(DescriptorHeapType::RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = rtvDescriptorHeap->GetNextCPUHandle();

	m_device->CreateRenderTargetView(viewInfo.m_source->m_resource, rtvDesc, cpuHandle);

	ResourceView* newResourceView = new ResourceView(viewInfo);
	newResourceView->m_descriptorHandle = cpuHandle;

	return newResourceView;
}

ResourceView* Renderer::CreateDepthStencilView(ResourceViewInfo const& viewInfo) const
{
	D3D12_DEPTH_STENCIL_VIEW_DESC* dsvDesc = viewInfo.m_dsvDesc;

	DescriptorHeap* dsvDescriptorHeap = GetDescriptorHeap(DescriptorHeapType::DSV);
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = dsvDescriptorHeap->GetNextCPUHandle();

	m_device->CreateDepthStencilView(viewInfo.m_source->m_resource, dsvDesc, cpuHandle);

	ResourceView* newResourceView = new ResourceView(viewInfo);
	newResourceView->m_descriptorHandle = cpuHandle;

	return newResourceView;
}

ResourceView* Renderer::CreateConstantBufferView(ResourceViewInfo const& viewInfo, DescriptorHeap* descriptorHeap) const
{
	D3D12_CONSTANT_BUFFER_VIEW_DESC* cbvDesc = viewInfo.m_cbvDesc;

	DescriptorHeap* cbvDescriptorHeap = (descriptorHeap) ? descriptorHeap : GetDescriptorHeap(DescriptorHeapType::SRV_UAV_CBV);
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = cbvDescriptorHeap->GetNextCPUHandle();

	m_device->CreateConstantBufferView(cbvDesc, cpuHandle);
	HRESULT deviceRemoved = m_device->GetDeviceRemovedReason();
	ThrowIfFailed(deviceRemoved, Stringf("DEVICE REMOVED %#04x", deviceRemoved).c_str());

	ResourceView* newResourceView = new ResourceView(viewInfo);
	newResourceView->m_descriptorHandle = cpuHandle;

	return newResourceView;
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
	DescriptorHeap* samplerHeap = GetDescriptorHeap(DescriptorHeapType::SAMPLER);

	m_device->CreateSampler(&samplerDesc, samplerHeap->GetHandleAtOffset(0));
	DescriptorHeap* gpuSamplerHeap = GetGPUDescriptorHeap(DescriptorHeapType::SAMPLER);
	m_device->CopyDescriptorsSimple(1, gpuSamplerHeap->GetCPUHandleForHeapStart(), samplerHeap->GetHandleAtOffset(0), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	//if (!SUCCEEDED(samplerStateCreationResult)) {
	//	ERROR_AND_DIE("ERROR CREATING SAMPLER STATE");
	//}
}

Texture* Renderer::GetCurrentRenderTarget() const
{
	if (!m_currentCamera) GetActiveRenderTarget();
	Texture* cameraColorTarget = m_currentCamera->GetRenderTarget();
	return  (cameraColorTarget) ? cameraColorTarget : GetActiveRenderTarget();
}

Texture* Renderer::GetCurrentDepthTarget() const
{
	if (!m_currentCamera) return m_defaultDepthTarget;
	Texture* cameraDepthTarget = m_currentCamera->GetDepthTarget();
	return  (cameraDepthTarget) ? cameraDepthTarget : m_defaultDepthTarget;
}

void Renderer::ApplyEffect(Material* effect, Camera const* camera, Texture* customDepth)
{
	FxContext newFxCtx = {};
	newFxCtx.m_cameraConstants.ProjectionMatrix = camera->GetProjectionMatrix();
	newFxCtx.m_cameraConstants.ViewMatrix = camera->GetViewMatrix();
	newFxCtx.m_cameraConstants.InvertedMatrix = camera->GetProjectionMatrix().GetInverted();

	newFxCtx.m_fx = effect;
	newFxCtx.m_depthTarget = (customDepth) ? customDepth : m_defaultDepthTarget;

	m_effectsCtxs.push_back(newFxCtx);
}

void Renderer::SetColorTarget(Texture* dst)
{
	if (dst) {
		ResourceView* renderTargetView = dst->GetOrCreateView(RESOURCE_BIND_RENDER_TARGET_BIT);
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = renderTargetView->GetHandle();
		m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
	}
	else {
		m_commandList->OMSetRenderTargets(0, NULL, FALSE, NULL);
	}

}

void Renderer::UploadAllPendingResources()
{
	// Unlit
	size_t vertexesSize = sizeof(Vertex_PCU) * m_immediateVertexes.size();
	m_immediateVBO->GuaranteeBufferSize(vertexesSize);
	m_immediateVBO->CopyCPUToGPU(m_immediateVertexes.data(), vertexesSize);

	size_t indexSize = sizeof(unsigned) * m_immediateIndices.size();
	m_immediateIBO->GuaranteeBufferSize(indexSize);
	m_immediateIBO->CopyCPUToGPU(m_immediateIndices.data(), indexSize);

	// Lit
	size_t vertexesDiffuseSize = sizeof(Vertex_PNCU) * m_immediateDiffuseVertexes.size();
	m_immediateDiffuseVBO->GuaranteeBufferSize(vertexesDiffuseSize);
	m_immediateDiffuseVBO->CopyCPUToGPU(m_immediateDiffuseVertexes.data(), vertexesDiffuseSize);

	/// Uploading all vertex and constant buffers
	m_ResourcesCommandList->Close();
	ID3D12CommandList* ppCommandLists[] = { m_ResourcesCommandList.Get() };
	ExecuteCommandLists(ppCommandLists, 1);

	ComPtr<ID3D12Fence1> fence;
	CreateFence(fence, "Rsc Fence");
	SignalFence(fence, 1);
	if (fence->GetCompletedValue() != 1)
	{
		HANDLE event = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
		fence->SetEventOnCompletion(1, event);

		WaitForSingleObjectEx(event, INFINITE, false);
		CloseHandle(event);
	}
}

void Renderer::DrawAllEffects()
{
	for (FxContext& ctx : m_effectsCtxs) {
		DrawEffect(ctx);
	}
}

void Renderer::DrawEffect(FxContext& ctx)
{
	Texture* activeTarget = GetActiveRenderTarget();
	Texture* backTarget = GetBackUpRenderTarget();
	/*if (m_isAntialiasingEnabled) {
		TextureCreateInfo info;
		info.m_name = "ApplyEffectTex";
		info.m_dimensions = activeTarget->GetDimensions();
		info.m_format = TextureFormat::R8G8B8A8_UNORM;
		info.m_bindFlags = TextureBindFlagBit::TEXTURE_BIND_RENDER_TARGET_BIT | TextureBindFlagBit::TEXTURE_BIND_SHADER_RESOURCE_BIT;
		info.m_memoryUsage = MemoryUsage::GPU;

		Texture* singleSampleActive = CreateTexture(info);
		m_deviceContext->ResolveSubresource(singleSampleActive->m_texture, 0, activeTarget->m_texture, 0, LocalToD3D11ColorFormat(info.m_format));

		if (customDepth) {
			CopyTextureWithShader(backTarget, singleSampleActive, customDepth, effect, camera);
		}
		else {
			CopyTextureWithShader(backTarget, singleSampleActive, m_depthBuffer, effect, camera);
		}


		DestroyTexture(singleSampleActive);
	}
	else {*/
	CopyTextureWithMaterial(backTarget, activeTarget, ctx.m_depthTarget, ctx.m_fx, ctx.m_cameraConstants);
	//}


	m_currentRenderTarget = (m_currentRenderTarget + 1) % 2;
}

void Renderer::CopyTextureWithMaterial(Texture* dst, Texture* src, Texture* depthBuffer, Material* effect, CameraConstants const& cameraConstants /*= nullptr*/)
{
	ClearTexture(Rgba8(0, 0, 0, 255), dst);
	Resource* srcResource = src->GetResource();
	Resource* dstResource = dst->GetResource();
	Resource* dsvResource = depthBuffer->GetResource();

	srcResource->TransitionTo(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, m_commandList.Get());
	dsvResource->TransitionTo(D3D12_RESOURCE_STATE_DEPTH_READ, m_commandList.Get());
	dstResource->TransitionTo(D3D12_RESOURCE_STATE_RENDER_TARGET, m_commandList.Get());
	BindMaterial(effect);
	SetColorTarget(dst);

	SetSamplerMode(SamplerMode::BILINEARCLAMP);

	ConstantBuffer& cBuffer = GetCurrentCameraBuffer();
	m_currentCameraCBufferSlot++;

	cBuffer.CopyCPUToGPU(&cameraConstants, sizeof(cameraConstants));
	CopyCBufferToHeap(&cBuffer, m_cbvHandleStart, 0);

	m_commandList->SetPipelineState(effect->m_PSO);
	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

	CopyTextureToHeap(src, m_srvHandleStart, 0);
	CopyTextureToHeap(depthBuffer, m_srvHandleStart, 1);

	DescriptorHeap* srvUAVCBVHeap = GetGPUDescriptorHeap(DescriptorHeapType::SRV_UAV_CBV);
	DescriptorHeap* samplerHeap = GetGPUDescriptorHeap(DescriptorHeapType::SAMPLER);
	ID3D12DescriptorHeap* allDescriptorHeaps[] = {
		srvUAVCBVHeap->GetHeap(),
		samplerHeap->GetHeap()
	};

	UINT numHeaps = sizeof(allDescriptorHeaps) / sizeof(ID3D12DescriptorHeap*);
	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
	m_commandList->SetDescriptorHeaps(numHeaps, allDescriptorHeaps);


	m_commandList->SetGraphicsRootDescriptorTable(0, srvUAVCBVHeap->GetGPUHandleAtOffset(m_cbvHandleStart));
	m_commandList->SetGraphicsRootDescriptorTable(1, srvUAVCBVHeap->GetGPUHandleAtOffset(m_srvHandleStart));
	m_commandList->SetGraphicsRootDescriptorTable(2, srvUAVCBVHeap->GetGPUHandleAtOffset(UAV_HANDLE_START));
	m_commandList->SetGraphicsRootDescriptorTable(3, samplerHeap->GetGPUHandleForHeapStart());

	m_cbvHandleStart += 1;
	m_srvHandleStart += 2;


	m_commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


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

	m_commandList->RSSetViewports(1, &viewport);
	m_commandList->RSSetScissorRects(1, &scissorRect);

	m_commandList->DrawInstanced(3, 1, 0, 0);
	//CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(dstResource->m_resource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);
	//m_commandList->ResourceBarrier(1, &resourceBarrier);
	//dstResource->m_currentState = D3D12_RESOURCE_STATE_COMMON;

	Material* defaultMat = GetDefaultMaterial();

	m_commandList->ClearState(defaultMat->m_PSO);
}

void Renderer::TrackResource(Resource* newResource)
{
	m_resources.push_back(newResource);
}

ID3D12CommandAllocator* Renderer::GetCommandAllocForCmdList(CommandListType cmdListType)
{
	auto commandAllocators = m_commandAllocators[m_currentBackBuffer];

	return commandAllocators[(unsigned int) cmdListType].Get();
}

void Renderer::SetBlendModeSpecs(BlendMode blendMode, D3D12_BLEND_DESC& blendDesc)
{

	blendDesc.RenderTarget[0].BlendEnable = true;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

	switch (blendMode) {
	case BlendMode::ALPHA:
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		break;
	case BlendMode::ADDITIVE:
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
		break;
	case BlendMode::OPAQUE:
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
		break;
	default:
		ERROR_AND_DIE(Stringf("Unknown / unsupported blend mode #%i", blendMode));
		break;
	}
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


void Renderer::ClearTexture(Rgba8 const& color, Texture* tex)
{
	float colorAsArray[4];
	color.GetAsFloats(colorAsArray);

	Resource* rtResource = tex->GetResource();
	rtResource->TransitionTo(D3D12_RESOURCE_STATE_RENDER_TARGET, m_commandList.Get());

	D3D12_CPU_DESCRIPTOR_HANDLE currentRTVHandle = tex->GetOrCreateView(RESOURCE_BIND_RENDER_TARGET_BIT)->GetHandle();;
	m_commandList->ClearRenderTargetView(currentRTVHandle, colorAsArray, 0, nullptr);
}

void Renderer::ResetGPUDescriptorHeaps()
{
	//#TODO       
}

ResourceView* Renderer::CreateResourceView(ResourceViewInfo const& resourceViewInfo, DescriptorHeap* descriptorHeap) const
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
		return CreateConstantBufferView(resourceViewInfo, descriptorHeap);
	case RESOURCE_BIND_UNORDERED_ACCESS_VIEW_BIT:
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

void Renderer::BindConstantBuffer(ConstantBuffer* cBuffer, unsigned int slot /*= 0*/)
{
	ImmediateContext& currentDrawCtx = m_immediateCtxs[m_currentDrawCtx];

	currentDrawCtx.m_boundCBuffers[slot] = cBuffer;
}

void Renderer::BindTexture(Texture const* texture, unsigned int slot /*= 0*/)
{
	ImmediateContext& currentDrawCtx = m_immediateCtxs[m_currentDrawCtx];

	currentDrawCtx.m_boundTextures[slot] = texture;
}

void Renderer::BindMaterial(Material* mat)
{
	ImmediateContext& currentDrawCtx = m_immediateCtxs[m_currentDrawCtx];

	if (!mat) mat = GetDefaultMaterial();
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
	ImmediateContext& currentDrawCtx = m_immediateCtxs[m_currentDrawCtx];

	currentDrawCtx.m_immediateVBO = &vertexBuffer;
	currentDrawCtx.m_vertexStart = 0;
	currentDrawCtx.m_vertexCount = (vertexBuffer->GetSize()) / vertexBuffer->GetStride();
}

void Renderer::BindIndexBuffer(IndexBuffer* const& indexBuffer, size_t indexCount)
{
	ImmediateContext& currentDrawCtx = m_immediateCtxs[m_currentDrawCtx];

	currentDrawCtx.m_immediateIBO = &indexBuffer;
	currentDrawCtx.m_indexStart = 0;
	currentDrawCtx.m_indexCount = indexCount;
}

void Renderer::BindStructuredBuffer(Buffer* const& buffer, unsigned int slot)
{
	ImmediateContext& currentDrawCtx = m_immediateCtxs[m_currentDrawCtx];

	currentDrawCtx.m_boundBuffers[slot] = buffer;
}

void Renderer::CopyTextureToHeap(Texture const* textureToBind, unsigned int handleStart, unsigned int slot)
{
	Texture* usedTex = const_cast<Texture*>(textureToBind);
	if (!textureToBind) {
		usedTex = m_defaultTexture;
	}

	ResourceView* rsv = usedTex->GetOrCreateView(RESOURCE_BIND_SHADER_RESOURCE_BIT);
	//DescriptorHeap* srvHeap = GetGPUDescriptorHeap(DescriptorHeapType::SRV_UAV_CBV);
	CopyResourceToHeap(usedTex->GetResource(), rsv, handleStart, slot, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

}

//	unsigned int resultingSlot = handleStart + slot;
//	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = srvHeap->GetHandleAtOffset(resultingSlot);
//	D3D12_CPU_DESCRIPTOR_HANDLE textureHandle = rsv->GetHandle();
//
//	usedTex->GetResource()->TransitionTo(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, m_commandList.Get());
//
//	//if (srvHandle.ptr != textureHandle.ptr) {
//	m_device->CopyDescriptorsSimple(1, srvHandle, rsv->GetHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
//	//}
//}

void Renderer::CopyCBufferToHeap(ConstantBuffer* bufferToBind, unsigned int handleStart, unsigned int slot /*= 0*/)
{
	if (!bufferToBind) return;

	ResourceView* rsv = bufferToBind->GetOrCreateView();
	//DescriptorHeap* srvHeap = GetGPUDescriptorHeap(DescriptorHeapType::SRV_UAV_CBV);

	CopyResourceToHeap(bufferToBind->m_buffer, rsv, handleStart, slot, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	/*D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = srvHeap->GetHandleAtOffset(handleStart + slot);
	D3D12_CPU_DESCRIPTOR_HANDLE cBufferHandle = rsv->GetHandle();


	bufferToBind->m_buffer->TransitionTo(D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_commandList.Get());
	m_device->CopyDescriptorsSimple(1, srvHandle, rsv->GetHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);*/

}

void Renderer::CopyBufferToHeap(Buffer* bufferToBind, unsigned int handleStart, unsigned int slot /*= 0*/)
{

	ResourceView* rsv = bufferToBind->GetOrCreateView(RESOURCE_BIND_SHADER_RESOURCE_BIT);

	//DescriptorHeap* srvHeap = GetGPUDescriptorHeap(DescriptorHeapType::SRV_UAV_CBV);
	CopyResourceToHeap(bufferToBind->m_buffer, rsv, handleStart, slot, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
}

void Renderer::CopyResourceToHeap(Resource* rsc, ResourceView* rsv, unsigned int handleStart, unsigned int slot, D3D12_RESOURCE_STATES endState)
{
	unsigned int accessSlot = handleStart + slot;
	DescriptorHeap* srvHeap = GetGPUDescriptorHeap(DescriptorHeapType::SRV_UAV_CBV);
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = srvHeap->GetHandleAtOffset(accessSlot);
	D3D12_CPU_DESCRIPTOR_HANDLE cBufferHandle = rsv->GetHandle();

	rsc->TransitionTo(endState, m_commandList.Get());
	m_device->CopyDescriptorsSimple(1, srvHandle, rsv->GetHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

ConstantBuffer& Renderer::GetNextCameraBuffer()
{
	m_currentCameraCBufferSlot++;
	if (m_currentCameraCBufferSlot > m_cameraCBOArray.size()) ERROR_AND_DIE("RAN OUT OF CONSTANT BUFFER SLOTS");
	ConstantBuffer& bufferToReturn = m_cameraCBOArray[m_currentCameraCBufferSlot];
	bufferToReturn.Initialize();
	return bufferToReturn;
}

ConstantBuffer& Renderer::GetNextModelBuffer()
{
	m_currentModelCBufferSlot++;
	if (m_currentModelCBufferSlot > m_modelCBOArray.size()) ERROR_AND_DIE("RAN OUT OF CONSTANT BUFFER SLOTS");
	ConstantBuffer& bufferToReturn = m_modelCBOArray[m_currentModelCBufferSlot];
	bufferToReturn.Initialize();
	return bufferToReturn;
}

ConstantBuffer& Renderer::GetNextLightBuffer()
{
	m_currentLightCBufferSlot++;
	if (m_currentLightCBufferSlot > m_lightCBOArray.size()) ERROR_AND_DIE("RAN OUT OF CONSTANT BUFFER SLOTS");
	ConstantBuffer& bufferToReturn = m_lightCBOArray[m_currentLightCBufferSlot];
	bufferToReturn.Initialize();
	return bufferToReturn;
}

ConstantBuffer& Renderer::GetCurrentCameraBuffer()
{
	ConstantBuffer& currentCBuffer = m_cameraCBOArray[m_currentCameraCBufferSlot];
	currentCBuffer.Initialize();

	return currentCBuffer;
}

ConstantBuffer& Renderer::GetCurrentModelBuffer()
{
	ConstantBuffer& currentCBuffer = m_modelCBOArray[m_currentModelCBufferSlot];
	currentCBuffer.Initialize();
	return currentCBuffer;
}

ConstantBuffer& Renderer::GetCurrentLightBuffer()
{
	ConstantBuffer& currentCBuffer = m_lightCBOArray[m_currentLightCBufferSlot];
	currentCBuffer.Initialize();
	return currentCBuffer;
}

void Renderer::DrawAllImmediateContexts()
{
	for (unsigned int ctxIndex = 0; ctxIndex < m_currentDrawCtx; ctxIndex++) {
		//for (ImmediateContext& ctx : m_immediateCtxs) {
		ImmediateContext& ctx = m_immediateCtxs[ctxIndex];
		DrawImmediateCtx(ctx);
	}
}

void Renderer::ClearAllImmediateContexts()
{
	/*std::uninitialized_fill_n(m_immediateCtxs, IMMEDIATE_CTX_AMOUNT, ImmediateContext());*/
	//memset(m_immediateCtxs, 0, sizeof(ImmediateContext) * IMMEDIATE_CTX_AMOUNT);
}

void Renderer::SetMaterialPSO(Material* mat)
{
	m_commandList->SetPipelineState(mat->m_PSO);
}

void Renderer::SetBlendMode(BlendMode newBlendMode)
{
	ImmediateContext& currentDrawCtx = m_immediateCtxs[m_currentDrawCtx];

	Material* currentMaterial = (currentDrawCtx.m_material) ? currentDrawCtx.m_material : GetDefaultMaterial();
	Material* siblingMat = g_theMaterialSystem->GetSiblingMaterial(currentMaterial, SiblingMatTypes::BLEND_MODE_SIBLING, (unsigned int)newBlendMode);
	currentDrawCtx.m_material = siblingMat;
}

void Renderer::SetCullMode(CullMode newCullMode)
{
	ImmediateContext& currentDrawCtx = m_immediateCtxs[m_currentDrawCtx];

	Material* currentMaterial = (currentDrawCtx.m_material) ? currentDrawCtx.m_material : GetDefaultMaterial();
	Material* siblingMat = g_theMaterialSystem->GetSiblingMaterial(currentMaterial, SiblingMatTypes::CULL_MODE_SIBLING, (unsigned int)newCullMode);
	currentDrawCtx.m_material = siblingMat;
}

void Renderer::SetFillMode(FillMode newFillMode)
{
	ImmediateContext& currentDrawCtx = m_immediateCtxs[m_currentDrawCtx];

	Material* currentMaterial = (currentDrawCtx.m_material) ? currentDrawCtx.m_material : GetDefaultMaterial();
	Material* siblingMat = g_theMaterialSystem->GetSiblingMaterial(currentMaterial, SiblingMatTypes::FILL_MODE_SIBLING, (unsigned int)newFillMode);
	currentDrawCtx.m_material = siblingMat;
}

void Renderer::BeginFrame()
{
	WaitForGPU();
	g_theMaterialSystem->BeginFrame();
	DebugRenderBeginFrame();

	m_currentModelCBufferSlot = 0;
	m_currentCameraCBufferSlot = 0;

	m_currentDrawCtx = 0;
	m_srvHandleStart = SRV_HANDLE_START;
	m_cbvHandleStart = CBV_HANDLE_START;

	if (m_uploadRequested) {
		//WaitForGPU();
		//m_commandList->
		m_commandList->Close();
		ID3D12CommandList* commLists[1] = { m_commandList.Get() };
		ExecuteCommandLists(commLists, 1);
		WaitForGPU();
		for (ID3D12Resource*& uploadHeap : m_frameUploadHeaps) {
			DX_SAFE_RELEASE(uploadHeap);
			uploadHeap = nullptr;
		}
		m_frameUploadHeaps.resize(0);
		m_uploadRequested = false;
	}

	Texture* currentRt = GetActiveRenderTarget();
	Resource* activeRTResource = currentRt->GetResource();

	// Command list allocators can only be reset when the associated 
  // command lists have finished execution on the GPU; apps should use 
  // fences to determine GPU execution progress.
	
	auto cmdAllocator = GetCommandAllocForCmdList(CommandListType::DefaultCommandList);

	ThrowIfFailed(cmdAllocator->Reset(), "FAILED TO RESET COMMAND ALLOCATOR");

	auto rscCmdAlloc = GetCommandAllocForCmdList(CommandListType::ResourcesCommandList);
	ThrowIfFailed(rscCmdAlloc->Reset(), "FAILED TO RESET RESOURCES COMMAND ALLOCATOR");
	rscCmdAlloc->Reset();
	ThrowIfFailed(m_ResourcesCommandList->Reset(rscCmdAlloc, nullptr), "COULD NOT RESET RESOURCES COMMAND LIST");

	// However, when ExecuteCommandList() is called on a particular command 
	// list, that command list can then be reset at any time and must be before 
	// re-recording.
	ThrowIfFailed(m_commandList->Reset(cmdAllocator, m_default2DMaterial->m_PSO), "COULD NOT RESET COMMAND LIST");

	m_isCommandListOpen = true;
	BindTexture(nullptr);
	activeRTResource->TransitionTo(D3D12_RESOURCE_STATE_RENDER_TARGET, m_commandList.Get());
	// Indicate that the back buffer will be used as a render target.
	//CD3DX12_RESOURCE_BARRIER resourceBarrierRTV = CD3DX12_RESOURCE_BARRIER::Transition(m_backBuffers[m_currentBackBuffer].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	//m_commandList->ResourceBarrier(1, &resourceBarrierRTV);
	//m_defaultRenderTarget->GetResource()->TransitionTo(D3D12_RESOURCE_STATE_RENDER_TARGET, m_commandList.Get());

	//D3D12_CPU_DESCRIPTOR_HANDLE currentRTVHandle = m_defaultRenderTarget->GetOrCreateView(ResourceBindFlagBit::RESOURCE_BIND_RENDER_TARGET_BIT)->GetHandle();
	D3D12_CPU_DESCRIPTOR_HANDLE currentRTVHandle = currentRt->GetOrCreateView(ResourceBindFlagBit::RESOURCE_BIND_RENDER_TARGET_BIT)->GetHandle();

	//CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_currentBackBuffer, m_RTVdescriptorSize);
	m_commandList->OMSetRenderTargets(1, &currentRTVHandle, FALSE, nullptr);

	ResetGPUDescriptorHeaps();
	ClearScreen(currentRt->GetClearColour());
	ClearDepth();
	// Record commands.
	//float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	//m_defaultRenderTarget->GetClearColour().GetAsFloats(clearColor);
	//m_commandList->ClearRenderTargetView(currentRTVHandle, clearColor, 0, nullptr);
	m_immediateVertexes.clear();
	m_immediateIndices.clear();

	BeginFrameImGui();

}

void Renderer::EndFrame()
{

	DebugRenderEndFrame();

	UploadAllPendingResources();

	DrawAllImmediateContexts();

	if (m_effectsCtxs.size() > 0) {
		DrawAllEffects();
	}

	Texture* currentRt = GetActiveRenderTarget();
	Resource* defaultRTResource = currentRt->GetResource();
	Resource* currentBackBuffer = GetActiveBackBuffer()->GetResource();
	UNUSED(defaultRTResource)

		D3D12_CPU_DESCRIPTOR_HANDLE currentRTVHandle = currentRt->GetOrCreateView(RESOURCE_BIND_RENDER_TARGET_BIT)->GetHandle();

	EndFrameImGui();

	//// Copy draws from default texture to backbuffer
	//currentBackBuffer->TransitionTo(D3D12_RESOURCE_STATE_COPY_DEST, m_commandList.Get());
	//defaultRTResource->TransitionTo(D3D12_RESOURCE_STATE_COPY_SOURCE, m_commandList.Get());
	//m_commandList->CopyResource(currentBackBuffer->m_resource, defaultRTResource->m_resource);


	currentBackBuffer->TransitionTo(D3D12_RESOURCE_STATE_PRESENT, m_commandList.Get());

	ThrowIfFailed(m_commandList->Close(), "COULD NOT CLOSE COMMAND LIST");
	m_isCommandListOpen = false;
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };

	ExecuteCommandLists(ppCommandLists, 1);

#if defined(ENGINE_DISABLE_VSYNC)
	m_swapChain->Present(0, 0);
#else
	m_swapChain->Present(1, 0);
#endif

	Flush(m_commandQueue, m_fence, m_fenceValues.data(), m_fenceEvent);
	// Flush Command Queue getting ready for next Frame
	//WaitForGPU();

	m_effectsCtxs.clear();
	m_effectsCtxs.resize(0);

	m_currentFrame++;
	g_theMaterialSystem->EndFrame();

}

void Renderer::Shutdown()
{
	Flush(m_commandQueue, m_fence, m_fenceValues.data(), m_fenceEvent);

	ShutdownImGui();

	for (auto& uploadHeap : m_frameUploadHeaps) {
		DX_SAFE_RELEASE(uploadHeap);
	}

	for (Texture*& texture : m_loadedTextures) {
		DestroyTexture(texture);
		texture = nullptr;
	}

	for (ShaderByteCode*& shaderByteCode : m_shaderByteCodes) {
		delete shaderByteCode;
		shaderByteCode = nullptr;
	}

	for (auto commandAllocList : m_commandAllocators) {
		for (auto cmdAllocator : commandAllocList) {
			cmdAllocator.Reset();
		}
	}

	for (auto commandList : m_commandLists) {
		commandList.Reset();
	}


	//ClearAllImmediateContexts();

	//for (auto backBuffer : m_backBuffers) {
	//	backBuffer.Reset();
	//}

	m_pipelineState.Reset();
	m_fence.Reset();
	for (DescriptorHeap*& descriptorHeap : m_defaultDescriptorHeaps) {
		delete descriptorHeap;
		descriptorHeap = nullptr;
	}
	for (DescriptorHeap*& descriptorHeap : m_defaultGPUDescriptorHeaps) {
		delete descriptorHeap;
		descriptorHeap = nullptr;
	}



	//for (int constantBufferInd = 0; constantBufferInd < m_cameraCBOArray.size(); constantBufferInd++) {
	//	ConstantBuffer& cameraBuffer = m_cameraCBOArray[constantBufferInd];
	//	ConstantBuffer& modelBuffer = m_modelCBOArray[constantBufferInd];

	//	if (cameraBuffer) {
	//		delete cameraBuffer;
	//		cameraBuffer = nullptr;
	//	}

	//	if (modelBuffer) {
	//		delete modelBuffer;
	//		modelBuffer = nullptr;
	//	}

	//}

	delete m_immediateVBO;
	m_immediateVBO = nullptr;

	delete m_immediateDiffuseVBO;
	m_immediateDiffuseVBO = nullptr;

	delete m_immediateIBO;
	m_immediateIBO = nullptr;

	m_commandList.Reset();
	m_ResourcesCommandList.Reset();
	m_rootSignature.Reset();

	m_commandQueue.Reset();
	m_swapChain.Reset();
	m_device.Reset();
	//m_dxgiFactory.Get()->Release();
	m_dxgiFactory.Reset();

	DebugRenderSystemShutdown();

	g_theMaterialSystem->Shutdown();
	delete g_theMaterialSystem;
	g_theMaterialSystem = nullptr;

	delete[] m_immediateCtxs;
	m_resources.clear();
}

void Renderer::BeginCamera(Camera const& camera)
{
	ImmediateContext& currentDrawCtx = m_immediateCtxs[m_currentDrawCtx];
	currentDrawCtx.Reset();

	m_currentCamera = &camera;
	if (camera.GetCameraMode() == CameraMode::Orthographic) {
		BindMaterial(m_default2DMaterial);
	}
	else {
		BindMaterial(m_default3DMaterial);
	}

	BindTexture(m_defaultTexture);
	SetSamplerMode(SamplerMode::POINTCLAMP);

	currentDrawCtx.m_renderTargets[0] = GetActiveBackBuffer();
	currentDrawCtx.m_depthTarget = m_defaultDepthTarget;

	CameraConstants cameraConstants = {};
	cameraConstants.ProjectionMatrix = camera.GetProjectionMatrix();
	cameraConstants.ViewMatrix = camera.GetViewMatrix();
	cameraConstants.InvertedMatrix = cameraConstants.ProjectionMatrix.GetInverted();

	ConstantBuffer& nextCameraBuffer = GetCurrentCameraBuffer();
	nextCameraBuffer.CopyCPUToGPU(&cameraConstants, sizeof(CameraConstants));
	m_currentCameraCBufferSlot++;

	ConstantBuffer& nextModelBuffer = GetCurrentModelBuffer();
	m_currentModelCBufferSlot++;

	currentDrawCtx.m_cameraCBO = &nextCameraBuffer;
	currentDrawCtx.m_modelConstants = ModelConstants();
	currentDrawCtx.m_modelCBO = &nextModelBuffer;
	m_hasUsedModelSlot = false;
	//BindConstantBuffer(m_cameraCBO, 2);
	//BindConstantBuffer(m_modelCBO, 3);
}

void Renderer::EndCamera(Camera const& camera)
{
	ImmediateContext& currentDrawCtx = m_immediateCtxs[m_currentDrawCtx];

	if (&camera != m_currentCamera) {
		ERROR_RECOVERABLE("USING A DIFFERENT CAMERA TO END CAMERA PASS");
	}

	if (m_hasUsedModelSlot) {
		ConstantBuffer* currentModelCBO = currentDrawCtx.m_modelCBO;
		currentModelCBO->CopyCPUToGPU(&currentDrawCtx.m_modelConstants, sizeof(ModelConstants));
		m_currentModelCBufferSlot++;
	}

	m_currentCamera = nullptr;
	currentDrawCtx.Reset();
	Material* defaultMat = GetDefaultMaterial();
	m_commandList->ClearState(defaultMat->m_PSO);
}

void Renderer::ClearScreen(Rgba8 const& color)
{
	Texture* currentRt = GetActiveBackBuffer();
	ClearTexture(color, currentRt);
}

void Renderer::ClearDepth(float clearDepth /*= 0.0f*/)
{
	ResourceView* dsvView = m_defaultDepthTarget->GetOrCreateView(RESOURCE_BIND_DEPTH_STENCIL_BIT);
	Resource* dsvRsc = m_defaultDepthTarget->GetResource();
	dsvRsc->TransitionTo(D3D12_RESOURCE_STATE_DEPTH_WRITE, m_commandList.Get());

	D3D12_CLEAR_FLAGS clearFlags = D3D12_CLEAR_FLAG_DEPTH;

	m_commandList->ClearDepthStencilView(dsvView->GetHandle(), clearFlags, clearDepth, 0, 0, NULL);
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

ImmediateContext::ImmediateContext(ImmediateContext const& otherCtx)
{
	m_modelConstants = otherCtx.m_modelConstants;
	m_isIndexedDraw = otherCtx.m_isIndexedDraw;
	m_isMeshDraw = otherCtx.m_isMeshDraw;
	m_immediateIBO = otherCtx.m_immediateIBO;
	m_cameraCBO = otherCtx.m_cameraCBO;
	m_modelCBO = otherCtx.m_modelCBO;
	m_boundTextures.clear();
	m_boundCBuffers.clear();
	m_boundBuffers.clear();

	m_boundTextures.insert(otherCtx.m_boundTextures.begin(), otherCtx.m_boundTextures.end());
	m_boundCBuffers.insert(otherCtx.m_boundCBuffers.begin(), otherCtx.m_boundCBuffers.end());
	m_boundBuffers.insert(otherCtx.m_boundBuffers.begin(), otherCtx.m_boundBuffers.end());

	m_vertexStart = otherCtx.m_vertexStart;
	m_vertexCount = otherCtx.m_vertexCount;
	m_indexStart = otherCtx.m_indexStart;
	m_indexCount = otherCtx.m_indexCount;
	m_meshThreads = otherCtx.m_meshThreads;
	//VertexBuffer* m_immediateBuffer = nullptr;
	m_material = otherCtx.m_material;
	for (int rtIndex = 0; rtIndex < 8; rtIndex++) {
		m_renderTargets[rtIndex] = otherCtx.m_renderTargets[rtIndex];
	}
	m_depthTarget = otherCtx.m_depthTarget;
	m_srvHandleStart = otherCtx.m_srvHandleStart;
	m_cbvHandleStart = otherCtx.m_cbvHandleStart;
}

void ImmediateContext::Reset()
{
	m_modelConstants = {};
	m_isIndexedDraw = false;
	m_isMeshDraw = false;
	m_immediateIBO = nullptr;
	m_cameraCBO = nullptr;
	m_modelCBO = nullptr;
	m_boundTextures.clear();
	m_boundCBuffers.clear();
	m_boundBuffers.clear();
	m_vertexStart = 0;
	m_vertexCount = 0;
	m_indexStart = 0;
	m_indexCount = 0;
	m_meshThreads = IntVec3::ZERO;
	//VertexBuffer* m_immediateBuffer = nullptr;
	m_material = nullptr;
	for (int rtIndex = 0; rtIndex < 8; rtIndex++) {
		m_renderTargets[8] = nullptr;
	}
	m_depthTarget = nullptr;
	m_srvHandleStart = 0;
	m_cbvHandleStart = 0;
}
