#pragma once
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Renderer/ResourceView.hpp"
#include "Engine/Core/Rgba8.hpp"
#include <vector>
#include <string>

class Resource;
class TextureView;
class Renderer;

enum class TextureFormat : int {
		R8G8B8A8_UNORM,
		R32G32B32A32_FLOAT,
		R32G32_FLOAT,
		D24_UNORM_S8_UINT,
		R24G8_TYPELESS,
		R32_FLOAT
};


struct TextureCreateInfo {
	std::string m_name = "Unnamed Texture";
	Renderer* m_owner = nullptr;
	IntVec2 m_dimensions = IntVec2::ZERO;
	size_t m_stride = 0;
	ResourceBindFlag m_bindFlags = RESOURCE_BIND_NONE;
	TextureFormat m_format = TextureFormat::R8G8B8A8_UNORM;
	TextureFormat m_clearFormat = TextureFormat::R8G8B8A8_UNORM;
	bool m_isMultiSample = false;
	Resource* m_handle = nullptr;
	void* m_initialData = nullptr;
	Rgba8 m_clearColour = Rgba8();
};


class Texture {
friend class Renderer;
public:
	/// <summary>
	/// Checks if view has been created, or creates it otherwise
	/// </summary>
	ResourceView* GetOrCreateView(ResourceBindFlagBit viewType);
	IntVec2 GetDimensions() const;
	Resource* GetResource() const;
	ID3D12Resource* GetRawResource() const;
	Rgba8 const GetClearColour() const;
	std::string const& GetImageFilePath() const { return m_name; }
private:
	/// Views are created in DX12 but they're not a separate structure
	ResourceView* CreateShaderResourceView();
	ResourceView* CreateRenderTargetView();
	ResourceView* CreateDepthStencilView();
	Texture();
	Texture(TextureCreateInfo const& createInfo);
	~Texture();
private:
	Renderer* m_owner = nullptr;
	std::string m_name = "Unammed Texture";
	TextureCreateInfo m_creationInfo;
	Resource* m_handle = nullptr;
	std::vector<ResourceView*> m_views;
};