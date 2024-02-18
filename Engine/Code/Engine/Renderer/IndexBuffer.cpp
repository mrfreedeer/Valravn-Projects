#include "Engine/Renderer/IndexBuffer.hpp"

IndexBuffer::IndexBuffer(BufferDesc const& bufferDesc):
	Buffer(bufferDesc)
{
	Initialize();
}

IndexBuffer::~IndexBuffer()
{

}

