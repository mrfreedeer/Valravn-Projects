#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/EngineCommon.hpp"

EulerAngles const EulerAngles::ZERO = EulerAngles();

EulerAngles::EulerAngles(float yawDegres, float pitchDegrees, float rollDegrees):
	m_yawDegrees(yawDegres),
	m_pitchDegrees(pitchDegrees),
	m_rollDegrees(rollDegrees)
{
}

void EulerAngles::GetVectors_XFwd_YLeft_ZUp(Vec3& out_forwardIBasis, Vec3& out_leftJBasis, Vec3& out_upKBasis) const
{
	float cosYaw = CosDegrees(m_yawDegrees);
	float sinYaw = SinDegrees(m_yawDegrees);

	float cosPitch = CosDegrees(m_pitchDegrees);
	float sinPitch = SinDegrees(m_pitchDegrees);

	float cosRoll = CosDegrees(m_rollDegrees);
	float sinRoll = SinDegrees(m_rollDegrees);

	out_forwardIBasis.x = cosYaw * cosPitch;
	out_forwardIBasis.y = sinYaw * cosPitch;
	out_forwardIBasis.z = -sinPitch;

	out_leftJBasis.x = (-sinYaw * cosRoll) + (cosYaw * sinPitch * sinRoll);
	out_leftJBasis.y = (cosYaw * cosRoll) + (sinYaw * sinPitch * sinRoll);
	out_leftJBasis.z = cosPitch * sinRoll;

	out_upKBasis.x = (sinYaw * sinRoll) + (cosYaw * sinPitch * cosRoll);
	out_upKBasis.y = (- cosYaw * sinRoll) + (sinYaw * sinPitch * cosRoll);
	out_upKBasis.z = (cosPitch * cosRoll);

}



Mat44 EulerAngles::GetMatrix_XFwd_YLeft_ZUp() const
{
	float cosYaw = CosDegrees(m_yawDegrees);
	float sinYaw = SinDegrees(m_yawDegrees);

	float cosPitch = CosDegrees(m_pitchDegrees);
	float sinPitch = SinDegrees(m_pitchDegrees);

	float cosRoll = CosDegrees(m_rollDegrees);
	float sinRoll = SinDegrees(m_rollDegrees);

	float matrixValues[16] = { 0 };

	matrixValues[0] = cosYaw * cosPitch;
	matrixValues[1] = sinYaw * cosPitch;
	matrixValues[2] = -sinPitch;

	matrixValues[4] = (-sinYaw * cosRoll) + (cosYaw * sinPitch * sinRoll);
	matrixValues[5] = (cosYaw * cosRoll) + (sinYaw * sinPitch * sinRoll);
	matrixValues[6] = cosPitch * sinRoll;

	matrixValues[8] = (sinYaw * sinRoll) + (cosYaw * sinPitch * cosRoll);
	matrixValues[9] = (-cosYaw * sinRoll) + (sinYaw * sinPitch * cosRoll);
	matrixValues[10] = (cosPitch * cosRoll);

	matrixValues[15] = 1.0f;


	return Mat44(matrixValues);
}

Vec3 const EulerAngles::GetXForward() const
{
	float cosYaw = CosDegrees(m_yawDegrees);
	float sinYaw = SinDegrees(m_yawDegrees);

	float cosPitch = CosDegrees(m_pitchDegrees);
	float sinPitch = SinDegrees(m_pitchDegrees);

	Vec3 out_forwardIBasis = Vec3::ZERO;
	out_forwardIBasis.x = cosYaw * cosPitch;
	out_forwardIBasis.y = sinYaw * cosPitch;
	out_forwardIBasis.z = -sinPitch;

	return out_forwardIBasis.GetNormalized();
}

Vec3 const EulerAngles::GetYLeft() const
{
	float cosYaw = CosDegrees(m_yawDegrees);
	float sinYaw = SinDegrees(m_yawDegrees);

	float cosPitch = CosDegrees(m_pitchDegrees);
	float sinPitch = SinDegrees(m_pitchDegrees);

	float cosRoll = CosDegrees(m_rollDegrees);
	float sinRoll = SinDegrees(m_rollDegrees);

	Vec3 out_leftJBasis = {};
	out_leftJBasis.x = (-sinYaw * cosRoll) + (cosYaw * sinPitch * sinRoll);
	out_leftJBasis.y = (cosYaw * cosRoll) + (sinYaw * sinPitch * sinRoll);
	out_leftJBasis.z = cosPitch * sinRoll;

	return out_leftJBasis;
}

Vec3 const EulerAngles::GetZUp() const
{
	float cosYaw = CosDegrees(m_yawDegrees);
	float sinYaw = SinDegrees(m_yawDegrees);

	float cosPitch = CosDegrees(m_pitchDegrees);
	float sinPitch = SinDegrees(m_pitchDegrees);

	float cosRoll = CosDegrees(m_rollDegrees);
	float sinRoll = SinDegrees(m_rollDegrees);

	Vec3 out_upKBasis = Vec3::ZERO;
	out_upKBasis.x = (sinYaw * sinRoll) + (cosYaw * sinPitch * cosRoll);
	out_upKBasis.y = (-cosYaw * sinRoll) + (sinYaw * sinPitch * cosRoll);
	out_upKBasis.z = (cosPitch * cosRoll);

	return out_upKBasis.GetNormalized();
}

EulerAngles const EulerAngles::operator*(float multiplier) const {
	EulerAngles anglesToReturn = *this;
	anglesToReturn.m_rollDegrees *= multiplier;
	anglesToReturn.m_pitchDegrees *= multiplier;
	anglesToReturn.m_yawDegrees *= multiplier;

	return anglesToReturn;
}

bool EulerAngles::operator!=(EulerAngles const& otherAngles) const {
	bool diffYaw = (otherAngles.m_yawDegrees != m_yawDegrees);
	bool diffPitch = (otherAngles.m_pitchDegrees != m_pitchDegrees);
	bool diffRoll = (otherAngles.m_rollDegrees != m_rollDegrees);

	return (diffYaw) || (diffPitch) || (diffRoll);
}

EulerAngles const EulerAngles::operator-(EulerAngles const& otherAngles) const {
	EulerAngles anglesToReturn = *this;
	anglesToReturn.m_rollDegrees -= otherAngles.m_rollDegrees;
	anglesToReturn.m_pitchDegrees -= otherAngles.m_pitchDegrees;
	anglesToReturn.m_yawDegrees -= otherAngles.m_yawDegrees;
	return anglesToReturn;
}

void EulerAngles::operator+=(EulerAngles const& otherAngles) {
	m_rollDegrees += otherAngles.m_rollDegrees;
	m_pitchDegrees += otherAngles.m_pitchDegrees;
	m_yawDegrees += otherAngles.m_yawDegrees;
}

void EulerAngles::operator*=(float multiplier) {
	m_rollDegrees *=  multiplier;
	m_pitchDegrees *= multiplier;
	m_yawDegrees *= multiplier;
}

void EulerAngles::SetFromText(const char* text)
{
	Strings angleInfo = SplitStringOnDelimiter(text, ',');

	if (angleInfo.size() == 3) {
		m_yawDegrees = static_cast<float>(std::atof(angleInfo[0].c_str()));
		m_pitchDegrees = static_cast<float>(std::atof(angleInfo[1].c_str()));
		m_rollDegrees = static_cast<float>(std::atof(angleInfo[2].c_str()));
		
		return;
	}


	ERROR_AND_DIE(Stringf("EULERANGLEs SETSTRING VECTOR TOO LONG: %s", text));
}

std::string const EulerAngles::ToString() const
{
	return Stringf("(%.2f, %.2f, %.2f)", m_yawDegrees, m_pitchDegrees, m_rollDegrees);
}

EulerAngles EulerAngles::CreateEulerAngleFromForward(Vec3 const& forward)
{
	EulerAngles eulerAngles;

	eulerAngles.m_yawDegrees = forward.GetAngleAboutZDegrees();
	eulerAngles.m_pitchDegrees = forward.GetAngleAboutYDegrees();
	
	return eulerAngles;
}
