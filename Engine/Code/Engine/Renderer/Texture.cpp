#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/GraphicsCommon.hpp"
#include "Engine/Renderer/D3D12/D3D12TypeConversions.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Renderer/D3D12/Resource.hpp"
#include <d3d12.h>


Texture::Texture(TextureCreateInfo const& createInfo) :
	m_name(createInfo.m_name),
	m_creationInfo(createInfo),
	m_owner(createInfo.m_owner)
{
}

Texture::Texture()
{

}

Texture::~Texture()
{
	for (ResourceView*& rView : m_views) {
		if (rView) {
			delete rView;
			rView = nullptr;
		}
	}
}

ResourceView* Texture::CreateShaderResourceView()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = new D3D12_SHADER_RESOURCE_VIEW_DESC();
	/*
	* SHADER COMPONENT MAPPING RGBA = 0,1,2,3
	* 4 Force a value of 0
	* 5 Force a value of 1
	*/
	srvDesc->Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(0, 1, 2, 3);
	srvDesc->Format = LocalToColourD3D12(m_creationInfo.m_format);
	srvDesc->ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc->Texture2D.MipLevels = 1;

	ResourceViewInfo viewInfo = {};
	viewInfo.m_srvDesc = srvDesc;
	viewInfo.m_viewType = RESOURCE_BIND_SHADER_RESOURCE_BIT;
	viewInfo.m_source = m_handle;

	ResourceView* newView = m_owner->CreateResourceView(viewInfo);
	m_views.push_back(newView);

	return newView;
}

ResourceView* Texture::CreateRenderTargetView()
{
	D3D12_RENDER_TARGET_VIEW_DESC* rtvDesc = new D3D12_RENDER_TARGET_VIEW_DESC();

	rtvDesc->ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	rtvDesc->Format = LocalToD3D12(m_creationInfo.m_format);

	ResourceViewInfo viewInfo = {};
	viewInfo.m_rtvDesc = rtvDesc;
	viewInfo.m_viewType = RESOURCE_BIND_RENDER_TARGET_BIT;
	viewInfo.m_source = m_handle;

	ResourceView* newView = m_owner->CreateResourceView(viewInfo);
	m_views.push_back(newView);

	return newView;

}

ResourceView* Texture::CreateDepthStencilView()
{
	D3D12_DEPTH_STENCIL_VIEW_DESC* dsvDesc = new D3D12_DEPTH_STENCIL_VIEW_DESC();
	dsvDesc->Format = LocalToD3D12(m_creationInfo.m_clearFormat);
	dsvDesc->ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc->Flags = D3D12_DSV_FLAG_NONE;

	ResourceViewInfo viewInfo = {};
	viewInfo.m_dsvDesc = dsvDesc;
	viewInfo.m_viewType = RESOURCE_BIND_DEPTH_STENCIL_BIT;
	viewInfo.m_source = m_handle;

	ResourceView* newView = m_owner->CreateResourceView(viewInfo);
	m_views.push_back(newView);

	return newView;
}



ResourceView* Texture::CreateUnorderedAccessView()
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc = new D3D12_UNORDERED_ACCESS_VIEW_DESC();
	uavDesc->Texture2D.MipSlice = 0;
	uavDesc->Format = LocalToD3D12(m_creationInfo.m_clearFormat);
	uavDesc->ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

	ResourceViewInfo viewInfo = {};
	viewInfo.m_uavDesc = uavDesc;
	viewInfo.m_viewType = RESOURCE_BIND_UNORDERED_ACCESS_VIEW_BIT;
	viewInfo.m_source = m_handle;

	ResourceView* newView = m_owner->CreateResourceView(viewInfo);
	m_views.push_back(newView);

	return newView;
}

ResourceView* Texture::GetOrCreateView(ResourceBindFlagBit viewType)
{
	for (ResourceView*& currentView : m_views) {
		if (currentView->m_viewInfo.m_viewType == viewType) {
			return currentView;
		}
	}

	switch (viewType)
	{
	case RESOURCE_BIND_SHADER_RESOURCE_BIT:
		return CreateShaderResourceView();
	case RESOURCE_BIND_RENDER_TARGET_BIT:
		return CreateRenderTargetView();
	case RESOURCE_BIND_DEPTH_STENCIL_BIT:
		return CreateDepthStencilView();
	case RESOURCE_BIND_UNORDERED_ACCESS_VIEW_BIT:
		return CreateUnorderedAccessView();
	case RESOURCE_BIND_NONE:
	default:
		ERROR_AND_DIE(Stringf("UNSUPPORTED VIEW %d", viewType));
		break;
	}

}

bool Texture::IsBindCompatible(ResourceBindFlag bindFlag) const
{
	bool isCompatible = m_creationInfo.m_bindFlags & bindFlag;
	return isCompatible;
}

IntVec2 Texture::GetDimensions() const
{
	return m_creationInfo.m_dimensions;
}

Resource* Texture::GetResource() const
{
	return m_handle;
}

ID3D12Resource* Texture::GetRawResource() const
{
	return m_handle->m_resource;
}

Rgba8 const Texture::GetClearColour() const
{
	return m_creationInfo.m_clearColour;
}

void Texture::TransitionTo(D3D12_RESOURCE_STATES newState, ID3D12GraphicsCommandList* commList)
{
	m_handle->TransitionTo(newState, commList);
}

void Texture::TransitionTo(D3D12_RESOURCE_STATES newState, ComPtr<ID3D12GraphicsCommandList> commList)
{
	m_handle->TransitionTo(newState, commList);
}
