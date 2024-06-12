#include "Engine/Renderer/GraphicsCommon.hpp"
#include "Engine/Core/StringUtils.hpp"

BlendMode ParseFromString(std::string const& strBlendMode, BlendMode defaultBlendMode)
{
	if (AreStringsEqualCaseInsensitive(strBlendMode, "ALPHA")) return BlendMode::ALPHA;
	if (AreStringsEqualCaseInsensitive(strBlendMode, "ADDITIVE")) return BlendMode::ADDITIVE;
	if (AreStringsEqualCaseInsensitive(strBlendMode, "OPAQUE")) return BlendMode::OPAQUE;
	return defaultBlendMode;
}

TextureFormat ParseFromString(std::string const& text, TextureFormat defaultFormat)
{
	if(AreStringsEqualCaseInsensitive(text, "R8G8B8A8_UNORM")) return TextureFormat::R8G8B8A8_UNORM;
	if(AreStringsEqualCaseInsensitive(text, "R32G32B32A32_FLOAT")) return TextureFormat::R32G32B32A32_FLOAT;
	if(AreStringsEqualCaseInsensitive(text, "R32G32_FLOAT")) return TextureFormat::R32G32_FLOAT;
	if(AreStringsEqualCaseInsensitive(text, "D24_UNORM_S8_UINT")) return TextureFormat::D24_UNORM_S8_UINT;
	if(AreStringsEqualCaseInsensitive(text, "R24G8_TYPELESS")) return TextureFormat::R24G8_TYPELESS;
	if(AreStringsEqualCaseInsensitive(text, "R32_FLOAT")) return TextureFormat::R32_FLOAT;
	if(AreStringsEqualCaseInsensitive(text, "UNKNOWN")) return TextureFormat::UNKNOWN;

	return defaultFormat;
}
