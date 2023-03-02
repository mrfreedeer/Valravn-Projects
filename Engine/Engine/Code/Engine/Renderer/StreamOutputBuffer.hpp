#pragma once
#include "Engine/Core/EngineCommon.hpp"

struct ID3D11Buffer;
struct ID3D11Device;
struct ID3D11ShaderResourceView;


class StreamOutputBuffer
{
	friend class Renderer;

public:
	StreamOutputBuffer(ID3D11Device* device, size_t size);
	StreamOutputBuffer(const StreamOutputBuffer& copy) = delete;
	virtual ~StreamOutputBuffer();


	ID3D11Buffer* m_buffer = nullptr;
	ID3D11Device* m_device = nullptr;

	size_t m_size = 0;

	Renderer* m_owner = nullptr;
};
