#include "Engine/Network/NetworkAddress.hpp"
#include "Engine/Network/NetworkCommon.hpp"
#include "Engine/Core/StringUtils.hpp"
#include <WS2tcpip.h>

std::string NetworkAddress::ToString() const
{
	return Stringf("%u.%u.%u.%u:%u", 
		(m_address & 0xff000000) >> 24,
		(m_address & 0x00ff0000) >> 16,
		(m_address & 0x0000ff00) >> 8,
		(m_address & 0x000000ff) >> 0,
		m_port
		);
}

bool NetworkAddress::operator==(NetworkAddress const& otherAddress) const
{
	return (m_address == otherAddress.m_address);
}

NetworkAddress NetworkAddress::FromString(std::string const& str)
{
	Strings parts = SplitStringOnDelimiter(str, ':');

	IN_ADDR addr;
	int result = ::inet_pton(AF_INET, parts[0].c_str(), &addr);
	
	if (result == SOCKET_ERROR) {
		return NetworkAddress();
	}

	uint16_t port = (uint16_t) ::atoi(parts[1].c_str());
	
	NetworkAddress address;

	address.m_address = ::ntohl(addr.S_un.S_addr);
	address.m_port = port;

	return address;
	
}

NetworkAddress NetworkAddress::GetLoopBack(uint16_t port)
{
	NetworkAddress addr;
	addr.m_address = 0x7f000001; // 127 0 0 1
	addr.m_port = port;
	return addr;
}

std::vector<NetworkAddress> NetworkAddress::GetAllInternal(uint16_t port)
{
	std::vector<NetworkAddress> internalAddresses;
	char hostName[256];
	int result = ::gethostname(hostName, sizeof(hostName));

	if (result == SOCKET_ERROR) {
		return internalAddresses;
	}

	addrinfo hints = {};
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	
	addrinfo* addresses = nullptr;
	int status = ::getaddrinfo(hostName, nullptr, &hints, &addresses);

	if (status != 0) {
		return internalAddresses;
	}

	addrinfo* iter = addresses;
	while (iter != nullptr) {
		NetworkAddress newInternal;
		sockaddr_in* ipv4 = (sockaddr_in*) iter->ai_addr;
		newInternal.m_address = ::ntohl(ipv4->sin_addr.S_un.S_addr);
		newInternal.m_port = port;

		internalAddresses.push_back(newInternal);

		iter = iter->ai_next;

	}

	return internalAddresses;
}
