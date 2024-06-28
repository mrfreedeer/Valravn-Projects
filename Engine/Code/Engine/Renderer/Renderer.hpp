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
	unsigned int m_immediateCtxCount = (2 << 13); // 16384
	unsigned int m_backBuffersCount = 2;
};

struct FxContext {
	ConstantBuffer* m_cameraCBO = nullptr;
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

	// Compute Shaders
	void Dispatch(unsigned int threadX, unsigned threadY, unsigned int threadZ);

	// Mesh Shaders
	void DispatchMesh(unsigned int threadX, unsigned threadY, unsigned int threadZ);

	void DrawVertexBuffer(VertexBuffer* const& vertexBuffer);
	void DrawIndexedVertexBuffer(VertexBuffer* const& vertexBuffer, IndexBuffer* const& indexBuffer, size_t indexCount);

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

	void SetModelMatrix(Mat44 const& modelMat);
	void SetModelColor(Rgba8 const& modelColor);

	ResourceView* CreateResourceView(ResourceViewInfo const& resourceViewInfo) const;
	BitmapFont* CreateOrGetBitmapFont(std::filesystem::path bitmapPath);
	Material* GetMaterialForName(char const* materialName);
	Material* GetMaterialForPath(std::filesystem::path const& materialPath);
	Material* GetDefaultMaterial(bool isUsing3D = true) const;

	// Binds
	void BindConstantBuffer(ConstantBuffer* cBuffer, unsigned int slot = 0);
	void BindTexture(Texture const* texture, unsigned int slot = 0);
	void BindMaterial(Material* mat);
	void BindComputeMaterial(Material* mat);
	void BindMaterialByName(char const* materialName);
	void BindMaterialByPath(std::filesystem::path materialPath);
	void BindVertexBuffer(VertexBuffer* const& vertexBuffer);
	void BindIndexBuffer(IndexBuffer* const& indexBuffer, size_t indexCount);
	void BindStructuredBuffer(Buffer* const& buffer, unsigned int slot);
	void BindRWStructuredBuffer(Buffer* const& buffer, unsigned int slot);
	void ClearBoundStructuredBuffers();

	// Setters
	void SetRenderTarget(Texture* dst, unsigned int slot = 0);
	void SetDepthRenderTarget(Texture* dst);
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

	// General
	void SetDebugName(ID3D12Object* object, char const* name);
	template<typename T_Object>
	void SetDebugName(ComPtr<T_Object> object, char const* name);
	void SetSamplerMode(SamplerMode samplerMode);

	void AddToUpdateQueue(Buffer* bufferToUpdate);
	Texture* GetCurrentRenderTarget() const;
	Texture* GetCurrentDepthTarget() const;
	void ApplyEffect(Material* effect, Camera const* camera = nullptr, Texture* customDepth = nullptr);
	void CopyTextureWithMaterial(Texture* dst, Texture* src, Texture* depthBuffer, Material* effect);

	ID3D12CommandAllocator* GetCommandAllocForCmdList(CommandListType cmdListType);
	ID3D12GraphicsCommandList6* GetCurrentCommandList(CommandListType cmdListType);

	void ResetGPUState();
	Texture* GetDefaultRenderTarget() const;
private:
	// ImGui
	void InitializeImGui();
	void ShutdownImGui();
	void BeginFrameImGui();
	void EndFrameImGui();

	// DX12 Initialization & Render Initialization
	void CreateDevice();
	void EnableDebugLayer();
	void CreateCommandQueues();
	void CreateSwapChain();
	void CreateDescriptorHeaps();
	void CreateFences();
	void CreateDefaultRootSignature();
	void CreateBackBuffers();

	ResourceView* CreateUnorderedAccessView(ResourceViewInfo const& resourceViewInfo) const;
	ResourceView* CreateShaderResourceView(ResourceViewInfo const& resourceViewInfo) const;
	ResourceView* CreateRenderTargetView(ResourceViewInfo const& viewInfo) const;
	ResourceView* CreateDepthStencilView(ResourceViewInfo const& viewInfo) const;
	ResourceView* CreateConstantBufferView(ResourceViewInfo const& viewInfo) const;

	DescriptorHeap* GetGPUDescriptorHeap(DescriptorHeapType descriptorHeapType) const;
	DescriptorHeap* GetCPUDescriptorHeap(DescriptorHeapType descriptorHeapType) const;

	// Textures
	BitmapFont* CreateBitmapFont(std::filesystem::path bitmapPath);
	void DestroyTexture(Texture* textureToDestroy);
	void CreateDefaultTextureTargets();
	void ClearTexture(Rgba8 const& color, Texture* tex);
	Texture* GetTextureForFileName(char const* imageFilePath);
	Texture* CreateTextureFromImage(Image const& image);
	Texture* CreateTextureFromFile(char const* imageFilePath);
	Texture* GetActiveRenderTarget() const;
	Texture* GetBackUpRenderTarget() const;
	Texture* GetActiveBackBuffer() const;
	Texture* GetBackUpBackBuffer() const;

	// Material and Compilation
	void CreatePSOForMaterial(Material* material);
	bool CompileShaderToByteCode(std::vector<unsigned char>& outByteCode, ShaderLoadInfo const& loadInfo);
	void CreateInputLayoutFromVS(std::vector<uint8_t>& shaderByteCode, std::vector<D3D12_SIGNATURE_PARAMETER_DESC>& elementsDescs, std::vector<std::string>& semanticNames);
	ShaderByteCode* CompileOrGetShaderBytes(ShaderLoadInfo const& shaderLoadInfo);
	ShaderByteCode* GetByteCodeForShaderSrc(ShaderLoadInfo const& shaderLoadInfo);
	void CreateGraphicsPSO(Material* material);
	void CreateComputePSO(Material* material);
	void CreateMeshShaderPSO(Material* material);
	void SetBlendModeSpecs(BlendMode const* blendMode, D3D12_BLEND_DESC& blendDesc);

	// Internal Resource Management
	void InitializeCBufferArrays();
	void CreateDefaultBufferObjects();
	/// <summary>
	/// // Uploads pending resources and inserts resource barriers before drawing
	/// </summary>
	void FinishPendingPrePassResourceTasks();
	void FinishPendingPreFxPassResourceTasks();
	void UploadImmediateVertexes();
	
	void DrawAllEffects();
	void DrawEffect(FxContext& fxCtx);

	void DrawAllImmediateContexts();
	void DrawImmediateContext(ImmediateContext& ctx);
	ImmediateContext& GetCurrentDrawCtx();
	ConstantBuffer* GetNextCBufferSlot(ConstantBufferType cBufferType);
	void SetContextDescriptorStarts(ImmediateContext& ctx);
	void SetModelBufferForCtx(ImmediateContext& ctx);
	void SetContextDrawInfo(ImmediateContext& ctx, unsigned int numVertexes, Vertex_PCU const* vertexes);
	void SetContextIndexedDrawInfo(ImmediateContext& ctx, unsigned int numVertexes, Vertex_PCU const* vertexes, unsigned int indexCount, unsigned int const* indexes);

	void SetContextDrawInfo(ImmediateContext& ctx, unsigned int numVertexes, Vertex_PNCU const* vertexes);
	void SetContextIndexedDrawInfo(ImmediateContext& ctx, unsigned int numVertexes, Vertex_PNCU const* vertexes, unsigned int indexCount, unsigned int const* indexes);

	void CopyCurrentDrawCtxToNext();
	void CopyTextureToDescriptorHeap(Texture const* texture, unsigned int handleStart, unsigned int slot);
	void CopyBufferToGPUHeap(Buffer* bufferToBind, ResourceBindFlagBit bindFlag, unsigned int handleStart, unsigned int slot);
	void CopyEngineCBuffersToGPUHeap(ImmediateContext& ctx);
	void CopyResourceToGPUHeap(ResourceView* rsv, unsigned int handleStart, unsigned int slot);
	void UpdateDescriptorsHandleStarts(ImmediateContext const& ctx);
	VertexBuffer* GetImmediateVBO(VertexType vertexType);
	void ExecuteCommandLists(unsigned int count, ID3D12CommandList** cmdLists);
	void AddBufferBindResourceBarrier(Buffer* bufferToBind, std::vector<D3D12_RESOURCE_BARRIER>& barriersArray);
	/// <summary>
	/// Execute all draw calls issued by Engine users (other Engine systems will be in a consecutive pass)
	/// </summary>
	void ExecuteMainRenderPass();
	void ExecuteEffectsRenderPass();
	void FillResourceBarriers(ImmediateContext& ctx, std::vector<D3D12_RESOURCE_BARRIER>& out_rscBarriers);
private:
	// This object must be first ALWAYS!!!!!
	LiveObjectReporter m_liveObjectReporter;
	RendererConfig m_config = {};
	D3D12_VIEWPORT m_viewport = {};
	D3D12_RECT m_scissorRect = {};

	// Lighting
	Light m_lights[MAX_LIGHTS];
	Vec3 m_directionalLight = Vec3(0.0f, 0.0f, -1.0f);
	Rgba8 m_directionalLightIntensity = Rgba8::WHITE;
	Rgba8 m_ambientIntensity = Rgba8::WHITE;
	Mat44 m_lightRenderTransform = Mat44();
	bool m_is3DDefault = true;
	bool m_isModelBufferDirty = false; // Keeping track on whether the model buffer can be reused between draw calls

	unsigned int m_currentCmdListIndex = 0;
	unsigned int m_currentBackBuffer = 0;
	unsigned int m_currentRenderTarget = 0;
	unsigned int m_currentDrawCtx = 0;
	unsigned int m_currentCameraBufferSlot = 0;
	unsigned int m_currentModelBufferSlot = 0;
	unsigned int m_currentLightBufferSlot = 0;
	unsigned int m_srvHandleStart = 0;
	unsigned int m_cbvHandleStart = 0;
	unsigned int m_uavHandleStart = 0;
	unsigned int m_currentFrame = 0;

	/*=================== ComPtrs =================== */
	ComPtr<ID3D12Device2> m_device;
	ComPtr<IDXGIFactory4> m_DXGIFactory;
	ComPtr<IDXGISwapChain3> m_swapChain;

	/*=================== Vectors =================== */
	std::vector<Texture*> m_loadedTextures;
	std::vector<BitmapFont*> m_loadedFonts;
	std::vector<ID3D12GraphicsCommandList6*> m_commandLists;
	std::vector<ID3D12CommandAllocator*> m_commandAllocators;

	std::vector<FxContext> m_effectsContexts;
	std::vector<Texture*> m_defaultRenderTargets;
	std::vector<Texture*> m_backBuffers;
	std::vector<D3D12_RESOURCE_BARRIER> m_pendingRscBarriers;
	std::set<Buffer*> m_pendingRscCopy;
	std::vector<ShaderByteCode*> m_shaderByteCodes;
	std::vector<Vertex_PCU> m_immediateVertexes;
	std::vector<unsigned int> m_immediateIndices;

	std::vector<Vertex_PNCU> m_immediateDiffuseVertexes;
	std::vector<unsigned int> m_immediateDiffuseIndices;
	std::vector<ConstantBuffer> m_cameraCBOArray = {};
	std::vector<ConstantBuffer> m_lightCBOArray = {};
	std::vector<ConstantBuffer> m_modelCBOArray = {};


	/*=================== Raw Pointers =================== */ 
	//	Internal resources
	Fence* m_fence = nullptr;
	Fence* m_resourcesFence = nullptr;

	//	Default resources
	Texture* m_defaultDepthTarget = nullptr;
	Texture* m_defaultTexture = nullptr;
	Material* m_default2DMaterial = nullptr;
	Material* m_default3DMaterial = nullptr;

	DescriptorHeap* m_GPUDescriptorHeaps[(size_t)DescriptorHeapType::MAX_GPU_VISIBLE] = {};
	DescriptorHeap* m_CPUDescriptorHeaps[(size_t)DescriptorHeapType::NUM_DESCRIPTOR_HEAPS] = {};
	ImmediateContext* m_immediateContexts = nullptr;
	ID3D12DescriptorHeap* m_ImGuiSrvDescHeap = nullptr;
	ID3D12RootSignature* m_defaultRootSignature = nullptr;
	ID3D12CommandQueue* m_graphicsCommandQueue = nullptr;

	
	Camera const* m_currentCamera = nullptr;

	VertexBuffer* m_immediateVBO = nullptr;
	VertexBuffer* m_immediateDiffuseVBO = nullptr;

	IndexBuffer* m_immediateIBO = nullptr;
	IndexBuffer* m_immediateDiffuseIBO = nullptr;

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

