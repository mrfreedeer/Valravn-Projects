#include "Engine/Network/TCPServer.hpp"
#include "Engine/Network/TCPConnection.hpp"
#include "Engine/Network/NetworkAddress.hpp"
#include "Engine/Network/NetworkCommon.hpp"

bool TCPServer::Host(uint16_t service, uint32_t backlog)
{
	NetworkAddress hostingAddress;
	hostingAddress.m_address = INADDR_ANY; // Any IP
	hostingAddress.m_port = service;

	SOCKET sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET) {
		::closesocket(sock);
		return false;
	}

	sockaddr_in ipv4Addr;
	ipv4Addr.sin_family = AF_INET;
	ipv4Addr.sin_addr.S_un.S_addr = ::htonl((ULONG)hostingAddress.m_address);
	ipv4Addr.sin_port = ::htons(hostingAddress.m_port);

	int result = ::bind(sock, (sockaddr*)&ipv4Addr, (int) sizeof(ipv4Addr));
	if (SOCKET_ERROR == result) {
		::closesocket(sock);
		return false;
	}

	m_socketHandle = sock;
	m_address = hostingAddress;

	result = ::listen(sock, backlog);
	if (result == SOCKET_ERROR) {
		Close();
	}

	return true;
}

TCPConnection* TCPServer::Accept()
{
	if (IsClosed()) {
		return nullptr;
	}

	sockaddr_storage addr;
	int addrLen = sizeof(addr);

	SocketHandle handle = ::accept(m_socketHandle, (sockaddr*) &addr, &addrLen);

	if (handle == INVALID_SOCKET) {
		return nullptr;
	}

	if (addr.ss_family != AF_INET) {
		::closesocket(handle);
		return nullptr;
	}

	sockaddr_in* ipv4Addr = (sockaddr_in*)&addr;

	NetworkAddress address;
	address.m_address = ::ntohl(ipv4Addr->sin_addr.S_un.S_addr);
	address.m_port = ::ntohs(ipv4Addr->sin_port);

	TCPConnection* connection = new TCPConnection();
	connection->m_socketHandle = handle;
	connection->m_address = address;

	return connection;


}
