#pragma once
#include "Engine/Core/EngineCommon.hpp"

struct ID3D11Buffer;
struct ID3D11Device;


class VertexBuffer
{
	friend class Renderer;

public:
	VertexBuffer(ID3D11Device* device, size_t size, size_t strideSize = 0, MemoryUsage memoryUsage = MemoryUsage::Dynamic, void const* data = nullptr);
	VertexBuffer(const VertexBuffer& copy) = delete;
	virtual ~VertexBuffer();

	unsigned int GetStride() const;
	bool GuaranteeBufferSize(size_t newsize);


	ID3D11Buffer* m_buffer = nullptr;
	ID3D11Device* m_device = nullptr;
	size_t m_size = 0;
	size_t m_stride = 0;
};
