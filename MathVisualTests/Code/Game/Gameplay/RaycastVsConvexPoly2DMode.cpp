#include "Engine/Math/FloatRange.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Core/BufferUtils.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Game/Gameplay/RaycastVsConvexPoly2DMode.hpp"
#include "Game/Gameplay/ConvexPolyShape2D.hpp"
#include "Game/Gameplay/AABB2Tree.hpp"
#include "Game/Gameplay/AABB2Shape2D.hpp"
#include "Game/Gameplay/ConvexSceneUtils.hpp"
#include "Engine/Core/NamedProperties.hpp"
#include <queue>

RaycastVsConvexPoly2DMode* pointerToSelf = nullptr;

struct TOCEntry {
	unsigned char m_chunkType = 0XFF;
	unsigned int m_headerStart = 0;
	unsigned int m_chunkTotalSize = 0;
};

std::vector<TOCEntry> toc;

TOCEntry* FindTocEntry(unsigned char chunkType) {
	for (TOCEntry& entry : toc) {
		if (entry.m_chunkType == chunkType) {
			return &entry;
		}
	}

	return nullptr;
}


RaycastVsConvexPoly2DMode::RaycastVsConvexPoly2DMode(Game* pointerToGame, Vec2 const& cameraSize, Vec2 const& UICameraSize) :
	GameMode(pointerToGame, cameraSize, UICameraSize)
{
	m_worldSize = cameraSize;
	m_gameModeName = "Raycasts vs ConvexPoly2D";
	m_helperText = "F8 to randomize. S/E set ray start/end; Hold T = slow, W/R = Rotate, U/I = +/- max depth, N/M = double/halve rays, ,/. = double/halve shapes";
	m_baseHelperText = m_helperText;
	m_rayStart = Vec2(20.0f, 20.0f);
	m_rayEnd = Vec2(100.0f, 40.0f);
	pointerToSelf = this;
}

RaycastVsConvexPoly2DMode::~RaycastVsConvexPoly2DMode()
{
}

void RaycastVsConvexPoly2DMode::Startup()
{
	SubscribeEventCallbackFunction("SaveScene", RaycastVsConvexPoly2DMode::SaveGHCSScene);
	SubscribeEventCallbackFunction("LoadScene", RaycastVsConvexPoly2DMode::LoadGHCSScene);
	CreateReaminingConvexPoly2D();

	//std::vector<Vec2> ccwPositions;
	//ccwPositions.emplace_back(10.0f, 10.0f);
	//ccwPositions.emplace_back(20.0f, 10.0f);
	//ccwPositions.emplace_back(25.0f, 17.0f);
	//ccwPositions.emplace_back(24.0f, 21.0f);
	//ccwPositions.emplace_back(14.0f, 35.0f);

	//ConvexPolyShape2D* newShape = new  ConvexPolyShape2D(ccwPositions, transparentBlue, solidBlue);
	//newShape->m_convexHull2D = newShape->m_convexPoly2D.GetConvexHull();

	//m_allShapes.push_back(newShape);

}

void RaycastVsConvexPoly2DMode::Shutdown()
{
	for (int shapeIndex = 0; shapeIndex < m_allShapes.size(); shapeIndex++) {
		Shape2D*& shape = m_allShapes[shapeIndex];
		if (shape) {
			delete shape;
			shape = nullptr;
		}
	}

	for (Shape2D* aabb2Shape : m_hitAABB2s) {
		delete aabb2Shape;
	}

	delete m_shapesBuffer;
	m_shapesBuffer = nullptr;
	m_hitAABB2s.clear();
	m_allShapes.clear();
	m_allConvexPolys.clear();
	m_normalColorShapes.clear();
	m_highlightedShapes.clear();
	m_existingPolys = 0;
	DeleteAABB2Tree();
	m_bvhDebugVerts.clear();
	UnsubscribeEventCallbackFunction("SaveScene", RaycastVsConvexPoly2DMode::SaveGHCSScene);
	UnsubscribeEventCallbackFunction("LoadScene", RaycastVsConvexPoly2DMode::LoadGHCSScene);
	toc.clear();
}

void RaycastVsConvexPoly2DMode::Update(float deltaSeconds)
{
	UpdateInput(deltaSeconds);
	m_raycastTotalTime = 0;
	m_raycastCounter = 0;

	if (m_isTreeDirty && m_useBVH) {
		DeleteAABB2Tree();
		CreateAABB2Tree();
		m_isTreeDirty = false;
	}

	if (m_halfPolygons) {
		HalveCurrentConvexPoly2D();
		m_halfPolygons = false;
	}
	DoInvisibleRaycasts();
	if (m_useBVH) {
		RaycastVsAABB2Tree(m_rayStart, m_rayEnd);
	}
	else {
		AddVertsForRaycastVsConvexPolys2D();
	}

	UpdateSelectedPoly(deltaSeconds);

	if (m_didAnyShapeChange && !(m_isMovingShape || m_isChangingShape)) {
		CreateVertexBuffer();
		DeleteAABB2Tree();
		CreateAABB2Tree();
		m_didAnyShapeChange = false;
	}

	float raycastTimeFlt = static_cast<float>(m_raycastTotalTime) / 1'000'000.0f;
	float raycastAvgTime = static_cast<float>(raycastTimeFlt) / static_cast<float>(m_raycastCounter);
	float raycastsPerMs = 1.0f / raycastAvgTime;

	m_helperText = m_baseHelperText;
	m_helperText += (m_useBVH) ? ", BVH: ON" : ", BVH: OFF";

	DebugAddScreenText(Stringf("Invisible Rays: %d", m_numInvisibleRays), Vec2(0.0f, m_UICameraSize.y * 0.90f), 0.0f, Vec2::ZERO, m_textCellHeight, Rgba8::WHITE, Rgba8::WHITE);
	DebugAddScreenText(Stringf("Convex Poly Count %d", m_amountOfPolys), Vec2(0.0f, m_UICameraSize.y * 0.87f), 0.0f, Vec2::ZERO, m_textCellHeight, Rgba8::WHITE, Rgba8::WHITE);
	DebugAddScreenText(Stringf("Raycast Avg Time (ms): %f", raycastAvgTime), Vec2(0.0f, m_UICameraSize.y * 0.84f), 0.0f, Vec2::ZERO, m_textCellHeight, Rgba8::WHITE, Rgba8::WHITE);
	DebugAddScreenText(Stringf("Raycast Per MS: %.2f", raycastsPerMs), Vec2(0.0f, m_UICameraSize.y * 0.81f), 0.0f, Vec2::ZERO, m_textCellHeight, Rgba8::WHITE, Rgba8::WHITE);
	DebugAddScreenText(Stringf("Raycast Total: %.2f", raycastTimeFlt), Vec2(0.0f, m_UICameraSize.y * 0.78f), 0.0f, Vec2::ZERO, m_textCellHeight, Rgba8::WHITE, Rgba8::WHITE);
	DebugAddScreenText(Stringf("Max depth: %d", m_maxDepth), Vec2(0.0f, m_UICameraSize.y * 0.75f), 0.0f, Vec2::ZERO, m_textCellHeight, Rgba8::WHITE, Rgba8::WHITE);
	DebugAddScreenText(Stringf("FPS: %.2f", 1.0f / deltaSeconds), Vec2(0.0f, m_UICameraSize.y * 0.71f), 0.0f, Vec2::ZERO, m_textCellHeight, Rgba8::WHITE, Rgba8::WHITE);
	GameMode::Update(deltaSeconds);
}

void RaycastVsConvexPoly2DMode::Render() const
{
	g_theRenderer->BeginCamera(m_worldCamera);
	g_theRenderer->ClearScreen(Rgba8::BLACK);
	if (g_drawDebug) {
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	}
	else {
		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	}
	g_theRenderer->SetRasterizerState(CullMode::NONE, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
	g_theRenderer->BindTexture(nullptr);

	if (m_useBVH && g_drawDebug) {
		RenderBVH();
	}

	RenderNormalColorShapes();
	RenderHighlightedColorShapes();
	g_theRenderer->DrawVertexArray(m_raycastVsConvexPolyCollisionVerts);

	std::vector<Vertex_PCU> arrowVerts;

	if (m_impactedShape) {
		if (m_raycastResult.m_impactDist >= 1.0f) {
			AddVertsForArrow2D(arrowVerts, m_rayStart, m_raycastResult.m_impactPos, Rgba8::RED, 0.5f);
		}
		AddVertsForArrow2D(arrowVerts, m_raycastResult.m_impactPos, m_rayEnd, Rgba8::GRAY, 0.5f);
	}
	else {
		AddVertsForArrow2D(arrowVerts, m_rayStart, m_rayEnd, Rgba8::GREEN, 0.5f);
	}

	g_theRenderer->DrawVertexArray(arrowVerts);
	g_theRenderer->EndCamera(m_worldCamera);

	RenderUI();
	DebugRenderWorld(m_worldCamera);
	DebugRenderScreen(m_UICamera);
}



bool RaycastVsConvexPoly2DMode::SaveGHCSScene(EventArgs& args)
{
	std::string fileName = args.GetValue("filename", "unnamed");
	std::string fullPath = "Data/Scenes/" + fileName;
	if (AreStringsEqualCaseInsensitive(fileName, "unnamed")) return false;

	pointerToSelf->SaveSceneToFile(fullPath);

	return true;
}

bool RaycastVsConvexPoly2DMode::LoadGHCSScene(EventArgs& args)
{
	std::string fileName = args.GetValue("filename", "unnamed");
	std::string fullPath = "Data/Scenes/" + fileName;
	if (AreStringsEqualCaseInsensitive(fileName, "unnamed")) return false;

	pointerToSelf->LoadSceneFromFile(fullPath);

	return true;
}

void RaycastVsConvexPoly2DMode::RenderNormalColorShapes() const
{
	//size_t objectsToDraw = m_shapesBuffer->GetSize() / m_shapesBuffer->GetStride();

	//g_theRenderer->BindVertexBuffer(m_shapesBuffer, sizeof(Vertex_PCU));
	g_theRenderer->DrawVertexBuffer(m_shapesBuffer);
	float planeDrawDist = 1000.0f;
	float lineThickness = 1.0f;

	Vec2 rayFwd = m_rayEnd - m_rayStart;
	float rayDist = rayFwd.NormalizeAndGetPreviousLength();

	if (m_amountOfPolys == 1) {
		std::vector<Vertex_PCU> convexHullVerts;
		ConvexPolyShape2D* asConvexPoly = dynamic_cast<ConvexPolyShape2D*>(m_allShapes[0]);

		for (Plane2D const& plane : asConvexPoly->m_convexHull2D.m_planes) {
			Vec2 middlePoint = plane.m_planeNormal * plane.m_distToPlane;
			Vec2 lineStart = middlePoint + plane.m_planeNormal.GetRotated90Degrees() * planeDrawDist;
			Vec2 lineEnd = middlePoint - plane.m_planeNormal.GetRotated90Degrees() * planeDrawDist;

			RaycastResult2D rayVsPlane = RaycastVsPlane(m_rayStart, rayFwd, rayDist, plane);
			Rgba8 planeColor = Rgba8::GREEN;

			if (rayVsPlane.m_didImpact) {
				if (DotProduct2D(rayFwd, plane.m_planeNormal) > 0) {
					planeColor = Rgba8::YELLOW;
				}
				else {
					planeColor = Rgba8::RED;
				}

			}

			AddVertsForLineSegment2D(convexHullVerts, LineSegment2(lineStart, lineEnd), planeColor, lineThickness);
		}

		//AddVertsForConvexHull2D(convexHullVerts, asConvexPoly->m_convexHull2D, Rgba8::RED, 1000.0f, 1.0f);
		g_theRenderer->DrawVertexArray(convexHullVerts);
	}

	if (g_drawDebug) {
		g_theRenderer->DrawVertexArray(m_shapeDebugDiscs);
	}
}

void RaycastVsConvexPoly2DMode::RenderHighlightedColorShapes() const
{
	for (int shapeIndex = 0; shapeIndex < m_highlightedShapes.size(); shapeIndex++) {
		Shape2D const* shape = m_highlightedShapes[shapeIndex];
		if (!shape) continue;
		shape->RenderHighlighted();
	}
}

void RaycastVsConvexPoly2DMode::DoInvisibleRaycasts()
{
	//ProfileLogScope raycastsProfile("Raycast Profile", false, &m_raycastTotalTime);

	for (int raycastInd = 0; raycastInd < m_numInvisibleRays; raycastInd++) {
		Vec2 startRay = GetRandomPositionInWorld();
		Vec2 endRay = GetRandomPositionInWorld();

		if (m_useBVH) {

			RaycastVsAABB2Tree(startRay, endRay, false);
		}
		else {
			ProfileLogScope raycastsProfile("Raycast Profile", false, &m_raycastTotalTime);

			Vec2 forward = endRay - startRay;
			float rayLength = forward.NormalizeAndGetPreviousLength();
			m_raycastCounter++;

			for (Shape2D* shape : m_allShapes) {
				ConvexPolyShape2D* asConvexPoly2D = dynamic_cast<ConvexPolyShape2D*>(shape);

				if (asConvexPoly2D) {
					RaycastResult2D raycastVsDisc = RaycastVsDisc(startRay, forward, rayLength, asConvexPoly2D->m_position, asConvexPoly2D->m_radius);
					if (raycastVsDisc.m_didImpact) {
						RaycastVsConvexHull2D(startRay, forward, rayLength, asConvexPoly2D->m_convexHull2D);
					}
				}
			}
		}

	}
}

ConvexPolyShape2D* RaycastVsConvexPoly2DMode::GetShapeUnderMouse() const
{
	AABB2 worldBoundingBox(Vec2::ZERO, m_worldSize);

	Vec2 cursorPosition = worldBoundingBox.GetPointAtUV(g_theWindow->GetNormalizedCursorPos());


	for (Shape2D* shape : m_allShapes) {
		if (!shape) continue;
		ConvexPolyShape2D* asConvexPolyShape = dynamic_cast<ConvexPolyShape2D*>(shape);
		if (!asConvexPolyShape) continue;

		bool isMouseInside = IsPointInsideConvexHull2D(cursorPosition, asConvexPolyShape->m_convexHull2D);
		if (isMouseInside) {
			return asConvexPolyShape;
		}

	}

	return nullptr;
}

void RaycastVsConvexPoly2DMode::ParseToc(unsigned int tocStart)
{
	toc.clear();
	BufferParser buffParser(m_currentScene);
	buffParser.GoToOffset(tocStart);

	buffParser.SetEndianness((BufferEndianness)m_currentFileEndianness);

	bool isHeaderCorrect = ParseTocHeader(buffParser);
	if (!isHeaderCorrect) {
		ERROR_RECOVERABLE("TOC HEADER IS INCORRECT");
		return;
	}

	unsigned char tocEntries = buffParser.ParseChar();

	for (unsigned int entryInd = 0; entryInd < tocEntries; entryInd++) {
		TOCEntry newEntry = {};
		newEntry.m_chunkType = buffParser.ParseChar();
		newEntry.m_headerStart = buffParser.ParseUint32();
		newEntry.m_chunkTotalSize = buffParser.ParseUint32();
		toc.push_back(newEntry);
	}

	bool isEndSeqCorrect = ParseTocEndSequence(buffParser);
	if (!isEndSeqCorrect) {
		ERROR_RECOVERABLE("TOC END SEQUENCE IS INCORRECT");
		return;
	}

}

void RaycastVsConvexPoly2DMode::AppendToc(std::vector<unsigned char>& sceneBuffer) const
{
	WriteTocHeaderToBuffer(sceneBuffer);

	BufferWriter bufWriter(sceneBuffer);
	bufWriter.AppendeByte((unsigned char)toc.size());
	for (TOCEntry entry : toc) {
		bufWriter.AppendChar(entry.m_chunkType);
		bufWriter.AppendUint32(entry.m_headerStart);
		bufWriter.AppendUint32(entry.m_chunkTotalSize);
	}

	WriteTocEndSequenceToBuffer(sceneBuffer);
}



void RaycastVsConvexPoly2DMode::CreateOrUpdateTocEntry(unsigned char chunkType, unsigned int totalSize, unsigned int headerStart) const
{

	bool createTocEntry = false;
	if (m_rewritingExistingScene) {
		TOCEntry* existingEntry = FindTocEntry(chunkType);
		if (existingEntry) {
			existingEntry->m_chunkTotalSize = totalSize;
			existingEntry->m_headerStart = headerStart;
		}

		createTocEntry = (existingEntry == nullptr);
	}

	if ((!m_rewritingExistingScene) || (createTocEntry)) {
		TOCEntry newTocEntry = {};
		newTocEntry.m_chunkTotalSize = totalSize;
		newTocEntry.m_chunkType = chunkType;
		newTocEntry.m_headerStart = headerStart;

		toc.push_back(newTocEntry);
	}
}

void RaycastVsConvexPoly2DMode::WriteSceneChunkToBuffer(std::vector<unsigned char>& fileBuffer, unsigned char endianness) const
{
	unsigned int chunkHeaderStart = (unsigned int)fileBuffer.size();

	WriteChunkHeaderToBuffer(fileBuffer, ChunkType::SceneInfoChunk, endianness, 20);
	BufferWriter bufWriter(fileBuffer);

	bufWriter.AppendAABB2(m_worldCamera.GetCameraBounds());
	bufWriter.AppendUint32((unsigned int)m_amountOfPolys);

	WriteChunkHeaderEndSequence(fileBuffer);

	CreateOrUpdateTocEntry(ChunkType::SceneInfoChunk, 34, chunkHeaderStart);

}

void RaycastVsConvexPoly2DMode::WriteConvexPolyChunkToBuffer(std::vector<unsigned char>& fileBuffer, unsigned char chunkEndianness) const
{
	unsigned int chunkSize = 0; // Unknown at this time
	unsigned int chunkHeaderStart = (unsigned int)fileBuffer.size();
	unsigned int chunkSizePos = (unsigned int)fileBuffer.size() + 6;

	WriteChunkHeaderToBuffer(fileBuffer, ChunkType::ConvexPolysChunk, chunkEndianness, chunkSize);

	BufferWriter bufWriter(fileBuffer);

	unsigned int startingChunkPos = (unsigned int)fileBuffer.size();
	bufWriter.AppendUint32((unsigned int)m_amountOfPolys);

	for (Shape2D* shape : m_allConvexPolys) {
		ConvexPolyShape2D* asConvexPolyShape = dynamic_cast<ConvexPolyShape2D*>(shape);
		ConvexPoly2D const& convexPoly = asConvexPolyShape->m_convexPoly2D;
		convexPoly.WritePolyToBuffer(fileBuffer);

	}

	unsigned int endingChunkPos = (unsigned int)fileBuffer.size();
	chunkSize = endingChunkPos - startingChunkPos;

	WriteChunkHeaderEndSequence(fileBuffer);
	unsigned int totalChunkSize = chunkSize + 14;

	bufWriter.OverwriteUint32(chunkSizePos, chunkSize);

	CreateOrUpdateTocEntry(ChunkType::ConvexPolysChunk, totalChunkSize, chunkHeaderStart);


}

void RaycastVsConvexPoly2DMode::WriteBoundingDiscsChunkToBuffer(std::vector<unsigned char>& fileBuffer, unsigned char endianness) const
{
	unsigned int chunkSize = 0; // Unknown at this time
	unsigned int chunkHeaderStart = (unsigned int)fileBuffer.size();
	unsigned int chunkSizePos = (unsigned int)fileBuffer.size() + 6;

	WriteChunkHeaderToBuffer(fileBuffer, ChunkType::BoundingDiscsChunk, endianness, chunkSize);

	BufferWriter bufWriter(fileBuffer);

	unsigned int startingChunkPos = (unsigned int)fileBuffer.size();

	bufWriter.AppendUint32((unsigned int) m_allConvexPolys.size());

	for (Shape2D* shape : m_allConvexPolys) {
		ConvexPolyShape2D* asConvexPoly = dynamic_cast<ConvexPolyShape2D*>(shape);
		if(!shape) continue;

		bufWriter.AppendVec2(asConvexPoly->m_discCenter);
		bufWriter.AppendFloat(asConvexPoly->m_radius);

	}

	unsigned int endingChunkPos = (unsigned int)fileBuffer.size();
	chunkSize = endingChunkPos - startingChunkPos;

	WriteChunkHeaderEndSequence(fileBuffer);
	unsigned int totalChunkSize = chunkSize + 14;

	bufWriter.OverwriteUint32(chunkSizePos, chunkSize);

	CreateOrUpdateTocEntry(ChunkType::BoundingDiscsChunk, totalChunkSize, chunkHeaderStart);

}



void RaycastVsConvexPoly2DMode::WriteAABB2TreeChunkToBuffer(std::vector<unsigned char>& fileBuffer, unsigned char endianness) const
{
	unsigned int chunkSize = 0; // Unknown at this time
	unsigned int chunkHeaderStart = (unsigned int)fileBuffer.size();
	unsigned int chunkSizePos = (unsigned int)fileBuffer.size() + 6;

	WriteChunkHeaderToBuffer(fileBuffer, ChunkType::AABB2TreeChunk, endianness, chunkSize);

	BufferWriter bufWriter(fileBuffer);

	unsigned int startingChunkPos = (unsigned int)fileBuffer.size();

	std::vector<AABB2TreeNode*> allTreeNodes;
	int amountOfNodes = 0;
	GetAABB2TreeInVector(allTreeNodes, -1, amountOfNodes);

	bufWriter.AppendUint32((unsigned int)amountOfNodes);
	for (int nodeNumber = 0; nodeNumber < allTreeNodes.size(); nodeNumber++) {
		AABB2TreeNode const* currentNode = allTreeNodes[nodeNumber];
		if (!currentNode) continue;
		bufWriter.AppendInt32(nodeNumber);

		bufWriter.AppendAABB2(currentNode->m_bounds);

		bufWriter.AppendUint32((unsigned int)currentNode->m_containedShapesIndexes.size());
		for (int containedIndex : currentNode->m_containedShapesIndexes) {
			bufWriter.AppendInt32(containedIndex);
		}

	}
	unsigned int endingChunkPos = (unsigned int)fileBuffer.size();
	chunkSize = endingChunkPos - startingChunkPos;

	WriteChunkHeaderEndSequence(fileBuffer);
	unsigned int totalChunkSize = chunkSize + 14;

	bufWriter.OverwriteUint32(chunkSizePos, chunkSize);

	CreateOrUpdateTocEntry(ChunkType::AABB2TreeChunk, totalChunkSize, chunkHeaderStart);
}




void RaycastVsConvexPoly2DMode::ParseSceneChunk(BufferParser& bufferParser)
{
	m_allShapes.clear();
	m_allConvexPolys.clear();
	m_amountOfPolys = 0;
	m_allConvexPolys.reserve(m_amountOfPolys);

	AABB2 worldBounds = bufferParser.ParseAABB2();
	m_worldCamera.SetOrthoView(worldBounds.m_mins, worldBounds.m_maxs);
	m_amountOfPolys = bufferParser.ParseUint32();
}



void RaycastVsConvexPoly2DMode::ParseConvexPolyChunk(BufferParser& bufferParser)
{
	unsigned int numPolys = bufferParser.ParseUint32();
	Rgba8 transparentBlue(60, 60, 255, 120);
	Rgba8 solidBlue(0, 0, 255, 220);


	if (numPolys != (unsigned int)m_amountOfPolys) {
		ERROR_RECOVERABLE("TRYING TO PARSE DIFFERENT AMOUNT OF POLYGONS THAN STATED IN SCENE CHUNK");
		return;
	}


	for (unsigned int polyIndex = 0; polyIndex < numPolys; polyIndex++) {
		unsigned char numVertexes = bufferParser.ParseByte();
		std::vector<Vec2> vertexes;
		vertexes.reserve(numVertexes);

		for (int vertexIndex = 0; vertexIndex < numVertexes; vertexIndex++) {
			Vec2 vertex = bufferParser.ParseVec2();
			vertexes.push_back(vertex);
		}

		ConvexPolyShape2D* newPoly = new ConvexPolyShape2D(vertexes, transparentBlue, solidBlue);
		newPoly->m_convexHull2D = newPoly->m_convexPoly2D.GetConvexHull();
		m_allShapes.push_back(newPoly);
		m_allConvexPolys.push_back(newPoly);
	}

	bool isEndRight = ParseChunkHeaderEndSequence(bufferParser);
	if (!isEndRight) {
		ERROR_RECOVERABLE("CURRENT CHUNK END SEQUENCE IS INCORRECT");
	}

	CreateVertexBuffer();
}

void RaycastVsConvexPoly2DMode::ParseAABB2TreeChunk(BufferParser& bufferParser)
{
	m_bvhDebugVerts.clear();
	std::vector<AABB2TreeNode*> allTreeNodes;
	unsigned int amountOfNodes = bufferParser.ParseUint32();

	allTreeNodes.push_back(nullptr);
	for (unsigned int nodeIndex = 0; nodeIndex < amountOfNodes; nodeIndex++) {

		AABB2TreeNode* newTreeNode = new AABB2TreeNode();

		int nodeNumber = bufferParser.ParseInt32();
		allTreeNodes[nodeNumber] = newTreeNode;

		AABB2 bounds = bufferParser.ParseAABB2();

		newTreeNode->m_bounds = bounds;
		AddVertsForAABB2D(m_bvhDebugVerts, newTreeNode->m_bounds, GetRandomColor());

		allTreeNodes.push_back(nullptr);
		allTreeNodes.push_back(nullptr);

		unsigned int numContainedShapesIndexes = bufferParser.ParseUint32();
		for (unsigned int shapeIndex = 0; shapeIndex < numContainedShapesIndexes; shapeIndex++) {
			int containedIndex = bufferParser.ParseInt32();
			newTreeNode->m_containedShapesIndexes.push_back(containedIndex);
		}
	}

	for (unsigned int nodeIndex = 0; nodeIndex < allTreeNodes.size(); nodeIndex++) {
		AABB2TreeNode* treeNode = allTreeNodes[nodeIndex];

		if (!treeNode) continue;

		int firstChild = (nodeIndex * 2) + 1;
		int secondChild = (nodeIndex * 2) + 2;

		if (firstChild < allTreeNodes.size()) {
			treeNode->m_firstChild = allTreeNodes[firstChild];
		}
		if (firstChild < allTreeNodes.size()) {
			treeNode->m_secondChild = allTreeNodes[secondChild];
		}
	}

	if (allTreeNodes.size() > 0) {
		m_convexPolyTreeRoot = allTreeNodes[0];
		m_isTreeDirty = false;
	}
}

void RaycastVsConvexPoly2DMode::ParseBoundingDiscsChunk(BufferParser& bufferParser)
{
	unsigned int numDiscs = bufferParser.ParseUint32();

	if (numDiscs != (unsigned int)m_amountOfPolys) {
		ERROR_RECOVERABLE("MISMATCH IN BOUNDING DISC QUANTITY");
		return;
	}

	for (unsigned int discIndex = 0; discIndex < numDiscs; discIndex++) {
		Vec2 discCenter = bufferParser.ParseVec2();
		float discRadius = bufferParser.ParseFloat();

		Shape2D* shape = m_allConvexPolys[discIndex];
		ConvexPolyShape2D* asPoly = dynamic_cast<ConvexPolyShape2D*>(shape);

		if (asPoly) {
			asPoly->m_discCenter = discCenter;
			asPoly->m_radius = discRadius;
		}

	}
}

void RaycastVsConvexPoly2DMode::ParseChunk(BufferParser& bufferParser)
{
	ChunkType chunkType = ChunkType::INVALID_CHUNK;
	unsigned char chunkEndianness = 0;
	unsigned int chunkSize = 0;
	bool isChunkHeaderRight = ParseChunkHeader(bufferParser, chunkType, chunkEndianness, chunkSize);

	if (!isChunkHeaderRight) {
		ERROR_RECOVERABLE("CURRENT CHUNK HEADER IS INCORRECT");
	}

	bufferParser.SetEndianness((BufferEndianness)chunkEndianness);

	switch (chunkType)
	{
	case SceneInfoChunk:
		ParseSceneChunk(bufferParser);
		break;
	case ConvexPolysChunk:
		ParseConvexPolyChunk(bufferParser);
		break;
	case ConvexHullsChunk:
		break;
	case BoundingDiscsChunk:
		ParseBoundingDiscsChunk(bufferParser);
		break;
	case BoundingAABBsChunk:
		break;
	case AABB2TreeChunk:
		ParseAABB2TreeChunk(bufferParser);
		break;
	case OBB2TreeChunk:
		break;
	case ConvexHullTreeChunk:
		break;
	case AsymmetricQuadTreeChunk:
		break;
	case SymmetricQuadTreeChunk:
		break;
	case TiledBitRegionsChunk:
		break;
	case ColumnRowBitRegionsChunk:
		break;
	case Disc2TreeChunk:
		break;
	case BSPChunk:
		break;
	case CompositeTreeChunk:
		break;
	case ConvexPolyTreeChunk:
		break;
	case ObjectColorsChunk:
		break;
	case SceneGeneralDebugChunk:
		break;

	case SceneRaycastDebugChunk:
		break;
	case INVALID_CHUNK:
		break;
	default:
		break;
	}

}

void RaycastVsConvexPoly2DMode::RewriteScene(std::vector<unsigned char>& saveBuffer, unsigned char chunkType, unsigned int headerStart, unsigned int totalChunkSize) const
{
	BufferParser bufParser(m_currentScene);
	bufParser.GoToOffset(headerStart);

	ChunkType throwawayType = ChunkType::INVALID_CHUNK;
	unsigned char chunkEndianness = 0;
	unsigned int chunkSize = 0;

	ParseChunkHeader(bufParser, throwawayType, chunkEndianness, chunkSize);



	switch (chunkType)
	{
	case SceneInfoChunk:
		WriteSceneChunkToBuffer(saveBuffer, chunkEndianness);
		break;
	case ConvexPolysChunk:
		WriteConvexPolyChunkToBuffer(saveBuffer, chunkEndianness);
		break;
	case AABB2TreeChunk:
		WriteAABB2TreeChunkToBuffer(saveBuffer, chunkEndianness);
		break;
	case BoundingDiscsChunk:
		WriteBoundingDiscsChunkToBuffer(saveBuffer, chunkEndianness);
		break;
	case INVALID_CHUNK:
		ERROR_RECOVERABLE("TRYING TO WRITE AN INVALID CHUNK TO BUFFER");
		break;
	default:
		CopyChunkAsIs(saveBuffer, headerStart, totalChunkSize);
		break;
	}
}

void RaycastVsConvexPoly2DMode::CopyChunkAsIs(std::vector<unsigned char>& saveBuffer, unsigned int headerStart, unsigned int totalChunkSize) const
{
	unsigned int endPosition = headerStart + totalChunkSize;
	saveBuffer.insert(saveBuffer.end(), m_currentScene[headerStart], m_currentScene[endPosition]);
}

void RaycastVsConvexPoly2DMode::SaveSceneToFile(std::filesystem::path fileName) const
{
	fileName.replace_extension(".ghcs");

	std::vector<unsigned char> newSceneBuffer;
	newSceneBuffer.reserve(8000);

	WriteHeaderToBuffer(newSceneBuffer);

	if (m_rewritingExistingScene) {
		for (TOCEntry const& entry : toc) {
			RewriteScene(newSceneBuffer, entry.m_chunkType, entry.m_headerStart, entry.m_chunkTotalSize);
		}
	}
	else {
		unsigned char nativeEndianness = (unsigned char)GetNativeEndianness();
		WriteSceneChunkToBuffer(newSceneBuffer, nativeEndianness);
		WriteConvexPolyChunkToBuffer(newSceneBuffer, nativeEndianness);
		WriteBoundingDiscsChunkToBuffer(newSceneBuffer, nativeEndianness);
		WriteAABB2TreeChunkToBuffer(newSceneBuffer, nativeEndianness);
	}

	unsigned int tocLocationStart = (unsigned int)newSceneBuffer.size();
	BufferWriter bufWriter(newSceneBuffer);
	bufWriter.OverwriteUint32(8, tocLocationStart);
	AppendToc(newSceneBuffer);


	FileWriteFromBuffer(newSceneBuffer, fileName.string());
}


void RaycastVsConvexPoly2DMode::LoadSceneFromFile(std::filesystem::path fileName)
{
	fileName.replace_extension(".ghcs");
	m_currentScene.clear();

	FileReadToBuffer(m_currentScene, fileName.string());

	BufferParser buffParser(m_currentScene);

	unsigned int tocLocation = 0;

	bool isHeaderCorrect = ParseHeader(buffParser, m_currentFileEndianness, tocLocation);

	if (!isHeaderCorrect) {
		ERROR_RECOVERABLE("HEADER IS INCORRECTLY FORMATTED");
		return;
	}


	ParseToc(tocLocation);

	bool foundSceneInfo = false;
	bool foundConvexPolyChunk = false;

	unsigned char sceneInfoByte = (unsigned char)ChunkType::SceneInfoChunk;
	unsigned char convexPolysByte = (unsigned char)ChunkType::ConvexPolysChunk;


	for (TOCEntry const& tocEntry : toc) {
		if (!foundSceneInfo && (tocEntry.m_chunkType > sceneInfoByte)) {
			ERROR_RECOVERABLE("SCENE CHUNK WAS NOT FOUND");
			return;
		}
		else if (tocEntry.m_chunkType == sceneInfoByte) {
			foundSceneInfo = true;
		}

		if (!foundConvexPolyChunk && (tocEntry.m_chunkType > convexPolysByte)) {
			ERROR_RECOVERABLE("CONVEX POLYS CHUNK WAS NOT FOUND");
			return;
		}
		else if (tocEntry.m_chunkType == convexPolysByte) {
			foundConvexPolyChunk = true;
		}


		buffParser.GoToOffset(tocEntry.m_headerStart);
		ParseChunk(buffParser);
	}
}

void RaycastVsConvexPoly2DMode::CreateAABB2Tree()
{
	std::vector<AABB2TreeNode*> candidates;
	for (int shapeIndex = 0; shapeIndex < m_allShapes.size(); shapeIndex++) {
		Shape2D* shape = m_allShapes[shapeIndex];
		ConvexPolyShape2D* asConvexPoly = dynamic_cast<ConvexPolyShape2D*>(shape);
		if (asConvexPoly) {
			AABB2TreeNode* newNode = new AABB2TreeNode();
			newNode->m_bounds = asConvexPoly->m_convexPoly2D.GetBoundingBox();
			newNode->m_containedShapesIndexes.push_back(shapeIndex);
			candidates.push_back(newNode);
			AddVertsForAABB2D(m_bvhDebugVerts, newNode->m_bounds, GetRandomColor());
		}
	}

	std::vector<AABB2TreeNode*> usedCandidates;
	while (candidates.size() > 1) {
		usedCandidates.clear();
		std::vector<AABB2TreeNode*> newCandidates;
		for (int currentNodeInd = 0; currentNodeInd < candidates.size(); currentNodeInd++) {
			AABB2TreeNode* currentNode = candidates[currentNodeInd];

			auto currentNodeIt = std::find(usedCandidates.begin(), usedCandidates.end(), currentNode);
			bool isCurrentNodeTaken = (currentNodeIt != usedCandidates.end());
			if (isCurrentNodeTaken) continue;

			AABB2TreeNode* bestCandidate = nullptr;
			float closestDistance = FLT_MAX;
			Vec2 currentNodeBoundCenter = currentNode->m_bounds.GetCenter();

			for (int mergeCandidateInd = currentNodeInd + 1; mergeCandidateInd < candidates.size(); mergeCandidateInd++) {
				AABB2TreeNode* mergeCandidate = candidates[mergeCandidateInd];
				auto candidateIt = std::find(usedCandidates.begin(), usedCandidates.end(), mergeCandidate);
				bool isCandidateTaken = (candidateIt != usedCandidates.end());
				if (isCandidateTaken) continue;


				Vec2 mergeCandidateCenter = mergeCandidate->m_bounds.GetCenter();
				float distanceSqr = GetDistanceSquared2D(mergeCandidateCenter, currentNodeBoundCenter);
				if (closestDistance > distanceSqr) {
					closestDistance = distanceSqr;
					bestCandidate = mergeCandidate;
				}

			}

			bool foundBetterMatchNextRound = false;

			for (int newCandidateInd = 0; (newCandidateInd < newCandidates.size()) && !foundBetterMatchNextRound; newCandidateInd++) {
				AABB2TreeNode* currentNewCand = newCandidates[newCandidateInd];
				Vec2 newCandCenter = currentNewCand->m_bounds.GetCenter();
				float distanceSqr = GetDistanceSquared2D(newCandCenter, currentNodeBoundCenter);
				if (closestDistance > distanceSqr) {
					foundBetterMatchNextRound = true;
				}

			}

			if (!foundBetterMatchNextRound && bestCandidate) {
				AABB2 combinedBounds = currentNode->m_bounds;
				combinedBounds.StretchToIncludePoint(bestCandidate->m_bounds.m_mins);
				combinedBounds.StretchToIncludePoint(bestCandidate->m_bounds.m_maxs);

				AABB2TreeNode* combinedNode = new AABB2TreeNode();
				combinedNode->m_bounds = combinedBounds;
				combinedNode->m_containedShapesIndexes.insert(combinedNode->m_containedShapesIndexes.begin(), currentNode->m_containedShapesIndexes.begin(), currentNode->m_containedShapesIndexes.end());
				combinedNode->m_containedShapesIndexes.insert(combinedNode->m_containedShapesIndexes.end(), bestCandidate->m_containedShapesIndexes.begin(), bestCandidate->m_containedShapesIndexes.end());

				combinedNode->m_firstChild = currentNode;
				combinedNode->m_secondChild = bestCandidate;
				newCandidates.push_back(combinedNode);
				usedCandidates.push_back(currentNode);
				usedCandidates.push_back(bestCandidate);
				AddVertsForAABB2D(m_bvhDebugVerts, combinedNode->m_bounds, GetRandomColor());
			}
			else {
				newCandidates.push_back(currentNode);
			}
		}

		candidates.clear();
		candidates.insert(candidates.begin(), newCandidates.begin(), newCandidates.end());
	}

	m_convexPolyTreeRoot = candidates[0];

}

void RaycastVsConvexPoly2DMode::DeleteAABB2Tree()
{
	m_bvhDebugVerts.clear();
	std::queue<AABB2TreeNode*> nodesToDelete;
	nodesToDelete.push(m_convexPolyTreeRoot);

	while (!nodesToDelete.empty()) {
		AABB2TreeNode*& currentNode = nodesToDelete.front();
		nodesToDelete.pop();
		if (currentNode) {
			if (currentNode->m_firstChild) {
				nodesToDelete.push(currentNode->m_firstChild);
			}
			if (currentNode->m_secondChild) {
				nodesToDelete.push(currentNode->m_secondChild);
			}
		}

		delete currentNode;
		currentNode = nullptr;
	}

	m_convexPolyTreeRoot = nullptr;

}

void RaycastVsConvexPoly2DMode::CreateReaminingConvexPoly2D()
{
	FloatRange radiusRange = g_gameConfigBlackboard.GetValue("CONVEX_POLY_RADIUS_RANGE", FloatRange::ZERO_TO_ONE);
	FloatRange distBetPoints = g_gameConfigBlackboard.GetValue("CONVEX_POLY_DISTANCE_BETWEEN_POINTS", FloatRange(10.0f, 45.0f));

	Rgba8 transparentBlue(60, 60, 255, 120);
	Rgba8 solidBlue(0, 0, 255, 220);

	int polysToCreate = (m_amountOfPolys - m_existingPolys);

	for (int convexPolyInd = 0; convexPolyInd < polysToCreate; convexPolyInd++) {
		float randAngleStart = rng.GetRandomFloatInRange(0.0f, 360.0f);

		float currentAngle = randAngleStart;
		float polyRadius = rng.GetRandomFloatInRange(radiusRange);
		Vec2 randPos = GetClampedRandomPositionInWorld(polyRadius);

		std::vector<Vec2> polyPoints;
		while ((currentAngle - randAngleStart) < 360.0f) {
			Vec2 newPoint = randPos + (Vec2::MakeFromPolarDegrees(currentAngle) * polyRadius);
			polyPoints.push_back(newPoint);
			currentAngle += rng.GetRandomFloatInRange(distBetPoints);
		}

		ConvexPolyShape2D* newShape = new  ConvexPolyShape2D(polyPoints, transparentBlue, solidBlue);
		newShape->m_convexHull2D = newShape->m_convexPoly2D.GetConvexHull();
		m_allShapes.push_back(newShape);
		m_allConvexPolys.push_back(newShape);
		m_existingPolys++;

	}

	CreateVertexBuffer();
	if (m_useBVH) {
		DeleteAABB2Tree();
		CreateAABB2Tree();
	}
}

void RaycastVsConvexPoly2DMode::CreateVertexBuffer()
{
	if (m_shapesBuffer) {
		delete m_shapesBuffer;
	}

	m_shapeDebugDiscs.clear();

	std::vector<Vertex_PCU> allVerts;
	std::vector<Vertex_PCU> fillingsVerts;
	for (int shapeIndex = 0; shapeIndex < m_allShapes.size(); shapeIndex++) {
		Shape2D* shape = m_allShapes[shapeIndex];
		ConvexPolyShape2D const* shapeAsConvexPoly = dynamic_cast<ConvexPolyShape2D*>(shape);
		if (!shape)continue;

		shapeAsConvexPoly->AddVertsForEdges(allVerts, Rgba8::WHITE);
		shapeAsConvexPoly->AddVertsForFillings(fillingsVerts);

		//AddVertsForDisc2D(m_shapeDebugDiscs, shapeAsConvexPoly->m_position, shapeAsConvexPoly->m_radius, Rgba8::BLUE, 12);


	}
	allVerts.insert(allVerts.end(), fillingsVerts.begin(), fillingsVerts.end());
	BufferDesc newVBufferDesc = {};
	newVBufferDesc.data = allVerts.data();
	newVBufferDesc.descriptorHeap = nullptr;
	newVBufferDesc.memoryUsage = MemoryUsage::Dynamic;
	newVBufferDesc.owner = g_theRenderer;
	newVBufferDesc.size = allVerts.size() * sizeof(Vertex_PCU);
	newVBufferDesc.stride = sizeof(Vertex_PCU);

	m_shapesBuffer = new VertexBuffer(newVBufferDesc);

	//g_theRenderer->CopyCPUToGPU(allVerts.data(), allVerts.size() * sizeof(Vertex_PCU), m_shapesBuffer);
}

void RaycastVsConvexPoly2DMode::UpdateSelectedPoly(float deltaSeconds)
{
	if (!(m_isMovingShape || m_isChangingShape)) return;
	if (!m_selectedShape) return;
	AABB2 worldBoundingBox(Vec2::ZERO, m_worldSize);

	Vec2 cursorPosition = worldBoundingBox.GetPointAtUV(g_theWindow->GetNormalizedCursorPos());


	m_highlightedShapes.push_back(m_selectedShape);

	if (m_angularVelocity != 0.0f) {
		m_selectedShape->m_convexPoly2D.RotateAroundPoint(cursorPosition, m_angularVelocity * deltaSeconds);
		m_didAnyShapeChange = true;
	}
	if (m_scaleSpeed != 1.0f) {
		m_selectedShape->m_convexPoly2D.ScaleAroundPoint(cursorPosition, 1.0f + m_scaleSpeed * deltaSeconds);
		m_didAnyShapeChange = true;

	}

	Vec2 dispToCurrentPos = cursorPosition - m_prevMousePosition;
	if (m_isMovingShape && dispToCurrentPos.GetLengthSquared() > 0) {
		m_didAnyShapeChange = true;
		m_selectedShape->m_convexPoly2D.Translate(dispToCurrentPos);
	}

	if (m_didAnyShapeChange) {
		m_selectedShape->m_convexHull2D = m_selectedShape->m_convexPoly2D.GetConvexHull();
		m_selectedShape->UpdateBoundingDisc();
	}


	m_prevMousePosition = cursorPosition;
}

void RaycastVsConvexPoly2DMode::HalveCurrentConvexPoly2D()
{
	for (int polyIndex = m_amountOfPolys; polyIndex < m_allShapes.size(); polyIndex++) {
		Shape2D* shape = m_allShapes[polyIndex];
		if (shape) {
			delete shape;
			m_allShapes[polyIndex] = nullptr;
		}
	}

	m_allShapes.resize(m_amountOfPolys);
	m_allConvexPolys.resize(m_amountOfPolys);
	m_existingPolys = m_amountOfPolys;

	CreateVertexBuffer();
	DeleteAABB2Tree();
	CreateAABB2Tree();
}

void RaycastVsConvexPoly2DMode::RaycastVsAABB2Tree(Vec2 const& rayStart, Vec2 const& rayEnd, bool addVertsForImpact)
{

	for (Shape2D* aabb2Shape : m_hitAABB2s) {
		delete aabb2Shape;
	}

	m_hitAABB2s.clear();
	m_normalColorShapes.clear();
	m_highlightedShapes.clear();
	m_raycastVsConvexPolyCollisionVerts.clear();

	std::queue<AABB2TreeNode*> nodesToCheck;
	std::vector<int> indexesToCheck;
	Vec2 rayFwd = rayEnd - rayStart;
	float rayLength = rayFwd.NormalizeAndGetPreviousLength();

	AABB2TreeNode* lastHitNode = nullptr;

	int currentLevel = 0;
	nodesToCheck.push(m_convexPolyTreeRoot);
	int nodesInCurrentLevel = 1;
	int nodesCheckedInLevel = 0;
	int nextLevelNodes = 0;
	while ((currentLevel < m_maxDepth) && (nodesToCheck.size() > 0)) {
		AABB2TreeNode* currentNode = nodesToCheck.front();
		AABB2 const& nodeBounds = currentNode->m_bounds;
		nodesToCheck.pop();
		nodesCheckedInLevel++;

		if (nodesCheckedInLevel == nodesInCurrentLevel) {
			currentLevel++;
			nodesCheckedInLevel = 0;
			nodesInCurrentLevel = nextLevelNodes;
			nextLevelNodes = 0;
		}

		RaycastResult2D hitsAABB2 = RaycastVsBox(rayStart, rayFwd, rayLength, nodeBounds);
		if (hitsAABB2.m_didImpact) {
			lastHitNode = currentNode;
			bool aboveMaxDepth = (m_maxDepth == -1) || (currentLevel < m_maxDepth);
			if (aboveMaxDepth && currentNode->HasChildren()) {
				if (currentNode->m_firstChild) {
					nodesToCheck.push(currentNode->m_firstChild);
					nextLevelNodes++;
				}
				if (currentNode->m_secondChild) {
					nodesToCheck.push(currentNode->m_secondChild);
					nextLevelNodes++;
				}
			}
			else {
				indexesToCheck.insert(indexesToCheck.end(), currentNode->m_containedShapesIndexes.begin(), currentNode->m_containedShapesIndexes.end());
			}

		}


	}

	if (addVertsForImpact && lastHitNode) {
		if (g_drawDebug) {
			AABB2 const& lastHitBounds = lastHitNode->m_bounds;
			AABB2Shape2D* hitAABB2 = new AABB2Shape2D(lastHitBounds.GetDimensions(), lastHitBounds.GetCenter(), Rgba8(255, 255, 255, 100), Rgba8(255, 255, 255, 100));
			m_highlightedShapes.push_back(hitAABB2);
		}
	}

	ProfileLogScope raycastProfile("RaycastProfile", false, &m_raycastTotalTime);
	m_raycastCounter++;
	if (indexesToCheck.size() > 0) {

		m_impactedShape = false;
		RaycastResult2D closestRaycast;
		closestRaycast.m_impactDist = ARBITRARILY_LARGE_VALUE;
		ConvexPolyShape2D* closestHitShape = nullptr;

		for (int checkIndex = 0; checkIndex < indexesToCheck.size(); checkIndex++) {
			int shapeIndex = indexesToCheck[checkIndex];
			ConvexPolyShape2D* convexPoly = dynamic_cast<ConvexPolyShape2D*>(m_allShapes[shapeIndex]);
			if (!convexPoly) continue;

			RaycastResult2D raycastVsDisc = RaycastVsDisc(rayStart, rayFwd, rayLength, convexPoly->m_position, convexPoly->m_radius);

			if (!raycastVsDisc.m_didImpact) continue;

			RaycastResult2D raycastResult = RaycastVsConvexHull2D(rayStart, rayFwd, rayLength, convexPoly->m_convexHull2D);

			if (raycastResult.m_didImpact) {
				if (raycastResult.m_impactDist < closestRaycast.m_impactDist) {
					closestRaycast = raycastResult;
					closestHitShape = convexPoly;
				}

				m_impactedShape = addVertsForImpact && true; // Invisible raycasts use this too
			}
		}


		if (addVertsForImpact) {
			m_raycastResult = closestRaycast;
			if (m_impactedShape) {
				AddVertsForRaycastImpactOnConvexPoly2D(*closestHitShape, m_raycastResult);
			}
		}
	}


}

void RaycastVsConvexPoly2DMode::GetAABB2TreeInVector(std::vector<AABB2TreeNode*>& container, int maxDepthStored, int& amountOfNodes) const
{
	int actualAmountOfNodes = 0;

	int currentNodeChecked = 0;

	int currentLevel = 0;
	container.push_back(m_convexPolyTreeRoot);
	int nodesInCurrentLevel = 1;
	int nodesCheckedInLevel = 0;
	int nextLevelNodes = 0;


	while (((currentLevel < maxDepthStored) || (maxDepthStored == -1)) && (currentNodeChecked < container.size())) {
		AABB2TreeNode* currentNode = container[currentNodeChecked];
		nodesCheckedInLevel++;

		if (nodesCheckedInLevel == nodesInCurrentLevel) {
			currentLevel++;
			nodesCheckedInLevel = 0;
			nodesInCurrentLevel = nextLevelNodes;
			nextLevelNodes = 0;
		}


		bool aboveMaxDepth = (maxDepthStored == -1) || (currentLevel < maxDepthStored);
		if (aboveMaxDepth && currentNode) {
			actualAmountOfNodes++;
			container.push_back(currentNode->m_firstChild);
			container.push_back(currentNode->m_secondChild);
			nextLevelNodes += 2;
		}

		currentNodeChecked++;
	}

	amountOfNodes = actualAmountOfNodes;
}

void RaycastVsConvexPoly2DMode::UpdateInput(float deltaSeconds)
{
	if (g_theConsole->GetMode() == DevConsoleMode::SHOWN) return;
	UNUSED(deltaSeconds);
	XboxController controller = g_theInput->GetController(0);

	Vec2 rayStartMoveVelocity = Vec2::ZERO;
	Vec2 rayEndMoveVelocity = Vec2::ZERO;
	m_angularVelocity = 0.0f;
	m_scaleSpeed = 1.0f;
	/*if (g_theInput->IsKeyDown('W') || g_theInput->IsKeyDown(KEYCODE_UPARROW)) {
		rayStartMoveVelocity = Vec2(0.0f, 1.0f);
	}

	if (g_theInput->IsKeyDown('A') || g_theInput->IsKeyDown(KEYCODE_LEFTARROW)) {
		rayStartMoveVelocity = Vec2(-1.0f, 0.0f);
	}

	if (g_theInput->IsKeyDown('S') || g_theInput->IsKeyDown(KEYCODE_DOWNARROW)) {
		rayStartMoveVelocity = Vec2(0.0f, -1.0f);
	}

	if (g_theInput->IsKeyDown('D') || g_theInput->IsKeyDown(KEYCODE_RIGHTARROW)) {
		rayStartMoveVelocity = Vec2(1.0f, 0.0f);
	}

	if (g_theInput->IsKeyDown('I') || g_theInput->IsKeyDown(KEYCODE_UPARROW)) {
		rayEndMoveVelocity = Vec2(0.0f, 1.0f);
	}

	if (g_theInput->IsKeyDown('J') || g_theInput->IsKeyDown(KEYCODE_LEFTARROW)) {
		rayEndMoveVelocity = Vec2(-1.0f, 0.0f);
	}

	if (g_theInput->IsKeyDown('K') || g_theInput->IsKeyDown(KEYCODE_DOWNARROW)) {
		rayEndMoveVelocity = Vec2(0.0f, -1.0f);
	}

	if (g_theInput->IsKeyDown('L') || g_theInput->IsKeyDown(KEYCODE_RIGHTARROW)) {
		rayEndMoveVelocity = Vec2(1.0f, 0.0f);
	}


	m_rayStart += rayStartMoveVelocity * m_dotSpeed * deltaSeconds;
	m_rayEnd += rayEndMoveVelocity * m_dotSpeed * deltaSeconds;*/

	AABB2 worldBoundingBox(Vec2::ZERO, m_worldSize);

	if (g_theInput->WasKeyJustPressed(KEYCODE_LEFT_MOUSE)) {
		m_isMovingShape = true;
		m_selectedShape = GetShapeUnderMouse();
		m_prevMousePosition = worldBoundingBox.GetPointAtUV(g_theWindow->GetNormalizedCursorPos());
	}

	if (g_theInput->WasKeyJustReleased(KEYCODE_LEFT_MOUSE)) {
		m_isMovingShape = false;
		m_selectedShape = nullptr;
	}


	if (g_theInput->IsKeyDown('S')) {
		m_rayStart = worldBoundingBox.GetPointAtUV(g_theWindow->GetNormalizedCursorPos());
	}

	if (g_theInput->IsKeyDown('E')) {
		m_rayEnd = worldBoundingBox.GetPointAtUV(g_theWindow->GetNormalizedCursorPos());
	}

	if (g_theInput->WasKeyJustPressed(188)) { // ,
		m_amountOfPolys *= 2;
		CreateReaminingConvexPoly2D();
		m_isTreeDirty = !m_useBVH;
	}

	if (g_theInput->WasKeyJustPressed(190)) { // ,
		if (m_amountOfPolys > 1) {
			m_amountOfPolys /= 2;
			m_halfPolygons = true;
			m_isTreeDirty = !m_useBVH;
		}
	}

	if (g_theInput->WasKeyJustPressed('N')) {
		m_numInvisibleRays *= 2;
		if (m_numInvisibleRays == 0) m_numInvisibleRays = 1;
	}

	if (g_theInput->WasKeyJustPressed('M')) {
		m_numInvisibleRays /= 2;
	}

	if (g_theInput->IsKeyDown('W')) { // Clockwise rotation
		m_angularVelocity += -m_rotationSpeed;
	}

	if (g_theInput->IsKeyDown('R')) {
		m_angularVelocity += m_rotationSpeed;
	}

	bool wasShapeChangingKeyPressed = g_theInput->WasKeyJustPressed('L');
	wasShapeChangingKeyPressed = wasShapeChangingKeyPressed || g_theInput->WasKeyJustPressed('K');
	wasShapeChangingKeyPressed = wasShapeChangingKeyPressed || g_theInput->WasKeyJustPressed('W');
	wasShapeChangingKeyPressed = wasShapeChangingKeyPressed || g_theInput->WasKeyJustPressed('R');

	bool wasShapeChangingKeyReleased = g_theInput->WasKeyJustReleased('L');
	wasShapeChangingKeyReleased = wasShapeChangingKeyReleased || g_theInput->WasKeyJustReleased('K');
	wasShapeChangingKeyReleased = wasShapeChangingKeyReleased || g_theInput->WasKeyJustReleased('W');
	wasShapeChangingKeyReleased = wasShapeChangingKeyReleased || g_theInput->WasKeyJustReleased('R');

	if (wasShapeChangingKeyPressed) {
		m_selectedShape = GetShapeUnderMouse();
		m_isChangingShape = true;
	}

	if (wasShapeChangingKeyReleased) {
		m_selectedShape = nullptr;
		m_isChangingShape = false;
	}

	if (g_theInput->IsKeyDown('L')) { // Clockwise rotation
		m_scaleSpeed = m_inflateSpeed;
	}

	if (g_theInput->IsKeyDown('K')) {
		m_scaleSpeed = m_deflateSpeed;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F2)) {
		m_useBVH = !m_useBVH;
	}

	if (g_theInput->WasKeyJustPressed('U')) {
		m_maxDepth++;
	}

	if (g_theInput->WasKeyJustPressed('I')) {
		m_maxDepth--;
		if (m_maxDepth < -1) m_maxDepth = -1;
	}

}

void RaycastVsConvexPoly2DMode::AddVertsForRaycastVsConvexPolys2D()
{
	m_normalColorShapes.clear();
	m_highlightedShapes.clear();
	m_raycastVsConvexPolyCollisionVerts.clear();

	Vec2 rayFwd = m_rayEnd - m_rayStart;

	float rayDistance = rayFwd.NormalizeAndGetPreviousLength();

	m_impactedShape = false;
	RaycastResult2D closestRaycast;
	closestRaycast.m_impactDist = ARBITRARILY_LARGE_VALUE;
	int closestImpactedShapeIndex = -1;



	ProfileLogScope raycastsProfile("Raycast Profile", false, &m_raycastTotalTime);
	m_raycastCounter++;
	for (int shapeIndex = 0; shapeIndex < m_allShapes.size(); shapeIndex++) {
		Shape2D* shape = m_allShapes[shapeIndex];
		ConvexPolyShape2D const* shapeAsConvexPoly = dynamic_cast<ConvexPolyShape2D*>(shape);
		if (!shape)continue;

		RaycastResult2D raycastVsDisc = RaycastVsDisc(m_rayStart, rayFwd, rayDistance, shapeAsConvexPoly->m_position, shapeAsConvexPoly->m_radius);
		if (!raycastVsDisc.m_didImpact) continue;

		RaycastResult2D raycastResult;
		raycastResult = RaycastVsConvexHull2D(m_rayStart, rayFwd, rayDistance, shapeAsConvexPoly->m_convexHull2D);


		if (raycastResult.m_didImpact) {
			if (raycastResult.m_impactDist < closestRaycast.m_impactDist) {
				closestRaycast = raycastResult;
				closestImpactedShapeIndex = shapeIndex;
			}
			m_impactedShape = true;
		}

	}

	m_raycastResult = closestRaycast;
	if (m_impactedShape) {
		AddVertsForRaycastImpactOnConvexPoly2D(*m_allShapes[closestImpactedShapeIndex], m_raycastResult);
	}
}

void RaycastVsConvexPoly2DMode::AddVertsForRaycastImpactOnConvexPoly2D(Shape2D& shape, RaycastResult2D& raycastResult)
{
	m_highlightedShapes.push_back(&shape);
	Vec2 const& impactPos = raycastResult.m_impactPos;
	Vec2 impactPosArrowEnd = impactPos + raycastResult.m_impactNormal * 5.0f;

	AddVertsForDisc2D(m_raycastVsConvexPolyCollisionVerts, impactPos, 0.5f, Rgba8::WHITE);
	AddVertsForArrow2D(m_raycastVsConvexPolyCollisionVerts, impactPos, impactPosArrowEnd, Rgba8::YELLOW, 0.5f, 1.0f);
}

void RaycastVsConvexPoly2DMode::RenderBVH() const
{
	g_theRenderer->DrawVertexArray(m_bvhDebugVerts);
}

void RaycastVsConvexPoly2DMode::UpdateInputFromKeyboard()
{
}

void RaycastVsConvexPoly2DMode::UpdateInputFromController()
{
}

