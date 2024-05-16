#include "Engine/Renderer/D3D12/Resource.hpp"
#include "Engine/Renderer/GraphicsCommon.hpp"
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

bool Resource::AddResourceBarrierToList(D3D12_RESOURCE_STATES newState, std::vector< D3D12_RESOURCE_BARRIER>& rscBarriers)
{
	D3D12_RESOURCE_STATES currentState = (D3D12_RESOURCE_STATES)m_currentState;
	if (currentState == newState)return false;

	CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_resource, currentState, newState);
	m_currentState = (int) newState;

	rscBarriers.push_back(resourceBarrier);

	return true;
}

bool Resource::AddResourceBarrierToList(std::vector< D3D12_RESOURCE_BARRIER>& rscBarriers)
{
	if(!IsBound()) return false;

	D3D12_RESOURCE_STATES newState = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
	for (unsigned int resourceBindBit = 0; resourceBindBit <= ResourceBindState::LAST; resourceBindBit++) {
		unsigned int shiftBit = (1 << resourceBindBit);
		if (shiftBit & m_stateFlags) {
			newState = GetResourceState((ResourceBindState)shiftBit);
		}
	}
	AddResourceBarrierToList(newState, rscBarriers);

	return true;

}

void Resource::MarkForBinding(ResourceBindState bindState)
{
	m_stateFlags &= ~(bindState);
	m_stateFlags |= bindState;
}

void Resource::MarkForVertexAndCBufferBind()
{
	MarkForBinding(ResourceBindState::VERTEX_AND_CONSTANT_BUFFER);

}

void Resource::Map(void*& dataMap)
{
	ThrowIfFailed( m_resource->Map(0, nullptr, &dataMap), "FAILED TO MAP RESOURCE");
}

void Resource::Unmap()
{
	m_resource->Unmap(0, nullptr);
}

D3D12_RESOURCE_STATES Resource::GetResourceState(ResourceBindState bindState)
{
	switch (bindState) {
	case VERTEX_AND_CONSTANT_BUFFER:
		return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	case PIXEL_SHADER:
		return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	case ALL_SHADER:
		return D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
	default:
		ERROR_AND_DIE(Stringf("INVALID RESOURCE STATE %d", bindState).c_str());
	}
	
}

Resource::Resource()
{
}

Resource::~Resource()
{
	DX_SAFE_RELEASE(m_resource);
}


