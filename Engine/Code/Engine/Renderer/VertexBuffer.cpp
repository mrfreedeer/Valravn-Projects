#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"

VertexBuffer::VertexBuffer(BufferDesc const& bufferDesc):
	Buffer(bufferDesc)
{
	m_bufferType = BufferType::VertexBuffer;
	Initialize();
}

VertexBuffer::~VertexBuffer()
{
}

