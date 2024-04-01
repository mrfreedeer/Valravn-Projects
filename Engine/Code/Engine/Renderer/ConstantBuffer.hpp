#pragma once
#include "Engine/Core/Buffer.hpp"

class ResourceView;

class ConstantBuffer : public Buffer
{
	friend class Renderer;

public:
	ConstantBuffer(BufferDesc const& bufferDesc);
	~ConstantBuffer();
	ResourceView* GetOrCreateView();
	virtual void Initialize() override;
	virtual void CopyCPUToGPU(void const* data, size_t sizeInBytes) override;
private:
	size_t AlignToCBufferStride(size_t size) const;
private:
	ResourceView* m_bufferView = nullptr;
	bool m_initialized = false;
};
