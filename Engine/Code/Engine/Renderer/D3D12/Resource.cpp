#include "Engine/Renderer/D3D12/Resource.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"
#include <d3d12.h>
#include <d3dx12.h>

void Resource::TransitionTo(D3D12_RESOURCE_STATES newState, ID3D12GraphicsCommandList* commList)
{
	D3D12_RESOURCE_STATES currentState = (D3D12_RESOURCE_STATES)m_currentState;
	if (currentState == newState)return;

	CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_resource, currentState, newState);
	commList->ResourceBarrier(1, &resourceBarrier);
	m_currentState = (int)newState;

}

void Resource::TransitionTo(D3D12_RESOURCE_STATES newState, ComPtr<ID3D12GraphicsCommandList> commList)
{
	TransitionTo(newState, commList.Get());
}

bool Resource::AddResourceBarrierToList(D3D12_RESOURCE_STATES newState, std::vector< D3D12_RESOURCE_BARRIER>& rscBarriers)
{
	D3D12_RESOURCE_STATES currentState = (D3D12_RESOURCE_STATES)m_currentState;
	if (currentState == newState)return false;

	CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_resource, currentState, newState);
	m_currentState = (int) newState;

	rscBarriers.push_back(resourceBarrier);

	return true;
}

bool Resource::AddUAVResourceBarrierToList(std::vector< D3D12_RESOURCE_BARRIER>& rscBarriers)
{
	CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::UAV(m_resource);
	rscBarriers.push_back(resourceBarrier);
	return true;
}

void Resource::Map(void*& dataMap)
{
	ThrowIfFailed( m_resource->Map(0, nullptr, &dataMap), "FAILED TO MAP RESOURCE");
}

void Resource::Unmap()
{
	m_resource->Unmap(0, nullptr);
}


Resource::Resource(ID3D12Device2* device):
	m_device(device)
{
}

Resource::~Resource()
{
	DX_SAFE_RELEASE(m_resource);
}


