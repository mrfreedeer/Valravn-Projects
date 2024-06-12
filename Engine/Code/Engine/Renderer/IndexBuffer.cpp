#include "Engine/Renderer/IndexBuffer.hpp"

IndexBuffer::IndexBuffer(BufferDesc const& bufferDesc):
	Buffer(bufferDesc)
{
	m_bufferType = BufferType::IndexBuffer;
	Initialize();
}

IndexBuffer::~IndexBuffer()
{

}

