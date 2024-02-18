#pragma once
#include "Engine/Network/TCPSocket.hpp"
#include "Engine/Network/NetworkAddress.hpp"

class TCPConnection;

class TCPServer : public TCPSocket {

public:
	bool Host(uint16_t service, uint32_t backlog = 16);
	TCPConnection* Accept();

	NetworkAddress m_address;
};