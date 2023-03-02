#pragma once

struct ID3D11Buffer;
struct ID3D11Device;

class ConstantBuffer
{
	friend class Renderer;

public:
	ConstantBuffer(ID3D11Device* device, size_t sizeBytes);
	ConstantBuffer(const ConstantBuffer& copy) = delete;
	virtual ~ConstantBuffer();

	virtual bool GuaranteeBufferSize(size_t newSizeBytes);

	ID3D11Buffer* m_buffer = nullptr;
	ID3D11Device* m_device = nullptr;
	size_t m_size = 0;
};
