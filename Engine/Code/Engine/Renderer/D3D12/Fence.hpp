#pragma once
struct ID3D12Fence1;
struct ID3D12Device2;
struct ID3D12CommandQueue;

class Fence {
friend class Renderer;
public:
	Fence(ID3D12Device2* device, ID3D12CommandQueue* commandQueue, unsigned int initialValue = 0);
	~Fence();

	void Signal();
	void Wait();
private:
	ID3D12Fence1* m_fence = nullptr;
	ID3D12CommandQueue* m_commandQueue = nullptr;
	void* m_fenceEvent = nullptr;
	unsigned int m_fenceValue = 0;

};