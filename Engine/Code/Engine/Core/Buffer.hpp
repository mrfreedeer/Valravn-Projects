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
	char const* debugName = "GPU Buffer";
	size_t size = 0;
	size_t stride = 0;
	MemoryUsage memoryUsage = MemoryUsage::Upload;
	void const* data = nullptr;
};

enum class BufferType {
	UNKNOWN = -1,
	VertexBuffer,
	IndexBuffer,
	ConstantBuffer,
	StructuredBuffer,
	NUM_BUFFER_TYPES
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
	bool IsPendingCopy() const { return m_isPendingCopy;}
	void ClearPendingCopies();
	Resource* GetResource();
	void ResetCopyState();
protected:
	virtual void Initialize();
	virtual void CreateBuffer(Resource* const& buffer, bool isUpload = false);
	ResourceView* CreateShaderResourceView();
	ResourceView* CreateUnorederedAccessView();
	ResourceView* CreateConstantBufferView();
protected:
	BufferDesc m_bufferDesc = {};
	Renderer* m_owner = nullptr;
	ID3D12Device2* m_device = nullptr;
	size_t m_size = 0;
	size_t m_stride = 0;
	MemoryUsage m_memoryUsage = MemoryUsage::Upload;
	Resource* m_buffer = nullptr;
	Resource* m_uploadBuffer = nullptr;
	std::vector<ResourceView*> m_views;
	std::string m_name = "";
	bool m_isPendingCopy = false;
	BufferType m_bufferType = BufferType::UNKNOWN;

};

// Helper class to automatically initialize buffers. We don't want to expose Initialize()
class StructuredBuffer :public Buffer {
public:
	StructuredBuffer(BufferDesc const& bufDesc);
	~StructuredBuffer();
};

