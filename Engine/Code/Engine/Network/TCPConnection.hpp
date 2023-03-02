#pragma once
#include "Engine/Network/TCPSocket.hpp"
#include "Engine/Network/NetworkAddress.hpp"
#include <string>

constexpr size_t CONNECTION_BUFFER_SIZE = 4096;

enum class ConnectionState {
	DISCONNECTED,
	CONNECTING,
	CONNECTED
};

class TCPConnection : public TCPSocket {
public:
	bool Connect(NetworkAddress const& address);
	size_t Send(void const* data, size_t const dataSize);
	size_t Receive(void* buff, size_t maxBytesToRead);
	size_t ReceiveFull();

	bool IsConnected() const;
	bool IsMessageEcho() const { return m_isLastMessageEcho; }

	void SendString(std::string const& str);
	bool CheckForConnection();

	std::string GetLastMessage() const;
public:
	NetworkAddress m_address;

private:
	ConnectionState m_connectionState = ConnectionState::DISCONNECTED;
	uint8_t m_buffer[CONNECTION_BUFFER_SIZE] = {};
	size_t m_bytesReceived = 0;
	size_t m_bytesExpected = 0;

	std::string m_lastMessage = "";
	bool m_isLastMessageEcho = true;
};
