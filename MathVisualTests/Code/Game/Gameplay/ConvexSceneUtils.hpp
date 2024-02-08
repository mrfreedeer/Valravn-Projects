#pragma once

#include <vector>
class BufferParser;

enum ChunkType : unsigned char {
	SceneInfoChunk = 1,
	ConvexPolysChunk = 2,
	ConvexHullsChunk = 0x80,
	BoundingDiscsChunk = 0x81,
	BoundingAABBsChunk = 0x82,
	AABB2TreeChunk = 0x83,
	OBB2TreeChunk = 0x84,
	ConvexHullTreeChunk = 0x85,
	AsymmetricQuadTreeChunk = 0x86,
	SymmetricQuadTreeChunk = 0x87,
	TiledBitRegionsChunk = 0x88,
	ColumnRowBitRegionsChunk = 0x89,
	Disc2TreeChunk = 0x8A,
	BSPChunk = 0x8B,
	CompositeTreeChunk = 0x8C,
	ConvexPolyTreeChunk = 0x8D,
	ObjectColorsChunk = 0xE0,
	SceneGeneralDebugChunk = 0xF0,
	SceneRaycastDebugChunk = 0xF1,
	INVALID_CHUNK = 0xFF,
};



bool ParseHeader(BufferParser& bufferParser, unsigned char& endianness, unsigned int& tocLocation);
bool ParseChunkHeader(BufferParser& bufferParser, ChunkType& chunkType, unsigned char& chunkEndianness, unsigned int& chunkSize);
bool ParseChunkHeaderEndSequence(BufferParser& bufferParser);
bool ParseTocHeader(BufferParser& bufferParser);
bool ParseTocEndSequence(BufferParser& bufferParser);


void WriteHeaderToBuffer(std::vector<unsigned char>& sceneBuffer);
void WriteChunkHeaderToBuffer(std::vector<unsigned char>& sceneBuffer, ChunkType chunkType, unsigned char endianness, unsigned int chunkSize);
void WriteChunkHeaderEndSequence(std::vector<unsigned char>& sceneBuffer);
void WriteTocHeaderToBuffer(std::vector<unsigned char>& sceneBuffer);
void WriteTocEndSequenceToBuffer(std::vector<unsigned char>& sceneBuffer);