#include "Engine/Renderer/D3D12/DescriptorHeap.hpp"
#include "Engine/Renderer/D3D12/D3D12TypeConversions.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Renderer/GraphicsCommon.hpp"
#include <d3d12.h>
#include <d3dx12.h>

DescriptorHeap::DescriptorHeap(Renderer* owner, DescriptorHeapType descType, size_t numDescriptors, bool visibleFromGPU):
	m_owner(owner),
	m_type(descType),
	m_descriptorCount(numDescriptors)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = (UINT)numDescriptors;
	desc.Type = LocalToD3D12(descType);
	if (visibleFromGPU) {
		desc.Flags |= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	}

	ComPtr<ID3D12Device2> device = m_owner->m_device;

	HRESULT descHeapCreation = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_descriptorHeap));
	m_owner->SetDebugName(m_descriptorHeap, "DescriptorHeap");

	if (!SUCCEEDED(descHeapCreation)) {
		ERROR_AND_DIE("FAILED TO CREATE DESCRIPTOR HEAP");
	}

	m_remainingDescriptors = m_descriptorCount;
	m_currentDescriptor = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr;
	m_descriptorHandleSize = device->GetDescriptorHandleIncrementSize(desc.Type);
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetNextCPUHandle()
{
	if((!m_descriptorHeap) || (m_remainingDescriptors <= 0)) ERROR_AND_DIE("RAN OUT OF DESCRIPTORS");

	D3D12_CPU_DESCRIPTOR_HANDLE returnHandle = {};
	returnHandle.ptr = m_currentDescriptor;
	m_currentDescriptor += m_descriptorHandleSize;
	m_remainingDescriptors--;

	return returnHandle;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetHandleAtOffset(size_t offset)
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE handleToReturn(m_descriptorHeap->GetCPUDescriptorHandleForHeapStart(), (UINT)offset, (UINT)m_descriptorHandleSize);

	return handleToReturn;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGPUHandleAtOffset(size_t offset)
{
	CD3DX12_GPU_DESCRIPTOR_HANDLE handleToReturn(m_descriptorHeap->GetGPUDescriptorHandleForHeapStart(), (UINT)offset, (UINT)m_descriptorHandleSize);

	return handleToReturn;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGPUHandleForHeapStart()
{
	return m_descriptorHeap->GetGPUDescriptorHandleForHeapStart();
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetCPUHandleForHeapStart()
{
	return m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
}


DescriptorHeap::~DescriptorHeap()
{
	DX_SAFE_RELEASE(m_descriptorHeap);
}

