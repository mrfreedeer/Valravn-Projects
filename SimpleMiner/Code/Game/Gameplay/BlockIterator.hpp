#pragma once

struct Vec3;
class Chunk;
class Block;
struct AABB3;

class BlockIterator {
public:
	BlockIterator() = default;
	BlockIterator(Chunk* chunk, int blockIndex);
	Block* GetBlock() const;
	Vec3 const GetBlockCenter() const;
	Chunk* GetChunk() const;
	int GetIndex() const { return m_blockIndex; }
	AABB3 const GetBlockBounds() const;

	BlockIterator GetEastNeighbor() const;
	BlockIterator GetWestNeighbor() const;
	BlockIterator GetNorthNeighbor() const;
	BlockIterator GetSouthNeighbor() const;
	BlockIterator GetTopNeighbor() const;
	BlockIterator GetBottomNeighbor() const;


private:
	Chunk* m_chunk = nullptr;
	int m_blockIndex = 0;
};