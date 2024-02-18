#include "Engine/Core/BufferUtils.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/IntRange.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/IntVec3.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Vec4.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/Plane2D.hpp"
#include "Engine/Math/Plane3D.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/Vertex_PNCU.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"

BufferEndianness GetNativeEndianness()
{
	int num = 1;
	bool isLittleEndian = (*(char*)&num == 1);
	return (isLittleEndian) ? BufferEndianness::LITTLEENDIAN : BufferEndianness::BIGENDIAN;
}

void Flip2Bytes(unsigned char* bytesToFlip) {

	unsigned char secByte = bytesToFlip[1];

	bytesToFlip[1] = bytesToFlip[0];
	bytesToFlip[0] = secByte;
}

void Flip4Bytes(unsigned char* bytesToFlip) {

	unsigned char lastByte = bytesToFlip[3];
	unsigned char secByte = bytesToFlip[1];

	bytesToFlip[3] = bytesToFlip[0];
	bytesToFlip[0] = lastByte;
	bytesToFlip[1] = bytesToFlip[2];
	bytesToFlip[2] = secByte;
}

void Flip8Bytes(unsigned char* bytesToFlip) {
	unsigned char copyArray[8] = { 0 };
	memcpy(&copyArray, bytesToFlip, sizeof(copyArray));

	bytesToFlip[0] = copyArray[7];
	bytesToFlip[7] = copyArray[0];
	bytesToFlip[1] = copyArray[6];
	bytesToFlip[6] = copyArray[1];
	bytesToFlip[2] = copyArray[5];
	bytesToFlip[5] = copyArray[2];
	bytesToFlip[3] = copyArray[4];
	bytesToFlip[4] = copyArray[3];

}

BufferParser::BufferParser(std::vector<unsigned char> const& buffer, BufferEndianness endianness) :
	m_data(buffer.data()),
	m_size(buffer.size())
{
	SetEndianness(endianness);
}

BufferParser::BufferParser(void const* buffer, size_t size, BufferEndianness endianness) :
	m_data((unsigned char const*)buffer),
	m_size(size)
{
	SetEndianness(endianness);
}

void BufferParser::GoToOffset(size_t offsetFromBeginning)
{
	m_currentPosition = offsetFromBeginning;
}

char BufferParser::ParseChar()
{
	if (m_currentPosition >= m_size) {
		ERROR_RECOVERABLE("TRYING TO PARSE BEYOND BUFFER END");
		return -1;
	}
	char valueAsChar = static_cast<char>(m_data[m_currentPosition]);
	m_currentPosition++;
	return valueAsChar;
}

unsigned char BufferParser::ParseByte()
{
	if (m_currentPosition >= m_size) {
		ERROR_RECOVERABLE("TRYING TO PARSE BEYOND BUFFER END");
		return 0xFF;
	}

	unsigned char value = m_data[m_currentPosition];
	m_currentPosition++;
	return value;
}

bool BufferParser::ParseBool()
{
	if (m_currentPosition >= m_size) {
		ERROR_RECOVERABLE("TRYING TO PARSE BEYOND BUFFER END");
		return false;
	}

	bool value = static_cast<bool>(m_data[m_currentPosition]);
	m_currentPosition++;
	return value;
}

short BufferParser::ParseShort()
{
	union {
		short resultingValue;
		unsigned char asUChar[2];
	} conversion;

	if ((m_currentPosition + 2) > m_size) {
		ERROR_RECOVERABLE("TRYING TO PARSE BEYOND BUFFER END");
		return -1;
	}

	conversion.asUChar[0] = m_data[m_currentPosition];
	conversion.asUChar[1] = m_data[m_currentPosition + 1];

	m_currentPosition += 2;

	if (m_shouldFlipBytes) {
		Flip2Bytes(conversion.asUChar);
	}

	return conversion.resultingValue;
}

unsigned short BufferParser::ParseUShort()
{
	union {
		unsigned short resultingValue;
		unsigned char asUChar[2];
	} conversion;

	if ((m_currentPosition + 2) > m_size) {
		ERROR_RECOVERABLE("TRYING TO PARSE BEYOND BUFFER END");
		return 0xFFFF;
	}

	conversion.asUChar[0] = m_data[m_currentPosition];
	conversion.asUChar[1] = m_data[m_currentPosition + 1];

	m_currentPosition += 2;

	if (m_shouldFlipBytes) {
		Flip2Bytes(conversion.asUChar);
	}

	return conversion.resultingValue;
}

unsigned int BufferParser::ParseUint32()
{
	union {
		unsigned int resultingValue;
		unsigned char asUChar[4];
	} conversion;

	if ((m_currentPosition + 4) > m_size) {
		ERROR_RECOVERABLE("TRYING TO PARSE BEYOND BUFFER END");
		return 0xFFFFFFFF;
	}

	conversion.asUChar[0] = m_data[m_currentPosition];
	conversion.asUChar[1] = m_data[m_currentPosition + 1];
	conversion.asUChar[2] = m_data[m_currentPosition + 2];
	conversion.asUChar[3] = m_data[m_currentPosition + 3];


	m_currentPosition += 4;

	if (m_shouldFlipBytes) {
		Flip4Bytes(conversion.asUChar);
	}

	return conversion.resultingValue;
}

int BufferParser::ParseInt32()
{
	union {
		int resultingValue;
		unsigned char asUChar[4];
	} conversion;

	if ((m_currentPosition + 4) > m_size) {
		ERROR_RECOVERABLE("TRYING TO PARSE BEYOND BUFFER END");
		return -1;
	}

	conversion.asUChar[0] = m_data[m_currentPosition];
	conversion.asUChar[1] = m_data[m_currentPosition + 1];
	conversion.asUChar[2] = m_data[m_currentPosition + 2];
	conversion.asUChar[3] = m_data[m_currentPosition + 3];


	m_currentPosition += 4;

	if (m_shouldFlipBytes) {
		Flip4Bytes(conversion.asUChar);
	}

	return conversion.resultingValue;
}

float BufferParser::ParseFloat()
{
	union {
		float resultingValue;
		unsigned char asUChar[4];
	} conversion;

	if ((m_currentPosition + 4) > m_size) {
		ERROR_RECOVERABLE("TRYING TO PARSE BEYOND BUFFER END");
		return 0.0f;
	}

	conversion.asUChar[0] = m_data[m_currentPosition];
	conversion.asUChar[1] = m_data[m_currentPosition + 1];
	conversion.asUChar[2] = m_data[m_currentPosition + 2];
	conversion.asUChar[3] = m_data[m_currentPosition + 3];


	m_currentPosition += 4;

	if (m_shouldFlipBytes) {
		Flip4Bytes((unsigned char*)&conversion.resultingValue);
	}

	return conversion.resultingValue;
}

double BufferParser::ParseDouble()
{
	union {
		double resultingValue;
		unsigned char asUChar[8];
	} conversion;

	if ((m_currentPosition + 8) > m_size) {
		ERROR_RECOVERABLE("TRYING TO PARSE BEYOND BUFFER END");
		return 0.0f;
	}

	conversion.asUChar[0] = m_data[m_currentPosition];
	conversion.asUChar[1] = m_data[m_currentPosition + 1];
	conversion.asUChar[2] = m_data[m_currentPosition + 2];
	conversion.asUChar[3] = m_data[m_currentPosition + 3];
	conversion.asUChar[4] = m_data[m_currentPosition + 4];
	conversion.asUChar[5] = m_data[m_currentPosition + 5];
	conversion.asUChar[6] = m_data[m_currentPosition + 6];
	conversion.asUChar[7] = m_data[m_currentPosition + 7];


	m_currentPosition += 8;

	if (m_shouldFlipBytes) {
		Flip8Bytes(conversion.asUChar);
	}

	return conversion.resultingValue;
}

void BufferParser::ParseStringZeroTerminated(std::string& storeStr)
{
	for (; m_data[m_currentPosition] != '\0'; m_currentPosition++) {
		storeStr += static_cast<char>(m_data[m_currentPosition]);
	}
	storeStr += '\0';
	m_currentPosition++;
}

void BufferParser::ParseStringAfter32BitLength(std::string& storeStr)
{
	unsigned int strSize = ParseUint32();

	if ((m_currentPosition + strSize) > m_size) {
		ERROR_RECOVERABLE("TRYING TO PARSE BEYOND BUFFER END");
		return;
	}

	for (unsigned int index = 0; index < strSize; index++, m_currentPosition++) {
		storeStr += m_data[m_currentPosition];
	}

	m_currentPosition++;
}

Rgba8 BufferParser::ParseRgba()
{
	if ((m_currentPosition + 4) > m_size) {
		ERROR_RECOVERABLE("TRYING TO PARSE BEYOND BUFFER END");
		return Rgba8();
	}

	Rgba8 resultingColor;
	resultingColor.r = (m_data[m_currentPosition]);
	resultingColor.g = (m_data[m_currentPosition + 1]);
	resultingColor.b = (m_data[m_currentPosition + 2]);
	resultingColor.a = (m_data[m_currentPosition + 3]);

	m_currentPosition += 4;

	return resultingColor;
}

IntVec2 BufferParser::ParseIntVec2()
{
	int x = ParseInt32();
	int y = ParseInt32();
	return IntVec2(x, y);
}

IntVec3 BufferParser::ParseIntVec3()
{
	int x = ParseInt32();
	int y = ParseInt32();
	int z = ParseInt32();
	return IntVec3(x, y, z);
}

Vec2 BufferParser::ParseVec2()
{
	float x = ParseFloat();
	float y = ParseFloat();

	return Vec2(x, y);
}

Vec3 BufferParser::ParseVec3()
{
	float x = ParseFloat();
	float y = ParseFloat();
	float z = ParseFloat();

	return Vec3(x, y, z);
}

Vec4 BufferParser::ParseVec4()
{
	float x = ParseFloat();
	float y = ParseFloat();
	float z = ParseFloat();
	float w = ParseFloat();

	return Vec4(x, y, z, w);
}

Vertex_PCU BufferParser::ParseVertexPCU()
{
	Vec3 position = ParseVec3();
	Rgba8 color = ParseRgba();
	Vec2 uv = ParseVec2();

	return Vertex_PCU(position, color, uv);
}

Vertex_PNCU BufferParser::ParseVertexPNCU()
{
	Vec3 position = ParseVec3();
	Vec3 normals = ParseVec3();
	Rgba8 color = ParseRgba();
	Vec2 uv = ParseVec2();

	return Vertex_PNCU(position, normals, color, uv);
}

AABB2 BufferParser::ParseAABB2()
{
	Vec2 mins = ParseVec2();
	Vec2 maxs = ParseVec2();
	return AABB2(mins, maxs);
}

AABB3 BufferParser::ParseAABB3()
{
	Vec3 mins = ParseVec3();
	Vec3 maxs = ParseVec3();
	return AABB3(mins, maxs);
}

OBB2 BufferParser::ParseOBB2()
{
	Vec2 center = ParseVec2();
	Vec2 halfDims = ParseVec2();
	return OBB2(center, halfDims);
}

Plane2D BufferParser::ParsePlane2D()
{
	Plane2D plane;
	plane.m_planeNormal = ParseVec2();
	plane.m_distToPlane = ParseFloat();
	return plane;
}

Plane3D BufferParser::ParsePlane3D()
{
	Plane3D plane;
	plane.m_planeNormal = ParseVec3();
	plane.m_distToPlane = ParseFloat();
	return plane;
}

EulerAngles BufferParser::ParseEulerAngles()
{
	float yaw = ParseFloat();
	float pitch = ParseFloat();
	float roll = ParseFloat();

	return EulerAngles(yaw, pitch, roll);
}

FloatRange BufferParser::ParseFloatRange()
{
	float min = ParseFloat();
	float max = ParseFloat();


	return FloatRange(min, max);
}

IntRange BufferParser::ParseIntRange()
{
	int min = ParseInt32();
	int max = ParseInt32();


	return IntRange(min, max);
}

Mat44 BufferParser::ParseMat44()
{
	Mat44 newMat;
	for (int floatInd = 0; floatInd < 16; floatInd++) {
		newMat.m_values[floatInd] = ParseFloat();
	}
	return newMat;
}

size_t BufferParser::GetTotalSize() const
{
	return m_size;
}

size_t BufferParser::GetRemainingSize() const
{
	return m_size - m_currentPosition;
}


void BufferParser::SetEndianness(BufferEndianness newEndianness)
{
	m_endianness = newEndianness;
	BufferEndianness nativeEndianness = GetNativeEndianness();

	if (m_endianness == BufferEndianness::DEFAULT) {
		m_endianness = nativeEndianness;
	}

	m_shouldFlipBytes = (nativeEndianness != m_endianness);
}

BufferWriter::BufferWriter(std::vector<unsigned char>& buffer, BufferEndianness endianness) :
	m_buffer(&buffer)
{
	SetEndianness(endianness);
}

BufferWriter::BufferWriter(std::vector<unsigned char>* buffer, BufferEndianness endianness) :
	m_buffer(buffer)
{
	SetEndianness(endianness);
}

void BufferWriter::OverwriteUint32(unsigned int startingPosition, unsigned int newInt32) const
{
	unsigned char* asArray = (unsigned char*)&newInt32;
	if (m_shouldFlipBytes) {
		Flip4Bytes(asArray);
	}

	(*m_buffer)[startingPosition] = asArray[0];
	(*m_buffer)[size_t(startingPosition + 1)] = asArray[1];
	(*m_buffer)[size_t(startingPosition + 2)] = asArray[2];
	(*m_buffer)[size_t(startingPosition + 3)] = asArray[3];

}

void BufferWriter::SetEndianness(BufferEndianness newEndianness)
{
	m_endianness = newEndianness;
	BufferEndianness nativeEndianness = GetNativeEndianness();

	if (m_endianness == BufferEndianness::DEFAULT) {
		m_endianness = nativeEndianness;
	}

	m_shouldFlipBytes = (nativeEndianness != m_endianness);
}

void BufferWriter::AppendChar(char charToAdd) const
{
	m_buffer->push_back(static_cast<unsigned char>(charToAdd));
}

void BufferWriter::AppendeByte(unsigned char byteToAdd) const
{
	m_buffer->push_back(byteToAdd);
}

void BufferWriter::AppendBool(bool boolToAdd) const
{
	unsigned char boolAsbyte = (boolToAdd) ? 1 : 0;
	m_buffer->push_back(boolAsbyte);
}

void BufferWriter::AppendShort(short shortToAdd) const
{
	unsigned char* asArray = (unsigned char*)&shortToAdd;
	if (m_shouldFlipBytes) {
		Flip2Bytes(asArray);
	}

	m_buffer->push_back(asArray[0]);
	m_buffer->push_back(asArray[1]);

}
void BufferWriter::AppendUShort(unsigned short uShortToAdd) const
{
	unsigned char* asArray = (unsigned char*)&uShortToAdd;
	if (m_shouldFlipBytes) {
		Flip2Bytes(asArray);
	}

	m_buffer->push_back(asArray[0]);
	m_buffer->push_back(asArray[1]);
}
void BufferWriter::AppendDouble(double doubleToAdd) const
{
	unsigned char* asArray = (unsigned char*)&doubleToAdd;
	if (m_shouldFlipBytes) {
		Flip8Bytes(asArray);
	}
	m_buffer->insert(m_buffer->end(), asArray, asArray + 8);
}

void BufferWriter::AppendUint32(unsigned int uint32ToAdd) const
{
	unsigned char* asArray = (unsigned char*)&uint32ToAdd;
	Append4Bytes(asArray);
}

void BufferWriter::AppendInt32(int int32ToAdd) const
{
	unsigned char* asArray = (unsigned char*)&int32ToAdd;
	Append4Bytes(asArray);

}

void BufferWriter::AppendFloat(float floatToadd) const
{
	unsigned char* asArray = (unsigned char*)&floatToadd;
	Append4Bytes(asArray);
}

void BufferWriter::Append4Bytes(unsigned char* bytesToAdd) const
{
	if (m_shouldFlipBytes) {
		Flip4Bytes(bytesToAdd);
	}
	m_buffer->insert(m_buffer->end(), bytesToAdd, bytesToAdd + 4);
}

void BufferWriter::AppendStringZeroTerminated(std::string const& stringToAdd) const
{
	for (char stringChar : stringToAdd) {
		m_buffer->push_back((unsigned char)stringChar);
	}
	m_buffer->push_back('\0');

}

void BufferWriter::AppendStringAfter32BitLength(std::string const& stringToAdd) const
{
	int stringSize = (int)stringToAdd.size();

	AppendInt32(stringSize);
	for (char stringChar : stringToAdd) {
		m_buffer->push_back((unsigned char)stringChar);
	}

}

void BufferWriter::AppendRgba(Rgba8 const& rgbaToAdd) const
{
	m_buffer->push_back(rgbaToAdd.r);
	m_buffer->push_back(rgbaToAdd.g);
	m_buffer->push_back(rgbaToAdd.b);
	m_buffer->push_back(rgbaToAdd.a);
}

void BufferWriter::AppendIntVec2(IntVec2 const& intVec2ToAdd) const
{
	AppendInt32(intVec2ToAdd.x);
	AppendInt32(intVec2ToAdd.y);
}

void BufferWriter::AppendIntVec3(IntVec3 const& intVec3ToAdd) const
{
	AppendInt32(intVec3ToAdd.x);
	AppendInt32(intVec3ToAdd.y);
	AppendInt32(intVec3ToAdd.z);
}

void BufferWriter::AppendVec2(Vec2 const& vec2ToAdd) const
{
	AppendFloat(vec2ToAdd.x);
	AppendFloat(vec2ToAdd.y);
}

void BufferWriter::AppendVec3(Vec3 const& vec3ToAdd) const
{
	AppendFloat(vec3ToAdd.x);
	AppendFloat(vec3ToAdd.y);
	AppendFloat(vec3ToAdd.z);
}

void BufferWriter::AppendVec4(Vec4 const& vec4ToAdd) const
{
	AppendFloat(vec4ToAdd.x);
	AppendFloat(vec4ToAdd.y);
	AppendFloat(vec4ToAdd.z);
	AppendFloat(vec4ToAdd.w);
}

void BufferWriter::AppendVertexPCU(Vertex_PCU const& vertexToAdd) const
{
	AppendVec3(vertexToAdd.m_position);
	AppendRgba(vertexToAdd.m_color);
	AppendVec2(vertexToAdd.m_uvTexCoords);
}

void BufferWriter::AppendVertexPNCU(Vertex_PNCU const& vertexToAdd) const
{
	AppendVec3(vertexToAdd.m_position);
	AppendVec3(vertexToAdd.m_normal);
	AppendRgba(vertexToAdd.m_color);
	AppendVec2(vertexToAdd.m_uvTexCoords);

}

void BufferWriter::AppendAABB2(AABB2 const& aabb2ToAdd) const
{
	AppendVec2(aabb2ToAdd.m_mins);
	AppendVec2(aabb2ToAdd.m_maxs);
}

void BufferWriter::AppendAABB3(AABB3 const& aabb3ToAdd) const
{
	AppendVec3(aabb3ToAdd.m_mins);
	AppendVec3(aabb3ToAdd.m_maxs);
}

void BufferWriter::AppendOBB2(OBB2 const& obb2ToAdd) const
{
	AppendVec2(obb2ToAdd.m_center);
	AppendVec2(obb2ToAdd.m_halfDimensions);
}

void BufferWriter::AppendPlane2D(Plane2D const& plane2dToAdd) const
{
	AppendVec2(plane2dToAdd.m_planeNormal);
	AppendFloat(plane2dToAdd.m_distToPlane);
}

void BufferWriter::AppendPlane3D(Plane3D const& plane3dToAdd) const
{
	AppendVec3(plane3dToAdd.m_planeNormal);
	AppendFloat(plane3dToAdd.m_distToPlane);
}

void BufferWriter::AppendEulerAngles(EulerAngles const& eulerAnglesToAdd) const
{
	AppendFloat(eulerAnglesToAdd.m_yawDegrees);
	AppendFloat(eulerAnglesToAdd.m_pitchDegrees);
	AppendFloat(eulerAnglesToAdd.m_rollDegrees);
}

void BufferWriter::AppendFloatRange(FloatRange const& floatRangeToAdd) const
{
	AppendFloat(floatRangeToAdd.m_min);
	AppendFloat(floatRangeToAdd.m_max);
}

void BufferWriter::AppendIntRange(IntRange const& intRangeToAdd) const
{
	AppendInt32(intRangeToAdd.m_min);
	AppendInt32(intRangeToAdd.m_max);
}

void BufferWriter::AppendMat44(Mat44 const& matToAdd) const
{
	for (int floatInd = 0; floatInd < 16; floatInd++) {
		AppendFloat(matToAdd.m_values[floatInd]);
	}
}

