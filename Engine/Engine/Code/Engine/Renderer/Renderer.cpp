#define WIN32_LEAN_AND_MEAN		// Always #define this before #including <windows.h>
#include <windows.h>			// #include this (massive, platform-specific) header in very few places
#include "Engine/Renderer/Renderer.hpp"
#include "ThirdParty/stb/stb_image.h"
#include "ThirdParty/ImGUI/imgui.h"
#include "ThirdParty/ImGUI//imgui_impl_win32.h"
#include "ThirdParty/ImGUI/imgui_impl_dx11.h"

#include "Engine/Math/Vec4.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/Image.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Window/Window.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/StreamOutputBuffer.hpp"
#include "Engine/Renderer/UnorderedAccessBuffer.hpp"
#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Renderer/DefaultShader.hpp"
#include "Engine/Renderer/DebugRendererSystem.hpp"
#include "Engine/Renderer/TextureView.hpp"
#include "Engine/Renderer/D3D11/D3D11TypeConversions.hpp"
#include "Game/EngineBuildPreferences.hpp"

#include <d3d11.h> 
#include <dxgi.h>
#include <d3dcompiler.h>

#pragma comment( lib, "d3d11.lib" )       // direct3D library
#pragma comment( lib, "dxgi.lib" )        // directx graphics interface
#pragma comment( lib, "d3dcompiler.lib" ) // shader compiler
#pragma comment( lib, "dxguid.lib" )


#if defined(ENGINE_DEBUG_RENDER)
#include <dxgidebug.h>
#endif



struct CameraConstants {
	Mat44 ProjectionMatrix;
	Mat44 ViewMatrix;
	Mat44 InvertedMatrix;
};


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

constexpr int g_cameraBufferSlot = 2;

Renderer::Renderer(RendererConfig const& config) :
	m_config(config)
{
}

void Renderer::Startup(BlendMode blendMode)
{
	EnableDetailedD11Logging();

	UNUSED(blendMode);
#if defined(ENGINE_ANTIALIASING)
	m_isAntialiasingEnabled = true;
	m_antialiasingLevel = ANTIALIASING_LEVEL;
#endif
	m_isAntialiasingEnabledNextFrame = m_isAntialiasingEnabled;

	CreateRenderContext();

	//glEnable(GL_BLEND);

	CreateDeviceAndSwapChain();
	CreateRenderTargetView();
	CreateViewportAndRasterizerState();

	m_defaultShader = CreateShader("Default", g_defaultShaderSource);
	//m_defaultShader = CreateShader("Data/Shaders/Default");

	m_immediateVBO = new VertexBuffer(m_device, 1, sizeof(Vertex_PCU));
	m_immediateVBO_PNCU = new VertexBuffer(m_device, 1, sizeof(Vertex_PNCU));
	m_indexBuffer = new IndexBuffer(m_device, 1);
	m_cameraCBO = new ConstantBuffer(m_device, sizeof(CameraConstants));
	m_modelCBO = new ConstantBuffer(m_device, sizeof(ModelConstants));
	m_lightCBO = new ConstantBuffer(m_device, sizeof(LightConstants));

	m_defaultTexture = new Texture();
	Image whiteTexelImg(IntVec2(1, 1), Rgba8::WHITE);
	whiteTexelImg.m_imageFilePath = "DefaultTexture";
	m_defaultTexture = CreateTextureFromImage(whiteTexelImg);


	CreateDepthStencil();



	DebugRenderConfig debugSystemConfig;
	debugSystemConfig.m_renderer = this;
	debugSystemConfig.m_startHidden = false;
	debugSystemConfig.m_fontName = "Data/Images/SquirrelFixedFont";

	DebugRenderSystemStartup(debugSystemConfig);
#if defined(ENGINE_USE_IMGUI)
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	Window* window = Window::GetWindowContext();

	ImGui_ImplWin32_Init((HWND)window->m_osWindowHandle);
	ImGui_ImplDX11_Init(m_device, m_deviceContext);

	ImGui::StyleColorsDark();
#endif



}

void Renderer::BeginFrame() {
	DebugRenderBeginFrame();

#if defined(ENGINE_USE_IMGUI)
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
#endif
}

void Renderer::EndFrame()
{
	/*HWND hwnd = (HWND)m_config.m_window->m_osWindowHandle;
	HDC hdc = ::GetDC(hwnd);*/

	//::SwapBuffers(hdc); // Note: call this only once at the very end of each frame
	DebugRenderEndFrame();

	if (m_isAntialiasingEnabled) {
			m_deviceContext->ResolveSubresource(m_renderTarget->m_texture, 0, m_backBuffers[m_activeBackBuffer]->m_texture, 0, LocalToD3D11ColorFormat(m_renderTarget->m_format));
	}
	else {
		CopyTexture(m_renderTarget, m_backBuffers[m_activeBackBuffer]);
	}

	if (m_rebuildAATexturesNextFrame || (m_isAntialiasingEnabledNextFrame != m_isAntialiasingEnabled)) {
		m_isAntialiasingEnabled = m_isAntialiasingEnabledNextFrame;
		CreateRenderTargetView();
		CreateDepthStencil();
	}

	m_appliedEffect = false;
#if defined(ENGINE_USE_IMGUI)
	TextureView* renderTargetview = m_renderTarget->GetRenderTargetView(m_isAntialiasingEnabled);

	m_deviceContext->OMSetRenderTargets(1, &renderTargetview->m_renderTgtView, nullptr);

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
#endif

#if defined(ENGINE_DISABLE_VSYNC)
	m_swapChain->Present(0, 0);
#else
	m_swapChain->Present(1, 0);
#endif

}

void Renderer::Shutdown()
{
#if defined(ENGINE_USE_IMGUI)
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
#endif
	DebugRenderSystemShutdown();

	DX_SAFE_RELEASE(m_device);
	DX_SAFE_RELEASE(m_deviceContext);
	DX_SAFE_RELEASE(m_swapChain);
	DX_SAFE_RELEASE(m_rasterizerState);
	DX_SAFE_RELEASE(m_blendState);
	DX_SAFE_RELEASE(m_samplerState);
	DX_SAFE_RELEASE(m_depthStencilState);

	delete m_immediateVBO;
	m_immediateVBO = nullptr;

	delete m_immediateVBO_PNCU;
	m_immediateVBO_PNCU = nullptr;

	delete m_SSAOKernels_UAV;
	m_SSAOKernels_UAV = nullptr;

	delete m_cameraCBO;
	m_cameraCBO = nullptr;

	delete m_modelCBO;
	m_modelCBO = nullptr;

	delete m_lightCBO;
	m_lightCBO = nullptr;

	delete m_indexBuffer;
	m_indexBuffer = nullptr;

	DestroyTexture(m_depthBuffer);
	DestroyTexture(m_renderTarget);
	DestroyTexture(m_backBuffers[0]);
	DestroyTexture(m_backBuffers[1]);

	m_depthBuffer = nullptr;
	m_renderTarget = nullptr;
	m_backBuffers[0] = nullptr;
	m_backBuffers[1] = nullptr;

	for (int textureIndex = 0; textureIndex < m_loadedTextures.size(); textureIndex++) {
		delete m_loadedTextures[textureIndex];
		m_loadedTextures[textureIndex] = nullptr;
	}

	for (int shaderIndex = 0; shaderIndex < m_loadedShaders.size(); shaderIndex++) {
		delete m_loadedShaders[shaderIndex];
		m_loadedShaders[shaderIndex] = nullptr;
	}

	EnableDetailedD11Shutdown();
}

void Renderer::ClearScreen(const Rgba8& clearColor)
{
	float* colorAsFloatArray = new float[4];
	clearColor.GetAsFloats(colorAsFloatArray);

	Texture* color = GetCurrentColorTarget();
	if (color) {
		TextureView* renderTgtView = color->GetRenderTargetView(m_isAntialiasingEnabled);

		m_deviceContext->ClearRenderTargetView(renderTgtView->m_renderTgtView, colorAsFloatArray);
	}


	ClearDepth();

	//glClearColor(red, green, blue, alpha); // Note; glClearColor takes colors as floats in [0,1], not bytes in [0,255]
	//glClear(GL_COLOR_BUFFER_BIT); // ALWAYS clear the screen at the top of each frame's Render()!
}

void Renderer::CreateRenderContext()
{
	// Creates an OpenGL rendering context (RC) and binds it to the current window's device context (DC)
	//PIXELFORMATDESCRIPTOR pixelFormatDescriptor;
	//memset(&pixelFormatDescriptor, 0, sizeof(pixelFormatDescriptor));
	//pixelFormatDescriptor.nSize = sizeof(pixelFormatDescriptor);
	//pixelFormatDescriptor.nVersion = 1;
	//pixelFormatDescriptor.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	//pixelFormatDescriptor.iPixelType = PFD_TYPE_RGBA;
	//pixelFormatDescriptor.cColorBits = 24;
	//pixelFormatDescriptor.cDepthBits = 24;
	//pixelFormatDescriptor.cAccumBits = 0;
	//pixelFormatDescriptor.cStencilBits = 8;

	//HWND hwnd = (HWND)m_config.m_window->m_osWindowHandle;
	//HDC hdc = ::GetDC(hwnd);

	//int pixelFormatCode = ChoosePixelFormat(hdc, &pixelFormatDescriptor);
	//SetPixelFormat(hdc, pixelFormatCode, &pixelFormatDescriptor);
	//HGLRC hglrc = wglCreateContext(hdc);
	//wglMakeCurrent(hdc, hglrc);
}

void Renderer::BeginCamera(const Camera& camera)
{
	m_camera = &camera;

	BindShader(m_defaultShader);
	BindTexture(m_defaultTexture);
	SetSamplerMode(SamplerMode::POINTCLAMP);
	BindSamplerState();
	SetBlendMode(BlendMode::ALPHA);
	SetRasterizerState(CullMode::NONE, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
	SetDepthStencilState(DepthTest::ALWAYS, false);


	CameraConstants cameraConstants = {};
	cameraConstants.ProjectionMatrix = camera.GetProjectionMatrix();
	cameraConstants.ViewMatrix = camera.GetViewMatrix();
	cameraConstants.InvertedMatrix = cameraConstants.ProjectionMatrix.GetInverted();



	CopyCPUToGPU(&cameraConstants, sizeof(cameraConstants), m_cameraCBO);
	BindConstantBuffer(g_cameraBufferSlot, m_cameraCBO);

	m_modelConstants.ModelMatrix = Mat44();
	Rgba8::WHITE.GetAsFloats(m_modelConstants.ModelColor);

	CopyAndBindModelConstants();


	float clientDimensionsX = static_cast<float>(m_config.m_window->GetClientDimensions().x);
	float clientDimensionsY = static_cast<float>(m_config.m_window->GetClientDimensions().y);

	Vec2 cameraBoundSize = camera.GetViewport().GetDimensions();

	AABB2 viewportBounds = AABB2(0.0f, 0.0f, clientDimensionsX, clientDimensionsY).GetBoxWithin(camera.GetViewport());

	float cameraWidth = clientDimensionsX * cameraBoundSize.x;
	float cameraHeight = clientDimensionsY * cameraBoundSize.y;

	D3D11_VIEWPORT viewport{
		viewportBounds.m_mins.x,
		clientDimensionsY - viewportBounds.m_maxs.y,
		cameraWidth,
		cameraHeight,
		0,
		1
	};

	// todo: eventually color comes from camera



	m_deviceContext->RSSetViewports(1, &viewport);

	Texture* color = GetCurrentColorTarget();
	TextureView* rtv = color->GetRenderTargetView(m_isAntialiasingEnabled);

	// todo: eventually depth target comes from camera
	Texture* depth = GetCurrentDepthTarget();
	if (depth) {
		TextureView* dsv = depth->GetDepthStencilView(m_isAntialiasingEnabled);
		m_deviceContext->OMSetRenderTargets(1, &rtv->m_renderTgtView, dsv->m_depthStencilView);

	}



	/*glLoadIdentity();
	glOrtho(bottomLeft.x, topRight.x, bottomLeft.y, topRight.y, 0.f, 1.f);*/
}

void Renderer::BeginDepthOnlyCamera(Camera const& lightCamera)
{
	m_camera = &lightCamera;

	BindShader(m_defaultShader);
	BindTexture(m_defaultTexture);
	SetSamplerMode(SamplerMode::POINTCLAMP);
	BindSamplerState();
	SetBlendMode(BlendMode::ALPHA);
	SetRasterizerState(CullMode::NONE, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
	SetDepthStencilState(DepthTest::ALWAYS, false);


	CameraConstants cameraConstants = {};
	cameraConstants.ProjectionMatrix = lightCamera.GetProjectionMatrix();
	cameraConstants.ViewMatrix = lightCamera.GetViewMatrix();
	cameraConstants.InvertedMatrix = cameraConstants.ProjectionMatrix.GetInverted();



	CopyCPUToGPU(&cameraConstants, sizeof(cameraConstants), m_cameraCBO);
	BindConstantBuffer(g_cameraBufferSlot, m_cameraCBO);

	m_modelConstants.ModelMatrix = Mat44();
	Rgba8::WHITE.GetAsFloats(m_modelConstants.ModelColor);

	CopyAndBindModelConstants();


	float clientDimensionsX = static_cast<float>(m_config.m_window->GetClientDimensions().x);
	float clientDimensionsY = static_cast<float>(m_config.m_window->GetClientDimensions().y);

	Vec2 cameraBoundSize = lightCamera.GetViewport().GetDimensions();

	AABB2 viewportBounds = AABB2(0.0f, 0.0f, clientDimensionsX, clientDimensionsY).GetBoxWithin(lightCamera.GetViewport());

	float cameraWidth = clientDimensionsX * cameraBoundSize.x;
	float cameraHeight = clientDimensionsY * cameraBoundSize.y;

	D3D11_VIEWPORT viewport{
		viewportBounds.m_mins.x,
		clientDimensionsY - viewportBounds.m_maxs.y,
		cameraWidth,
		cameraHeight,
		0,
		1
	};

	m_deviceContext->RSSetViewports(1, &viewport);

	ID3D11RenderTargetView* noRenderTargets[1] = { 0 };
	// todo: eventually depth target comes from camera
	Texture* depth = lightCamera.GetDepthTarget();
	if (depth) {
		TextureView* dsv = depth->GetDepthStencilView();
		m_deviceContext->OMSetRenderTargets(0, noRenderTargets, dsv->m_depthStencilView);
	}
}

void Renderer::EndCamera(const Camera&)
{
	m_deviceContext->ClearState();
	m_camera = nullptr;
}

void Renderer::DispatchCS(int threadX, int threadY, int threadZ)
{
	m_deviceContext->Dispatch((UINT)threadX, (UINT)threadY, (UINT)threadZ);
}

void Renderer::Draw(int numVertexes, int startOffset, TopologyMode topology)
{
	D3D11_PRIMITIVE_TOPOLOGY d11Topology = (D3D11_PRIMITIVE_TOPOLOGY)topology;
	m_deviceContext->IASetInputLayout(m_currentShader->m_inputLayout);
	m_deviceContext->IASetPrimitiveTopology(d11Topology);
	m_deviceContext->Draw(numVertexes, startOffset);
}

void Renderer::DrawVertexArray(int numVertexes, const Vertex_PCU* vertexes, TopologyMode topology) const
{
	/*glBegin(GL_TRIANGLES);
	{
		for (int vertIndex = 0; vertIndex < numVertexes; vertIndex++) {
			Vertex_PCU const& vertex = vertexes[vertIndex];
			glColor4ub(vertex.m_color.r, vertex.m_color.g, vertex.m_color.b, vertex.m_color.a);
			glTexCoord2f(vertex.m_uvTexCoords.x, vertex.m_uvTexCoords.y);

			glVertex3f(vertex.m_position.x, vertex.m_position.y, vertex.m_position.z);
		}
	}
	glEnd();*/
	CopyCPUToGPU(&m_modelConstants, sizeof(m_modelConstants), m_modelCBO);
	BindConstantBuffer(g_modelBufferSlot, m_modelCBO);

	CopyCPUToGPU(vertexes, numVertexes * sizeof(Vertex_PCU), m_immediateVBO);
	DrawVertexBuffer(m_immediateVBO, numVertexes, 0, sizeof(Vertex_PCU), topology);
}

void Renderer::DrawVertexArray(int numVertexes, const Vertex_PNCU* vertexes, TopologyMode topology) const
{
	/*glBegin(GL_TRIANGLES);
	{
		for (int vertIndex = 0; vertIndex < numVertexes; vertIndex++) {
			Vertex_PCU const& vertex = vertexes[vertIndex];
			glColor4ub(vertex.m_color.r, vertex.m_color.g, vertex.m_color.b, vertex.m_color.a);
			glTexCoord2f(vertex.m_uvTexCoords.x, vertex.m_uvTexCoords.y);

			glVertex3f(vertex.m_position.x, vertex.m_position.y, vertex.m_position.z);
		}
	}
	glEnd();*/

	CopyCPUToGPU(&m_modelConstants, sizeof(m_modelConstants), m_modelCBO);

	BindConstantBuffer(g_modelBufferSlot, m_modelCBO);

	CopyCPUToGPU(vertexes, numVertexes * sizeof(Vertex_PNCU), m_immediateVBO_PNCU);
	DrawVertexBuffer(m_immediateVBO_PNCU, numVertexes, 0, sizeof(Vertex_PNCU), topology);
}

void Renderer::DrawVertexArray(std::vector<Vertex_PCU> const& vertexes, TopologyMode topology) const
{
	DrawVertexArray((int)vertexes.size(), vertexes.data(), topology);
}

void Renderer::DrawVertexArray(std::vector<Vertex_PNCU> const& vertexes, TopologyMode topology) const
{
	DrawVertexArray((int)vertexes.size(), vertexes.data(), topology);
}

void Renderer::DrawIndexedVertexBuffer(VertexBuffer* vbo, int vertexOffset, IndexBuffer* ibo, int indexCount, int indexOffset, unsigned int sizeOfVertex, TopologyMode topology) const
{
	BindVertexBuffer(vbo, sizeOfVertex, topology);
	BindIndexBuffer(ibo);
	m_deviceContext->DrawIndexed(indexCount, indexOffset, vertexOffset);
}

void Renderer::DrawIndexedVertexArray(int numVertexes, Vertex_PCU const* vertexes, int numIndices, unsigned int const* indices, TopologyMode topology) const
{
	CopyCPUToGPU(&m_modelConstants, sizeof(m_modelConstants), m_modelCBO);
	BindConstantBuffer(g_modelBufferSlot, m_modelCBO);

	CopyCPUToGPU(vertexes, numVertexes * sizeof(Vertex_PCU), m_immediateVBO);
	CopyCPUToGPU(indices, numIndices * sizeof(unsigned int), m_indexBuffer);
	DrawIndexedVertexBuffer(m_immediateVBO, 0, m_indexBuffer, numIndices, 0, sizeof(Vertex_PCU), topology);
}

void Renderer::DrawIndexedVertexArray(std::vector<Vertex_PCU> const& vertexes, std::vector<unsigned int> const& indices, TopologyMode topology) const
{
	DrawIndexedVertexArray((int)vertexes.size(), vertexes.data(), (int)indices.size(), indices.data(), topology);
}

void Renderer::DrawIndexedVertexArray(int numVertexes, Vertex_PNCU const* vertexes, int numIndices, unsigned int const* indices, TopologyMode topology) const
{
	CopyCPUToGPU(&m_modelConstants, sizeof(m_modelConstants), m_modelCBO);
	BindConstantBuffer(g_modelBufferSlot, m_modelCBO);
	CopyAndBindLightConstants();

	CopyCPUToGPU(vertexes, numVertexes * sizeof(Vertex_PNCU), m_immediateVBO);
	CopyCPUToGPU(indices, numIndices * sizeof(unsigned int), m_indexBuffer);
	DrawIndexedVertexBuffer(m_immediateVBO, 0, m_indexBuffer, numIndices, 0, sizeof(Vertex_PNCU), topology);
}

void Renderer::DrawIndexedVertexArray(std::vector<Vertex_PNCU> const& vertexes, std::vector<unsigned int> const& indices, TopologyMode topology) const
{
	DrawIndexedVertexArray((int)vertexes.size(), vertexes.data(), (int)indices.size(), indices.data(), topology);
}

Texture* Renderer::GetTextureForFileName(char const* imageFilePath)
{
	Texture* textureToGet = nullptr;

	for (int textureIndex = 0; textureIndex < m_loadedTextures.size(); textureIndex++) {
		Texture*& loadedTexture = m_loadedTextures[textureIndex];
		if (loadedTexture->GetImageFilePath() == imageFilePath) {
			return loadedTexture;
		}
	}
	return textureToGet;
}

Texture* Renderer::CreateOrGetTextureFromFile(char const* imageFilePath)
{
	// See if we already have this texture previously loaded
	Texture* existingTexture = GetTextureForFileName(imageFilePath);
	if (existingTexture)
	{
		return existingTexture;
	}

	// Never seen this texture before!  Let's load it.
	Texture* newTexture = CreateTextureFromFile(imageFilePath);
	return newTexture;
}


Shader* Renderer::CreateOrGetShader(std::filesystem::path shaderName)
{
	std::string filename = shaderName.replace_extension(".hlsl").string();

	Shader* shaderSearched = GetShaderForName(filename.c_str());

	if (!shaderSearched) {
		return CreateShader(filename);
	}

	return shaderSearched;
}

void Renderer::BindShaderByName(char const* shaderName)
{
	Shader* shaderSearched = GetShaderForName(shaderName);

	if (!shaderSearched) {
		ERROR_AND_DIE(Stringf("SHADER WITH NAME %s WAS NOT FOUND", shaderName));
	}

	BindShader(shaderSearched);
}

void Renderer::BindShader(Shader* shader)
{
	if (!shader) {
		m_currentShader = m_defaultShader;
	}
	else {
		m_currentShader = shader;
	}
	m_deviceContext->IASetInputLayout(m_currentShader->m_inputLayout);

	if (m_currentShader->m_vertexShader) {
		m_deviceContext->VSSetShader(m_currentShader->m_vertexShader, NULL, 0);
	}


	// GS, DS, HS and CS are optional
	if (m_currentShader->m_geometryShader) {
		m_deviceContext->GSSetShader(m_currentShader->m_geometryShader, NULL, 0);
	}
	else {
		m_deviceContext->GSSetShader(NULL, NULL, 0);
	}

	if (m_currentShader->m_hullShader) {
		m_deviceContext->HSSetShader(m_currentShader->m_hullShader, NULL, 0);
	}
	else {
		m_deviceContext->HSSetShader(NULL, NULL, 0);
	}

	if (m_currentShader->m_domainShader) {
		m_deviceContext->DSSetShader(m_currentShader->m_domainShader, NULL, 0);
	}
	else {
		m_deviceContext->DSSetShader(NULL, NULL, 0);
	}

	if (m_currentShader->m_computeShader) {
		m_deviceContext->CSSetShader(m_currentShader->m_computeShader, NULL, 0);
	}
	else {
		m_deviceContext->CSSetShader(NULL, NULL, 0);
	}

	if (m_currentShader->m_pixelShader) {
		if (m_isAntialiasingEnabled && m_currentShader->m_MSAAPixelShader) {
			m_deviceContext->PSSetShader(m_currentShader->m_MSAAPixelShader, NULL, 0);
		}
		else {
			m_deviceContext->PSSetShader(m_currentShader->m_pixelShader, NULL, 0);
		}
	}

}

Shader* Renderer::GetShaderForName(char const* shaderName)
{
	std::string shaderNoExtension = std::filesystem::path(shaderName).replace_extension("").string();
	for (int shaderIndex = 0; shaderIndex < m_loadedShaders.size(); shaderIndex++) {
		Shader* shader = m_loadedShaders[shaderIndex];
		if (shader->GetName() == shaderNoExtension) {
			return shader;
		}
	}
	return nullptr;
}

Shader* Renderer::CreateCSOnlyShader(std::filesystem::path shaderName)
{
	std::string shaderFileName = shaderName.filename().replace_extension("cso").string();
	std::string compiledObjectName = "Data/CompiledShaders/" + shaderFileName;

	std::string filename = shaderName.replace_extension(".hlsl").string();
	std::string shaderSource;

	FileReadToString(shaderSource, shaderName.replace_extension("hlsl").string().c_str());

	ShaderConfig config;
	config.m_name = shaderName.string();
	Shader* newShader = new Shader(config);
	std::string debugName = "CS | " + shaderName.string();

	std::vector<uint8_t> CSShaderByteCode;

	if (FileExists(compiledObjectName)) {
		FileReadToBuffer(CSShaderByteCode, compiledObjectName);
	}
	else {
		CompileShaderToByteCode(CSShaderByteCode, shaderName.string().c_str(), shaderSource.c_str(), config.m_computeShaderEntryPoint.c_str(), "cs_5_0");
	}
	m_device->CreateComputeShader(CSShaderByteCode.data(), CSShaderByteCode.size(), NULL, &newShader->m_computeShader);
	SetDebugName(newShader->m_computeShader, debugName.c_str());

	m_loadedShaders.push_back(newShader);

	return newShader;
}

Shader* Renderer::CreateShader(std::filesystem::path shaderName, bool createWithStreamOutput, int numDeclarationEntries, SODeclarationEntry* soEntries)
{
	std::string shaderSource;
	FileReadToString(shaderSource, shaderName.replace_extension("hlsl").string().c_str());
	return CreateShader(shaderName.replace_extension("").string().c_str(), shaderSource.c_str(), createWithStreamOutput, numDeclarationEntries, soEntries);
}


Shader* Renderer::CreateShader(char const* shaderName, char const* shaderSource, bool createWithStreamOutput, int numDeclarationEntries, SODeclarationEntry* soEntries)
{
	ShaderConfig config;
	config.m_name = shaderName;

	Shader* newShader = new Shader(config);
	std::string baseName = shaderName;
	std::string debugName;

	std::vector<uint8_t> vertexShaderByteCode;
	CompileShaderToByteCode(vertexShaderByteCode, shaderName, shaderSource, config.m_vertexEntryPoint.c_str(), "vs_5_0");
	m_device->CreateVertexShader(vertexShaderByteCode.data(), vertexShaderByteCode.size(), NULL, &newShader->m_vertexShader);

	debugName = baseName + "|VShader";
	SetDebugName(newShader->m_vertexShader, debugName.c_str());

	std::string shaderNameAsString(shaderName);

	CreateInputLayoutFromVS(vertexShaderByteCode, &newShader->m_inputLayout);


	std::vector<uint8_t> pixelShaderByteCode;
	CompileShaderToByteCode(pixelShaderByteCode, shaderName, shaderSource, config.m_pixelEntryPoint.c_str(), "ps_5_0");
	m_device->CreatePixelShader(pixelShaderByteCode.data(), pixelShaderByteCode.size(), NULL, &newShader->m_pixelShader);

	debugName = baseName + "|PShader";
	SetDebugName(newShader->m_pixelShader, debugName.c_str());

	/*
	* Pixel Shader is compiled twice to support toggling antialiasing on runtime
	*/
	pixelShaderByteCode.clear();
	CompileShaderToByteCode(pixelShaderByteCode, shaderName, shaderSource, config.m_pixelEntryPoint.c_str(), "ps_5_0", true);
	m_device->CreatePixelShader(pixelShaderByteCode.data(), pixelShaderByteCode.size(), NULL, &newShader->m_MSAAPixelShader);

	debugName = baseName + "|MSAA-PShader";
	SetDebugName(newShader->m_MSAAPixelShader, debugName.c_str());

	std::vector<uint8_t> geometryShaderByteCode;

	bool compiledGeometryShader = CompileShaderToByteCode(geometryShaderByteCode, shaderName, shaderSource, config.m_geometryEntryPoint.c_str(), "gs_5_0");

	if (compiledGeometryShader) {
		if (createWithStreamOutput) {

			D3D11_SO_DECLARATION_ENTRY* DXSOEntries = new D3D11_SO_DECLARATION_ENTRY[numDeclarationEntries];
			for (int entryInd = 0; entryInd < numDeclarationEntries; entryInd++) {
				D3D11_SO_DECLARATION_ENTRY newEntry = {};
				SODeclarationEntry const& currentEntry = soEntries[entryInd];

				newEntry.Stream = 0;
				newEntry.SemanticName = currentEntry.SemanticName.c_str();
				newEntry.SemanticIndex = currentEntry.SemanticIndex;
				newEntry.StartComponent = currentEntry.StartComponent;
				newEntry.ComponentCount = currentEntry.ComponentCount;
				newEntry.OutputSlot = currentEntry.OutputSlot;
				DXSOEntries[entryInd] = newEntry;
			}
			if (DXSOEntries) {
				m_device->CreateGeometryShaderWithStreamOutput(geometryShaderByteCode.data(), geometryShaderByteCode.size(), DXSOEntries, numDeclarationEntries, NULL, 0, 0, NULL, &newShader->m_geometryShader);
			}
			delete[] DXSOEntries;
		}
		else {
			m_device->CreateGeometryShader(geometryShaderByteCode.data(), geometryShaderByteCode.size(), NULL, &newShader->m_geometryShader);
		}
		debugName = baseName;
		debugName += "|GShader";

		SetDebugName(newShader->m_geometryShader, debugName.c_str());
	}


	std::vector<uint8_t> hullShaderByteCode;

	bool compiledHullShader = CompileShaderToByteCode(hullShaderByteCode, shaderName, shaderSource, config.m_hullShaderEntryPoint.c_str(), "hs_5_0");

	if (compiledHullShader) {
		m_device->CreateHullShader(hullShaderByteCode.data(), hullShaderByteCode.size(), NULL, &newShader->m_hullShader);
		debugName = baseName;
		debugName += "|HShader";

		SetDebugName(newShader->m_hullShader, debugName.c_str());
	}

	std::vector<uint8_t> domainShaderByteCode;

	bool compiledDomainShader = CompileShaderToByteCode(domainShaderByteCode, shaderName, shaderSource, config.m_domainShaderEntryPoint.c_str(), "ds_5_0");

	if (compiledDomainShader) {
		m_device->CreateDomainShader(domainShaderByteCode.data(), domainShaderByteCode.size(), NULL, &newShader->m_domainShader);
		debugName = baseName;
		debugName += "|DShader";

		SetDebugName(newShader->m_domainShader, debugName.c_str());
	}

	std::vector<uint8_t> computeShaderByteCode;

	bool compiledComputeShader = CompileShaderToByteCode(computeShaderByteCode, shaderName, shaderSource, config.m_computeShaderEntryPoint.c_str(), "cs_5_0");

	if (compiledComputeShader) {
		m_device->CreateComputeShader(computeShaderByteCode.data(), computeShaderByteCode.size(), NULL, &newShader->m_computeShader);
		debugName = baseName;
		debugName += "|CShader";

		SetDebugName(newShader->m_computeShader, debugName.c_str());
	}

	m_loadedShaders.push_back(newShader);

	return newShader;
}

bool Renderer::CompileShaderToByteCode(std::vector<unsigned char>& outByteCode, char const* name, char const* source, char const* entryPoint, char const* target, bool isAntialiasingOn)
{
	ID3DBlob* shaderBlob;
	ID3DBlob* shaderErrorBlob;

	UINT compilerFlags = 0;

#if defined(ENGINE_DEBUG_RENDER)
	compilerFlags |= D3DCOMPILE_DEBUG;
	compilerFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	compilerFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

	std::string aaLevelAsStr = std::to_string(m_antialiasingLevel);
	char const* aaCStr = aaLevelAsStr.c_str();
	D3D_SHADER_MACRO macros[] = {
		"ENGINE_ANTIALIASING", (isAntialiasingOn) ? "1" : "0",
		"ANTIALIASING_LEVEL", aaCStr,
		NULL, NULL
	};
	HRESULT shaderCompileResult = D3DCompile(source, strlen(source), name, macros, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, target, compilerFlags, 0, &shaderBlob, &shaderErrorBlob);

	bool isOptional = AreStringsEqualCaseInsensitive(target, "gs_5_0");
	isOptional = isOptional || AreStringsEqualCaseInsensitive(target, "hs_5_0");
	isOptional = isOptional || AreStringsEqualCaseInsensitive(target, "ds_5_0");
	isOptional = isOptional || AreStringsEqualCaseInsensitive(target, "cs_5_0");

	if (!SUCCEEDED(shaderCompileResult)) {

		std::string errorString = std::string((char*)shaderErrorBlob->GetBufferPointer());
		DX_SAFE_RELEASE(shaderErrorBlob);
		DX_SAFE_RELEASE(shaderBlob);

		if (isOptional) {
			return false;
		}
		else {
			DebuggerPrintf(Stringf("%s NOT COMPILING: %s", name, errorString.c_str()).c_str());
			ERROR_AND_DIE("FAILED TO COMPILE SHADER TO BYTECODE");

		}
	}


	outByteCode.resize(shaderBlob->GetBufferSize());
	memcpy(outByteCode.data(), shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize());

	DX_SAFE_RELEASE(shaderBlob);

	return true;
}

void Renderer::CopyAndBindModelConstants() const
{
	CopyCPUToGPU(&m_modelConstants, sizeof(m_modelConstants), m_modelCBO);
	BindConstantBuffer(g_modelBufferSlot, m_modelCBO);
}

void Renderer::CopyAndBindLightConstants() const
{
	LightConstants lightConstants = {};
	lightConstants.DirectionalLight = m_directionalLight;
	m_directionalLightIntensity.GetAsFloats(lightConstants.DirectionalLightIntensity);
	m_ambientIntensity.GetAsFloats(lightConstants.AmbientIntensity);

	lightConstants.MaxOcclusionPerSample = m_SSAOMaxOcclusionPerSample;
	lightConstants.SampleRadius = m_SSAOSampleRadius;
	lightConstants.SampleSize = m_SSAOSampleSize;
	lightConstants.SSAOFalloff = m_SSAOFalloff;

	memcpy(&lightConstants.Lights, &m_lights, sizeof(m_lights));

	CopyCPUToGPU(&lightConstants, sizeof(lightConstants), m_lightCBO);
	BindConstantBuffer(1, m_lightCBO);

}

void Renderer::CopyCPUToGPU(const void* data, size_t sizeInBytes, VertexBuffer* vbo) const
{
	D3D11_MAPPED_SUBRESOURCE mappedSubresource = {};
	vbo->GuaranteeBufferSize(sizeInBytes);

	m_deviceContext->Map(vbo->m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);
	memcpy(mappedSubresource.pData, data, sizeInBytes);
	m_deviceContext->Unmap(vbo->m_buffer, 0);
}

void Renderer::CopyCPUToGPU(const void* data, size_t sizeInBytes, ConstantBuffer* cbo) const
{
	D3D11_MAPPED_SUBRESOURCE mappedSubresource = {};
	cbo->GuaranteeBufferSize(sizeInBytes);

	m_deviceContext->Map(cbo->m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);
	memcpy(mappedSubresource.pData, data, sizeInBytes);
	m_deviceContext->Unmap(cbo->m_buffer, 0);
}

void Renderer::CopyCPUToGPU(const void* data, size_t sizeInBytes, IndexBuffer* ibo) const
{
	D3D11_MAPPED_SUBRESOURCE mappedSubresource = {};
	ibo->GuaranteeBufferSize(sizeInBytes);

	m_deviceContext->Map(ibo->m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);
	memcpy(mappedSubresource.pData, data, sizeInBytes);
	m_deviceContext->Unmap(ibo->m_buffer, 0);
}

void Renderer::CopyTextureWithShader(Texture* dst, Texture* src, Texture* depthBuffer, Shader* effect, Camera const* camera)
{
	BindShader(effect);
	SetColorTarget(dst);
	BindTexture(src, 0, SHADER_BIND_PIXEL_SHADER, false);
	BindTexture(depthBuffer, 1, SHADER_BIND_PIXEL_SHADER, false);
	SetSamplerMode(SamplerMode::BILINEARCLAMP);
	SetRasterizerState(CullMode::NONE, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
	BindSamplerState();

	if (camera) {
		CameraConstants cameraConstants = {};
		cameraConstants.ProjectionMatrix = camera->GetProjectionMatrix();
		cameraConstants.ViewMatrix = camera->GetViewMatrix();
		cameraConstants.InvertedMatrix = cameraConstants.ProjectionMatrix.GetInverted();
		CopyCPUToGPU(&cameraConstants, sizeof(cameraConstants), m_cameraCBO);
		BindConstantBuffer(g_cameraBufferSlot, m_cameraCBO);
	}
	m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	D3D11_VIEWPORT viewport = {};
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<float>(m_config.m_window->GetClientDimensions().x);
	viewport.Height = static_cast<float>(m_config.m_window->GetClientDimensions().y);
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1;
	m_deviceContext->RSSetViewports(1, &viewport);


	m_deviceContext->Draw(3, 0);


	m_deviceContext->ClearState();
}


void Renderer::DownsampleTextureWithShader(Texture* dst, Texture* src, Shader* effect)
{
	BindShader(effect);
	ClearDepthTexture(dst);

	TextureView* dsv = dst->GetDepthStencilView(false);

	ID3D11RenderTargetView* noRenderTargets[1] = { 0 };
	m_deviceContext->OMSetRenderTargets(1, noRenderTargets, dsv->m_depthStencilView);
	BindTexture(src, 0);
	SetSamplerMode(SamplerMode::BILINEARCLAMP);
	SetRasterizerState(CullMode::NONE, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
	BindSamplerState();

	m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	D3D11_VIEWPORT viewport = {};
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<float>(m_config.m_window->GetClientDimensions().x);
	viewport.Height = static_cast<float>(m_config.m_window->GetClientDimensions().y);
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1;
	m_deviceContext->RSSetViewports(1, &viewport);

	m_deviceContext->Draw(3, 0);

	m_deviceContext->ClearState();
}

void Renderer::ApplyEffect(Shader* effect, Camera const* camera, Texture* customDepth)
{
	Texture* activeTarget = GetActiveColorTarget();
	Texture* backTarget = GetBackupColorTarget();

	if (m_isAntialiasingEnabled) {
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
	else {
		CopyTextureWithShader(backTarget, activeTarget, m_depthBuffer, effect, camera);
	}


	m_activeBackBuffer = (m_activeBackBuffer + 1) % 2;

}

void Renderer::BindConstantBuffer(int slot, ConstantBuffer* cbo) const
{
	m_deviceContext->VSSetConstantBuffers(slot, 1, &cbo->m_buffer);
	m_deviceContext->PSSetConstantBuffers(slot, 1, &cbo->m_buffer);
	m_deviceContext->GSSetConstantBuffers(slot, 1, &cbo->m_buffer);
	m_deviceContext->HSSetConstantBuffers(slot, 1, &cbo->m_buffer);
	m_deviceContext->DSSetConstantBuffers(slot, 1, &cbo->m_buffer);
	m_deviceContext->CSSetConstantBuffers(slot, 1, &cbo->m_buffer);
}

void Renderer::BindVertexBuffer(VertexBuffer* vbo, unsigned int sizeOfVertex, TopologyMode topology) const
{
	UINT offset = 0;

	D3D11_PRIMITIVE_TOPOLOGY d11Topology = (D3D11_PRIMITIVE_TOPOLOGY)topology;

	m_deviceContext->IASetVertexBuffers(0, 1, &vbo->m_buffer, &sizeOfVertex, &offset);
	m_deviceContext->IASetInputLayout(m_currentShader->m_inputLayout);
	m_deviceContext->IASetPrimitiveTopology(d11Topology);
}

void Renderer::BindIndexBuffer(IndexBuffer* ibo) const
{
	m_deviceContext->IASetIndexBuffer(ibo->m_buffer, DXGI_FORMAT_R32_UINT, 0);
}

void Renderer::BindSSAOKernels(int slot) const
{
	SetShaderResource(m_SSAOKernels_UAV, slot, SHADER_BIND_PIXEL_SHADER);
}

void Renderer::DrawVertexBuffer(VertexBuffer* vbo, int vertexCount, int vertexOffset, unsigned int sizeOfVertex, TopologyMode topology) const
{
	BindVertexBuffer(vbo, sizeOfVertex, topology);
	m_deviceContext->Draw(vertexCount, vertexOffset);
}

void Renderer::SetModelMatrix(Mat44 const& modelMat)
{
	m_modelConstants.ModelMatrix = modelMat;
}

void Renderer::SetModelColor(Rgba8 const& modelColor)
{
	modelColor.GetAsFloats(m_modelConstants.ModelColor);
}

Texture* Renderer::CreateTextureFromImage(Image const& image)
{

	TextureCreateInfo ci{};
	ci.m_name = image.GetImageFilePath();
	ci.m_dimensions = image.GetDimensions();
	ci.m_initialData = image.GetRawData();
	ci.m_initialSizeBytes = image.GetSizeBytes();


	Texture* newTexture = CreateTexture(ci);
	SetDebugName(newTexture->m_texture, newTexture->m_name.c_str());

	m_loadedTextures.push_back(newTexture);

	return newTexture;
}

void Renderer::SetDepthStencilState(DepthTest depthTest, bool writeDepth)
{
	DX_SAFE_RELEASE(m_depthStencilState);
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};

	depthStencilDesc.DepthEnable = TRUE;
	depthStencilDesc.DepthWriteMask = (writeDepth) ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
	depthStencilDesc.DepthFunc = (D3D11_COMPARISON_FUNC)depthTest;

	HRESULT depthStencilStateCreateHR = m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilState);

	if (!SUCCEEDED(depthStencilStateCreateHR)) {
		ERROR_AND_DIE("ERROR CREATING RASTERIZER STATE CREATION");
	}

	m_deviceContext->OMSetDepthStencilState(m_depthStencilState, 0);
}

Texture* Renderer::CreateTextureFromFile(char const* imageFilePath)
{
	//IntVec2 dimensions = IntVec2::ZERO;		// This will be filled in for us to indicate image width & height
	//int bytesPerTexel = 0; // This will be filled in for us to indicate how many color components the image had (e.g. 3=RGB=24bit, 4=RGBA=32bit)
	//int numComponentsRequested = 0; // don't care; we support 3 (24-bit RGB) or 4 (32-bit RGBA)

	//								// Load (and decompress) the image RGB(A) bytes from a file on disk into a memory buffer (array of bytes)
	//stbi_set_flip_vertically_on_load(1); // We prefer uvTexCoords has origin (0,0) at BOTTOM LEFT
	//unsigned char* texelData = stbi_load(imageFilePath, &dimensions.x, &dimensions.y, &bytesPerTexel, numComponentsRequested);

	//// Check if the load was successful
	//GUARANTEE_OR_DIE(texelData, Stringf("Failed to load image \"%s\"", imageFilePath));

	//Texture* newTexture = CreateTextureFromData(imageFilePath, dimensions, bytesPerTexel, texelData);

	//// Free the raw image texel data now that we've sent a copy of it down to the GPU to be stored in video memory
	//stbi_image_free(texelData);

	Image loadedImage(imageFilePath);
	Texture* newTexture = CreateTextureFromImage(loadedImage);


	return newTexture;
}

void Renderer::CreateDepthStencil()
{
	Window* window = Window::GetWindowContext();

	if (m_depthBuffer) DestroyTexture(m_depthBuffer);

	TextureCreateInfo info;
	info.m_name = "DefaultDepth";
	info.m_dimensions = IntVec2(window->GetClientDimensions());
	info.m_format = TextureFormat::R24G8_TYPELESS;
	info.m_bindFlags = TextureBindFlagBit::TEXTURE_BIND_DEPTH_STENCIL_BIT | TextureBindFlagBit::TEXTURE_BIND_SHADER_RESOURCE_BIT;
	info.m_memoryUsage = MemoryUsage::GPU;

	if (m_isAntialiasingEnabled) {
		info.m_sampleCount = m_antialiasingLevel;
	}
	else {
		info.m_sampleCount = 1;
	}

	m_depthBuffer = CreateTexture(info);

}

void Renderer::ClearDepth(float value) const
{
	Texture* currentDepthTarget = GetCurrentDepthTarget();

	TextureView* dsv = currentDepthTarget->GetDepthStencilView(m_isAntialiasingEnabled);
	if (dsv) {
		m_deviceContext->ClearDepthStencilView(dsv->m_depthStencilView, D3D11_CLEAR_DEPTH, value, 0);
	}
}

void Renderer::ClearDepthTexture(Texture* depthTexture, float value /*= 1.0f*/) const
{
	if (depthTexture) {
		TextureView* dsv = depthTexture->GetDepthStencilView(m_isAntialiasingEnabled);

		m_deviceContext->ClearDepthStencilView(dsv->m_depthStencilView, D3D11_CLEAR_DEPTH, value, 0);
	}
}

bool Renderer::CreateInputLayoutFromVS(std::vector<uint8_t>& shaderByteCode, ID3D11InputLayout** pInputLayout)
{
	// Reflect shader info
	ID3D11ShaderReflection* pVertexShaderReflection = NULL;
	if (FAILED(D3DReflect((void*)shaderByteCode.data(), shaderByteCode.size(), IID_ID3D11ShaderReflection, (void**)&pVertexShaderReflection)))
	{
		return false;
	}

	// Get shader info
	D3D11_SHADER_DESC shaderDesc;
	pVertexShaderReflection->GetDesc(&shaderDesc);

	// Read input layout description from shader info
	std::vector<D3D11_INPUT_ELEMENT_DESC> inputLayoutDesc;
	for (UINT i = 0; i < shaderDesc.InputParameters; i++)
	{
		D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
		pVertexShaderReflection->GetInputParameterDesc(i, &paramDesc);

		// fill out input element desc
		D3D11_INPUT_ELEMENT_DESC elementDesc;
		elementDesc.SemanticName = paramDesc.SemanticName;
		elementDesc.SemanticIndex = paramDesc.SemanticIndex;
		elementDesc.InputSlot = 0;
		elementDesc.AlignedByteOffset = (i == 0) ? 0 : D3D11_APPEND_ALIGNED_ELEMENT;
		elementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		elementDesc.InstanceDataStepRate = 0;

		// determine DXGI format
		if (paramDesc.Mask == 1)
		{
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32_FLOAT;
		}
		else if (paramDesc.Mask <= 3)
		{
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
		}
		else if (paramDesc.Mask <= 7)
		{
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
		}
		else if (paramDesc.Mask <= 15)
		{
			if (AreStringsEqualCaseInsensitive(elementDesc.SemanticName, "COLOR")) {
				elementDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			}
			else {
				if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			}


		}

		//save element desc
		inputLayoutDesc.push_back(elementDesc);
	}

	// Try to create Input Layout
	HRESULT hr = m_device->CreateInputLayout(&inputLayoutDesc[0], (UINT)inputLayoutDesc.size(), (void*)shaderByteCode.data(), (UINT)shaderByteCode.size(), pInputLayout);
	pVertexShaderReflection->Release();

	if (!SUCCEEDED(hr)) {
		ERROR_AND_DIE("SOMETHING WENT WRONG CREATING INPUT LAYOUT");
	}

	//Free allocation shader reflection memory

	return true;
}


Texture* Renderer::CreateTextureFromData(char const* name, IntVec2 dimensions, int bytesPerTexel, uint8_t* texelData)
{
	// Check if the load was successful
	GUARANTEE_OR_DIE(texelData, Stringf("CreateTextureFromData failed for \"%s\" - texelData was null!", name));
	GUARANTEE_OR_DIE(bytesPerTexel >= 3 && bytesPerTexel <= 4, Stringf("CreateTextureFromData failed for \"%s\" - unsupported BPP=%i (must be 3 or 4)", name, bytesPerTexel));
	GUARANTEE_OR_DIE(dimensions.x > 0 && dimensions.y > 0, Stringf("CreateTextureFromData failed for \"%s\" - illegal texture dimensions (%i x %i)", name, dimensions.x, dimensions.y));

	Texture* newTexture = new Texture();
	newTexture->m_name = name;
	newTexture->m_dimensions = dimensions;

	//// Enable OpenGL texturing
	//glEnable(GL_TEXTURE_2D);

	//// Tell OpenGL that our pixel data is single-byte aligned
	//glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	//// Ask OpenGL for an unused texName (ID number) to use for this texture
	//glGenTextures(1, (GLuint*)&newTexture->m_openglTextureID);

	//// Tell OpenGL to bind (set) this as the currently active texture
	//glBindTexture(GL_TEXTURE_2D, newTexture->m_openglTextureID);

	//// Set texture clamp vs. wrap (repeat) default settings
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // GL_CLAMP or GL_REPEAT
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // GL_CLAMP or GL_REPEAT

	//																// Set magnification (texel > pixel) and minification (texel < pixel) filters
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // one of: GL_NEAREST, GL_LINEAR
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // one of: GL_NEAREST, GL_LINEAR, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_LINEAR

	//																	 // Pick the appropriate OpenGL format (RGB or RGBA) for this texel data
	//GLenum bufferFormat = GL_RGBA; // the format our source pixel data is in; any of: GL_RGB, GL_RGBA, GL_LUMINANCE, GL_LUMINANCE_ALPHA, ...
	//if (bytesPerTexel == 3)
	//{
	//	bufferFormat = GL_RGB;
	//}
	//GLenum internalFormat = bufferFormat; // the format we want the texture to be on the card; technically allows us to translate into a different texture format as we upload to OpenGL

	//									  // Upload the image texel data (raw pixels bytes) to OpenGL under this textureID
	//glTexImage2D(			// Upload this pixel data to our new OpenGL texture
	//	GL_TEXTURE_2D,		// Creating this as a 2d texture
	//	0,					// Which mipmap level to use as the "root" (0 = the highest-quality, full-res image), if mipmaps are enabled
	//	internalFormat,		// Type of texel format we want OpenGL to use for this texture internally on the video card
	//	dimensions.x,		// Texel-width of image; for maximum compatibility, use 2^N + 2^B, where N is some integer in the range [3,11], and B is the border thickness [0,1]
	//	dimensions.y,		// Texel-height of image; for maximum compatibility, use 2^M + 2^B, where M is some integer in the range [3,11], and B is the border thickness [0,1]
	//	0,					// Border size, in texels (must be 0 or 1, recommend 0)
	//	bufferFormat,		// Pixel format describing the composition of the pixel data in buffer
	//	GL_UNSIGNED_BYTE,	// Pixel color components are unsigned bytes (one byte per color channel/component)
	//	texelData);		// Address of the actual pixel data bytes/buffer in system memory

	m_loadedTextures.push_back(newTexture);
	return newTexture;
}

void Renderer::BindTexture(const Texture* texture, uint32_t idx, ShaderBindBit bindFlags, bool canUseTextureMSDims)
{
	if (texture == m_lastBoundTexture) {
		return;
	}

	if (texture)
	{
		Texture* tex = const_cast<Texture*>(texture);
		TextureView* view = tex->GetShaderResourceView(canUseTextureMSDims && m_isAntialiasingEnabled);


		if (bindFlags & SHADER_BIND_VERTEX_SHADER) {
			m_deviceContext->VSSetShaderResources(idx, 1, &(view->m_shaderRscView));
		}
		if (bindFlags & SHADER_BIND_HULL_SHADER) {
			m_deviceContext->HSSetShaderResources(idx, 1, &(view->m_shaderRscView));
		}
		if (bindFlags & SHADER_BIND_DOMAIN_SHADER) {
			m_deviceContext->DSSetShaderResources(idx, 1, &(view->m_shaderRscView));
		}
		if (bindFlags & SHADER_BIND_GEOMETRY_SHADER) {
			m_deviceContext->GSSetShaderResources(idx, 1, &(view->m_shaderRscView));
		}
		if (bindFlags & SHADER_BIND_PIXEL_SHADER) {
			m_deviceContext->PSSetShaderResources(idx, 1, &(view->m_shaderRscView));
		}
		if (bindFlags & SHADER_BIND_COMPUTE_SHADER) {
			m_deviceContext->CSSetShaderResources(idx, 1, &(view->m_shaderRscView));
		}

		/*glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texture->m_openglTextureID);*/
	}
	else
	{
		TextureView* view = m_defaultTexture->GetShaderResourceView(canUseTextureMSDims && m_isAntialiasingEnabled);
		if (bindFlags & SHADER_BIND_VERTEX_SHADER) {
			m_deviceContext->VSSetShaderResources(idx, 1, &(view->m_shaderRscView));
		}
		if (bindFlags & SHADER_BIND_HULL_SHADER) {
			m_deviceContext->HSSetShaderResources(idx, 1, &(view->m_shaderRscView));
		}
		if (bindFlags & SHADER_BIND_DOMAIN_SHADER) {
			m_deviceContext->DSSetShaderResources(idx, 1, &(view->m_shaderRscView));
		}
		if (bindFlags & SHADER_BIND_GEOMETRY_SHADER) {
			m_deviceContext->GSSetShaderResources(idx, 1, &(view->m_shaderRscView));
		}
		if (bindFlags & SHADER_BIND_PIXEL_SHADER) {
			m_deviceContext->PSSetShaderResources(idx, 1, &(view->m_shaderRscView));
		}
		if (bindFlags & SHADER_BIND_COMPUTE_SHADER) {
			m_deviceContext->CSSetShaderResources(idx, 1, &(view->m_shaderRscView));
		}
		//glDisable(GL_TEXTURE_2D);
	}

	m_lastBoundTexture = texture;
}

BitmapFont* Renderer::CreateOrGetBitmapFont(char const* filePath)
{

	for (int loadedFontIndex = 0; loadedFontIndex < m_loadedFonts.size(); loadedFontIndex++) {
		BitmapFont*& bitmapFont = m_loadedFonts[loadedFontIndex];
		if (bitmapFont->m_fontFilePathNameWithNoExtension == filePath) {
			return bitmapFont;
		}
	}

	return CreateBitmapFont(filePath);
}

void Renderer::SetBlendMode(BlendMode blendMode)
{
	D3D11_BLEND_DESC blendModeDesc = {};
	blendModeDesc.RenderTarget[0].BlendEnable = true;
	blendModeDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	blendModeDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendModeDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendModeDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	blendModeDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

	switch (blendMode) {
	case BlendMode::ALPHA:
		blendModeDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendModeDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		break;
	case BlendMode::ADDITIVE:
		blendModeDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendModeDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
		break;
	case BlendMode::OPAQUE:
		blendModeDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
		blendModeDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
		break;
	default:
		ERROR_AND_DIE(Stringf("Unknown / unsupported blend mode #%i", blendMode));
		break;
	}

	m_device->CreateBlendState(&blendModeDesc, &m_blendState);
	m_deviceContext->OMSetBlendState(m_blendState, {}, 0xffffffff);

	DX_SAFE_RELEASE(m_blendState);
}


BitmapFont* Renderer::CreateBitmapFont(char const* filePath)
{
	std::string filePathString(filePath);
	filePathString.append(".png");
	Texture* bitmapTexture = CreateOrGetTextureFromFile(filePathString.c_str());
	BitmapFont* newBitmapFont = new BitmapFont(filePath, *bitmapTexture);

	m_loadedFonts.push_back(newBitmapFont);
	return newBitmapFont;
}

void Renderer::CreateDeviceAndSwapChain()
{
	Window* window = Window::GetWindowContext();
	RECT clientRect = {};

	::GetClientRect((HWND)window->m_osWindowHandle, &clientRect);

	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferDesc.Width = clientRect.right;
	swapChainDesc.BufferDesc.Height = clientRect.bottom;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.OutputWindow = (HWND)m_config.m_window->m_osWindowHandle;
	swapChainDesc.Windowed = true;

	//#if defined(ENGINE_ANTIALIASING)
	//	swapChainDesc.SampleDesc.Count = (UINT)ANTIALIASING_LEVEL;
	//	swapChainDesc.SampleDesc.Quality = 0;
	//	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	//#else
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	//#endif


	swapChainDesc.Flags = 0;

	UINT flags = 0;
#if defined(ENGINE_DEBUG_RENDER)
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flags, NULL, 0, D3D11_SDK_VERSION,
		&swapChainDesc, &m_swapChain, &m_device, nullptr, &m_deviceContext);

	if (!SUCCEEDED(hr)) {
		ERROR_AND_DIE("ERROR TRYING TO CREATE DEVICE AND SWAP CHAIN");
	}
}

void Renderer::CreateRenderTargetView()
{
	if (m_renderTarget) DestroyTexture(m_renderTarget);

	ID3D11Texture2D* texture2D = nullptr;

	HRESULT swapChainGetBuffer = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&texture2D);
	if (!SUCCEEDED(swapChainGetBuffer) || !texture2D) {
		ERROR_AND_DIE("SWAP CHAIN GET BUFFER ERROR");
	}

	D3D11_TEXTURE2D_DESC desc;
	texture2D->GetDesc(&desc);

	TextureCreateInfo info;
	info.m_name = "DefaultColor";
	info.m_dimensions = IntVec2(desc.Width, desc.Height);
	ASSERT_OR_DIE(desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM, "Swapchain isn't R8G8B8A - add conversion to local format");
	info.m_format = TextureFormat::R8G8B8A8_UNORM;
	info.m_bindFlags = TextureBindFlagBit::TEXTURE_BIND_RENDER_TARGET_BIT;
	info.m_memoryUsage = MemoryUsage::GPU;


	// If render target uses MSAA
	if (m_isAntialiasingEnabled) {
		info.m_sampleCount = m_antialiasingLevel;
	}

	info.m_handle = texture2D;
	m_renderTarget = CreateTexture(info);
	DX_SAFE_RELEASE(texture2D);



	info.m_name = "DefaultColor";
	info.m_handle = nullptr;
	info.m_bindFlags |= TextureBindFlagBit::TEXTURE_BIND_SHADER_RESOURCE_BIT;

	if (m_backBuffers[0]) DestroyTexture(m_backBuffers[0]);
	if (m_backBuffers[1]) DestroyTexture(m_backBuffers[1]);

	m_backBuffers[0] = CreateTexture(info);

	info.m_handle = nullptr;
	m_backBuffers[1] = CreateTexture(info);


}

void Renderer::CreateViewportAndRasterizerState()
{
	D3D11_VIEWPORT viewport = {};

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<float>(m_config.m_window->GetClientDimensions().x);
	viewport.Height = static_cast<float>(m_config.m_window->GetClientDimensions().y);
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1;

	m_deviceContext->RSSetViewports(1, &viewport);

	SetRasterizerState(CullMode::NONE, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
}

void Renderer::SetSamplerMode(SamplerMode samplerMode)
{
	DX_SAFE_RELEASE(m_samplerState);
	D3D11_SAMPLER_DESC samplerDesc = {};
	switch (samplerMode)
	{
	case SamplerMode::POINTCLAMP:
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		break;
	case SamplerMode::POINTWRAP:
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		break;
	case SamplerMode::BILINEARCLAMP:
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		break;
	case SamplerMode::BILINEARWRAP:
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		break;
	default:
		break;
	}
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	HRESULT samplerStateCreationResult = m_device->CreateSamplerState(&samplerDesc, &m_samplerState);
	if (!SUCCEEDED(samplerStateCreationResult)) {
		ERROR_AND_DIE("ERROR CREATING SAMPLER STATE");
	}
}

Texture* Renderer::CreateTexture(TextureCreateInfo const& createInfo)
{

	ID3D11Texture2D* handle = createInfo.m_handle;

	if (handle == nullptr) {
		D3D11_TEXTURE2D_DESC pTextureDesc = { 0 };
		pTextureDesc.Width = createInfo.m_dimensions.x;
		pTextureDesc.Height = createInfo.m_dimensions.y;
		pTextureDesc.MipLevels = 1;
		pTextureDesc.ArraySize = 1;
		pTextureDesc.Format = LocalToD3D11(createInfo.m_format);
		pTextureDesc.SampleDesc.Count = createInfo.m_sampleCount;
		pTextureDesc.Usage = LocalToD3D11(createInfo.m_memoryUsage);
		pTextureDesc.BindFlags = LocalToD3D11BindFlags(createInfo.m_bindFlags);

		D3D11_SUBRESOURCE_DATA pIntialData{};
		D3D11_SUBRESOURCE_DATA* pInitialDataPtr = nullptr;
		if (createInfo.m_initialData != nullptr) {
			pIntialData.pSysMem = createInfo.m_initialData;
			pIntialData.SysMemPitch = (UINT)createInfo.m_initialSizeBytes / (UINT)createInfo.m_dimensions.y;
			pInitialDataPtr = &pIntialData;

		}

		HRESULT textureResult = m_device->CreateTexture2D(&pTextureDesc, pInitialDataPtr, &handle);
		if (!SUCCEEDED(textureResult))
		{
			std::string errorString = "Error while creating 2d texture: ";
			errorString.append(createInfo.m_name);
			ERROR_AND_DIE(errorString);
		}

	}
	else { // handle != nullptr
		handle->AddRef();

	}

	Texture* tex = new Texture(createInfo.m_format);
	tex->m_owner = this;
	tex->m_name = createInfo.m_name;
	tex->m_dimensions = createInfo.m_dimensions;
	tex->m_texture = handle;
	tex->m_allowedBinds = createInfo.m_bindFlags;

	return tex;
}

void Renderer::DestroyTexture(Texture* texture)
{
	if (texture != nullptr) {
		delete texture;

	}
}
/*
* This only works for UAVS with FLOAT, UNORM or SNORM format
*/
void Renderer::ClearUAV(UnorderedAccessBuffer* uav, float fillValue)
{
	FLOAT clearingValue[4] = { fillValue, fillValue, fillValue, fillValue };
	m_deviceContext->ClearUnorderedAccessViewFloat(uav->m_UAV, clearingValue);
}

void Renderer::SetAntiAliasing(bool isEnabled, unsigned int aaLevel /*= 1*/)
{
	m_isAntialiasingEnabledNextFrame = isEnabled;
	if (m_antialiasingLevel != aaLevel) {
		m_rebuildAATexturesNextFrame = true;
	}
	m_antialiasingLevel = aaLevel;
}

Texture* Renderer::GetCurrentColorTarget() const
{
	if(!m_camera) GetActiveColorTarget();
	Texture* cameraColorTarget = m_camera->GetColorTarget();
	return  (cameraColorTarget) ? cameraColorTarget : GetActiveColorTarget();
}

Texture* Renderer::GetCurrentDepthTarget() const
{
	if(!m_camera) return m_depthBuffer;
	Texture* cameraDepthTarget = m_camera->GetDepthTarget();
	return  (cameraDepthTarget) ? cameraDepthTarget : m_depthBuffer;
}

void Renderer::SetColorTarget(Texture* dst)
{
	if (dst) {
		TextureView* renderTargetView = dst->GetRenderTargetView(m_isAntialiasingEnabled);
		m_deviceContext->OMSetRenderTargets(1, &renderTargetView->m_renderTgtView, nullptr);
	}
	else {
		m_deviceContext->OMSetRenderTargets(0, NULL, NULL);
	}

}

void Renderer::SetStreamOutputTarget(StreamOutputBuffer* streamOutputBuffer)
{
	m_deviceContext->SOSetTargets(1, &streamOutputBuffer->m_buffer, 0);

}

void Renderer::SetInputLayoutTopology(TopologyMode newTopology)
{
	m_deviceContext->IASetPrimitiveTopology((D3D11_PRIMITIVE_TOPOLOGY)newTopology);
}



void Renderer::SetShaderResource(UnorderedAccessBuffer* uav, int slot, ShaderBindBit bindFlags) const
{
	ID3D11ShaderResourceView* const* src = nullptr;
	if (uav) {
		src = &uav->m_SRV;
		if (bindFlags & SHADER_BIND_VERTEX_SHADER) {
			m_deviceContext->VSSetShaderResources(slot, 1, src);
		}
		if (bindFlags & SHADER_BIND_HULL_SHADER) {
			m_deviceContext->HSSetShaderResources(slot, 1, src);
		}
		if (bindFlags & SHADER_BIND_DOMAIN_SHADER) {
			m_deviceContext->DSSetShaderResources(slot, 1, src);
		}
		if (bindFlags & SHADER_BIND_GEOMETRY_SHADER) {
			m_deviceContext->GSSetShaderResources(slot, 1, src);
		}
		if (bindFlags & SHADER_BIND_PIXEL_SHADER) {
			m_deviceContext->PSSetShaderResources(slot, 1, src);
		}
		if (bindFlags & SHADER_BIND_COMPUTE_SHADER) {
			m_deviceContext->CSSetShaderResources(slot, 1, src);
		}
	}
}

void Renderer::SetUAV(UnorderedAccessBuffer* uav, int slot) const
{
	if (uav) {
		m_deviceContext->CSSetUnorderedAccessViews(slot, 1, &uav->m_UAV, NULL);
	}
	else {
		ID3D11UnorderedAccessView* ppUAViewNULL[1] = { NULL };
		m_deviceContext->CSSetUnorderedAccessViews(slot, 1, ppUAViewNULL, NULL);
	}
}



void Renderer::ClearState()
{
	m_deviceContext->ClearState();
}

Texture* Renderer::GetActiveColorTarget() const
{
	return m_backBuffers[m_activeBackBuffer];
}

Texture* Renderer::GetBackupColorTarget() const
{
	int otherInd = (m_activeBackBuffer + 1) % 2;
	return m_backBuffers[otherInd];
}

void Renderer::CopyTexture(Texture* dst, Texture* src)
{
	m_deviceContext->CopyResource(dst->m_texture, src->m_texture);
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

void Renderer::ClearLights()
{
	for (Light& light : m_lights) {
		light = Light();
	}
}

void Renderer::CreateSSAOKernelsUAV(std::vector<Vec4> kernels)
{
	if (m_SSAOKernels_UAV) {
		delete m_SSAOKernels_UAV;
		m_SSAOKernels_UAV = nullptr;
	}

	m_SSAOKernels_UAV = new UnorderedAccessBuffer(m_device, kernels.data(), kernels.size(), sizeof(Vec4), UAVType::STRUCTURED, TextureFormat::R32G32B32A32_FLOAT);
}

void Renderer::SetOcclusionPerSample(float occlusionPerSample)
{
	m_SSAOMaxOcclusionPerSample = occlusionPerSample;
}

void Renderer::SetSSAOFalloff(float SSAOFalloff)
{
	m_SSAOFalloff = SSAOFalloff;
}

void Renderer::SetSSAOSampleRadius(float sampleRadius)
{
	m_SSAOSampleRadius = sampleRadius;
}

void Renderer::SetSampleSize(int newSampleSize)
{
	m_SSAOSampleSize = newSampleSize;
}




void Renderer::BindSamplerState()
{
	m_deviceContext->PSSetSamplers(0, 1, &m_samplerState);
}

void Renderer::EnableDetailedD11Logging()
{
#if defined(ENGINE_DEBUG_RENDER)
	m_dxgiDebugModule = (void*) ::LoadLibraryA("dxgidebug.dll");
	typedef HRESULT(WINAPI* GetDebugModuleCB)(REFIID, void**);
	((GetDebugModuleCB) ::GetProcAddress((HMODULE)m_dxgiDebugModule, "DXGIGetDebugInterface"))(__uuidof(IDXGIDebug), &m_dxgiDebug);
#endif
}

void Renderer::EnableDetailedD11Shutdown()
{
#if defined(ENGINE_DEBUG_RENDER)
	((IDXGIDebug*)m_dxgiDebug)->ReportLiveObjects(DXGI_DEBUG_ALL, (DXGI_DEBUG_RLO_FLAGS)(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
	((IDXGIDebug*)m_dxgiDebug)->Release();
	m_dxgiDebug = nullptr;
	::FreeLibrary((HMODULE)m_dxgiDebugModule);
	m_dxgiDebugModule = nullptr;
#endif
}

void Renderer::SetDebugName(ID3D11DeviceChild* object, char const* name)
{
#if defined(ENGINE_DEBUG_RENDER)
	object->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen(name), name);
#else
	UNUSED(object);
	UNUSED(name);
#endif
}

void Renderer::SetRasterizerState(CullMode cullMode, FillMode fillMode, WindingOrder windingOrder)
{
	DX_SAFE_RELEASE(m_rasterizerState);
	D3D11_RASTERIZER_DESC rasterizerStruct = {};

	D3D11_FILL_MODE rasterizerFillMode = (fillMode == FillMode::SOLID) ? D3D11_FILL_SOLID : D3D11_FILL_WIREFRAME;
	D3D11_CULL_MODE rasterizerCullMode = D3D11_CULL_NONE;

	if (cullMode == CullMode::FRONT) {
		rasterizerCullMode = D3D11_CULL_FRONT;
	}
	else if (cullMode == CullMode::BACK) {
		rasterizerCullMode = D3D11_CULL_BACK;
	}

	rasterizerStruct.FillMode = rasterizerFillMode;
	rasterizerStruct.CullMode = rasterizerCullMode;
	rasterizerStruct.FrontCounterClockwise = (windingOrder == WindingOrder::COUNTERCLOCKWISE);
	rasterizerStruct.DepthClipEnable = true;
	rasterizerStruct.AntialiasedLineEnable = true;
	if (m_isAntialiasingEnabled) {
		rasterizerStruct.MultisampleEnable = true;

	}


	HRESULT rasterizerStateCreationHR = m_device->CreateRasterizerState(&rasterizerStruct, &m_rasterizerState);
	if (!SUCCEEDED(rasterizerStateCreationHR)) {
		ERROR_AND_DIE("ERROR CREATING RASTERIZER STATE CREATION");
	}

	m_deviceContext->RSSetState(m_rasterizerState);
}
