#pragma once
#include "Engine/Core/Rgba8.hpp"

struct ID3D12GraphicsCommandList6;

class PixEventReporter {
friend class Renderer;
public:
	~PixEventReporter();
	PixEventReporter(ID3D12GraphicsCommandList6* graphicsCmdList, char const* markerName, Rgba8 const& color = Rgba8::WHITE);
private:
	
	ID3D12GraphicsCommandList6* m_cmdList = nullptr;
	char const* m_marker;
	Rgba8 m_color = Rgba8::WHITE;
};