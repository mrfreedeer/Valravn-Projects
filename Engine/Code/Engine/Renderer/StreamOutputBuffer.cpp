//#include "Engine/Renderer/StreamOutputBuffer.hpp"
//#include "Engine/Renderer/Renderer.hpp"
//#include "Engine/Core/ErrorWarningAssert.hpp"
//#include <d3d11.h> 
//
//StreamOutputBuffer::StreamOutputBuffer(ID3D11Device* device, size_t size) :
//	m_device(device),
//	m_size(size)
//{
//	D3D11_BUFFER_DESC bufferDesc = {};
//	bufferDesc.ByteWidth = static_cast<UINT>(size);
//	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
//	bufferDesc.BindFlags = D3D11_BIND_STREAM_OUTPUT | D3D11_BIND_VERTEX_BUFFER;
//
//	HRESULT bufferCreationResult = m_device->CreateBuffer(&bufferDesc, NULL, &m_buffer);
//	if (!SUCCEEDED(bufferCreationResult)) {
//		ERROR_AND_DIE("STREAM OUTPUT BUFFER CREATION RESULT ERROR");
//	}
//
//}
//
//StreamOutputBuffer::~StreamOutputBuffer()
//{
//	DX_SAFE_RELEASE(m_buffer);
//	m_buffer = nullptr;
//
//}
