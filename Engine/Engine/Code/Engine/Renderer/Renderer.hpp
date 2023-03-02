#include "Engine/Core/Rgba8.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/Vertex_PNCU.hpp"
#include "Engine/Renderer/Shader.hpp"
#include <filesystem>
#include <cstdint>
#include <vector>

#pragma once

#define DX_SAFE_RELEASE(dxObject)			\
{											\
	if (( dxObject) != nullptr)				\
	{										\
		(dxObject)->Release();				\
		(dxObject) = nullptr;				\
	}										\
}

#undef OPAQUE


enum class BlendMode
{
	ALPHA = 1,
	ADDITIVE,
	OPAQUE
};

enum class CullMode {
	NONE = 1,
	FRONT,
	BACK,
	NUM_CULL_MODES
};

enum class FillMode {
	SOLID = 1,
	WIREFRAME,
	NUM_FILL_MODES
};

enum class WindingOrder {
	CLOCKWISE = 1,
	COUNTERCLOCKWISE
};


enum class DepthTest // Transformed directly to DX11 (if standard changes, unexpected behavior might result) check when changing to > DX11
{
	NEVER = 1,
	LESS = 2,
	EQUAL = 3,
	LESSEQUAL = 4,
	GREATER = 5,
	NOTEQUAL = 6,
	GREATEREQUAL = 7,
	ALWAYS = 8,
};

enum class SamplerMode
{
	POINTCLAMP,
	POINTWRAP,
	BILINEARCLAMP,
	BILINEARWRAP,
};

enum class TopologyMode {// Transformed directly to DX11 (if standard changes, unexpected behavior might result) check when changing to > DX11
	UNDEFINED,
	POINTLIST,
	LINELIST,
	LINESTRIP,
	TRIANGLELIST,
	TRIANGLESTRIP,
	LINELIST_ADJ = 10,
	LINESTRIP_ADJ = 11,
	TRIANGLELIST_ADJ = 12,
	TRIANGLESTRIP_ADJ = 13,
	CONTROL_POINT_PATCHLIST_1 = 33,
	CONTROL_POINT_PATCHLIST_2 = 34,
	CONTROL_POINT_PATCHLIST_3 = 35,
	CONTROL_POINT_PATCHLIST_4 = 36,
	CONTROL_POINT_PATCHLIST_5 = 37,
	CONTROL_POINT_PATCHLIST_6 = 38,
	CONTROL_POINT_PATCHLIST_7 = 39,
	CONTROL_POINT_PATCHLIST_8 = 40,
	CONTROL_POINT_PATCHLIST_9 = 41,
	CONTROL_POINT_PATCHLIST_10 = 42,
	CONTROL_POINT_PATCHLIST_11 = 43,
	CONTROL_POINT_PATCHLIST_12 = 44,
	CONTROL_POINT_PATCHLIST_13 = 45,
	CONTROL_POINT_PATCHLIST_14 = 46,
	CONTROL_POINT_PATCHLIST_15 = 47,
	CONTROL_POINT_PATCHLIST_16 = 48,
	CONTROL_POINT_PATCHLIST_17 = 49,
	CONTROL_POINT_PATCHLIST_18 = 50,
	CONTROL_POINT_PATCHLIST_19 = 51,
	CONTROL_POINT_PATCHLIST_20 = 52,
	CONTROL_POINT_PATCHLIST_21 = 53,
	CONTROL_POINT_PATCHLIST_22 = 54,
	CONTROL_POINT_PATCHLIST_23 = 55,
	CONTROL_POINT_PATCHLIST_24 = 56,
	CONTROL_POINT_PATCHLIST_25 = 57,
	CONTROL_POINT_PATCHLIST_26 = 58,
	CONTROL_POINT_PATCHLIST_27 = 59,
	CONTROL_POINT_PATCHLIST_28 = 60,
	CONTROL_POINT_PATCHLIST_29 = 61,
	CONTROL_POINT_PATCHLIST_30 = 62,
	CONTROL_POINT_PATCHLIST_31 = 63,
	CONTROL_POINT_PATCHLIST_32 = 64,
};

enum ShaderBindBit : unsigned int {

	SHADER_BIND_NONE = 0,
	SHADER_BIND_VERTEX_SHADER = (1 << 0),
	SHADER_BIND_HULL_SHADER = (1 << 1),
	SHADER_BIND_DOMAIN_SHADER = (1 << 2),
	SHADER_BIND_GEOMETRY_SHADER = (1 << 3),
	SHADER_BIND_PIXEL_SHADER = (1 << 4),
	SHADER_BIND_COMPUTE_SHADER = (1 << 5),
	SHADER_BIND_RENDERING_PIPELINE = 0b00011111
};

class Window;

struct RendererConfig {
	Window* m_window = nullptr;
};

class Texture;
class BitmapFont;
class VertexBuffer;
class StreamOutputBuffer;
class UnorderedAccessBuffer;
class ConstantBuffer;
class IndexBuffer;
class Image;

struct IntVec2;
struct Vec4;

struct ID3D11ShaderReflection;
struct ID3D11Device;
struct ID3D11DeviceChild;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11RenderTargetView;
struct ID3D11RasterizerState;
struct ID3D11BlendState;
struct ID3D11SamplerState;
struct ID3D11DepthStencilState;
struct ID3D11DepthStencilView;
struct ID3D11Texture2D;
struct ID3D11Texture2D;
struct ID3D11InputLayout;
struct TextureCreateInfo;

constexpr int g_modelBufferSlot = 3;
struct ModelConstants {
	Mat44 ModelMatrix;
	float ModelColor[4];
	float ModelPadding[4];
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

struct SODeclarationEntry {
	std::string SemanticName = "";
	unsigned int SemanticIndex = 0;
	unsigned char StartComponent;
	unsigned char ComponentCount;
	unsigned char OutputSlot;
};

constexpr int MAX_LIGHTS = 8;
class Renderer {
public:
	Renderer(RendererConfig const& config);
	void Startup(BlendMode blendMode = BlendMode::ALPHA);
	void BeginFrame();
	void EndFrame();
	void Shutdown();

	void ClearScreen(const Rgba8& clearColor);
	void ClearDepth(float value = 1.0f) const;
	void ClearDepthTexture(Texture* depthTexture, float value = 1.0f) const;
	void BeginCamera(const Camera& camera);
	void BeginDepthOnlyCamera(Camera const& lightCamera);
	void EndCamera(const Camera& camera);

	void DispatchCS(int threadX, int threadY, int threadZ);

	// For when a Vertex Buffer is not needed
	void Draw(int numVertexes, int startOffset, TopologyMode topology = TopologyMode::TRIANGLELIST);
	void DrawVertexArray(int numVertexes, const Vertex_PCU* vertexes, TopologyMode topology = TopologyMode::TRIANGLELIST) const;
	void DrawVertexArray(std::vector<Vertex_PCU> const& vertexes, TopologyMode topology = TopologyMode::TRIANGLELIST) const;
	void DrawVertexArray(int numVertexes, const Vertex_PNCU* vertexes, TopologyMode topology = TopologyMode::TRIANGLELIST) const;
	void DrawVertexArray(std::vector<Vertex_PNCU> const& vertexes, TopologyMode topology = TopologyMode::TRIANGLELIST) const;
	void DrawIndexedVertexArray(int numVertexes, Vertex_PCU const* vertexes, int numIndices, unsigned int const* indices, TopologyMode topology = TopologyMode::TRIANGLELIST) const;
	void DrawIndexedVertexArray(std::vector<Vertex_PCU> const& vertexes, std::vector<unsigned int> const& indices, TopologyMode topology = TopologyMode::TRIANGLELIST) const;
	void DrawIndexedVertexArray(int numVertexes, Vertex_PNCU const* vertexes, int numIndices, unsigned int const* indices, TopologyMode topology = TopologyMode::TRIANGLELIST) const;
	void DrawIndexedVertexArray(std::vector<Vertex_PNCU> const& vertexes, std::vector<unsigned int> const& indices, TopologyMode topology = TopologyMode::TRIANGLELIST) const;
	void ClearScreen(Rgba8& color);

	void CreateRenderContext();

	Texture* GetTextureForFileName(char const* imageFilePath);
	Texture* CreateOrGetTextureFromFile(char const* imageFilePath);
	Texture* CreateTextureFromData(char const* name, IntVec2 dimensions, int bytesPerTexel, uint8_t* texelData);
	void BindTexture(Texture const* texture, uint32_t idx = 0, ShaderBindBit bindFlags = SHADER_BIND_PIXEL_SHADER, bool canUseTextureMSDims = true);

	BitmapFont* CreateOrGetBitmapFont(char const* filePath);

	void SetBlendMode(BlendMode blendMode);


	Shader* CreateCSOnlyShader(std::filesystem::path shaderName);
	Shader* CreateShader(std::filesystem::path  shaderName, bool createWithStreamOutput = false, int numDeclarationEntries = 0, SODeclarationEntry* soEntries = nullptr);
	Shader* CreateShader(char const* shaderName, char const* shaderSource, bool createWithStreamOutput = false, int numDeclarationEntries = 0, SODeclarationEntry* soEntries = nullptr);
	Shader* CreateOrGetShader(std::filesystem::path shaderName);
	void BindShaderByName(char const* shaderName);
	void BindShader(Shader* shader);
	Shader* GetShaderForName(char const* shaderName);
	bool CompileShaderToByteCode(std::vector<unsigned char>& outByteCode, char const* name, char const* source, char const* entryPoint, char const* target, bool isAntialiasingOn = false);

	void CopyAndBindModelConstants() const;
	void CopyAndBindLightConstants() const;

	void CopyCPUToGPU(const void* data, size_t sizeInBytes, VertexBuffer* vbo) const;
	void CopyCPUToGPU(const void* data, size_t sizeInBytes, ConstantBuffer* cbo) const;
	void CopyCPUToGPU(const void* data, size_t sizeInBytes, IndexBuffer* ibo) const;

	void CopyTextureWithShader(Texture* dst, Texture* src, Texture* depthBuffer, Shader* effect, Camera const* camera = nullptr);
	void DownsampleTextureWithShader(Texture* dst, Texture* src, Shader* effect);
	void ApplyEffect(Shader* effect, Camera const* camera = nullptr, Texture* customDepth = nullptr);

	void BindConstantBuffer(int slot, ConstantBuffer* cbo) const;
	void BindVertexBuffer(VertexBuffer* vbo, unsigned int sizeOfVertex, TopologyMode topology = TopologyMode::TRIANGLELIST) const;
	void BindIndexBuffer(IndexBuffer* ibo) const;
	void BindSSAOKernels(int slot) const;

	void DrawVertexBuffer(VertexBuffer* vbo, int vertexCount, int vertexOffset = 0, unsigned int sizeOfVertex = sizeof(Vertex_PCU), TopologyMode topology = TopologyMode::TRIANGLELIST) const;
	void DrawIndexedVertexBuffer(VertexBuffer* vbo, int vertexOffset, IndexBuffer* ibo, int indexCount, int indexOffset, unsigned int sizeOfVertex = sizeof(Vertex_PCU), TopologyMode topology = TopologyMode::TRIANGLELIST) const;

	void SetModelMatrix(Mat44 const& modelMat);
	void SetModelColor(Rgba8 const& modelColor);
	void SetRasterizerState(CullMode cullMode, FillMode fillMode, WindingOrder windingOrder);
	void SetDepthStencilState(DepthTest depthTest, bool writeDepth);
	void SetSamplerMode(SamplerMode samplerMode);
	void SetDirectionalLight(Vec3 const& direction);
	void SetDirectionalLightIntensity(Rgba8 const& intensity);
	void SetAmbientIntensity(Rgba8 const& intensity);

	bool SetLight(Light const& light, int index);
	void SetLightRenderMatrix(Mat44 gameToRenderMatrix) { m_lightRenderTransform = gameToRenderMatrix; }
	void ClearLights();
	bool IsAntialiasingOn() const { return m_isAntialiasingEnabled; }
	int GetAntiaAliasingLevel() const { return m_antialiasingLevel; }
	void CreateSSAOKernelsUAV(std::vector<Vec4> kernels);
	void SetOcclusionPerSample(float occlusionPerSample);
	void SetSSAOFalloff(float SSAOFalloff);
	void SetSSAOSampleRadius(float sampleRadius);
	void SetSampleSize(int newSampleSize);


	Texture* CreateTexture(TextureCreateInfo const& createInfo);
	Texture* GetCurrentColorTarget() const;
	Texture* GetCurrentDepthTarget() const;
	void SetColorTarget(Texture* dst);
	void SetStreamOutputTarget(StreamOutputBuffer* streamOutputBuffer);
	void SetInputLayoutTopology(TopologyMode newTopology);
	void SetShaderResource(UnorderedAccessBuffer* uav, int slot = 1, ShaderBindBit bindFlags = SHADER_BIND_RENDERING_PIPELINE) const;
	void SetUAV(UnorderedAccessBuffer* uav, int slot = 0) const;
	void ClearState();

	Texture* GetActiveColorTarget() const;
	Texture* GetBackupColorTarget() const;

	void CopyTexture(Texture* dst, Texture* src);
	void DestroyTexture(Texture* texture);

	void ClearUAV(UnorderedAccessBuffer* uav, float fillValue);
	void SetAntiAliasing(bool isEnabled, unsigned int aaLevel = 1);
public:
	Vec3 m_directionalLight = Vec3(0.0f, 0.0f, -1.0f);
	Rgba8 m_directionalLightIntensity = Rgba8::WHITE;
	Rgba8 m_ambientIntensity = Rgba8::WHITE;
	ID3D11Device* m_device = nullptr;

private:
	Texture* CreateTextureFromImage(Image const& image);
	Texture* CreateTextureFromFile(char const* imageFilePath);
	BitmapFont* CreateBitmapFont(char const* filePath);
	void CreateDeviceAndSwapChain();
	void CreateRenderTargetView();
	void CreateViewportAndRasterizerState();
	void BindSamplerState();
	void CreateDepthStencil();
	bool CreateInputLayoutFromVS(std::vector<uint8_t>& shaderByteCode, ID3D11InputLayout** pInputLayout);

	void EnableDetailedD11Logging();
	void EnableDetailedD11Shutdown();
	void SetDebugName(ID3D11DeviceChild* object, char const* name);
private:

	RendererConfig m_config;
	std::vector<Texture*> m_loadedTextures;
	std::vector<BitmapFont*> m_loadedFonts;
	Texture const* m_lastBoundTexture = nullptr;

	std::vector<Shader*>		m_loadedShaders;
	Shader const* m_currentShader = nullptr;
	IndexBuffer* m_indexBuffer = nullptr;
	VertexBuffer* m_immediateVBO = nullptr;
	VertexBuffer* m_immediateVBO_PNCU = nullptr;
	ConstantBuffer* m_cameraCBO = nullptr;
	ConstantBuffer* m_modelCBO = nullptr;
	ConstantBuffer* m_lightCBO = nullptr;
	UnorderedAccessBuffer* m_SSAOKernels_UAV = nullptr;

	Shader* m_defaultShader = nullptr;
	Texture* m_defaultTexture = nullptr;
	ModelConstants m_modelConstants;
	Light m_lights[MAX_LIGHTS];

	Camera const* m_camera = nullptr;

protected:
	ID3D11DeviceContext* m_deviceContext = nullptr;
	IDXGISwapChain* m_swapChain = nullptr;
	ID3D11RasterizerState* m_rasterizerState = nullptr;
	ID3D11BlendState* m_blendState = nullptr;
	ID3D11SamplerState* m_samplerState = nullptr;
	ID3D11DepthStencilState* m_depthStencilState = nullptr;

	Texture* m_depthBuffer = nullptr;
	Texture* m_renderTarget = nullptr;
	Texture* m_backBuffers[2] = {};

	int m_activeBackBuffer = 0;

	void* m_dxgiDebugModule = nullptr;
	void* m_dxgiDebug = nullptr;

	bool m_isAntialiasingEnabled = false;
	unsigned int m_antialiasingLevel = 1;
	bool m_isAntialiasingEnabledNextFrame = false;
	bool m_rebuildAATexturesNextFrame = false;
	bool m_appliedEffect = false;

	float m_SSAOMaxOcclusionPerSample = 0.00000175f;
	float m_SSAOFalloff = 0.00000001f;
	float m_SSAOSampleRadius = 0.00001f;
	int m_SSAOSampleSize = 64;
	Mat44 m_lightRenderTransform = Mat44();
};