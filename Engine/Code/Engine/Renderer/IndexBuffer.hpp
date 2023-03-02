#pragma once

struct ID3D11Buffer;
struct ID3D11Device;

class IndexBuffer
{
	friend class Renderer;

public:
	IndexBuffer(ID3D11Device* device, size_t size);
	IndexBuffer(const IndexBuffer& copy) = delete;
	virtual ~IndexBuffer();

	unsigned int GetStride() const;
	bool GuaranteeBufferSize(size_t newsize);


	ID3D11Buffer* m_buffer = nullptr;
	ID3D11Device* m_device = nullptr;
	size_t m_size = 0;
};
