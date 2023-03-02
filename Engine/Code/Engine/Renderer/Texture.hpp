#pragma once
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/IntVec2.hpp"
#include <vector>

struct ID3D11ShaderResourceView;
struct ID3D11Texture2D;

class TextureView;

enum class TextureFormat : int {
		R8G8B8A8_UNORM,
		R32G32B32A32_FLOAT,
		D24_UNORM_S8_UINT,
		R24G8_TYPELESS,
		R32_FLOAT
};

enum  TextureBindFlagBit : unsigned int {
	TEXTURE_BIND_NONE = 0,
	TEXTURE_BIND_SHADER_RESOURCE_BIT = (1 << 0),
	TEXTURE_BIND_RENDER_TARGET_BIT = (1 << 1),
	TEXTURE_BIND_DEPTH_STENCIL_BIT = (1 << 2),
};


typedef unsigned int TextureBindFlag;

struct TextureCreateInfo {
	std::string m_name = "Unnamed texture";
	IntVec2 m_dimensions = IntVec2::ZERO;
	TextureFormat m_format = TextureFormat::R8G8B8A8_UNORM;
	TextureBindFlag m_bindFlags = TextureBindFlagBit::TEXTURE_BIND_SHADER_RESOURCE_BIT;
	MemoryUsage m_memoryUsage = MemoryUsage::Default;

	void const* m_initialData = nullptr;
	size_t m_initialSizeBytes = 0;
	ID3D11Texture2D* m_handle = nullptr;
	unsigned int m_sampleCount = 1;
};

struct TextureViewInfo {
	TextureBindFlag m_type;
	inline bool operator==(TextureViewInfo const& other) const {
		return m_type == other.m_type;
	}
	bool m_isMultiSampled = false;
};

class Texture
{
	friend class Renderer; // Only the Renderer can create new Texture objects!

private:
	Texture() {}; // can't instantiate directly; must ask Renderer to do it for you
	Texture(TextureFormat format);
	Texture(Texture const& copy) = delete; // No copying allowed!  This represents GPU memory.
	~Texture();

	TextureView* GetShaderResourceView(bool multiSampled = false);
	TextureView* GetDepthStencilView(bool multiSampled = false);
	TextureView* GetRenderTargetView(bool multiSampled = false);

	TextureView* GetOrCreateView(TextureViewInfo const& viewInfo);

public:
	IntVec2				GetDimensions() const { return m_dimensions; }
	std::string const& GetImageFilePath() const { return m_name; }
protected:
	std::string			m_name = "";
	IntVec2				m_dimensions = {};

	ID3D11Texture2D* m_texture = nullptr;
	TextureBindFlag m_allowedBinds = TextureBindFlagBit::TEXTURE_BIND_NONE;
	std::vector<TextureView*> m_views;

	Renderer* m_owner = nullptr;
	TextureFormat m_format = TextureFormat::R8G8B8A8_UNORM;
};

