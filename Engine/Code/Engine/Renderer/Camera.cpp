#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/Texture.hpp"


void Camera::SetColorTarget(Texture* color)
{
	m_colorTarget = color;
}

Texture* Camera::GetRenderTarget() const
{
	return m_colorTarget;
}

void Camera::SetDepthTarget(Texture* depth)
{
	m_depthTarget = depth;
}

Texture* Camera::GetDepthTarget() const
{
	return m_depthTarget;
}

void Camera::SetOrthoView(Vec2 const& leftPoint, Vec2 const& rightPoint)
{
	this->bottomLeft = leftPoint;
	this->topRight = rightPoint;
	m_mode = CameraMode::Orthographic;
}

void Camera::SetPerspectiveView(float aspect, float fov, float nearZ, float farZ)
{
	m_aspect = aspect;
	m_fov = fov;
	m_near = nearZ;
	m_far = farZ;

	m_mode = CameraMode::Perspective;
}

void Camera::SetCameraMode(CameraMode newCameraMode)
{
	m_mode = newCameraMode;
}

void Camera::SetTransform(Vec3 const& position, EulerAngles const& orientation)
{
	m_viewPosition = position;
	m_viewOrientation = orientation;
}

void Camera::SetPosition(Vec3 const& position)
{
	m_viewPosition = position;
}

void Camera::SetOrientation(EulerAngles const& orientation)
{
	m_viewOrientation = orientation;
}

void Camera::SetViewToRenderTransform(Vec3 const& iBasis, Vec3 const& jBasis, Vec3 const& kBasis)
{
	m_iBasis = iBasis;
	m_jBasis = jBasis;
	m_kBasis = kBasis;
}


Vec2 Camera::GetOrthoBottomLeft() const
{
	return this->bottomLeft;
}

Vec2 Camera::GetOrthoTopRight() const
{
	return this->topRight;
}

void Camera::TranslateCamera(float x, float y) {
	TranslateCamera(Vec2(x, y));
}
void Camera::TranslateCamera(Vec2 const& translationVec) {
	this->bottomLeft += translationVec;
	this->topRight += translationVec;
}

Mat44 Camera::GetProjectionMatrix() const
{
	Mat44 renderToGame = GetRenderMatrix();
	Mat44 gameToRender = renderToGame.GetOrthonormalInverse();

	Mat44 projectionMatrix; 

	switch (m_mode)
	{
	case CameraMode::Orthographic:
		projectionMatrix = GetOrthoMatrix();
		break;
	case CameraMode::Perspective:
		projectionMatrix = GetPerspectiveMatrix();
		break;
	}
	projectionMatrix.Append(gameToRender);

	return projectionMatrix;
}

Mat44 Camera::GetViewMatrix() const
{
	Mat44 viewMatrix;
	viewMatrix.SetTranslation3D(m_viewPosition);
	viewMatrix.Append(m_viewOrientation.GetMatrix_XFwd_YLeft_ZUp());

	return viewMatrix.GetOrthonormalInverse();
}

Mat44 Camera::GetRenderMatrix() const
{
	return Mat44(m_iBasis, m_jBasis, m_kBasis, Vec3::ZERO);
}

Mat44 Camera::GetOrthoMatrix() const
{
	return Mat44::CreateOrthoProjection(bottomLeft.x, topRight.x, bottomLeft.y, topRight.y, 0.0f, 1.0f);
}

Mat44 Camera::GetPerspectiveMatrix() const
{
	return Mat44::CreatePerspectiveProjection(m_fov, m_aspect, m_near, m_far);
}

