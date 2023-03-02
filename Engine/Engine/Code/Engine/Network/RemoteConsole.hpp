#pragma once
#include <vector>
#include <string>
#include "Engine/Core/EventSystem.hpp"

class DevConsole;
class Renderer;
class TCPConnection;
class TCPServer;

class NetworkAddress;

struct RDCHeader {
	uint16_t m_payloadSize;
};

#pragma pack(push,1)
struct RDCPayload{
	uint8_t m_isEcho;
	uint16_t m_messageSize;
};
#pragma pack(pop)

struct RemoteConsoleConfig {
	DevConsole* m_console = nullptr;
};
enum class RemoteConsoleState {
	DISCONNECTED,
	JOINING,
	CLIENT,
	ATTEMPTING_HOST,
	HOST
};

class RemoteConsole {
public:
	RemoteConsole(RemoteConsoleConfig const& config);

	void Startup();
	void Shutdown();

	void TryToHost(uint16_t port);
	void TryToJoin(NetworkAddress const& hostAddr);

	void Update();
	void Render(Renderer const& renderer);
	void ProcessConnections();
	void SendCommand(int connIndex, std::string const& cmd);
	void SendCommand(std::string const& cmd);
	void SendEcho(int connIndex, std::string const& echo);
	void SendEcho(std::string const& echo);

	void KillConnection(NetworkAddress const& netAddress);
	void KillConnection(int connIndex);
	void BanConnection(int connIndex);
	void Quit();
	std::string GetState() const;
	std::string GetConnectionInfo() const;

	static bool RCJoin(EventArgs& args);
	static bool RCHost(EventArgs& args);
	static bool RPC(EventArgs& args);
	static bool RCA(EventArgs& args);
	static bool RCEcho(EventArgs& args);
	static bool RCKick(EventArgs& args);
	static bool RCLeave(EventArgs& args);
	static bool RCBan(EventArgs& args);

public:
	DevConsole* m_devConsole = nullptr;

private:
	void ClearConnections();

	RemoteConsoleConfig m_config;
	std::vector<TCPConnection*> m_connections;
	TCPServer* m_server = nullptr;
	uint16_t m_hostAttempt = 0;
	uint16_t m_hostPort = 3121;

	bool m_initialHostAttempt = false;

	RemoteConsoleState m_state = RemoteConsoleState::DISCONNECTED;
	std::vector<NetworkAddress> m_blacklistAddresses;
	static std::vector<std::string> s_invalidRCCmds;
};