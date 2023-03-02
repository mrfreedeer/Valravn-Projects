#pragma once
#include <cstdint>
typedef uintptr_t SocketHandle;

class NetworkAddress;

class Socket {
public:
	Socket();

	bool CheckForFatalError();
	bool IsValid() const;
	bool IsClosed() const;

	void SetBlocking( bool isBlocking);

	void Close();

public:
	SocketHandle m_socketHandle;
};