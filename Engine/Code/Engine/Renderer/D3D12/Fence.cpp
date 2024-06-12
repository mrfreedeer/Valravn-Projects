#include "Engine/Renderer/D3D12/Fence.hpp"
#include "Engine/Renderer/GraphicsCommon.hpp"
#include <d3d12.h>

Fence::Fence(ID3D12Device2* device, ID3D12CommandQueue* commandQueue, unsigned int initialValue /*= 0*/):
	m_fenceValue(initialValue),
	m_commandQueue(commandQueue)
{
	ThrowIfFailed(device->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)), "FAILED CREATING FENCE");

	// Create an event handle to use for frame synchronization.
	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (m_fenceEvent == nullptr)
	{
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()), "FAILED GETTING LAST ERROR");
	}


}

Fence::~Fence()
{
	DX_SAFE_RELEASE(m_fence);
}

void Fence::Signal()
{
	m_fenceValue++;
	ThrowIfFailed(m_commandQueue->Signal(m_fence, m_fenceValue), "FAILED TO SIGNAL FENCE");
}

void Fence::Wait()
{
	if (m_fence->GetCompletedValue() < m_fenceValue)
	{
		ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent), "FAILED TO SET EVENT ON COMPLETION");
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
}
