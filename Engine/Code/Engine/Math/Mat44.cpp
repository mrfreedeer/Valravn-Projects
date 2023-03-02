#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Vec4.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/EngineCommon.hpp"

Mat44::Mat44()
{
	m_values[Ix] = 1;
	m_values[Jy] = 1;
	m_values[Kz] = 1;
	m_values[Tw] = 1;
}

Mat44::Mat44(Vec2 const& iBasis2D, Vec2 const& jBasis2D, Vec2 const& translation2D)
{
	m_values[Kz] = 1;
	m_values[Tw] = 1;

	m_values[Ix] = iBasis2D.x;
	m_values[Iy] = iBasis2D.y;

	m_values[Jx] = jBasis2D.x;
	m_values[Jy] = jBasis2D.y;

	m_values[Tx] = translation2D.x;
	m_values[Ty] = translation2D.y;
}

Mat44::Mat44(Vec3 const& iBasis3D, Vec3 const& jBasis3D, Vec3 const& kBasis3D, Vec3 const& translation3D)
{
	m_values[Tw] = 1;


	m_values[Ix] = iBasis3D.x;
	m_values[Iy] = iBasis3D.y;
	m_values[Iz] = iBasis3D.z;

	m_values[Jx] = jBasis3D.x;
	m_values[Jy] = jBasis3D.y;
	m_values[Jz] = jBasis3D.z;

	m_values[Kx] = kBasis3D.x;
	m_values[Ky] = kBasis3D.y;
	m_values[Kz] = kBasis3D.z;


	m_values[Tx] = translation3D.x;
	m_values[Ty] = translation3D.y;
	m_values[Tz] = translation3D.z;
}

Mat44::Mat44(Vec4 const& iBasis3D, Vec4 const& jBasis3D, Vec4 const& kBasis3D, Vec4 const& translation3D)
{
	m_values[Ix] = iBasis3D.x;
	m_values[Iy] = iBasis3D.y;
	m_values[Iz] = iBasis3D.z;
	m_values[Iw] = iBasis3D.w;

	m_values[Jx] = jBasis3D.x;
	m_values[Jy] = jBasis3D.y;
	m_values[Jz] = jBasis3D.z;
	m_values[Jw] = jBasis3D.w;

	m_values[Kx] = kBasis3D.x;
	m_values[Ky] = kBasis3D.y;
	m_values[Kz] = kBasis3D.z;
	m_values[Kw] = kBasis3D.w;

	m_values[Tx] = translation3D.x;
	m_values[Ty] = translation3D.y;
	m_values[Tz] = translation3D.z;
	m_values[Tw] = translation3D.w;
}

Mat44::Mat44(float const* sixteenValuesBasisMajor)
{
	for (int valueIndex = 0; valueIndex < 16; valueIndex++) {
		m_values[valueIndex] = sixteenValuesBasisMajor[valueIndex];
	}
}

Mat44 const Mat44::CreateTranslation2D(Vec2 const& translationXY)
{
	Mat44 newMat;

	newMat.m_values[Tx] = translationXY.x;
	newMat.m_values[Ty] = translationXY.y;

	return newMat;
}

Mat44 const Mat44::CreateTranslation3D(Vec3 const& translationXY)
{
	Mat44 newMat;

	newMat.m_values[Tx] = translationXY.x;
	newMat.m_values[Ty] = translationXY.y;
	newMat.m_values[Tz] = translationXY.z;

	return newMat;
}

Mat44 const Mat44::CreateUniformScale2D(float uniformScaleXY)
{
	Mat44 newMat;

	newMat.m_values[Ix] = uniformScaleXY;
	newMat.m_values[Jy] = uniformScaleXY;

	return newMat;
}

Mat44 const Mat44::CreateUniformScale3D(float uniformScaleXYZ)
{
	Mat44 newMat;

	newMat.m_values[Ix] = uniformScaleXYZ;
	newMat.m_values[Jy] = uniformScaleXYZ;
	newMat.m_values[Kz] = uniformScaleXYZ;

	return newMat;
}

Mat44 const Mat44::CreateNonUniformScale2D(Vec2 const& nonUniformScaleXY)
{
	Mat44 newMat;

	newMat.m_values[Ix] = nonUniformScaleXY.x;
	newMat.m_values[Jy] = nonUniformScaleXY.y;

	return newMat;
}

Mat44 const Mat44::CreateNonUniformScale3D(Vec3 const& nonUniformScaleXYZ)
{
	Mat44 newMat;

	newMat.m_values[Ix] = nonUniformScaleXYZ.x;
	newMat.m_values[Jy] = nonUniformScaleXYZ.y;
	newMat.m_values[Kz] = nonUniformScaleXYZ.z;

	return newMat;
}

Mat44 const Mat44::CreateZRotationDegrees(float rotationDegreesAboutZ)
{
	Mat44 newMat;

	newMat.m_values[Ix] = CosDegrees(rotationDegreesAboutZ);
	newMat.m_values[Iy] = SinDegrees(rotationDegreesAboutZ);

	newMat.m_values[Jx] = -SinDegrees(rotationDegreesAboutZ);
	newMat.m_values[Jy] = CosDegrees(rotationDegreesAboutZ);

	return newMat;
}

Mat44 const Mat44::CreateYRotationDegrees(float rotationDegreesAboutY)
{
	Mat44 newMat;

	newMat.m_values[Ix] = CosDegrees(rotationDegreesAboutY);
	newMat.m_values[Iz] = -SinDegrees(rotationDegreesAboutY);

	newMat.m_values[Kx] = SinDegrees(rotationDegreesAboutY);
	newMat.m_values[Kz] = CosDegrees(rotationDegreesAboutY);

	return newMat;
}

Mat44 const Mat44::CreateXRotationDegrees(float rotationDegreesAboutX)
{
	Mat44 newMat;

	newMat.m_values[Jy] = CosDegrees(rotationDegreesAboutX);
	newMat.m_values[Jz] = SinDegrees(rotationDegreesAboutX);

	newMat.m_values[Ky] = -SinDegrees(rotationDegreesAboutX);
	newMat.m_values[Kz] = CosDegrees(rotationDegreesAboutX);

	return newMat;
}

Mat44 const Mat44::CreateOrthoProjection(float left, float right, float bottom, float top, float zNear, float zFar)
{
	float xDiff = 1 / (right - left);
	float orthoIx = 2 * xDiff;
	float orthoTx = (right + left) * (-xDiff);

	float yDiff = 1 / (top - bottom);
	float orthoJy = 2 * yDiff;
	float orthoTy = (top + bottom) * (-yDiff);


	float zDiff = 1 / (zFar - zNear);
	float orthoKz = 1 * zDiff;
	float orthoTz = (zNear) * (-zDiff);

	Mat44 orthoProjection;

	orthoProjection.m_values[Ix] = orthoIx;
	orthoProjection.m_values[Tx] = orthoTx;

	orthoProjection.m_values[Jy] = orthoJy;
	orthoProjection.m_values[Ty] = orthoTy;

	orthoProjection.m_values[Kz] = orthoKz;
	orthoProjection.m_values[Tz] = orthoTz;

	return orthoProjection;
}

Mat44 const Mat44::CreatePerspectiveProjection(float fovYDegrees, float aspect, float zNear, float zFar)
{
	Mat44 perspectiveProj;
	float angle = fovYDegrees * 0.5f;

	float sy = CosDegrees(angle) / SinDegrees(angle);
	float sx = sy / aspect;
	float sz = zFar / (zFar - zNear);
	float tz = (zNear * zFar) / (zNear - zFar);

	perspectiveProj.m_values[Ix] = sx;
	perspectiveProj.m_values[Jy] = sy;
	perspectiveProj.m_values[Kz] = sz;
	perspectiveProj.m_values[Kw] = 1;
	perspectiveProj.m_values[Tz] = tz;
	perspectiveProj.m_values[Tw] = 0;

	return perspectiveProj;
}

Mat44 const Mat44::CreateLookAtMatrix(Vec3 const& forward)
{
	Vec3 worldKBasis(0.0f, 0.0f, 1.0f);
	Vec3 worldJBasis(0.0f, 1.0f, 0.0f);

	Vec3 kBasis, jBasis;

	if (DotProduct3D(worldKBasis, forward) < 1.0f) {
		jBasis = CrossProduct3D(worldKBasis, forward).GetNormalized();
		kBasis = CrossProduct3D(forward, jBasis).GetNormalized();
	}
	else {
		kBasis = CrossProduct3D(forward, worldJBasis).GetNormalized();
		jBasis = CrossProduct3D(kBasis, forward).GetNormalized();
	}

	Mat44 modelMatrix;
	modelMatrix.SetIJK3D(forward, jBasis, kBasis);

	return modelMatrix;
}

Vec2 const Mat44::TransformVectorQuantity2D(Vec2 const& vectorQuantityXY) const
{
	float transformedX = m_values[Ix] * vectorQuantityXY.x + m_values[Jx] * vectorQuantityXY.y;
	float transformedY = m_values[Iy] * vectorQuantityXY.x + m_values[Jy] * vectorQuantityXY.y;
	return Vec2(transformedX, transformedY);
}

Vec3 const Mat44::TransformVectorQuantity3D(Vec3 const& vectorQuantityXYZ) const
{
	float transformedX = m_values[Ix] * vectorQuantityXYZ.x + m_values[Jx] * vectorQuantityXYZ.y + m_values[Kx] * vectorQuantityXYZ.z;
	float transformedY = m_values[Iy] * vectorQuantityXYZ.x + m_values[Jy] * vectorQuantityXYZ.y + m_values[Ky] * vectorQuantityXYZ.z;
	float transformedZ = m_values[Iz] * vectorQuantityXYZ.x + m_values[Jz] * vectorQuantityXYZ.y + m_values[Kz] * vectorQuantityXYZ.z;
	return Vec3(transformedX, transformedY, transformedZ);
}

Vec2 const Mat44::TransformPosition2D(Vec2 const& positionXY) const
{
	float transformedX = m_values[Ix] * positionXY.x + m_values[Jx] * positionXY.y + m_values[Tx];
	float transformedY = m_values[Iy] * positionXY.x + m_values[Jy] * positionXY.y + m_values[Ty];
	return Vec2(transformedX, transformedY);
}

Vec3 const Mat44::TransformPosition3D(Vec3 const& position3D) const
{
	float transformedX = m_values[Ix] * position3D.x + m_values[Jx] * position3D.y + m_values[Kx] * position3D.z + m_values[Tx];
	float transformedY = m_values[Iy] * position3D.x + m_values[Jy] * position3D.y + m_values[Ky] * position3D.z + m_values[Ty];
	float transformedZ = m_values[Iz] * position3D.x + m_values[Jz] * position3D.y + m_values[Kz] * position3D.z + m_values[Tz];
	return Vec3(transformedX, transformedY, transformedZ);
}

Vec4 const Mat44::TransformHomogeneous3D(Vec4 const& homogeneousPoint3D) const
{
	float transformedX = m_values[Ix] * homogeneousPoint3D.x + m_values[Jx] * homogeneousPoint3D.y + m_values[Kx] * homogeneousPoint3D.z + m_values[Tx] * homogeneousPoint3D.w;
	float transformedY = m_values[Iy] * homogeneousPoint3D.x + m_values[Jy] * homogeneousPoint3D.y + m_values[Ky] * homogeneousPoint3D.z + m_values[Ty] * homogeneousPoint3D.w;
	float transformedZ = m_values[Iz] * homogeneousPoint3D.x + m_values[Jz] * homogeneousPoint3D.y + m_values[Kz] * homogeneousPoint3D.z + m_values[Tz] * homogeneousPoint3D.w;
	float transformedW = m_values[Iw] * homogeneousPoint3D.x + m_values[Jw] * homogeneousPoint3D.y + m_values[Kw] * homogeneousPoint3D.z + m_values[Tw] * homogeneousPoint3D.w;
	return Vec4(transformedX, transformedY, transformedZ, transformedW);
}

Vec3 const Mat44::RightAppendVectorQuantity3D(Vec3 const& vectorQuantityXYZ)
{
	float x = m_values[Ix] * vectorQuantityXYZ.x + m_values[Jx] * vectorQuantityXYZ.y + m_values[Kx] * vectorQuantityXYZ.z;
	float y = m_values[Iy] * vectorQuantityXYZ.x + m_values[Jy] * vectorQuantityXYZ.y + m_values[Ky] * vectorQuantityXYZ.z;
	float z = m_values[Iz] * vectorQuantityXYZ.x + m_values[Jz] * vectorQuantityXYZ.y + m_values[Kz] * vectorQuantityXYZ.z;
	return Vec3(x, y, z);
}

float* Mat44::GetAsFloatArray()
{
	return m_values;
}

float const* Mat44::GetAsFloatArray() const
{
	return m_values;
}

Vec2 const Mat44::GetIBasis2D() const
{
	return Vec2(m_values[Ix], m_values[Iy]);
}

Vec2 const Mat44::GetJBasis2D() const
{
	return Vec2(m_values[Jx], m_values[Jy]);
}

Vec2 const Mat44::GetTranslation2D() const
{
	return Vec2(m_values[Tx], m_values[Ty]);
}

Vec3 const Mat44::GetIBasis3D() const
{
	return Vec3(m_values[Ix], m_values[Iy], m_values[Iz]);
}

Vec3 const Mat44::GetJBasis3D() const
{
	return Vec3(m_values[Jx], m_values[Jy], m_values[Jz]);
}

Vec3 const Mat44::GetKBasis3D() const
{
	return Vec3(m_values[Kx], m_values[Ky], m_values[Kz]);
}

Vec3 const Mat44::GetTranslation3D() const
{
	return Vec3(m_values[Tx], m_values[Ty], m_values[Tz]);
}

Vec4 const Mat44::GetIBasis4D() const
{
	return Vec4(m_values[Ix], m_values[Iy], m_values[Iz], m_values[Iw]);
}

Vec4 const Mat44::GetJBasis4D() const
{
	return Vec4(m_values[Jx], m_values[Jy], m_values[Jz], m_values[Jw]);

}

Vec4 const Mat44::GetKBasis4D() const
{
	return Vec4(m_values[Kx], m_values[Ky], m_values[Kz], m_values[Kw]);
}

Vec4 const Mat44::GetTranslation4D() const
{
	return Vec4(m_values[Tx], m_values[Ty], m_values[Tz], m_values[Tw]);

}

Mat44 const Mat44::GetInverted() const
{
	double inv[16];
	double det;
	double m[16];
	unsigned int i;

	for (i = 0; i < 16; ++i) {
		m[i] = (double)m_values[i];
	}

	inv[0] = m[5] * m[10] * m[15] -
		m[5] * m[11] * m[14] -
		m[9] * m[6] * m[15] +
		m[9] * m[7] * m[14] +
		m[13] * m[6] * m[11] -
		m[13] * m[7] * m[10];

	inv[4] = -m[4] * m[10] * m[15] +
		m[4] * m[11] * m[14] +
		m[8] * m[6] * m[15] -
		m[8] * m[7] * m[14] -
		m[12] * m[6] * m[11] +
		m[12] * m[7] * m[10];

	inv[8] = m[4] * m[9] * m[15] -
		m[4] * m[11] * m[13] -
		m[8] * m[5] * m[15] +
		m[8] * m[7] * m[13] +
		m[12] * m[5] * m[11] -
		m[12] * m[7] * m[9];

	inv[12] = -m[4] * m[9] * m[14] +
		m[4] * m[10] * m[13] +
		m[8] * m[5] * m[14] -
		m[8] * m[6] * m[13] -
		m[12] * m[5] * m[10] +
		m[12] * m[6] * m[9];

	inv[1] = -m[1] * m[10] * m[15] +
		m[1] * m[11] * m[14] +
		m[9] * m[2] * m[15] -
		m[9] * m[3] * m[14] -
		m[13] * m[2] * m[11] +
		m[13] * m[3] * m[10];

	inv[5] = m[0] * m[10] * m[15] -
		m[0] * m[11] * m[14] -
		m[8] * m[2] * m[15] +
		m[8] * m[3] * m[14] +
		m[12] * m[2] * m[11] -
		m[12] * m[3] * m[10];

	inv[9] = -m[0] * m[9] * m[15] +
		m[0] * m[11] * m[13] +
		m[8] * m[1] * m[15] -
		m[8] * m[3] * m[13] -
		m[12] * m[1] * m[11] +
		m[12] * m[3] * m[9];

	inv[13] = m[0] * m[9] * m[14] -
		m[0] * m[10] * m[13] -
		m[8] * m[1] * m[14] +
		m[8] * m[2] * m[13] +
		m[12] * m[1] * m[10] -
		m[12] * m[2] * m[9];

	inv[2] = m[1] * m[6] * m[15] -
		m[1] * m[7] * m[14] -
		m[5] * m[2] * m[15] +
		m[5] * m[3] * m[14] +
		m[13] * m[2] * m[7] -
		m[13] * m[3] * m[6];

	inv[6] = -m[0] * m[6] * m[15] +
		m[0] * m[7] * m[14] +
		m[4] * m[2] * m[15] -
		m[4] * m[3] * m[14] -
		m[12] * m[2] * m[7] +
		m[12] * m[3] * m[6];

	inv[10] = m[0] * m[5] * m[15] -
		m[0] * m[7] * m[13] -
		m[4] * m[1] * m[15] +
		m[4] * m[3] * m[13] +
		m[12] * m[1] * m[7] -
		m[12] * m[3] * m[5];

	inv[14] = -m[0] * m[5] * m[14] +
		m[0] * m[6] * m[13] +
		m[4] * m[1] * m[14] -
		m[4] * m[2] * m[13] -
		m[12] * m[1] * m[6] +
		m[12] * m[2] * m[5];

	inv[3] = -m[1] * m[6] * m[11] +
		m[1] * m[7] * m[10] +
		m[5] * m[2] * m[11] -
		m[5] * m[3] * m[10] -
		m[9] * m[2] * m[7] +
		m[9] * m[3] * m[6];

	inv[7] = m[0] * m[6] * m[11] -
		m[0] * m[7] * m[10] -
		m[4] * m[2] * m[11] +
		m[4] * m[3] * m[10] +
		m[8] * m[2] * m[7] -
		m[8] * m[3] * m[6];

	inv[11] = -m[0] * m[5] * m[11] +
		m[0] * m[7] * m[9] +
		m[4] * m[1] * m[11] -
		m[4] * m[3] * m[9] -
		m[8] * m[1] * m[7] +
		m[8] * m[3] * m[5];

	inv[15] = m[0] * m[5] * m[10] -
		m[0] * m[6] * m[9] -
		m[4] * m[1] * m[10] +
		m[4] * m[2] * m[9] +
		m[8] * m[1] * m[6] -
		m[8] * m[2] * m[5];

	det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
	det = 1.0 / det;

	Mat44 ret;
	for (i = 0; i < 16; i++) {
		ret.m_values[i] = (float)(inv[i] * det);
	}

	return ret;
}

void Mat44::SetTranslation2D(Vec2 const& translationXY)
{
	m_values[Tx] = translationXY.x;
	m_values[Ty] = translationXY.y;
	m_values[Tz] = 0;
	m_values[Tw] = 1;
}

void Mat44::SetTranslation3D(Vec3 const& translationXYZ)
{
	m_values[Tx] = translationXYZ.x;
	m_values[Ty] = translationXYZ.y;
	m_values[Tz] = translationXYZ.z;
	m_values[Tw] = 1;
}

void Mat44::SetIJ2D(Vec2 const& iBasis2D, Vec2 const& jBasis2D)
{
	m_values[Ix] = iBasis2D.x;
	m_values[Iy] = iBasis2D.y;
	m_values[Iz] = 0;
	m_values[Iw] = 0;

	m_values[Jx] = jBasis2D.x;
	m_values[Jy] = jBasis2D.y;
	m_values[Jz] = 0;
	m_values[Jw] = 0;
}

void Mat44::SetIJT2D(Vec2 const& iBasis2D, Vec2 const& jBasis2D, Vec2 const& translationXY)
{
	m_values[Ix] = iBasis2D.x;
	m_values[Iy] = iBasis2D.y;
	m_values[Iz] = 0;
	m_values[Iw] = 0;

	m_values[Jx] = jBasis2D.x;
	m_values[Jy] = jBasis2D.y;
	m_values[Jz] = 0;
	m_values[Jw] = 0;

	m_values[Tx] = translationXY.x;
	m_values[Ty] = translationXY.y;
	m_values[Tz] = 0;
	m_values[Tw] = 1;
}

void Mat44::SetIJK3D(Vec3 const& iBasis3D, Vec3 const& jBasis3D, Vec3 const& kBasis3D)
{
	m_values[Ix] = iBasis3D.x;
	m_values[Iy] = iBasis3D.y;
	m_values[Iz] = iBasis3D.z;
	m_values[Iw] = 0;

	m_values[Jx] = jBasis3D.x;
	m_values[Jy] = jBasis3D.y;
	m_values[Jz] = jBasis3D.z;
	m_values[Jw] = 0;

	m_values[Kx] = kBasis3D.x;
	m_values[Ky] = kBasis3D.y;
	m_values[Kz] = kBasis3D.z;
	m_values[Kw] = 0;
}

void Mat44::SetIJKT3D(Vec3 const& iBasis3D, Vec3 const& jBasis3D, Vec3 const& kBasis3D, Vec3 const& translationXYZ)
{
	m_values[Ix] = iBasis3D.x;
	m_values[Iy] = iBasis3D.y;
	m_values[Iz] = iBasis3D.z;
	m_values[Iw] = 0;

	m_values[Jx] = jBasis3D.x;
	m_values[Jy] = jBasis3D.y;
	m_values[Jz] = jBasis3D.z;
	m_values[Jw] = 0;

	m_values[Kx] = kBasis3D.x;
	m_values[Ky] = kBasis3D.y;
	m_values[Kz] = kBasis3D.z;
	m_values[Kw] = 0;

	m_values[Tx] = translationXYZ.x;
	m_values[Ty] = translationXYZ.y;
	m_values[Tz] = translationXYZ.z;
	m_values[Tw] = 1;
}

void Mat44::SetIJKT4D(Vec4 const& iBasis4D, Vec4 const& jBasis4D, Vec4 const& kBasis4D, Vec4 const& translation4D)
{
	m_values[Ix] = iBasis4D.x;
	m_values[Iy] = iBasis4D.y;
	m_values[Iz] = iBasis4D.z;
	m_values[Iw] = iBasis4D.w;

	m_values[Jx] = jBasis4D.x;
	m_values[Jy] = jBasis4D.y;
	m_values[Jz] = jBasis4D.z;
	m_values[Jw] = jBasis4D.w;

	m_values[Kx] = kBasis4D.x;
	m_values[Ky] = kBasis4D.y;
	m_values[Kz] = kBasis4D.z;
	m_values[Kw] = kBasis4D.w;

	m_values[Tx] = translation4D.x;
	m_values[Ty] = translation4D.y;
	m_values[Tz] = translation4D.z;
	m_values[Tw] = translation4D.w;
}

void Mat44::Append(Mat44 const& appendThis)
{
	Mat44 result;
	result.m_values[Ix] = (m_values[Ix] * appendThis.m_values[Ix]) + (m_values[Jx] * appendThis.m_values[Iy]) + (m_values[Kx] * appendThis.m_values[Iz]) + (m_values[Tx] * appendThis.m_values[Iw]);
	result.m_values[Jx] = (m_values[Ix] * appendThis.m_values[Jx]) + (m_values[Jx] * appendThis.m_values[Jy]) + (m_values[Kx] * appendThis.m_values[Jz]) + (m_values[Tx] * appendThis.m_values[Jw]);
	result.m_values[Kx] = (m_values[Ix] * appendThis.m_values[Kx]) + (m_values[Jx] * appendThis.m_values[Ky]) + (m_values[Kx] * appendThis.m_values[Kz]) + (m_values[Tx] * appendThis.m_values[Kw]);
	result.m_values[Tx] = (m_values[Ix] * appendThis.m_values[Tx]) + (m_values[Jx] * appendThis.m_values[Ty]) + (m_values[Kx] * appendThis.m_values[Tz]) + (m_values[Tx] * appendThis.m_values[Tw]);

	result.m_values[Iy] = (m_values[Iy] * appendThis.m_values[Ix]) + (m_values[Jy] * appendThis.m_values[Iy]) + (m_values[Ky] * appendThis.m_values[Iz]) + (m_values[Ty] * appendThis.m_values[Iw]);
	result.m_values[Jy] = (m_values[Iy] * appendThis.m_values[Jx]) + (m_values[Jy] * appendThis.m_values[Jy]) + (m_values[Ky] * appendThis.m_values[Jz]) + (m_values[Ty] * appendThis.m_values[Jw]);
	result.m_values[Ky] = (m_values[Iy] * appendThis.m_values[Kx]) + (m_values[Jy] * appendThis.m_values[Ky]) + (m_values[Ky] * appendThis.m_values[Kz]) + (m_values[Ty] * appendThis.m_values[Kw]);
	result.m_values[Ty] = (m_values[Iy] * appendThis.m_values[Tx]) + (m_values[Jy] * appendThis.m_values[Ty]) + (m_values[Ky] * appendThis.m_values[Tz]) + (m_values[Ty] * appendThis.m_values[Tw]);

	result.m_values[Iz] = (m_values[Iz] * appendThis.m_values[Ix]) + (m_values[Jz] * appendThis.m_values[Iy]) + (m_values[Kz] * appendThis.m_values[Iz]) + (m_values[Tz] * appendThis.m_values[Iw]);
	result.m_values[Jz] = (m_values[Iz] * appendThis.m_values[Jx]) + (m_values[Jz] * appendThis.m_values[Jy]) + (m_values[Kz] * appendThis.m_values[Jz]) + (m_values[Tz] * appendThis.m_values[Jw]);
	result.m_values[Kz] = (m_values[Iz] * appendThis.m_values[Kx]) + (m_values[Jz] * appendThis.m_values[Ky]) + (m_values[Kz] * appendThis.m_values[Kz]) + (m_values[Tz] * appendThis.m_values[Kw]);
	result.m_values[Tz] = (m_values[Iz] * appendThis.m_values[Tx]) + (m_values[Jz] * appendThis.m_values[Ty]) + (m_values[Kz] * appendThis.m_values[Tz]) + (m_values[Tz] * appendThis.m_values[Tw]);

	result.m_values[Iw] = (m_values[Iw] * appendThis.m_values[Ix]) + (m_values[Jw] * appendThis.m_values[Iy]) + (m_values[Kw] * appendThis.m_values[Iz]) + (m_values[Tw] * appendThis.m_values[Iw]);
	result.m_values[Jw] = (m_values[Iw] * appendThis.m_values[Jx]) + (m_values[Jw] * appendThis.m_values[Jy]) + (m_values[Kw] * appendThis.m_values[Jz]) + (m_values[Tw] * appendThis.m_values[Jw]);
	result.m_values[Kw] = (m_values[Iw] * appendThis.m_values[Kx]) + (m_values[Jw] * appendThis.m_values[Ky]) + (m_values[Kw] * appendThis.m_values[Kz]) + (m_values[Tw] * appendThis.m_values[Kw]);
	result.m_values[Tw] = (m_values[Iw] * appendThis.m_values[Tx]) + (m_values[Jw] * appendThis.m_values[Ty]) + (m_values[Kw] * appendThis.m_values[Tz]) + (m_values[Tw] * appendThis.m_values[Tw]);

	*this = result;
}

void Mat44::AppendZRotation(float degreesRotationAboutZ)
{
	Mat44 zRotation = CreateZRotationDegrees(degreesRotationAboutZ);
	Append(zRotation);
}

void Mat44::AppendYRotation(float degreesRotationAboutY)
{
	Mat44 yRotation = CreateYRotationDegrees(degreesRotationAboutY);
	Append(yRotation);

}

void Mat44::AppendXRotation(float degreesRotationAboutX)
{
	Mat44 xRotation = CreateXRotationDegrees(degreesRotationAboutX);
	Append(xRotation);
}

void Mat44::AppendTranslation2D(Vec2 const& translationXY)
{
	Mat44 translationXYMat = CreateTranslation2D(translationXY);
	Append(translationXYMat);
}

void Mat44::AppendTranslation3D(Vec3 const& translationXYZ)
{
	Mat44 translationXYZMat = CreateTranslation3D(translationXYZ);
	Append(translationXYZMat);
}

void Mat44::AppendScaleUniform2D(float uniformScaleXY)
{
	Mat44 uniformScaleXYMat = CreateUniformScale2D(uniformScaleXY);
	Append(uniformScaleXYMat);

}

void Mat44::AppendScaleUniform3D(float uniformScaleXYZ)
{
	Mat44 uniformScale3D = CreateUniformScale3D(uniformScaleXYZ);
	Append(uniformScale3D);
}

void Mat44::AppendScaleNonUniform2D(Vec2 const& nonUniformScaleXY)
{
	Mat44 nonUniformScaleXYMat = CreateNonUniformScale2D(nonUniformScaleXY);
	Append(nonUniformScaleXYMat);
}

void Mat44::AppendScaleNonUniform3D(Vec3 const& nonUniformScaleXYZ)
{
	Mat44 nonUniformScale3D = CreateNonUniformScale3D(nonUniformScaleXYZ);
	Append(nonUniformScale3D);
}

void Mat44::Add(Mat44 const& otherMatrix)
{

	for (int index = 0; index < 16; index++) {
		m_values[index] += otherMatrix.m_values[index];
	}
}

void Mat44::Transpose()
{
	Mat44 originalMat(*this);

	m_values[Iy] = originalMat.m_values[Jx];
	m_values[Iz] = originalMat.m_values[Kx];
	m_values[Iw] = originalMat.m_values[Tx];

	m_values[Jx] = originalMat.m_values[Iy];
	m_values[Jz] = originalMat.m_values[Ky];
	m_values[Jw] = originalMat.m_values[Ty];

	m_values[Kx] = originalMat.m_values[Iz];
	m_values[Ky] = originalMat.m_values[Jz];
	m_values[Kw] = originalMat.m_values[Tz];

	m_values[Tx] = originalMat.m_values[Iw];
	m_values[Ty] = originalMat.m_values[Jw];
	m_values[Tz] = originalMat.m_values[Kw];
}

Mat44 const Mat44::GetOrthonormalInverse() const
{

	Mat44 matToReturn(*this);
	Vec3 translationOnly = GetTranslation3D();
	matToReturn.SetTranslation3D(Vec3::ZERO);

	matToReturn.Transpose();

	Mat44 inverseTranslation;
	inverseTranslation.SetTranslation3D(translationOnly * -1.0f);

	matToReturn.Append(inverseTranslation);



	return matToReturn;

}

void Mat44::Orthonormalize_XFwd_YLeft_ZUp()
{
	Vec3 ibasis = GetIBasis3D();
	ibasis.Normalize();

	Vec3 kbasis = GetKBasis3D();
	Vec3 jbasis = GetJBasis3D();

	kbasis -= GetProjectedOnto3D(kbasis, ibasis);
	kbasis.Normalize();

	jbasis -= GetProjectedOnto3D(jbasis, kbasis);
	jbasis -= GetProjectedOnto3D(jbasis, ibasis);
	jbasis.Normalize();

	SetIJK3D(ibasis, jbasis, kbasis);
}


