#pragma once
#include <cstdint>
#include <string>
#include <vector>

class NetworkAddress {

public:
	uint64_t m_address = 0;
	uint16_t m_port = 0;
	

	std::string ToString() const;
public:
	bool operator==(NetworkAddress const& otherAddress) const;


	static NetworkAddress FromString(std::string const& str);
	static NetworkAddress GetLoopBack(uint16_t port);
	static std::vector<NetworkAddress> GetAllInternal(uint16_t port);
};