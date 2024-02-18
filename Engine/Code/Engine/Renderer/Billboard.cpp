#include "Engine/Renderer/Billboard.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Renderer/Texture.hpp"

Billboard::Billboard(Vec3 const& billBoardWorldPos, Texture* texture) :
	m_position(billBoardWorldPos),
	m_texture(texture)
{
}

Billboard::~Billboard()
{
}

void Billboard::Render(Renderer* renderer, Camera const& camera, BillboardType billboardType) const
{
	renderer->SetSamplerMode(SamplerMode::POINTCLAMP);
	renderer->SetRasterizerState(CullMode::NONE, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
	renderer->SetDepthStencilState(DepthFunc::ALWAYS, false);
	renderer->SetBlendMode(BlendMode::ALPHA);

	Mat44 billboardMatrix = GetModelMatrixForBillboard(m_position, camera, billboardType);

	renderer->SetModelMatrix(billboardMatrix);
	renderer->BindTexture(m_texture);
	renderer->DrawVertexArray(m_verts);

}

void Billboard::TransformVertsForBillboard(Vertex_PCU* verts, int vertsAmount, Vec3 const& position, Camera const& camera, BillboardType billboardType)
{
	switch (billboardType)
	{
	case BillboardType::CameraFacingXYZ:
		TransformVertsForBillboardCameraFacingXYZ(verts, vertsAmount, position, camera);
		break;

	}

}

Mat44 const Billboard::GetModelMatrixForBillboard(Vec3 const& position, Camera const& camera, BillboardType billboardType)
{
	switch (billboardType)
	{
	case BillboardType::CameraFacingXYZ:
		return GetModelMatrixForBillboardCameraFacingXYZ(position, camera);
	case BillboardType::CameraFacingXY:
		return GetModelMatrixForBillboardCameraFacingXY(position, camera);
	case BillboardType::CameraOpposingXYZ:
		return GetModelMatrixForBillboardCameraOpposingXYZ(camera);
	case BillboardType::CameraOpposingXY:
		return GetModelMatrixForBillboardCameraOpposingXY(camera);
	}
	return Mat44();
}


Mat44 const Billboard::GetModelMatrixForBillboardCameraOpposingXYZ(Camera const& camera)
{
	Vec3 iBasis = (camera.GetViewOrientation().GetMatrix_XFwd_YLeft_ZUp().GetIBasis3D());
	Mat44 modelMatrix = Mat44::CreateLookAtMatrix(iBasis);
	modelMatrix.Append(camera.GetRenderMatrix());

	return modelMatrix;
}

Mat44 const Billboard::GetModelMatrixForBillboardCameraOpposingXY(Camera const& camera)
{
	Vec3 iBasis = (camera.GetViewOrientation().GetMatrix_XFwd_YLeft_ZUp().GetIBasis3D());
	iBasis.z = 0.0f;
	Mat44 modelMatrix = Mat44::CreateLookAtMatrix(iBasis);
	modelMatrix.Append(camera.GetRenderMatrix());

	return modelMatrix;
}

void Billboard::TransformVertsForBillboardCameraFacingXYZ(Vertex_PCU* verts, int vertsAmount, Vec3 const& position, Camera const& camera)
{
	Mat44 modelMatrix = GetModelMatrixForBillboardCameraFacingXYZ(position, camera);
	TransformVertexArray3D(vertsAmount, verts, modelMatrix);
}

Mat44 const Billboard::GetModelMatrixForBillboardCameraFacingXYZ(Vec3 const& position, Camera const& camera)
{
	Vec3 iBasis = (camera.GetViewPosition() - position).GetNormalized();
	
	Mat44 modelMatrix = Mat44::CreateLookAtMatrix(iBasis);

	Mat44 coordinateSystemMatrix(Vec3(0.0f, 1.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f), Vec3(-1.0f, 0.0f, 0.0f), Vec3::ZERO);

	modelMatrix.Append(coordinateSystemMatrix);

	return modelMatrix;
}

Mat44 const Billboard::GetModelMatrixForBillboardCameraFacingXY(Vec3 const& position, Camera const& camera)
{
	Vec3 iBasis = (camera.GetViewPosition() - position).GetNormalized();

	iBasis.z = 0.0f;

	Mat44 modelMatrix = Mat44::CreateLookAtMatrix(iBasis);

	Mat44 coordinateSystemMatrix(Vec3(0.0f, 1.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f), Vec3(-1.0f, 0.0f, 0.0f), Vec3::ZERO);

	modelMatrix.Append(coordinateSystemMatrix);

	return modelMatrix;
}
