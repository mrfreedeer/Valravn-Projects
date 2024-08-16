#include "Engine/Renderer/PixEventReporter.hpp"
#include <d3d12.h>
#include <ThirdParty/WinPixEventRuntime/Include/pix3.h>

PixEventReporter::~PixEventReporter()
{
	PIXEndEvent(m_cmdList);
}

PixEventReporter::PixEventReporter(ID3D12GraphicsCommandList6* graphicsCmdList, char const* markerName, Rgba8 const& color):
	m_cmdList(graphicsCmdList),
	m_marker(markerName),
	m_color(color)
{
	UINT64 markerColor = (UINT64)PIX_COLOR((BYTE)(color.r), (BYTE)(color.g), (BYTE)(color.b));

	PIXBeginEvent(graphicsCmdList, markerColor, markerName);
}
