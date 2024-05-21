#pragma once
#include <vector>
#include "Engine/Renderer/GraphicsCommon.hpp"


constexpr unsigned int CBUFFER_BIND_SHIFT = 0;
constexpr unsigned int PIXEL_SHADER_BIND_SHIFT = 1;
constexpr unsigned int ALL_SHADERS_BIND_SHIFT = 2;

enum ResourceBindState : unsigned int {
	VERTEX_AND_CONSTANT_BUFFER = (1 << CBUFFER_BIND_SHIFT),
	PIXEL_SHADER = (1 << PIXEL_SHADER_BIND_SHIFT),
	ALL_SHADER = (1 << ALL_SHADERS_BIND_SHIFT),
	LAST = ALL_SHADER
};

struct ID3D12Resource2;
struct ID3D12GraphicsCommandList;
enum D3D12_RESOURCE_STATES : int;
struct D3D12_RESOURCE_BARRIER;

struct ID3D12Device2;

class Resource {
	friend class Renderer;
	friend class Texture;
	friend class Buffer;
public:
	void TransitionTo(D3D12_RESOURCE_STATES newState, ID3D12GraphicsCommandList* commList);
	void TransitionTo(D3D12_RESOURCE_STATES newState, ComPtr<ID3D12GraphicsCommandList> commList);
	bool AddResourceBarrierToList(D3D12_RESOURCE_STATES newState, std::vector< D3D12_RESOURCE_BARRIER>& rscBarriers);
	/// <summary>
	/// Adds appropriate state if marked for binding internally
	/// </summary>
	/// <param name="rscBarriers"></param>
	/// <returns></returns>
	bool AddResourceBarrierToList(std::vector< D3D12_RESOURCE_BARRIER>& rscBarriers);
	void MarkForBinding(ResourceBindState bindState);
	void MarkForVertexAndCBufferBind();
	bool IsBound() const { return m_stateFlags != 0; }
	void ClearBinds() { m_stateFlags = 0;}
	void Map(void*& dataMap);
	void Unmap();

	static D3D12_RESOURCE_STATES GetResourceState(ResourceBindState bindState);
private:
	Resource(ID3D12Device2* device);
	~Resource();

	ID3D12Resource2* m_resource = nullptr;
	ID3D12Device2* m_device = nullptr;
	unsigned int m_stateFlags = 0;
	int m_currentState = 0;
};