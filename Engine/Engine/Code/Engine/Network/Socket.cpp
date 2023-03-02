#include "Engine/Network/Socket.hpp"
#include "Engine/Network/NetworkCommon.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Network/NetworkAddress.hpp"

Socket::Socket() :
	m_socketHandle(INVALID_SOCKET)
{

}

bool Socket::IsValid() const
{
	return !(m_socketHandle == INVALID_SOCKET);
}

bool Socket::IsClosed() const
{
	return m_socketHandle == INVALID_SOCKET;
}

void Socket::SetBlocking(bool isBlocking)
{
	u_long non_blocking = isBlocking ? 0 : 1;
	::ioctlsocket(m_socketHandle, FIONBIO, &non_blocking);
}


bool Socket::CheckForFatalError() 
{
	int errorCode = ::WSAGetLastError();

	switch (errorCode)
	{
	case 0:
	case WSAEWOULDBLOCK:
		return false;
		/*case WSAECONNRESET:
			g_theConsole->AddLine(DevConsole::ERROR_COLOR, Stringf("Network System: Socket hit a fatal error: 0x%08x", errorCode));
			return true;*/
	default:
		g_theConsole->AddLine(DevConsole::ERROR_COLOR, Stringf("Network System: Socket hit a fatal error: 0x%08x", errorCode));
		Close();
		return true;
	}

}

void Socket::Close()
{
	if (IsValid())
	{
		::closesocket(m_socketHandle);
		m_socketHandle = INVALID_SOCKET;
	}
}
