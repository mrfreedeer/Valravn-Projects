#include "Engine/Core/BufferUtils.hpp"
#include "Game/Gameplay/ConvexSceneUtils.hpp"


bool ParseHeader(BufferParser& bufferParser, unsigned char& endianness, unsigned int& tocLocation)
{
	bool isEverythingRight = true;

	isEverythingRight = isEverythingRight && (bufferParser.ParseChar() == 'G');
	isEverythingRight = isEverythingRight && (bufferParser.ParseChar() == 'H');
	isEverythingRight = isEverythingRight && (bufferParser.ParseChar() == 'C');
	isEverythingRight = isEverythingRight && (bufferParser.ParseChar() == 'S');
	isEverythingRight = isEverythingRight && (bufferParser.ParseByte() == 31);
	isEverythingRight = isEverythingRight && (bufferParser.ParseByte() == 1); // Major
	isEverythingRight = isEverythingRight && (bufferParser.ParseByte() == 1); // Minor
	
	if(!isEverythingRight) return false;

	endianness = bufferParser.ParseByte();
	bufferParser.SetEndianness((BufferEndianness)endianness);

	tocLocation = bufferParser.ParseUint32();


	isEverythingRight = isEverythingRight && (bufferParser.ParseChar() == 'E');
	isEverythingRight = isEverythingRight && (bufferParser.ParseChar() == 'N');
	isEverythingRight = isEverythingRight && (bufferParser.ParseChar() == 'D');
	isEverythingRight = isEverythingRight && (bufferParser.ParseChar() == 'H');

	if(!isEverythingRight) return false;

	return true;
}

bool ParseChunkHeader(BufferParser& bufferParser, ChunkType& chunkType, unsigned char& chunkEndianness, unsigned int& chunkSize)
{
	bool isEverythingRight = true;

	isEverythingRight = isEverythingRight && (bufferParser.ParseChar() == 'G');
	isEverythingRight = isEverythingRight && (bufferParser.ParseChar() == 'H');
	isEverythingRight = isEverythingRight && (bufferParser.ParseChar() == 'C');
	isEverythingRight = isEverythingRight && (bufferParser.ParseChar() == 'K');

	if(!isEverythingRight) return false;
	chunkType = (ChunkType)bufferParser.ParseByte();
	chunkEndianness = bufferParser.ParseByte();
	chunkSize = bufferParser.ParseUint32();

	return true;
}

bool ParseChunkHeaderEndSequence(BufferParser& bufferParser)
{
	bool isEverythingRight = true;

	isEverythingRight = isEverythingRight && (bufferParser.ParseChar() == 'E');
	isEverythingRight = isEverythingRight && (bufferParser.ParseChar() == 'N');
	isEverythingRight = isEverythingRight && (bufferParser.ParseChar() == 'D');
	isEverythingRight = isEverythingRight && (bufferParser.ParseChar() == 'C');

	return isEverythingRight;
}

bool ParseTocHeader(BufferParser& bufferParser)
{
	bool isEverythingRight = true;

	isEverythingRight = isEverythingRight && (bufferParser.ParseChar() == 'G');
	isEverythingRight = isEverythingRight && (bufferParser.ParseChar() == 'H');
	isEverythingRight = isEverythingRight && (bufferParser.ParseChar() == 'T');
	isEverythingRight = isEverythingRight && (bufferParser.ParseChar() == 'C');

	return isEverythingRight;

}

bool ParseTocEndSequence(BufferParser& bufferParser)
{
	bool isEverythingRight = true;

	isEverythingRight = isEverythingRight && (bufferParser.ParseChar() == 'E');
	isEverythingRight = isEverythingRight && (bufferParser.ParseChar() == 'N');
	isEverythingRight = isEverythingRight && (bufferParser.ParseChar() == 'D');
	isEverythingRight = isEverythingRight && (bufferParser.ParseChar() == 'T');

	return isEverythingRight;
}

void WriteHeaderToBuffer(std::vector<unsigned char>& sceneBuffer)
{
	BufferWriter bufWriter(sceneBuffer);
	bufWriter.AppendChar('G');
	bufWriter.AppendChar('H');
	bufWriter.AppendChar('C');
	bufWriter.AppendChar('S');
	bufWriter.AppendeByte((unsigned char)31);
	bufWriter.AppendeByte(1); // Major 
	bufWriter.AppendeByte(1); // Minor
	bufWriter.AppendeByte((unsigned char)bufWriter.GetEndianness());

	unsigned int tocLocationHolder = 0;

	bufWriter.AppendUint32(tocLocationHolder);
	bufWriter.AppendChar('E');
	bufWriter.AppendChar('N');
	bufWriter.AppendChar('D');
	bufWriter.AppendChar('H');
}

void WriteChunkHeaderToBuffer(std::vector<unsigned char>& sceneBuffer, ChunkType chunkType, unsigned char endianness, unsigned int chunkSize)
{
	BufferWriter bufWriter(sceneBuffer);
	bufWriter.AppendChar('G');
	bufWriter.AppendChar('H');
	bufWriter.AppendChar('C');
	bufWriter.AppendChar('K');

	bufWriter.AppendeByte(chunkType);
	bufWriter.AppendeByte(endianness);
	bufWriter.AppendUint32(chunkSize);
}

void WriteChunkHeaderEndSequence(std::vector<unsigned char>& sceneBuffer)
{
	BufferWriter bufWriter(sceneBuffer);
	bufWriter.AppendChar('E');
	bufWriter.AppendChar('N');
	bufWriter.AppendChar('D');
	bufWriter.AppendChar('C');
}

void WriteTocHeaderToBuffer(std::vector<unsigned char>& sceneBuffer)
{
	BufferWriter bufWriter(sceneBuffer);
	bufWriter.AppendChar('G');
	bufWriter.AppendChar('H');
	bufWriter.AppendChar('T');
	bufWriter.AppendChar('C');
}

void WriteTocEndSequenceToBuffer(std::vector<unsigned char>& sceneBuffer)
{
	BufferWriter bufWriter(sceneBuffer);
	bufWriter.AppendChar('E');
	bufWriter.AppendChar('N');
	bufWriter.AppendChar('D');
	bufWriter.AppendChar('T');
}

