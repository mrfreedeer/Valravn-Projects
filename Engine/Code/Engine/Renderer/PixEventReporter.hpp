#pragma once
#include "Engine/Core/Rgba8.hpp"

struct ID3D12GraphicsCommandList6;
class ImmediateContext;

enum class PixEventType {
	UNDEFINED = -1,
	BEGIN,
	END
};

struct PixEvent {
	PixEvent(ID3D12GraphicsCommandList6* cmdList, unsigned int ctxIndex, PixEventType eventType, char const* text, Rgba8 const& color):
		m_cmdList(cmdList),
		m_ctxIndex(ctxIndex),
		m_eventType(eventType),
		m_markerText(text),
		m_color(color){}

	unsigned int m_ctxIndex = (unsigned int)-1;
	PixEventType m_eventType = PixEventType::UNDEFINED;
	char const* m_markerText = nullptr;
	Rgba8 m_color = Rgba8::WHITE;
	ID3D12GraphicsCommandList6* m_cmdList = nullptr;
};