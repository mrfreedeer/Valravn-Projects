#include "Engine/Renderer/D3D12/Resource.hpp"
#include "Engine/Renderer/GraphicsCommon.hpp"
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

Resource::Resource()
{

}

Resource::~Resource()
{
	DX_SAFE_RELEASE(m_resource);
}


