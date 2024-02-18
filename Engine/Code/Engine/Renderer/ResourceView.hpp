#pragma once
#include <d3d12.h>

enum ResourceBindFlagBit : unsigned int {
	RESOURCE_BIND_NONE = 0,
	RESOURCE_BIND_SHADER_RESOURCE_BIT = (1 << 0),
	RESOURCE_BIND_RENDER_TARGET_BIT = (1 << 1),
	RESOURCE_BIND_DEPTH_STENCIL_BIT = (1 << 2),
	RESOURCE_BIND_UNORDERED_ACCESS_VIEW_BIT = (1 << 3),
	RESOURCE_BIND_CONSTANT_BUFFER_VIEW_BIT = (1 << 4)
};

typedef unsigned int ResourceBindFlag;
class Resource;

struct ResourceViewInfo {
	~ResourceViewInfo();

	Resource* m_source = nullptr;
	ResourceBindFlagBit m_viewType = RESOURCE_BIND_NONE;
	D3D12_SHADER_RESOURCE_VIEW_DESC* m_srvDesc = nullptr;
	D3D12_RENDER_TARGET_VIEW_DESC* m_rtvDesc = nullptr;
	D3D12_CONSTANT_BUFFER_VIEW_DESC* m_cbvDesc = nullptr;
	D3D12_UNORDERED_ACCESS_VIEW_DESC* m_uavDesc = nullptr;
	D3D12_DEPTH_STENCIL_VIEW_DESC* m_dsvDesc = nullptr;

	bool operator==(ResourceViewInfo const& otherView) const;
};

class ResourceView {
	friend class Texture;
	friend class Renderer;

public:
	D3D12_CPU_DESCRIPTOR_HANDLE GetHandle() { return m_descriptorHandle; }
	~ResourceView();
private:
	ResourceView(ResourceViewInfo const& viewInfo);


private:
	Resource* m_source = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE m_descriptorHandle;
	ResourceViewInfo m_viewInfo;
};
