#pragma once
#include "Engine/Math/RaycastUtils.hpp"
#include "Game/Gameplay/GameMode.hpp"

class ConvexPolyShape2D;
struct AABB2TreeNode;
class BufferParser;

class RaycastVsConvexPoly2DMode : public GameMode {
public:
	RaycastVsConvexPoly2DMode(Game* pointerToGame, Vec2 const& cameraSize, Vec2 const& UICameraSize);
	~RaycastVsConvexPoly2DMode();

	virtual void Startup() override;
	virtual void Shutdown() override;
	virtual void Update(float deltaSeconds) override;
	virtual void Render() const override;

	static bool SaveGHCSScene(EventArgs& args);
	static bool LoadGHCSScene(EventArgs& args);

private:
	void CreateAABB2Tree();
	void DeleteAABB2Tree();
	void CreateReaminingConvexPoly2D();
	void CreateVertexBuffer();
	void UpdateSelectedPoly(float deltaSeconds);
	void HalveCurrentConvexPoly2D();
	void RaycastVsAABB2Tree(Vec2 const& rayStart, Vec2 const& rayEnd,bool addVertsForImpact = true);
	void GetAABB2TreeInVector(std::vector<AABB2TreeNode*>& container, int maxDepthStored, int& amountOfNodes) const;

	virtual void UpdateInput(float deltaSeconds) override;
	void AddVertsForRaycastVsConvexPolys2D();
	void AddVertsForRaycastImpactOnConvexPoly2D(Shape2D& shape, RaycastResult2D& raycastResult);
	void RenderBVH() const;

	virtual void UpdateInputFromKeyboard();
	virtual void UpdateInputFromController();

	void RenderNormalColorShapes() const;
	void RenderHighlightedColorShapes() const;
	void DoInvisibleRaycasts();

	ConvexPolyShape2D* GetShapeUnderMouse() const;

	void ParseToc(unsigned int tocStart);
	void ParseSceneChunk(BufferParser& bufferParser);
	void ParseConvexPolyChunk(BufferParser& bufferParser);
	void ParseAABB2TreeChunk(BufferParser& bufferParser);
	void ParseBoundingDiscsChunk(BufferParser& bufferParser);
	void ParseChunk(BufferParser& bufferParser);
	void LoadSceneFromFile(std::filesystem::path fileName);


	void WriteBoundingDiscsChunkToBuffer(std::vector<unsigned char>& fileBuffer, unsigned char endianness) const;
	void WriteAABB2TreeChunkToBuffer(std::vector<unsigned char>& fileBuffer, unsigned char endianness) const;
	void WriteSceneChunkToBuffer(std::vector<unsigned char>& fileBuffer, unsigned char endianness) const;
	void AppendToc(std::vector<unsigned char>& fileBuffer) const;
	void CreateOrUpdateTocEntry(unsigned char chunkType, unsigned int totalSize, unsigned int headerStart) const;
	void WriteConvexPolyChunkToBuffer(std::vector<unsigned char>& fileBuffer, unsigned char chunkEndianness) const;
	void CopyChunkAsIs(std::vector<unsigned char>& saveBuffer, unsigned int headerStart, unsigned int totalChunkSize) const;
	void RewriteScene(std::vector<unsigned char>& saveBuffer, unsigned char chunkType, unsigned int headerStart, unsigned int totalChunkSize) const;
	void SaveSceneToFile(std::filesystem::path fileName) const;


private:
	std::vector<Shape2D*> m_allShapes;
	std::vector<Shape2D*> m_allConvexPolys;
	std::vector<Shape2D*> m_highlightedShapes;
	std::vector<Shape2D*> m_normalColorShapes;
	std::vector<Shape2D*> m_hitAABB2s;
	std::vector<Vertex_PCU> m_raycastVsConvexPolyCollisionVerts;
	std::vector<Vertex_PCU> m_shapeDebugDiscs;

	std::vector<unsigned char> m_currentScene;

	std::string m_baseHelperText = "";

	Vec2 m_rayStart = Vec2::ZERO;
	Vec2 m_rayEnd = Vec2::ZERO;

	RaycastResult2D m_raycastResult;
	bool m_impactedShape = false;
	float m_dotSpeed = g_gameConfigBlackboard.GetValue("DOT_SPEED", 40.0f);

	int m_amountOfPolys = g_gameConfigBlackboard.GetValue("CONVEX_POLY_INITIAL_AMOUNT", 16);
	int m_existingPolys = 0;
	bool m_halfPolygons = false;
	int m_numInvisibleRays = g_gameConfigBlackboard.GetValue("CONVEX_INVISIBLE_RAYS_AMOUNT", 1024);
	float m_rotationSpeed = g_gameConfigBlackboard.GetValue("CONVEX_ROTATION_SPEED", 15.0f);
	float m_angularVelocity = 0.0f;

	float m_inflateSpeed = g_gameConfigBlackboard.GetValue("CONVEX_INFLATE_SPEED", 1.0f);
	float m_deflateSpeed = g_gameConfigBlackboard.GetValue("CONVEX_DEFLATE_SPEED", -0.5f);
	float m_scaleSpeed = 1.0f;

	VertexBuffer* m_shapesBuffer = nullptr;
	bool m_isMovingShape = false;
	bool m_isChangingShape = false;
	bool m_didAnyShapeChange = false;
	Vec2 m_prevMousePosition = Vec2::ZERO;
	ConvexPolyShape2D* m_selectedShape = nullptr;
	AABB2TreeNode* m_convexPolyTreeRoot = nullptr;
	std::vector<Vertex_PCU> m_bvhDebugVerts;
	bool m_useBVH = false;
	uint64_t m_raycastTotalTime =  0;
	int m_raycastCounter = 0;
	int m_maxDepth = 1;
	bool m_isTreeDirty = true;
	bool m_rewritingExistingScene = false;
	unsigned char m_currentFileEndianness = 0;
};