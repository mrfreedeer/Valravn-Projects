#include "Engine/Renderer/D3D12/D3D12TypeConversions.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/Buffer.hpp"

DXGI_FORMAT LocalToD3D12(TextureFormat textureFormat)
{
	switch (textureFormat) {
	case TextureFormat::R8G8B8A8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM;
	case TextureFormat::D24_UNORM_S8_UINT: return DXGI_FORMAT_D24_UNORM_S8_UINT;
	case TextureFormat::R32G32B32A32_FLOAT: return DXGI_FORMAT_R32G32B32A32_FLOAT;
	case TextureFormat::R32G32_FLOAT: return DXGI_FORMAT_R32G32_FLOAT;
	case TextureFormat::R24G8_TYPELESS: return DXGI_FORMAT_R24G8_TYPELESS;
	case TextureFormat::R32_FLOAT: return DXGI_FORMAT_R32_FLOAT;
	case TextureFormat::D32_FLOAT: return DXGI_FORMAT_D32_FLOAT;
	case TextureFormat::UNKNOWN: return DXGI_FORMAT_UNKNOWN;
	default: ERROR_AND_DIE("Unsupported format");
	}
}

D3D12_FILL_MODE LocalToD3D12(FillMode fillMode)
{
	switch (fillMode)
	{
	case FillMode::SOLID: return D3D12_FILL_MODE_SOLID;
	case FillMode::WIREFRAME: return D3D12_FILL_MODE_WIREFRAME;
	default: ERROR_AND_DIE("Unsupported format");
	}
}

D3D12_CULL_MODE LocalToD3D12(CullMode cullMode)
{
	switch (cullMode)
	{
	case CullMode::NONE: return D3D12_CULL_MODE_NONE;
	case CullMode::FRONT: return D3D12_CULL_MODE_FRONT;
	case CullMode::BACK: return D3D12_CULL_MODE_BACK;
	default: ERROR_AND_DIE("Unsupported cull mode");
	}
}

BOOL LocalToD3D12(WindingOrder windingOrder)
{
	switch (windingOrder)
	{
	case WindingOrder::CLOCKWISE: return FALSE;
	case WindingOrder::COUNTERCLOCKWISE: return TRUE;
	default: ERROR_AND_DIE("Unsupported winding order");
	}
}

D3D12_COMPARISON_FUNC LocalToD3D12(DepthFunc depthTest)
{
	switch (depthTest) {
	case DepthFunc::NEVER: return D3D12_COMPARISON_FUNC_NEVER;
	case DepthFunc::LESS: return D3D12_COMPARISON_FUNC_LESS;
	case DepthFunc::EQUAL: return D3D12_COMPARISON_FUNC_EQUAL;
	case DepthFunc::LESSEQUAL: return D3D12_COMPARISON_FUNC_LESS_EQUAL;
	case DepthFunc::GREATER: return D3D12_COMPARISON_FUNC_GREATER;
	case DepthFunc::NOTEQUAL: return D3D12_COMPARISON_FUNC_NOT_EQUAL;
	case DepthFunc::GREATEREQUAL: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	case DepthFunc::ALWAYS: return D3D12_COMPARISON_FUNC_ALWAYS;
	default:
		ERROR_AND_DIE(Stringf("UNSUPPORTED DEPTH TEST %d", (int)depthTest).c_str());
	}

}

D3D12_VERTEX_BUFFER_VIEW LocalToD3D12(BufferView const& bufferView)
{
	return {bufferView.m_bufferLocation, (UINT)bufferView.m_sizeInBytes, (UINT)bufferView.m_strideInBytes};
}

DXGI_FORMAT LocalToColourD3D12(TextureFormat textureFormat)
{
	switch (textureFormat) {
	case TextureFormat::R24G8_TYPELESS: return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	case TextureFormat::D32_FLOAT: return DXGI_FORMAT_R32_FLOAT;
	default: return LocalToD3D12(textureFormat);
	}
}


D3D12_RESOURCE_FLAGS LocalToD3D12(ResourceBindFlag flags)
{
	D3D12_RESOURCE_FLAGS result = D3D12_RESOURCE_FLAG_NONE;

	if (flags & ResourceBindFlagBit::RESOURCE_BIND_SHADER_RESOURCE_BIT) {
		result |= D3D12_RESOURCE_FLAG_NONE;
	}

	if (flags & ResourceBindFlagBit::RESOURCE_BIND_RENDER_TARGET_BIT) {
		result |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	}

	if (flags & ResourceBindFlagBit::RESOURCE_BIND_DEPTH_STENCIL_BIT) {
		result |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	}

	if (flags & ResourceBindFlagBit::RESOURCE_BIND_UNORDERED_ACCESS_VIEW_BIT) {
		result |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	return result;
}


D3D12_DESCRIPTOR_HEAP_TYPE LocalToD3D12(DescriptorHeapType dHeapType)
{
	switch (dHeapType)
	{
	case DescriptorHeapType::SRV_UAV_CBV:
		return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	case DescriptorHeapType::RTV:
		return D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	case DescriptorHeapType::SAMPLER:
		return D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	case DescriptorHeapType::DSV:
		return D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	}
	ERROR_AND_DIE("UNKNOWN DESCRIPTOR HEAP TYPE");
}

