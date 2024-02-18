//#include "Engine/Renderer/D3D11/D3D11TypeConversions.hpp"
//#include "Engine/Renderer/Texture.hpp"
//#include "Engine/Core/EngineCommon.hpp"
//
//DXGI_FORMAT LocalToD3D11(TextureFormat format)
//{
//	switch (format) {
//	case TextureFormat::R8G8B8A8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM;
//	case TextureFormat::D24_UNORM_S8_UINT: return DXGI_FORMAT_D24_UNORM_S8_UINT;
//	case TextureFormat::R32G32B32A32_FLOAT: return DXGI_FORMAT_R32G32B32A32_FLOAT;
//	case TextureFormat::R32G32_FLOAT: return DXGI_FORMAT_R32G32_FLOAT;
//	case TextureFormat::R24G8_TYPELESS: return DXGI_FORMAT_R24G8_TYPELESS;
//	case TextureFormat::R32_FLOAT: return DXGI_FORMAT_R32_FLOAT;
//	default: ERROR_AND_DIE("Unsupported format");
//
//	}
//}
//
//DXGI_FORMAT LocalToD3D11ColorFormat(TextureFormat format)
//{
//	switch (format) {
//	case TextureFormat::R24G8_TYPELESS: return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
//	default: return LocalToD3D11(format);
//
//	}
//}
//
//D3D11_USAGE LocalToD3D11(MemoryUsage hint)
//{
//	switch (hint) {
//	case MemoryUsage::Default: return D3D11_USAGE_IMMUTABLE;
//	case MemoryUsage::GPU: return D3D11_USAGE_DEFAULT;
//	case MemoryUsage::Dynamic: return D3D11_USAGE_DYNAMIC;
//	default: ERROR_AND_DIE("Unsupported format");
//
//	}
//}
//
//UINT LocalToD3D11BindFlags(TextureBindFlag const flags)
//{
//	UINT result = 0;
//
//	if (flags & TextureBindFlagBit::TEXTURE_BIND_SHADER_RESOURCE_BIT) {
//		result |= D3D11_BIND_SHADER_RESOURCE;
//
//	}
//
//	if (flags & TextureBindFlagBit::TEXTURE_BIND_RENDER_TARGET_BIT) {
//		result |= D3D11_BIND_RENDER_TARGET;
//
//	}
//
//	if (flags & TextureBindFlagBit::TEXTURE_BIND_DEPTH_STENCIL_BIT) {
//		result |= D3D11_BIND_DEPTH_STENCIL;
//
//	}
//
//	return result;
//}