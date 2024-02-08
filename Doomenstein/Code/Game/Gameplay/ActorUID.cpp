#include "Game/Gameplay/ActorUID.hpp"

const ActorUID ActorUID::INVALID;

ActorUID::ActorUID()
{
	m_data = 0xffffffff;
}

ActorUID::ActorUID(int index, int salt)
{
	int bitShiftedIndex = index << 16;
	m_data = (bitShiftedIndex | salt);
}

void ActorUID::Invalidate()
{
	m_data = 0xffffffff;
}

bool ActorUID::IsValid() const
{
	return m_data != 0xffffffff;
}

int ActorUID::GetIndex() const
{
	return m_data>>16;
}

bool ActorUID::operator==(const ActorUID& other) const
{
	return other.m_data == m_data;
}

bool ActorUID::operator!=(const ActorUID& other) const
{
	return other.m_data != m_data;
}