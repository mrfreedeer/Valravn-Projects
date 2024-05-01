#pragma once
#include <cstdint>

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct D3D12_GPU_DESCRIPTOR_HANDLE;
struct ID3D12DescriptorHeap;
class Renderer;


enum class DescriptorHeapType {
	UNDEFINED = -1,
	SRV_UAV_CBV,
	SAMPLER,
	RTV,
	DSV,
	NUM_DESCRIPTOR_HEAPS,
	MAX_GPU_VISIBLE = SAMPLER + 1,
};

class DescriptorHeap {
public:
	DescriptorHeap(Renderer* owner, DescriptorHeapType descType, size_t numDescriptors, bool visibleFromGPU = false);
	~DescriptorHeap();
	D3D12_CPU_DESCRIPTOR_HANDLE GetNextCPUHandle();
	D3D12_CPU_DESCRIPTOR_HANDLE GetHandleAtOffset(size_t offset);
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandleAtOffset(size_t offset);
	ID3D12DescriptorHeap* GetHeap() { return m_descriptorHeap; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandleForHeapStart();
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandleForHeapStart();
	DescriptorHeapType GetHeapType() const { return m_type; }

private:
	Renderer* m_owner = nullptr;
	ID3D12DescriptorHeap* m_descriptorHeap = nullptr;
	DescriptorHeapType m_type = DescriptorHeapType::UNDEFINED;
	size_t m_descriptorCount = 0;
	size_t m_remainingDescriptors = 0;
	size_t m_currentDescriptor = 0;
	size_t m_descriptorHandleSize = 0;

};