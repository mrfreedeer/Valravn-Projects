#pragma once
#include "Engine/Math/Vec3.hpp"
#include "Engine/Renderer/Camera.hpp"
#include <vector>

struct Vertex_PCU;
struct Mat44;

enum class BillboardType
{
	CameraFacingXY,
	CameraFacingXYZ,
	CameraOpposingXY,
	CameraOpposingXYZ
};

class Texture;
class Renderer;

class Billboard {

public:
	Billboard(Vec3 const& billBoardWorldPos, Texture* texture = nullptr);
	~Billboard();
	void Render(Renderer* renderer, Camera const& camera, BillboardType billboardType) const;

	static void TransformVertsForBillboard(Vertex_PCU* verts, int vertsAmount, Vec3 const& position, Camera const& camera, BillboardType billboardType);
	static Mat44 const GetModelMatrixForBillboard(Vec3 const& position, Camera const& camera, BillboardType billboardType);
	static Mat44 const GetModelMatrixForBillboardCameraOpposingXYZ(Camera const& camera);
	static Mat44 const GetModelMatrixForBillboardCameraOpposingXY(Camera const& camera);
	static Mat44 const GetModelMatrixForBillboardCameraFacingXYZ(Vec3 const& position, Camera const& camera);
	static Mat44 const GetModelMatrixForBillboardCameraFacingXY(Vec3 const& position, Camera const& camera);
	static void TransformVertsForBillboardCameraFacingXYZ(Vertex_PCU* verts, int vertsAmount, Vec3 const& position, Camera const& camera);

public:
	std::vector<Vertex_PCU> m_verts;

private:
	Vec3 m_position = Vec3::ZERO;
	Texture* m_texture;
};