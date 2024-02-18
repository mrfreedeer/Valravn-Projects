#include "Engine/Renderer/OrbitCamera.hpp"
#include "Engine/Core/EngineCommon.hpp"



void OrbitCamera::SetFocalPoint(Vec3 const& newFocalPoint)
{
	m_focalPoint = newFocalPoint;

	RecomputePosition();

}

void OrbitCamera::SetOrbitOrientation(EulerAngles const& newOrientation)
{
	m_orbitOrientation = newOrientation;

	Vec3 displacementDistance = -m_orbitOrientation.GetXForward() * m_radius;
	Vec3 newPosition = m_focalPoint + displacementDistance;

	SetTransform(newPosition, newOrientation);
}

void OrbitCamera::SetOrbitRadius(float newRadius)
{
	m_radius = newRadius;

	RecomputePosition();
}


void OrbitCamera::AddOrientation(EulerAngles const& orientationToAdd)
{
	m_orbitOrientation += orientationToAdd;
	m_orbitOrientation.m_pitchDegrees = Clamp(m_orbitOrientation.m_pitchDegrees, -80.0f, 80.0f);

	RecomputePosition();
}

void OrbitCamera::RecomputePosition()
{
	Vec3 displacementDistance = -m_orbitOrientation.GetXForward() * m_radius;
	Vec3 newPosition = m_focalPoint + displacementDistance;
	SetPosition(newPosition);

	Vec3 dirToFocalPoint = (m_focalPoint - m_viewPosition).GetNormalized();

	SetOrientation(EulerAngles::CreateEulerAngleFromForward(dirToFocalPoint));

}
