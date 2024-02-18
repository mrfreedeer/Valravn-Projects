#include "Engine/Renderer/Mesh.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"

int LoadPlyModelInfoFromHeader(Strings const& source, int& uvIndexStart, int& amountOfverts, int& amountOfindices) {
	bool foundUv = false;
	int UVStart = 0;
	uvIndexStart = -1;


	for (int stringIndex = 0; stringIndex < source.size(); stringIndex++) {
		std::string const& currentString = source[stringIndex];
		Strings stringSplitBySpace = SplitStringOnDelimiter(currentString, ' ');
		std::string trimmedString0 = TrimStringCopy(stringSplitBySpace[0]);

		if(AreStringsEqualCaseInsensitive(trimmedString0, "element")) {
		//if (!_stricmp(stringSplitBySpace[0].c_str(), "element")) { // Case insensity 0 == strings are equal
			if (stringSplitBySpace.size() == 3) {
				int quantity = stoi(stringSplitBySpace[2]);
				if (AreStringsEqualCaseInsensitive(TrimStringCopy(stringSplitBySpace[1]), "vertex")) {
					amountOfverts = quantity;
				}
				else {
					amountOfindices = quantity;
				}
			}
		}

		if (AreStringsEqualCaseInsensitive(trimmedString0, "property")) {
		//if (!_stricmp(stringSplitBySpace[0].c_str(), "property")) {
			std::string trimmedString2 = TrimStringCopy(stringSplitBySpace[2]);
			if (AreStringsEqualCaseInsensitive(trimmedString2, "s") || AreStringsEqualCaseInsensitive(trimmedString2, "u")) {
				foundUv = true;
				uvIndexStart = UVStart;

			}
			else {
				if (!foundUv) {
					UVStart++;
				}
			}
		}

		if (AreStringsEqualCaseInsensitive(trimmedString0, "end_header")) {
			return stringIndex + 1;
		}
	}

	return -1;
}


void LoadPlyModel(std::filesystem::path filePath, Rgba8 const& color, std::vector<Vertex_PCU>& verts, std::vector<unsigned int>& indices) {
	std::string modelInfo = "";
	FileReadToString(modelInfo, filePath.string());

	Strings modelSplitByNewline = SplitStringOnDelimiter(modelInfo, '\n');


	int uvIndexStart = 0;
	int amountOfVerts = 0;
	int amoutOfIndices = 0;

	int dataIndex = LoadPlyModelInfoFromHeader(modelSplitByNewline, uvIndexStart, amountOfVerts, amoutOfIndices);

	verts.reserve(amountOfVerts);
	indices.reserve(amoutOfIndices);

	int indicesStartIndex = dataIndex + amountOfVerts;

	/*if (uvIndexStart == -1) {
		ERROR_RECOVERABLE(Stringf("COULD NOT FIND UVs IN MODEL FILE %s", filePath));
		return;
	}*/


	for (int stringIndex = dataIndex; stringIndex < modelSplitByNewline.size(); stringIndex++) {
		std::string const& currentString = modelSplitByNewline[stringIndex];

		if (currentString.empty()) continue;

		Strings stringSplitBySpace = SplitStringOnDelimiter(currentString, ' ');


		if (stringIndex < indicesStartIndex) {

			float x = stof(stringSplitBySpace[0]);
			float y = stof(stringSplitBySpace[1]);
			float z = stof(stringSplitBySpace[2]);

			float u = 0.0f;
			float v = 0.0f;
			if (uvIndexStart != -1) {
				u = stof(stringSplitBySpace[uvIndexStart]);
				v = stof(stringSplitBySpace[(size_t)uvIndexStart + 1]);
			}

			verts.emplace_back(x, y, z, color, u, v);
		}
		else {
			int indexOne = stoi(stringSplitBySpace[1]); // Line starts with amount of indices. Assumed to be always 3
			int indexTwo = stoi(stringSplitBySpace[2]);
			int indexThree = stoi(stringSplitBySpace[3]);

			indices.push_back(indexOne);
			indices.push_back(indexTwo);
			indices.push_back(indexThree);
		}
	}
}


void LoadPlyModel(std::filesystem::path filePath, Rgba8 const& color, std::vector<Vertex_PNCU>& verts, std::vector<unsigned int>& indices)
{
	std::string modelInfo = "";
	FileReadToString(modelInfo, filePath.string());

	Strings modelSplitByNewline = SplitStringOnDelimiter(modelInfo, '\n');


	int uvIndexStart = 0;
	int amountOfVerts = 0;
	int amoutOfIndices = 0;

	int dataIndex = LoadPlyModelInfoFromHeader(modelSplitByNewline, uvIndexStart, amountOfVerts, amoutOfIndices);

	verts.reserve(amountOfVerts);
	indices.reserve(amoutOfIndices);

	int indicesStartIndex = dataIndex + amountOfVerts;


	for (int stringIndex = dataIndex; stringIndex < modelSplitByNewline.size(); stringIndex++) {
		std::string const& currentString = modelSplitByNewline[stringIndex];

		if (currentString.empty()) continue;

		Strings stringSplitBySpace = SplitStringOnDelimiter(currentString, ' ');


		if (stringIndex < indicesStartIndex) {

			float x = stof(stringSplitBySpace[0]);
			float y = stof(stringSplitBySpace[1]);
			float z = stof(stringSplitBySpace[2]);

			float nx = stof(stringSplitBySpace[3]);
			float ny = stof(stringSplitBySpace[4]);
			float nz = stof(stringSplitBySpace[5]);

			float u = 0.0f;
			float v = 0.0f;
			if (uvIndexStart != -1) {
				u = stof(stringSplitBySpace[uvIndexStart]);
				v = stof(stringSplitBySpace[(size_t)uvIndexStart + 1]);
			}

			verts.emplace_back(x, y, z, nx, ny, nz, color, u, v);
		}
		else {
			int indexOne = stoi(stringSplitBySpace[1]); // Line starts with amount of indices. Assumed to be always 3
			int indexTwo = stoi(stringSplitBySpace[2]);
			int indexThree = stoi(stringSplitBySpace[3]);

			indices.push_back(indexOne);
			indices.push_back(indexTwo);
			indices.push_back(indexThree);
		}
	}
}

void LoadMeshFromPlyFile(std::filesystem::path filePath, Rgba8 const& color, std::vector<Vertex_PCU>& verts, std::vector<unsigned int>& indices)
{
	std::string fileExtension = filePath.extension().string();

	LoadPlyModel(filePath, color, verts, indices);

}


void LoadMeshFromPlyFile(std::filesystem::path filePath, Rgba8 const& color, std::vector<Vertex_PNCU>& verts, std::vector<unsigned int>& indices)
{
	std::string fileExtension = filePath.extension().string();

	LoadPlyModel(filePath, color, verts, indices);
}

MeshBuilder::MeshBuilder(MeshImportOptions const& importOptions) :
	m_importOptions(importOptions)
{
}

void MeshBuilder::ImportFromObj(std::filesystem::path filePath)
{
	PROFILE_LOG_SCOPE(Mesh_Importing);
	std::string modelInfo;
	FileReadToString(modelInfo, filePath.string());

	std::vector<Vec3> vertexPositions;
	std::vector<Vec3> vertexNormals;
	std::vector<Vec3> vertexTextures;

	Mat44 inverseTranform = m_importOptions.m_transform.GetOrthonormalInverse();

	std::vector<Vertex_PNCU>& createdVertexes = m_vertexes;

	Strings modelSplitByNewline = SplitStringOnDelimiter(modelInfo, '\n');
	bool vertexReserved = false;


	for (int lineIndex = 0; lineIndex < modelSplitByNewline.size(); lineIndex++) {
		std::string const& currentLine = modelSplitByNewline[lineIndex];

		Strings lineSplitBySpace = SplitStringOnSpace(currentLine);

		if (lineSplitBySpace.size() == 0) continue;
		if (lineSplitBySpace[0].empty()) continue;
		if (lineSplitBySpace[0] == "#") continue;
		if (lineSplitBySpace[0] == "mtllib") continue;

		if (lineSplitBySpace[0] == "v") {
			float x = stof(lineSplitBySpace[1]);
			float y = stof(lineSplitBySpace[2]);
			float z = stof(lineSplitBySpace[3]);

			vertexPositions.emplace_back(inverseTranform.TransformPosition3D(Vec3(x, y, z)));
		}

		if (lineSplitBySpace[0] == "vn") {
			float nx = stof(lineSplitBySpace[1]);
			float ny = stof(lineSplitBySpace[2]);
			float nz = stof(lineSplitBySpace[3]);

			vertexNormals.emplace_back(nx, ny, nz);
		}

		if (lineSplitBySpace[0] == "vt") {
			float u = stof(lineSplitBySpace[1]);
			float v = stof(lineSplitBySpace[2]);
			if (m_importOptions.m_invertUV) {
				v = 1.0f - v;
			}

			float w = 0;

			vertexTextures.emplace_back(u, v, w);
		}

		if (lineSplitBySpace[0] == "f") {
			if (!vertexReserved) createdVertexes.reserve(vertexPositions.size()); // An estimate of a minimum of vertexes



			for (int subStrIndex = 1; subStrIndex < lineSplitBySpace.size(); subStrIndex++) {
				Strings subSplitBySlash = SplitStringOnDelimiter(lineSplitBySpace[subStrIndex], '/');

				Vertex_PNCU newVertex;

				// OBJ INDEX STARTS FROM 1!!

				size_t positionIndex = stoi(subSplitBySlash[0]);
				positionIndex--;

				size_t normalIndex = 0;
				size_t textureIndex = 0;

				Vec3 position = vertexPositions[positionIndex];

				Vec3 normal = Vec3::ZERO;
				Vec2 uv = Vec2::ZERO;
				Rgba8 color = m_importOptions.m_color;

				if ((subSplitBySlash.size() > 1) && (!subSplitBySlash[1].empty())) {
					textureIndex = stoi(subSplitBySlash[1]);
					textureIndex--;

					Vec3 textureVec = vertexTextures[textureIndex];

					uv = Vec2(textureVec.x, textureVec.y);
				}

				if ((subSplitBySlash.size() > 2) && (!subSplitBySlash[2].empty())) {
					normalIndex = stoi(subSplitBySlash[2]);
					normalIndex--;

					normal = vertexNormals[normalIndex];
				}

				if (subStrIndex == 4) {

					Vertex_PNCU firstVertex = createdVertexes[createdVertexes.size() - 3];
					Vertex_PNCU secondVertex = createdVertexes[createdVertexes.size() - 1];

					createdVertexes.emplace_back(firstVertex);
					createdVertexes.emplace_back(secondVertex);
				}

				createdVertexes.emplace_back(position, normal, m_importOptions.m_color, uv);

				if (subStrIndex == 3) {
					if (m_importOptions.m_reverseWindingOrder) {
						Vertex_PNCU tempVertex = createdVertexes[createdVertexes.size() - 3];

						createdVertexes[createdVertexes.size() - 3] = createdVertexes[createdVertexes.size() - 1];
						createdVertexes[createdVertexes.size() - 1] = tempVertex;
					}
				}
			}

		}

	}


	m_vertexCount = (unsigned int)m_vertexes.size();
}

void MeshBuilder::Scale(float scale)
{
	m_importOptions.m_scale = scale;

	for (int vertexIndex = 0; vertexIndex < m_vertexes.size(); vertexIndex++) {
		Vertex_PNCU& vertex = m_vertexes[vertexIndex];
		vertex.m_position *= scale;

	}

}

void MeshBuilder::Transform(Mat44 const& newTransform)
{
	m_importOptions.m_transform.Append(newTransform);

	for (int vertexIndex = 0; vertexIndex < m_vertexes.size(); vertexIndex++) {
		Vertex_PNCU& vertex = m_vertexes[vertexIndex];
		vertex.m_position = newTransform.TransformPosition3D(vertex.m_position);
	}

}

void MeshBuilder::ReverseWindingOrder() {
	m_importOptions.m_reverseWindingOrder = !m_importOptions.m_reverseWindingOrder;

	for (int vertexIndex = 0; vertexIndex < m_vertexes.size(); vertexIndex += 3) {
		Vertex_PNCU tempVertex = m_vertexes[vertexIndex];

		m_vertexes[vertexIndex] = m_vertexes[(size_t)vertexIndex + 2];
		m_vertexes[(size_t)vertexIndex + 2] = tempVertex;
	}
}

void MeshBuilder::InvertUV()
{
	for (int vertexIndex = 0; vertexIndex < m_vertexes.size(); vertexIndex++) {
		Vertex_PNCU& vertex = m_vertexes[vertexIndex];
		vertex.m_uvTexCoords.y = 1.0f - vertex.m_uvTexCoords.y;
	}
}

bool MeshBuilder::WriteToFile(std::filesystem::path filePath) {

	if (filePath.has_extension()) filePath.replace_extension("bime");
	else filePath += ".bime";

	FILE* file = nullptr;
	fopen_s(&file, filePath.string().c_str(), "wb+");

	if (file) {

		size_t vertAmount = m_vertexes.size();
		size_t indexAmount = m_indexes.size();

		fwrite(&vertAmount, sizeof(size_t), 1, file);
		fwrite(&indexAmount, sizeof(size_t), 1, file);

		fwrite(m_vertexes.data(), sizeof(Vertex_PNCU), m_vertexes.size(), file);
		fwrite(m_indexes.data(), sizeof(unsigned int), m_indexes.size(), file);

		fclose(file);
	}
	else {
		return false;
	}

	return true;
}



bool MeshBuilder::ReadFromFile(std::filesystem::path filePath) {

	if (filePath.has_extension()) filePath.replace_extension("bime");
	else filePath += ".bime";

	PROFILE_LOG_SCOPE(Read_from_binary);
	FILE* file = nullptr;
	fopen_s(&file, filePath.string().c_str(), "rb");

	if (file) {

		size_t vertAmount = 0;
		size_t indexAmount = 0;

		fread_s(&vertAmount, sizeof(size_t), sizeof(size_t), 1, file);
		fread_s(&indexAmount, sizeof(size_t), sizeof(size_t), 1, file);

		m_vertexes.clear();
		m_indexes.clear();

		m_vertexes.resize(vertAmount);
		m_indexes.resize(indexAmount);

		size_t bufferSize = sizeof(Vertex_PNCU) * m_vertexes.size();
		size_t indexBufferSize = sizeof(unsigned int) * m_indexes.size();

		fread_s(m_vertexes.data(), bufferSize, sizeof(Vertex_PNCU), m_vertexes.size(), file);
		fread_s(m_indexes.data(), indexBufferSize, sizeof(unsigned int), m_indexes.size(), file);

		fclose(file);

		m_vertexCount = (unsigned int)m_vertexes.size();
	}
	else {
		return false;
	}

	return true;
}

Mesh::Mesh(MeshBuilder const& meshBuilder, Renderer* renderer)
{
	if (!renderer) {
		ERROR_AND_DIE("RENDERER NOT FOUND TO CREATE MESH");
	}

	std::vector<Vertex_PNCU>const& vertexes = meshBuilder.m_vertexes;

	m_vertexCount = (unsigned int)meshBuilder.m_vertexes.size();
	m_stride = sizeof(Vertex_PNCU);
	MemoryUsage const& memoryUsage = meshBuilder.m_importOptions.m_memoryUsage;

	//#TODO DX12 FIXTHIS

	BufferDesc newVBufferDesc = {};
	newVBufferDesc.data = vertexes.data();
	newVBufferDesc.descriptorHeap = nullptr;
	newVBufferDesc.memoryUsage = memoryUsage;
	newVBufferDesc.owner = renderer;
	newVBufferDesc.size = meshBuilder.m_vertexes.size() * m_stride;
	newVBufferDesc.stride = m_stride;
	m_vertexBuffer = new VertexBuffer(newVBufferDesc);
	//m_vertexBuffer = new VertexBuffer(renderer->m_device, meshBuilder.m_vertexes.size() * m_stride, m_stride, memoryUsage, vertexes.data());


}

Mesh::~Mesh()
{
	delete m_vertexBuffer;
	if (m_useIndexes) {
		delete m_indexBuffer;
	}
}
