#include "Engine/Math/IntVec3.hpp"
#include "Game/Gameplay/BlockIterator.hpp"
#include "Game/Gameplay/Chunk.hpp"

BlockIterator::BlockIterator(Chunk* chunk, int blockIndex) :
	m_chunk(chunk),
	m_blockIndex(blockIndex)
{
}

Block* BlockIterator::GetBlock() const
{
	if (m_chunk) {
		if (m_blockIndex >= 0 && m_blockIndex < CHUNK_TOTAL_SIZE) {
			return &m_chunk->m_blocks[m_blockIndex];
		}
	}
	return nullptr;
}

Vec3 const BlockIterator::GetBlockCenter() const
{
	if (!m_chunk) return Vec3(-1.0f, -1.0f, -1.0f);
	IntVec2 chunkCoords = m_chunk->GetChunkCoords();
	IntVec3 localBlockCoords = Chunk::GetLocalCoordsForIndex(m_blockIndex);

	float x = (float)(chunkCoords.x * CHUNK_SIZE_X) + (float)localBlockCoords.x + 0.5f;
	float y = (float)(chunkCoords.y * CHUNK_SIZE_Y) + (float)localBlockCoords.y + 0.5f;

	Vec3 blockPos = Vec3(x, y, (float)localBlockCoords.z + 0.5f);

	return blockPos;
}

Chunk* BlockIterator::GetChunk() const
{
	return m_chunk;
}

AABB3 const BlockIterator::GetBlockBounds() const
{
	if (!m_chunk) return AABB3();
	IntVec2 chunkCoords = m_chunk->GetChunkCoords();
	IntVec3 localBlockCoords = Chunk::GetLocalCoordsForIndex(m_blockIndex);

	float minX = (float)(chunkCoords.x * CHUNK_SIZE_X) + (float)localBlockCoords.x;
	float minY = (float)(chunkCoords.y * CHUNK_SIZE_Y) + (float)localBlockCoords.y;
	float minZ = (float)(localBlockCoords.z);

	float maxX = minX + 1;
	float maxY = minY + 1;
	float maxZ = minZ + 1;

	return AABB3(minX, minY, minZ, maxX, maxY, maxZ);
}

BlockIterator BlockIterator::GetEastNeighbor() const
{
	if (!m_chunk) {
		return BlockIterator(nullptr, m_blockIndex);
	}

	int eastBlockIndex = m_blockIndex;
	Chunk* chunk = m_chunk;
	if ((m_blockIndex & CHUNK_MASK_X) == CHUNK_MASK_X) {
		eastBlockIndex &= ~CHUNK_MASK_X; // Clears out X Bits
		chunk = m_chunk->m_eastChunk;
	}
	else {
		eastBlockIndex++;
	}
	return BlockIterator(chunk, eastBlockIndex);
}

BlockIterator BlockIterator::GetWestNeighbor() const
{
	if (!m_chunk) {
		return BlockIterator(nullptr, m_blockIndex);
	}

	int westBlockIndex = m_blockIndex;
	Chunk* chunk = m_chunk;
	if ((m_blockIndex & CHUNK_MASK_X) == 0) {
		westBlockIndex |= CHUNK_MASK_X; // Sets all X Bits to 1. Same procedure below
		chunk = m_chunk->m_westChunk;
	}
	else {
		westBlockIndex--;
	}
	return BlockIterator(chunk, westBlockIndex);
}

BlockIterator BlockIterator::GetNorthNeighbor() const
{
	if (!m_chunk) {
		return BlockIterator(nullptr, m_blockIndex);
	}

	int northBlockIndex = m_blockIndex;
	Chunk* chunk = m_chunk;
	if ((m_blockIndex & CHUNK_MASK_Y) == CHUNK_MASK_Y) {
		northBlockIndex &= ~CHUNK_MASK_Y;
		chunk = m_chunk->m_northChunk;
	}
	else {
		northBlockIndex += (1 << CHUNKSHIFT_Y);
	}
	return BlockIterator(chunk, northBlockIndex);
}


BlockIterator BlockIterator::GetSouthNeighbor() const
{
	if (!m_chunk) {
		return BlockIterator(nullptr, m_blockIndex);
	}

	int southBlockIndex = m_blockIndex;
	Chunk* chunk = m_chunk;
	if ((m_blockIndex & CHUNK_MASK_Y) == 0) {
		southBlockIndex |= CHUNK_MASK_Y;
		chunk = m_chunk->m_southChunk;
	}
	else {
		southBlockIndex -= (1 << CHUNKSHIFT_Y);
	}
	return BlockIterator(chunk, southBlockIndex);
}

BlockIterator BlockIterator::GetTopNeighbor() const
{
	if (!m_chunk) {
		return BlockIterator(nullptr, m_blockIndex);
	}

	bool isZMax = (m_blockIndex & CHUNK_MASK_Z) == CHUNK_MASK_Z;
	Chunk* chunk = (isZMax) ? nullptr : m_chunk;
	int blockIndex = m_blockIndex;

	if (chunk) {
		blockIndex += (1 << CHUNKSHIFT_Z);
	}

	return BlockIterator(chunk, blockIndex);
}

BlockIterator BlockIterator::GetBottomNeighbor() const
{
	if (!m_chunk) {
		return BlockIterator(nullptr, m_blockIndex);
	}

	bool isZMin = (m_blockIndex & CHUNK_MASK_Z) == 0;
	Chunk* chunk = (isZMin) ? nullptr : m_chunk;
	int blockIndex = m_blockIndex;

	if (chunk) {
		blockIndex -= (1 << CHUNKSHIFT_Z);
	}

	return BlockIterator(chunk, blockIndex);
}
