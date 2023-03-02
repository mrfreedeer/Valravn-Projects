#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Renderer/TextureView.hpp"
#include "Engine/Renderer/D3D11/D3D11TypeConversions.hpp"
#include "Game/EngineBuildPreferences.hpp"
#include <d3d11.h>

Texture::Texture(TextureFormat format) :
	m_format(format)
{
}

Texture::~Texture()
{
	DX_SAFE_RELEASE(m_texture);

	for (TextureView* view : m_views) {
		delete view;
	}

	m_views.clear();
}

TextureView* Texture::GetShaderResourceView(bool multiSampled)
{
	TextureViewInfo info;
	info.m_type = TextureBindFlagBit::TEXTURE_BIND_SHADER_RESOURCE_BIT;
	info.m_isMultiSampled = multiSampled;
	return  GetOrCreateView(info);
}

TextureView* Texture::GetDepthStencilView(bool multiSampled)
{
	TextureViewInfo info;
	info.m_type = TextureBindFlagBit::TEXTURE_BIND_DEPTH_STENCIL_BIT;
	info.m_isMultiSampled = multiSampled;

	return  GetOrCreateView(info);
}

TextureView* Texture::GetRenderTargetView(bool multiSampled)
{
	TextureViewInfo info;
	info.m_type = TextureBindFlagBit::TEXTURE_BIND_RENDER_TARGET_BIT;
	info.m_isMultiSampled = multiSampled;
	return  GetOrCreateView(info);
}

TextureView* Texture::GetOrCreateView(TextureViewInfo const& viewInfo)
{
	ASSERT_OR_DIE(m_allowedBinds & viewInfo.m_type, "CREATING AN UNSUPPORTED VIEW ON A TEXTURE");


	for (TextureView* view : m_views) {
		if (view->m_info == viewInfo) {
			return view;
		}
	}


	TextureView* newView = new TextureView();
	newView->m_handle = nullptr;
	newView->m_source = this;
	m_texture->AddRef();
	newView->m_sourceHandle = m_texture;
	newView->m_info = viewInfo;

	ID3D11Device* device = m_owner->m_device;
	switch (viewInfo.m_type)
	{
	case TextureBindFlagBit::TEXTURE_BIND_SHADER_RESOURCE_BIT: {
		D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
		desc.Format = LocalToD3D11ColorFormat(m_format);
		if (viewInfo.m_isMultiSampled) {
			desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
		}
		else {
			desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		}
		desc.Texture2D.MostDetailedMip = 0;
		desc.Texture2D.MipLevels = 1;

		device->CreateShaderResourceView(m_texture, &desc, &newView->m_shaderRscView);
		break;
	}
	case TextureBindFlagBit::TEXTURE_BIND_RENDER_TARGET_BIT: {
		D3D11_RENDER_TARGET_VIEW_DESC desc = {};
		if (viewInfo.m_isMultiSampled) {
			desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
		}
		else {
			desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		}
		device->CreateRenderTargetView(m_texture, &desc, &newView->m_renderTgtView);
		break;
	}
	case TextureBindFlagBit::TEXTURE_BIND_DEPTH_STENCIL_BIT: {
		D3D11_DEPTH_STENCIL_VIEW_DESC desc = {};
		desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		if (viewInfo.m_isMultiSampled) {
			desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
		}
		else {
			desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		}
		desc.Flags = 0;
		desc.Texture2D.MipSlice = 0;

		device->CreateDepthStencilView(m_texture, &desc, &newView->m_depthStencilView);
		break;
	}
	default: ERROR_AND_DIE("UNHANDLED VIEW TYPE");
	}
	m_views.push_back(newView);

	return newView;
}

