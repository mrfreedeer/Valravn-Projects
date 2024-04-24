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
	std::string engineStr = ENGINE_DIR ;
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

void Renderer::Startup()
{
	
	UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
	// Enable the debug layer (requires the Graphics Tools "optional feature").
	// NOTE: Enabling the debug layer after device creation will invalidate the active device.
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();

			// Enable additional debug layers.
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif

	ComPtr<IDXGIFactory4> factory;
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)), "FAILED TO CREATE DXGI FACTORY");

	
		ComPtr<IDXGIAdapter1> hardwareAdapter;
		GetHardwareAdapter(factory.Get(), &hardwareAdapter, false);

		ThrowIfFailed(D3D12CreateDevice(
			hardwareAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&m_device)
		), "FAILED TO CREATE DEVICE");

	// Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)), "FAILED TO CREATE COMMANDQUEUE");

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
	ThrowIfFailed(factory->CreateSwapChainForHwnd(
		m_commandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
		windowHandle,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain
	), "Failed to create swap chain");

	// This sample does not support fullscreen transitions.
	ThrowIfFailed(factory->MakeWindowAssociation(windowHandle, DXGI_MWA_NO_ALT_ENTER), "Failed to make window association");

	ThrowIfFailed(swapChain.As(&m_swapChain), "Failed to get Swap Chain");
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// Create descriptor heaps.
	{
		// Describe and create a render target view (RTV) descriptor heap.
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = 4;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)), "Failed to create RTV HEAP");

		m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// Create frame resources.
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.MipLevels = 1;
		textureDesc.Format = DXGI_FORMAT_R32_FLOAT;
		textureDesc.Width = 2560;
		textureDesc.Height = 1280;
		textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		D3D12_CLEAR_VALUE clearVal = {};
		clearVal.Color[0] = 0.0f;
		clearVal.Color[1] = 0.0f;
		clearVal.Color[2] = 0.0f;
		clearVal.Color[3] = 0.0f;
		clearVal.Format = DXGI_FORMAT_R32_FLOAT;

		CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
		// Create a RTV for each frame.
		for (UINT n = 0; n < 2; n++)
		{
			ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])), "FAILED TO GET BACK BUFFER");
			m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
			rtvHandle.Offset(1, m_rtvDescriptorSize);

			ThrowIfFailed(m_device->CreateCommittedResource(
				&defaultHeap,
				D3D12_HEAP_FLAG_NONE,
				&textureDesc,
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				&clearVal,
				IID_PPV_ARGS(&m_floatRenderTargets[n])), "FAILED TO CREATE FLOAT RENDER TARGETS");


			m_device->CreateRenderTargetView(m_floatRenderTargets[n].Get(), nullptr, rtvHandle);
			rtvHandle.Offset(1, m_rtvDescriptorSize);

		}


	}

	ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)), "FAILED TO CREATE COMMAND ALLOCATOR");


	////////////////////////////////////////////////////////////////

	// Create an empty root signature.
	{
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error), "FAILED TO SERIALIZE ROOT SIGNATURE");
		ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)), "FIALED TO CREATE ROOT SIGNATURE");
	}

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
		

		ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"shaders.hlsl").c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr), "FAILED TO COMPILE VSHADER");
		ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"shaders.hlsl").c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr), "FAILED TO COMPILE PSHADER");

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
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
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

	// Create synchronization objects and wait until assets have been uploaded to the GPU.
	{
		ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)), "FAILED CREATING FENCE");
		m_fenceValue = 1;

		// Create an event handle to use for frame synchronization.
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr)
		{
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()), "FAILED GETTING LAST ERROR");
		}

		// Wait for the command list to execute; we are reusing the same command 
		// list in our main loop but for now, we just want to wait for setup to 
		// complete before continuing.
		  // Signal and increment the fence value.
		const UINT64 fence = m_fenceValue;
		ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence), "FAILED TO SIGNAL FENCE");
		m_fenceValue++;

		// Wait until the previous frame is finished.
		if (m_fence->GetCompletedValue() < fence)
		{
			ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent), "FAILED TO SET EVENT ON COMPLETION");
			WaitForSingleObject(m_fenceEvent, INFINITE);
		}

		m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
	}
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

	auto rtBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	// Indicate that the back buffer will be used as a render target.
	m_commandList->ResourceBarrier(1, &rtBarrier);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex * 2, m_rtvDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE secrtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex * 2 + 1, m_rtvDescriptorSize);
	m_commandList->OMSetRenderTargets(2, &rtvHandle, TRUE, nullptr);

	// Record commands.
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	const float secClearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_commandList->ClearRenderTargetView(secrtvHandle, secClearColor, 0, nullptr);
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	m_commandList->DrawInstanced(3, 1, 0, 0);

	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	// Indicate that the back buffer will now be used to present.
	m_commandList->ResourceBarrier(1, &barrier);

	ThrowIfFailed(m_commandList->Close(), "FAILED TO CLOSE COMMAND LIST");
}

void Renderer::EndFrame()
{
	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Present the frame.
	ThrowIfFailed(m_swapChain->Present(1, 0), "FAILED TO PRESENT");
	const UINT64 fence = m_fenceValue;
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence), "FAILED TO SIGNAL");
	m_fenceValue++;

	// Wait until the previous frame is finished.
	if (m_fence->GetCompletedValue() < fence)
	{
		ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent), "FAILED TO SET EVENT ON COMPLETION");
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void Renderer::Shutdown()
{

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

Texture* Renderer::CreateTexture(TextureCreateInfo& creationInfo)
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

DescriptorHeap* Renderer::GetDescriptorHeap(DescriptorHeapType descriptorHeapType) const
{
	return nullptr;
}

DescriptorHeap* Renderer::GetGPUDescriptorHeap(DescriptorHeapType descriptorHeapType) const
{
	return nullptr;

}

ResourceView* Renderer::CreateResourceView(ResourceViewInfo const& resourceViewInfo, DescriptorHeap* descriptorHeap /*= nullptr*/) const
{
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

void Renderer::CreatePSOForMaterial(Material* material)
{

}
