#include "Engine/Renderer/TextureView.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include <d3d11.h>

TextureView::TextureView() {
	m_handle = nullptr;
}

TextureView::~TextureView()
{
	DX_SAFE_RELEASE(m_sourceHandle);
	DX_SAFE_RELEASE(m_handle);
}
