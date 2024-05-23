#pragma once
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/IntVec3.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Renderer/D3D12/DescriptorHeap.hpp"
#include "Engine/Renderer/ResourceView.hpp"
#include "Engine/Renderer/GraphicsCommon.hpp"
#include "Engine/Renderer/MaterialSystem.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Game/EngineBuildPreferences.hpp"
#include <filesystem>
#include <cstdint>
#include <vector>
#include <set>
#include <map>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxcapi.h>
#include <d3d12shader.h>
//#include <d3dcompiler.h>


#pragma comment (lib, "Engine/Renderer/D3D12/dxcompiler.lib")
#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "dxgi.lib")
//#pragma comment (lib, "d3dcompiler.lib")
#pragma comment (lib, "dxguid.lib")


struct ID3D12Device13;

struct ShaderByteCode;
struct ShaderLoadInfo;
struct D3DX12_MESH_SHADER_PIPELINE_STATE_DESC;

class Window;
class Material;
class Buffer;
class VertexBuffer;
class IndexBuffer;
class Texture;
class Image;
class Camera;
class ConstantBuffer;
class BitmapFont;
class ImmediateContext;
class Fence;
struct Rgba8;
struct Vertex_PCU;
struct TextureCreateInfo;

extern MaterialSystem* g_theMaterialSystem;

struct RendererConfig {
	Window* m_window = nullptr;
	unsigned int m_backBuffersCount = 2;
};

struct FxContext {
	CameraConstants m_cameraConstants = {};
	Material* m_fx = nullptr;
	Texture* m_depthTarget = nullptr;
};

struct Light
{
	bool Enabled = false;
	Vec3 Position;
	//------------- 16 bytes
	Vec3 Direction;
	int LightType = 0; // 0 Point Light, 1 SpotLight
	//------------- 16 bytes
	float Color[4];
	//------------- 16 bytes // These are some decent default values
	float SpotAngle = 45.0f;
	float ConstantAttenuation = 0.1f;
	float LinearAttenuation = 0.2f;
	float QuadraticAttenuation = 0.5f;
	//------------- 16 bytes
	Mat44 ViewMatrix;
	//------------- 64 bytes
	Mat44 ProjectionMatrix;
	//------------- 64 bytes
};
//
//struct SODeclarationEntry {
//	std::string SemanticName = "";
//	unsigned int SemanticIndex = 0;
//	unsigned char StartComponent;
//	unsigned char ComponentCount;
//	unsigned char OutputSlot;
//};

constexpr int MAX_LIGHTS = 8;

typedef void* HANDLE;

/// <summary>
/// Object that reports all live objects
/// NEEDS TO BE THE LAST DESTROYED THING, OTHERWISE IT REPORTS FALSE POSITIVES
/// </summary>
struct LiveObjectReporter {
	~LiveObjectReporter();
};

class Renderer {
	friend class Buffer;
	friend class Texture;
	friend class DescriptorHeap;
	friend class MaterialSystem;
	unsigned int const IMMEDIATE_CTX_AMOUNT = 65536;

public:
	Renderer(RendererConfig const& config);
	~Renderer();
	void Startup();
	void BeginFrame();
	void EndFrame();
	void Shutdown();

	void BeginCamera(Camera const& camera);
	void EndCamera(Camera const& camera);

	void ClearScreen(Rgba8 const& color);
	void ClearRenderTarget(unsigned int slot, Rgba8 const& color);
	void ClearDepth(float clearDepth = 1.0f);
	Material* CreateOrGetMaterial(std::filesystem::path materialPathNoExt);
	Texture* CreateOrGetTextureFromFile(char const* imageFilePath);
	Texture* CreateTexture(TextureCreateInfo& creationInfo);

	// Mesh Shaders
	void DispatchMesh(unsigned int threadX, unsigned threadY, unsigned int threadZ);

	// Unlit vertex array
	void DrawVertexArray(unsigned int numVertexes, const Vertex_PCU* vertexes);
	void DrawVertexArray(std::vector<Vertex_PCU> const& vertexes);
	void DrawIndexedVertexArray(unsigned int numVertexes, const Vertex_PCU* vertexes, unsigned int numIndices, unsigned int const* indices);
	void DrawIndexedVertexArray(std::vector<Vertex_PCU> const& vertexes, std::vector<unsigned int> const& indices);

	// Lit vertex array
	void DrawVertexArray(unsigned int numVertexes, const Vertex_PNCU* vertexes);
	void DrawVertexArray(std::vector<Vertex_PNCU> const& vertexes);
	void DrawIndexedVertexArray(unsigned int numVertexes, const Vertex_PNCU* vertexes, unsigned int numIndices, unsigned int const* indices);
	void DrawIndexedVertexArray(std::vector<Vertex_PNCU> const& vertexes, std::vector<unsigned int> const& indices);
	void BindDepthAsTexture(Texture* depthTarget = nullptr, unsigned int slot = 0);

	void SetModelMatrix(Mat44 const& modelMat);
	void SetModelColor(Rgba8 const& modelColor);
	void ExecuteCommandLists(ID3D12CommandList** commandLists, unsigned int count);
	void WaitForGPU();

	ResourceView* CreateResourceView(ResourceViewInfo const& resourceViewInfo) const;
	BitmapFont* CreateOrGetBitmapFont(std::filesystem::path bitmapPath);
	Material* GetMaterialForName(char const* materialName);
	Material* GetMaterialForPath(std::filesystem::path const& materialPath);
	Material* GetDefaultMaterial() const;
	Material* GetDefault2DMaterial() const;
	Material* GetDefault3DMaterial() const;
	// Binds
	void BindConstantBuffer(ConstantBuffer* cBuffer, unsigned int slot = 0);
	void BindTexture(Texture const* texture, unsigned int slot = 0);
	void BindMaterial(Material* mat);
	void BindMaterialByName(char const* materialName);
	void BindMaterialByPath(std::filesystem::path materialPath);
	void BindVertexBuffer(VertexBuffer* const& vertexBuffer);
	void BindIndexBuffer(IndexBuffer* const& indexBuffer, size_t indexCount);
	void BindStructuredBuffer(Buffer* const& buffer, unsigned int slot);

	// Setters
	void SetRenderTarget(Texture* dst, unsigned int slot = 0);
	void SetMaterialPSO(Material* mat);
	void SetBlendMode(BlendMode newBlendMode);
	void SetCullMode(CullMode newCullMode);
	void SetFillMode(FillMode newFillMode);
	void SetWindingOrder(WindingOrder newWindingOrder);
	void SetRasterizerState(CullMode cullMode, FillMode fillMode, WindingOrder windingOrder);
	void SetDepthFunction(DepthFunc newDepthFunc);
	void SetWriteDepth(bool writeDepth);
	void SetDepthStencilState(DepthFunc newDepthFunc, bool writeDepth);
	void SetTopology(TopologyType newTopologyType);
	void SetDirectionalLight(Vec3 const& direction);
	void SetDirectionalLightIntensity(Rgba8 const& intensity);
	void SetAmbientIntensity(Rgba8 const& intensity);
	bool SetLight(Light const& light, int index);
	void SetLightRenderMatrix(Mat44 gameToRenderMatrix) { m_lightRenderTransform = gameToRenderMatrix; }
	void BindLightConstants();

	void DrawVertexBuffer(VertexBuffer* const& vertexBuffer);
	void DrawIndexedVertexBuffer(VertexBuffer* const& vertexBuffer, IndexBuffer* const& indexBuffer, size_t indexCount);

	// General
	void SetDebugName(ID3D12Object* object, char const* name);
	template<typename T_Object>
	void SetDebugName(ComPtr<T_Object> object, char const* name);
	void SetSamplerMode(SamplerMode samplerMode);

	void AddToUpdateQueue(Buffer* bufferToUpdate);
	Texture* GetCurrentRenderTarget() const;
	Texture* GetCurrentDepthTarget() const;
	void ApplyEffect(Material* effect, Camera const* camera = nullptr, Texture* customDepth = nullptr);
	void CopyTextureWithMaterial(Texture* dst, Texture* src, Texture* depthBuffer, Material* effect, CameraConstants const& cameraConstants = CameraConstants());
	void TrackResource(Resource* newResource);
	void RemoveResource(Resource* newResource);
	void SignalFence(ComPtr<ID3D12Fence1>& fence, unsigned int fenceValue);
	ID3D12CommandAllocator* GetCommandAllocForCmdList(CommandListType cmdListType);
	ID3D12GraphicsCommandList6* GetCurrentCommandList(CommandListType cmdListType);

	void FlushPendingWork();
	void ResetGPUState();
private:
	// ImGui
	void InitializeImGui();
	void ShutdownImGui();
	void BeginFrameImGui();
	void EndFrameImGui();

	// DX12 Initialization & Render Initialization
	void CreateDevice();
	void EnableDebugLayer();
	void CreateCommandQueue();
	void CreateSwapChain();
	void CreateDescriptorHeaps();
	void CreateFences();
	void CreateDefaultRootSignature();
	void CreateBackBuffers();
	ResourceView* CreateShaderResourceView(ResourceViewInfo const& resourceViewInfo) const;
	ResourceView* CreateRenderTargetView(ResourceViewInfo const& viewInfo) const;
	ResourceView* CreateDepthStencilView(ResourceViewInfo const& viewInfo) const;
	ResourceView* CreateConstantBufferView(ResourceViewInfo const& viewInfo) const;

	DescriptorHeap* GetGPUDescriptorHeap(DescriptorHeapType descriptorHeapType) const;
	DescriptorHeap* GetCPUDescriptorHeap(DescriptorHeapType descriptorHeapType) const;
	// Textures
	void DestroyTexture(Texture* textureToDestroy);

	// Material and Compilation
	void CreatePSOForMaterial(Material* material);
	bool CompileShaderToByteCode(std::vector<unsigned char>& outByteCode, ShaderLoadInfo const& loadInfo);


private:
	RendererConfig m_config = {};
	// This object must be first ALWAYS!!!!!
	LiveObjectReporter m_liveObjectReporter;

	Mat44 m_lightRenderTransform;
	unsigned int m_currentBackBuffer = 0;

	std::vector<Texture*> m_createdTextures;
	ComPtr<ID3D12Device2> m_device;
	ComPtr<IDXGIFactory4> m_DXGIFactory;
	ComPtr<ID3D12CommandQueue> m_commandQueue;
	ComPtr<IDXGISwapChain3> m_swapChain;
	
	ComPtr<ID3D12RootSignature> m_defaultRootSignature;
	DescriptorHeap* m_GPUDescriptorHeaps[(size_t)DescriptorHeapType::MAX_GPU_VISIBLE] = {};
	DescriptorHeap* m_CPUDescriptorHeaps[(size_t)DescriptorHeapType::NUM_DESCRIPTOR_HEAPS] = {};
	std::vector<ID3D12GraphicsCommandList6*> m_commandLists;
	std::vector<ID3D12CommandAllocator*> m_commandAllocators;


	std::vector<Texture*> m_backBuffers;
	Texture* m_floatRenderTargets[2] = {};
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12PipelineState> m_pipelineState;

	VertexBuffer* m_vBuffer = nullptr;
	// App resources.
	Fence* m_fence = nullptr;

	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;
};

template<typename T_Object>
void Renderer::SetDebugName(ComPtr<T_Object> object, char const* name)
{
	if (!object) return;
#if defined(ENGINE_DEBUG_RENDER)
	object->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen(name), name);
	//object->SetName(name);
#else
	UNUSED(object);
	UNUSED(name);
#endif
}

