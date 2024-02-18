#pragma once
#include "Engine/Core/Buffer.hpp"

class VertexBuffer : public Buffer
{
	friend class Renderer;
public:
	VertexBuffer(BufferDesc const& bufferDesc);
	virtual ~VertexBuffer();
	
private:
	VertexBuffer(const VertexBuffer& copy) = delete;
};
