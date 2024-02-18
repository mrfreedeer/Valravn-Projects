#include "Engine/Network/TCPConnection.hpp"
#include "Engine/Network/NetworkCommon.hpp"
#include "Engine/Network/NetworkAddress.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"

bool TCPConnection::Connect(NetworkAddress const& address)
{
	sockaddr_in ipv4;

	uint64_t uintAddr = address.m_address;
	uint16_t port = address.m_port;

	ipv4.sin_family = AF_INET;
	ipv4.sin_addr.S_un.S_addr = ::htonl((ULONG)uintAddr);
	ipv4.sin_port = ::htons(port);

	SOCKET sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET) {
		return false;
	}

	int result = ::connect(sock, (sockaddr*)&ipv4, (int)sizeof(ipv4));
	if (result == SOCKET_ERROR) {
		::closesocket(sock);
		return false;
	}

	m_socketHandle = sock;
	m_address = address;
	m_connectionState = ConnectionState::CONNECTING;
	return true;
}

size_t TCPConnection::Send(void const* data, size_t const dataSize)
{
	if (IsClosed()) {
		return 0;
	}

	int bytesSent = ::send(m_socketHandle, (char const*)data, (int)dataSize, 0);
	if (bytesSent > 0) {
		GUARANTEE_OR_DIE(bytesSent == dataSize, "BYTES SENT ARE NOT THE FULL SIZE OF THE MESSAGE");
		return bytesSent;
	}
	else if (bytesSent == 0) {
		Close();
		return 0;
	}
	else {
		CheckForFatalError();
		return 0;
	}
}

size_t TCPConnection::Receive(void* buff, size_t maxBytesToRead)
{
	int bytesRead = ::recv(m_socketHandle, (char*)buff, (int)maxBytesToRead, 0);
	if (bytesRead == 0) {
		Close();
		return 0;
	}
	else if (bytesRead > 0) {
		return (size_t)bytesRead;
	}
	else {
		CheckForFatalError();
		return 0;
	}

}

size_t TCPConnection::ReceiveFull()
{
	if ((m_bytesReceived < m_bytesExpected) || (m_bytesReceived == 0)) {
		size_t bytesToRead = 0;
		if (m_bytesReceived < 2 && m_bytesExpected == 0) {
			Receive(&bytesToRead, 2);
			if (bytesToRead < 2) return 0;
			m_bytesExpected = ::ntohs((u_short)bytesToRead);
		}
		else {
			bytesToRead = m_bytesExpected - m_bytesReceived;
		}

		
		m_bytesReceived += Receive(&m_buffer[m_bytesReceived], bytesToRead);
	}


	if (m_bytesReceived > 0 && (m_bytesReceived >= m_bytesExpected)) {
		size_t totalReceived = m_bytesReceived;
		m_bytesReceived = 0;
		m_bytesExpected = 0;

		m_isLastMessageEcho = (bool)m_buffer[0];
		size_t msgSize = 0;
		msgSize |= ((size_t)m_buffer[1]) << 8;
		msgSize |= m_buffer[2];

		m_lastMessage = std::string((char*)&m_buffer[3], msgSize);

		memset(m_buffer, 0, CONNECTION_BUFFER_SIZE);

		return totalReceived;
	}

	return 0;

}

bool TCPConnection::IsConnected() const
{
	return m_connectionState == ConnectionState::CONNECTED;
}

void TCPConnection::SendString(std::string const& str)
{
	Send(str.c_str(), str.size() + 1);
}

bool TCPConnection::CheckForConnection()
{
	if (IsConnected()) {
		return true;
	}

	if (IsClosed()) {
		return false;
	}


	WSAPOLLFD poll;
	poll.fd = m_socketHandle;
	poll.events = POLLWRNORM;

	int result = WSAPoll(&poll, 1, 0);
	if (result == SOCKET_ERROR) {
		Close();
	}

	if ((poll.revents & POLLHUP) != 0) {
		Close();
		return false;
	}

	if ((poll.revents & POLLWRNORM) != 0) {
		m_connectionState = ConnectionState::CONNECTED;
		return true;
	}

	return false;
}

std::string TCPConnection::GetLastMessage() const
{
	return m_lastMessage;
}

