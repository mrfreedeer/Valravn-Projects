#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/AABB2.hpp"


class Texture;
enum class CameraMode {
	Orthographic = 1,
	Perspective,
	NUM_CAMERA_MODES
};

class Camera {

public:


	void SetColorTarget(Texture* color);
	Texture* GetRenderTarget() const;

	void SetDepthTarget(Texture* depth);
	Texture* GetDepthTarget() const;

	void SetOrthoView(Vec2 const& bottomLeft, Vec2 const& topRight);
	void SetPerspectiveView(float aspect, float fov, float nearZ, float farZ);
	void SetCameraMode(CameraMode newCameraMode);
	void SetTransform(Vec3 const& position, EulerAngles const& orientation);
	void SetPosition(Vec3 const& position);
	void SetOrientation(EulerAngles const& orientation);
	void SetViewToRenderTransform(Vec3 const& iBasis, Vec3 const& jBasis, Vec3 const& kBasis);
	void SetFov(float degrees) { m_fov = degrees; }
	void SetViewport(AABB2 const& newViewport) { m_viewPort = newViewport; }

	float GetFovDegrees() const { return m_fov; }
	Vec2 GetOrthoBottomLeft() const;
	Vec2 GetOrthoTopRight() const;
	void TranslateCamera(float x, float y);
	void TranslateCamera(Vec2 const& translationVec);

	CameraMode GetCameraMode() const { return m_mode; }
	Mat44 GetProjectionMatrix() const;
	Mat44 GetViewMatrix() const;
	Mat44 GetRenderMatrix() const;
	Vec3 const GetViewPosition() const { return m_viewPosition; }
	EulerAngles const GetViewOrientation() const { return m_viewOrientation; }

	AABB2 GetViewport() const { return m_viewPort; }
	AABB2 const GetCameraBounds() const { return AABB2(bottomLeft, topRight); }
	float GetAspect() const { return m_aspect; }
	float GetNear() const { return m_near; }
	float GetFar() const { return m_far; }

protected:
	Mat44 GetOrthoMatrix() const;
	Mat44 GetPerspectiveMatrix() const;
protected:
	Vec2 bottomLeft;
	Vec2 topRight;

	float m_aspect = 0.0f;
	float m_fov = 0.0f;
	float m_near = 0.0f;
	float m_far = 0.0f;

	CameraMode m_mode = CameraMode::Orthographic;

	Vec3 m_viewPosition = Vec3::ZERO;
	EulerAngles m_viewOrientation = {};

	Vec3 m_iBasis = Vec3(1.0f, 0.0f, 0.0f);
	Vec3 m_jBasis = Vec3(0.0f, 1.0f, 0.0f);
	Vec3 m_kBasis = Vec3(0.0f, 0.0f, 1.0f);

	AABB2 m_viewPort = AABB2::ZERO_TO_ONE;


	Texture* m_colorTarget = nullptr;
	Texture* m_depthTarget = nullptr;
};