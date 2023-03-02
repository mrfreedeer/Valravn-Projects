#pragma once
#include <d3d11.h>

enum class TextureFormat;
enum class MemoryUsage;
typedef unsigned int TextureBindFlag;

DXGI_FORMAT LocalToD3D11(TextureFormat format);
DXGI_FORMAT LocalToD3D11ColorFormat(TextureFormat format);
D3D11_USAGE LocalToD3D11(MemoryUsage hint);
UINT LocalToD3D11BindFlags(TextureBindFlag const flags);
