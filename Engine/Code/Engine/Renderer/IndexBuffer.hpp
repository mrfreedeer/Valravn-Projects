#pragma once
#include "Engine/Core/Buffer.hpp"

class IndexBuffer : public Buffer
{
	friend class Renderer;

public:
	IndexBuffer(BufferDesc const& bufferDesc);
	virtual ~IndexBuffer();
};
