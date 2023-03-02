#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/Mat44.hpp"

struct Vertex_PCU;
struct Vertex_PNCU;
struct Rgba8;

void LoadMeshFromPlyFile(std::filesystem::path filePath, Rgba8 const& color, std::vector<Vertex_PCU>& verts, std::vector<unsigned int>& indices);
void LoadMeshFromPlyFile(std::filesystem::path filePath, Rgba8 const& color, std::vector<Vertex_PNCU>& verts, std::vector<unsigned int>& indices);

class VertexBuffer;
class IndexBuffer;
class Renderer;

struct MeshImportOptions {
	bool m_useIndices = false;
	bool m_reverseWindingOrder = false;
	bool m_invertUV = false;
	float m_scale = 1.0f;
	Mat44 m_transform;
	Rgba8 m_color = Rgba8::MAGENTA; // Magenta for untextured 
	MemoryUsage m_memoryUsage = MemoryUsage::Default;
	std::string m_name = "Unnamed Mesh";
};

class MeshBuilder {
public:
	MeshBuilder(MeshImportOptions const& importOptions);

	void ImportFromObj(std::filesystem::path filePath);
	void Scale(float scale);
	void Transform(Mat44 const& newTransform);
	void ReverseWindingOrder();
	void InvertUV();

	bool WriteToFile(std::filesystem::path filePath);
	bool ReadFromFile(std::filesystem::path filePath);

	std::vector<Vertex_PNCU> m_vertexes;

	MeshImportOptions m_importOptions;
	std::vector<unsigned int> m_indexes;

	unsigned int m_vertexCount = 0;

};


class Mesh {
public:
	Mesh() = default;
	~Mesh();
	Mesh(MeshBuilder const& meshBuilder, Renderer* renderer);

	VertexBuffer* m_vertexBuffer = nullptr;
	IndexBuffer* m_indexBuffer = nullptr;

	unsigned int m_vertexCount;
	size_t m_stride;

	bool m_useIndexes = false;
};