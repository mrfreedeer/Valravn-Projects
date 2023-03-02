#pragma once
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Renderer/Texture.hpp"

struct ID3D11Texture2D;
struct ID3D11View;
struct ID3D11ShaderResourceView;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;


class TextureView {
	friend class Renderer;
	friend class Texture;

private:
	TextureView();
	~TextureView();

	TextureView(TextureView const&) = delete;


private:
	Texture* m_source = nullptr;
	ID3D11Texture2D* m_sourceHandle = nullptr;
	TextureViewInfo m_info;
	union {
		ID3D11View* m_handle;
		ID3D11ShaderResourceView* m_shaderRscView;
		ID3D11RenderTargetView* m_renderTgtView;
		ID3D11DepthStencilView* m_depthStencilView;
	};
};