#pragma once
#include "Engine/Renderer/Camera.hpp"


class OrbitCamera : public Camera {

public:
	Vec3 GetFocalPoint() const { return m_focalPoint; }
	EulerAngles GetOrbitOrientation() const { return m_orbitOrientation; }
	float GetOrbitRadius() const { return m_radius; }

	void SetFocalPoint(Vec3 const& newFocalPoint);
	void SetOrbitOrientation(EulerAngles const& newOrientation);
	void SetOrbitRadius(float newRadius);

	void AddOrientation(EulerAngles const& orientationToAdd);


private:

	void RecomputePosition();

	Vec3 m_focalPoint = Vec3::ZERO;
	EulerAngles m_orbitOrientation = EulerAngles::ZERO;
	float m_radius = 0.0f;
};