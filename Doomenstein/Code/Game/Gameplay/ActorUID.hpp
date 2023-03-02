#pragma once

struct ActorUID
{
public:
	ActorUID();
	ActorUID( int index, int salt );

	void Invalidate();
	bool IsValid() const;
	int GetIndex() const;
	bool operator==( const ActorUID& other ) const;
	bool operator!=( const ActorUID& other ) const;

	static const ActorUID INVALID;

private:
	int m_data;
};

