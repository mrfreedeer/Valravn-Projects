#pragma once
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Renderer/D3D12/DescriptorHeap.hpp"
#include "Engine/Renderer/ResourceView.hpp"
#include <stdint.h>

class Resource;
class Renderer;
struct ID3D12Resource2;

struct BufferView {
	size_t m_bufferLocation;
	size_t m_sizeInBytes;
	size_t m_strideInBytes;
};

struct BufferDesc {
	Renderer* owner = nullptr;
	size_t size = 0;
	size_t stride = 0;
	MemoryUsage memoryUsage = MemoryUsage::Dynamic;
	void const* data = nullptr;
	DescriptorHeap* descriptorHeap = nullptr;
};

class Buffer {
	friend class Renderer;
public:
	Buffer(BufferDesc const& bufferDesc);
	virtual ~Buffer();

	/// <summary>
	/// Only works for dynamic buffers
	/// </summary>
	/// <param name="newsize"></param>
	/// <returns></returns>
	bool GuaranteeBufferSize(size_t newsize);
	size_t GetStride() const { return m_stride; }

	BufferView GetBufferView() const;
	ResourceView* GetOrCreateView(ResourceBindFlagBit viewType);

	virtual void CopyCPUToGPU(void const* data, size_t sizeInBytes);
	size_t GetSize() const { return m_size; }
protected:
	virtual void Initialize();
	virtual void CreateDynamicBuffer(void const* data);
	virtual void CreateBuffer(Resource*& buffer, bool isUpload = false);
	void CreateAndCopyToUploadBuffer(ID3D12Resource2*& uploadBuffer, void const* data);
	ResourceView* CreateShaderResourceView();
protected:
	Renderer* m_owner = nullptr;
	size_t m_size = 0;
	size_t m_stride = 0;
	MemoryUsage m_memoryUsage = MemoryUsage::Dynamic;
	void const* m_data = nullptr;
	DescriptorHeap* m_descriptorHeap = nullptr;
	Resource* m_buffer = nullptr;
	Resource* m_uploadResource = nullptr;
	std::vector<ResourceView*> m_views;

};

// Helper class to automatically initialize buffers. We don't want to expose Initialize()
class StructuredBuffer:public Buffer {
public:
	StructuredBuffer(BufferDesc const& bufDesc);
	~StructuredBuffer();
};

