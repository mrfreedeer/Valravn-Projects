#pragma once
#include <vector>
#include <string>

enum class BufferEndianness {
	DEFAULT,
	LITTLEENDIAN,
	BIGENDIAN,
};

struct Rgba8;
struct IntVec2;
struct IntVec3;
struct Vec2;
struct Vec3;
struct Vec4;
struct Vertex_PCU;
struct Vertex_PNCU;
struct Plane2D;
struct Plane3D;
struct AABB2;
struct AABB3;
struct EulerAngles;
struct FloatRange;
struct IntRange;
struct OBB2;
struct Mat44;

BufferEndianness GetNativeEndianness();
class BufferParser {
public:
	BufferParser(std::vector<unsigned char> const& buffer, BufferEndianness endianness = BufferEndianness::DEFAULT);
	BufferParser(void const* buffer, size_t size, BufferEndianness endianness = BufferEndianness::DEFAULT);

	void GoToOffset(size_t offsetFromBeginning);

	char ParseChar();
	unsigned char ParseByte();
	bool ParseBool();
	short ParseShort();
	unsigned short ParseUShort();
	unsigned int ParseUint32();
	int ParseInt32();
	float ParseFloat();
	double ParseDouble();
	void ParseStringZeroTerminated(std::string& storeStr);
	void ParseStringAfter32BitLength(std::string& storeStr);
	Rgba8 ParseRgba();
	IntVec2 ParseIntVec2();
	IntVec3 ParseIntVec3();
	Vec2 ParseVec2();
	Vec3 ParseVec3();
	Vec4 ParseVec4();
	Vertex_PCU ParseVertexPCU();
	Vertex_PNCU ParseVertexPNCU();
	AABB2 ParseAABB2();
	AABB3 ParseAABB3();
	OBB2 ParseOBB2();
	Plane2D ParsePlane2D();
	Plane3D ParsePlane3D();
	EulerAngles ParseEulerAngles();
	FloatRange ParseFloatRange();
	IntRange ParseIntRange();
	Mat44 ParseMat44();

	size_t GetTotalSize() const;
	size_t GetRemainingSize() const;
	BufferEndianness GetEndianness() const { return m_endianness; }
	void SetEndianness(BufferEndianness newEndianness);
private:
	bool m_shouldFlipBytes = false;
	unsigned char const* m_data;
	size_t m_size = 0;
	size_t m_currentPosition = 0;
	BufferEndianness m_endianness = BufferEndianness::DEFAULT;

};


class BufferWriter {
public:
	BufferWriter(std::vector<unsigned char>& buffer, BufferEndianness endianness = BufferEndianness::DEFAULT);
	BufferWriter(std::vector<unsigned char>* buffer, BufferEndianness endianness = BufferEndianness::DEFAULT);

	void OverwriteUint32(unsigned int startingPosition, unsigned int newInt32) const;
	void SetEndianness(BufferEndianness newEndianness);
	BufferEndianness GetEndianness() const { return m_endianness; }

	void AppendChar(char charToAdd) const;
	void AppendeByte(unsigned char byteToAdd) const;
	void AppendBool(bool boolToAdd) const;
	void AppendShort(short shortToAdd) const;
	void AppendUShort(unsigned short uShortToAdd) const;
	void AppendUint32(unsigned int uint32ToAdd) const;
	void AppendInt32(int int32ToAdd) const;
	void AppendFloat(float floatToadd) const;
	void AppendDouble(double doubleToAdd) const;
	void AppendStringZeroTerminated(std::string const& stringToAdd) const;
	void AppendStringAfter32BitLength(std::string const& stringToAdd) const;
	void AppendRgba(Rgba8 const& rgbaToAdd) const;
	void AppendIntVec2(IntVec2 const& intVec2ToAdd) const;
	void AppendIntVec3(IntVec3 const& intVec3ToAdd) const;
	void AppendVec2(Vec2 const& vec2ToAdd) const;
	void AppendVec3(Vec3 const& vec3ToAdd) const;
	void AppendVec4(Vec4 const& vec4ToAdd) const;
	void AppendVertexPCU(Vertex_PCU const& vertexToAdd) const;
	void AppendVertexPNCU(Vertex_PNCU const& vertexToAdd) const;
	void AppendAABB2(AABB2 const& aabb2ToAdd) const;
	void AppendAABB3(AABB3 const& aabb3ToAdd) const;
	void AppendOBB2(OBB2 const& obb2ToAdd) const;
	void AppendPlane2D(Plane2D const& plane2dToAdd) const;
	void AppendPlane3D(Plane3D const& plane3dToAdd) const;
	void AppendEulerAngles(EulerAngles const& eulerAnglesToAdd) const;
	void AppendFloatRange(FloatRange const& floatRangeToAdd) const;
	void AppendIntRange(IntRange const& intRangeToAdd) const;
	void AppendMat44(Mat44 const& matToAdd) const;
private:
	void Append4Bytes(unsigned char* bytesToAdd) const;

	std::vector<unsigned char>* m_buffer;
	bool m_shouldFlipBytes = false;
	BufferEndianness m_endianness = BufferEndianness::DEFAULT;
};
