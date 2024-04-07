#pragma once
#include <vector>

struct ID3D12Resource2;
struct ID3D12GraphicsCommandList;
enum D3D12_RESOURCE_STATES : int;
struct D3D12_RESOURCE_BARRIER;
class Resource {
	friend class Renderer;
	friend class Texture;
	friend class Buffer;
public:
	void TransitionTo(D3D12_RESOURCE_STATES newState, ID3D12GraphicsCommandList* commList);
	bool AddResourceBarrierToList(D3D12_RESOURCE_STATES newState, std::vector< D3D12_RESOURCE_BARRIER>& rscBarriers);
	void MarkForBinding() { m_isBound = true; }
private:
	Resource();
	~Resource();

	ID3D12Resource2* m_resource = nullptr;
	bool m_isBound = false;
	int m_currentState = 0;
};