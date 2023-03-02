#include "Engine/Network/RemoteConsole.hpp"
#include "Engine/Network/TCPConnection.hpp"
#include "Engine/Network/NetworkAddress.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Network/TCPServer.hpp"
#include "Engine/Network/NetworkCommon.hpp"

RemoteConsole* currentRemoteConsole = nullptr;


std::vector<std::string> RemoteConsole::s_invalidRCCmds = { "rckick", "rcleave", "rca", "rchost", "rcjoin" };

RemoteConsole::RemoteConsole(RemoteConsoleConfig const& config) :
	m_config(config),
	m_devConsole(config.m_console)
{
	currentRemoteConsole = this;

}

void RemoteConsole::Startup()
{
	m_state = RemoteConsoleState::DISCONNECTED;

	SubscribeEventCallbackFunction("RCJoin", RCJoin);
	SubscribeEventCallbackFunction("RCHost", RCHost);
	SubscribeEventCallbackFunction("RC", RPC);
	SubscribeEventCallbackFunction("RCA", RCA);
	SubscribeEventCallbackFunction("RCEcho", RCEcho);
	SubscribeEventCallbackFunction("RCKick", RCKick);
	SubscribeEventCallbackFunction("RCLeave", RCLeave);
	SubscribeEventCallbackFunction("RCBan", RCBan);





	//AttemptToHost(3121);

	TryToJoin(NetworkAddress::FromString("127.0.0.1:3121"));

	if (m_state == RemoteConsoleState::CLIENT) {
		m_initialHostAttempt = true;
	}
}

void RemoteConsole::Shutdown()
{
	UnsubscribeEventCallbackFunction("RCJoin", RCJoin);
	UnsubscribeEventCallbackFunction("RCHost", RCHost);
	UnsubscribeEventCallbackFunction("RC", RPC);
	UnsubscribeEventCallbackFunction("RCA", RCA);
	UnsubscribeEventCallbackFunction("RCEcho", RCEcho);
	UnsubscribeEventCallbackFunction("RCKick", RCKick);
	UnsubscribeEventCallbackFunction("RCLeave", RCLeave);
	UnsubscribeEventCallbackFunction("RCBan", RCBan);

}

void RemoteConsole::TryToHost(uint16_t port)
{
	m_hostPort = port;

	TCPServer* server = new TCPServer();
	m_state = RemoteConsoleState::ATTEMPTING_HOST;

	uint16_t currentHostPort = port + m_hostAttempt;

	ClearConnections();

	if (server->Host(currentHostPort)) {
		server->SetBlocking(false);
		m_server = server;
		m_state = RemoteConsoleState::HOST;
		m_devConsole->AddLine(DevConsole::INFO_MAJOR_COLOR, Stringf("Hosting on port: %u", currentHostPort));
		return;
	}
	else {
		m_devConsole->AddLine(DevConsole::WARNING_COLOR, Stringf("Failed to host on port: %u", currentHostPort));
		m_hostAttempt++;
		delete server;
	}
}

void RemoteConsole::TryToJoin(NetworkAddress const& hostAddr)
{
	TCPConnection* connection = new TCPConnection();
	if (connection->Connect(hostAddr)) {
		connection->SetBlocking(false);
		m_connections.push_back(connection);
		m_state = RemoteConsoleState::CLIENT;
	}
	else {
		bool isErrorFatal = connection->CheckForFatalError();

		if (isErrorFatal) {
			delete connection;
			m_state = RemoteConsoleState::DISCONNECTED;
			m_devConsole->AddLine(DevConsole::ERROR_COLOR, Stringf("Failed to join on address %s", hostAddr.ToString().c_str()));
		}
		else {
			m_connections.push_back(connection);
			m_state = RemoteConsoleState::JOINING;
			m_devConsole->AddLine(DevConsole::INFO_MINOR_COLOR, Stringf("Trying to join: %s", hostAddr.ToString().c_str()));
		}

	}
}

void RemoteConsole::Update()
{
	ProcessConnections();

	if (!m_initialHostAttempt && m_state == RemoteConsoleState::DISCONNECTED) {
		TryToHost(3121);
		m_initialHostAttempt = true;
	}

	if (m_state == RemoteConsoleState::ATTEMPTING_HOST) {
		TryToHost(m_hostPort);
		if (m_hostAttempt >= 32) {
			m_state = RemoteConsoleState::DISCONNECTED;
		}
	}

}

void RemoteConsole::Render(Renderer const& renderer)
{
	UNUSED(renderer);
}

void RemoteConsole::ProcessConnections()
{
	if (m_state == RemoteConsoleState::DISCONNECTED) return;

	if (m_server && m_state == RemoteConsoleState::HOST) {
		TCPConnection* cp = m_server->Accept();
		if (cp) {
			auto addressIt = std::find(m_blacklistAddresses.begin(), m_blacklistAddresses.end(), cp->m_address);
			if (addressIt == m_blacklistAddresses.end()) {
				m_connections.push_back(cp);
			}
			else {
				cp->Close();
				delete cp;

			}
		}
	}

	// process connections

	int index = 0;
	for (std::vector<TCPConnection*>::iterator it = m_connections.begin(); it != m_connections.end(); index++) {

		TCPConnection* connection = *it;

		bool isConnected = connection->CheckForConnection();

		if (isConnected && m_state == RemoteConsoleState::JOINING) {
			m_state = RemoteConsoleState::CLIENT;
		}

		if (isConnected) {
			size_t received = connection->ReceiveFull();
			if (received > 0) {
				bool isEcho = connection->IsMessageEcho();
				std::string msg = connection->GetLastMessage();

				EventArgs args;
				args.SetValue("msg", msg);
				args.SetValue("cmd", msg);
				args.SetValue("isReceiving", "true");
				args.SetValue("idx", std::to_string(index));
				if (isEcho) {
					args.SetValue("addr", connection->m_address.ToString());
					FireEvent("RCEcho", args);
				}
				else {
					FireEvent("RC", args);
				}

			}

		}
		if (connection->IsClosed()) {
			if (m_state == RemoteConsoleState::CLIENT) {
				m_devConsole->AddLine(DevConsole::WARNING_COLOR, "Connection terminated, returning to disconnected state");
				m_state = RemoteConsoleState::DISCONNECTED;
			}
			else if (m_state == RemoteConsoleState::JOINING) {
				m_devConsole->AddLine(DevConsole::ERROR_COLOR, "Failed to connect");
				m_state = RemoteConsoleState::DISCONNECTED;
			}
			it = m_connections.erase(it);
		}
		else {
			it++;
		}
	}
}

void RemoteConsole::SendCommand(int connIndex, std::string const& cmd)
{
	if (connIndex < 0 || connIndex >= m_connections.size()) {
		return;
	}

	TCPConnection* conn = m_connections[connIndex];

	//unsigned char buffer[512];
	size_t payloadSize = 1 // Echo
		+ 2 // cmd size with null
		+ cmd.size() + 1;

	//size_t packetSize = 2 + payloadSize;

	RDCHeader header;
	header.m_payloadSize = ::htons((u_short)payloadSize);

	RDCPayload payload;
	payload.m_isEcho = false;
	payload.m_messageSize = ::htons((uint16_t)(cmd.size() + 1));
	conn->Send(&header, sizeof(header));
	conn->Send(&payload, sizeof(payload));
	conn->Send(&cmd[0], cmd.size() + 1);


}

void RemoteConsole::SendCommand(std::string const& cmd)
{
	for (int connIndex = 0; connIndex < m_connections.size(); connIndex++) {
		SendCommand(connIndex, cmd);
	}

}

void RemoteConsole::SendEcho(int connIndex, std::string const& echo)
{
	if (connIndex < 0 || connIndex >= m_connections.size()) {
		return;
	}

	TCPConnection* conn = m_connections[connIndex];

	//unsigned char buffer[512];
	size_t payloadSize = 1 // Echo
		+ 2 // message size with null
		+ echo.size() + 1;

	//size_t packetSize = 2 + payloadSize;

	RDCHeader header;
	header.m_payloadSize = ::htons((u_short)payloadSize);

	RDCPayload payload;
	payload.m_isEcho = true;
	payload.m_messageSize = ::htons((uint16_t)(echo.size() + 1));
	conn->Send(&header, sizeof(header));
	conn->Send(&payload, sizeof(payload));

	conn->Send(&echo[0], echo.size() + 1);
}

void RemoteConsole::SendEcho(std::string const& echo)
{
	for (int connIndex = 0; connIndex < m_connections.size(); connIndex++) {
		SendEcho(connIndex, echo);
	}
}

void RemoteConsole::KillConnection(NetworkAddress const& netAddress)
{

	for (std::vector<TCPConnection*>::iterator addressIt = m_connections.begin(); addressIt != m_connections.end(); addressIt++) {
		TCPConnection* connection = *addressIt;
		if (connection->m_address == netAddress) {
			connection->Close();
			m_connections.erase(addressIt);
			return;
		}

	}

}

void RemoteConsole::KillConnection(int connIndex)
{
	if (connIndex < 0 || connIndex > m_connections.size()) return;
	TCPConnection* connection = m_connections[connIndex];
	connection->Close();
	m_connections.erase(m_connections.begin() + connIndex);

}

void RemoteConsole::BanConnection(int connIndex)
{
	if (connIndex < 0 || connIndex >= m_connections.size()) return;
	NetworkAddress bannedAddress = m_connections[connIndex]->m_address;

	m_blacklistAddresses.push_back(bannedAddress);
	KillConnection(connIndex);
}

void RemoteConsole::Quit()
{
	if (m_state != RemoteConsoleState::CLIENT) return;

	ClearConnections();

	m_devConsole->AddLine(DevConsole::WARNING_COLOR, "Quit all connections. Back to Disconnected State");
	m_state = RemoteConsoleState::DISCONNECTED;

	m_connections.resize(0);

}

std::string RemoteConsole::GetState() const
{
	switch (m_state)
	{
	case RemoteConsoleState::CLIENT: {
		return "CLIENT: ";
		break;
	}
	case RemoteConsoleState::HOST:
	{
		std::string allInteral = NetworkAddress::GetAllInternal(m_hostPort)[0].ToString();
		allInteral += "\n";
		return "HOSTING: " + allInteral;
		break;
	}
	default:
		return "DISCONNECTED";
		break;
	}

}

std::string RemoteConsole::GetConnectionInfo() const
{
	std::string connInfo;
	for (int connInd = 0; connInd < m_connections.size(); connInd++) {
		TCPConnection* conn = m_connections[connInd];
		connInfo += "[" + Stringf("%d", connInd);
		connInfo += "]: " + conn->m_address.ToString();
		std::string lastMessage = conn->GetLastMessage();
		if (!lastMessage.empty()) {
			connInfo += " => ";
			connInfo += (conn->IsMessageEcho())? "Echo: " : "Cmd: ";
			connInfo += lastMessage;
		}
		connInfo += '\n';
	}
	return connInfo;
}

bool RemoteConsole::RCJoin(EventArgs& args)
{
	if (!currentRemoteConsole) return true;

	std::string hostAddressStr = args.GetValue("addr", "127.0.0.1:3121");

	NetworkAddress hostAddress = NetworkAddress::FromString(hostAddressStr);

	currentRemoteConsole->TryToJoin(hostAddress);

	return true;
}

bool RemoteConsole::RCHost(EventArgs& args)
{
	if (!currentRemoteConsole) return true;

	int port = args.GetValue("port", 3121);

	currentRemoteConsole->TryToHost((uint16_t)port);

	return true;
}

bool RemoteConsole::RPC(EventArgs& args)
{
	if (!currentRemoteConsole) return true;

	std::string command = args.GetValue("cmd", "");
	int connectionIndex = args.GetValue("idx", 0);
	bool isReceiving = args.GetValue("isReceiving", false);


	if (command.empty()) return true;

	Strings commandSplitBySpace = SplitStringOnDelimiter(command, ' ');

	if (commandSplitBySpace.size() == 0)return true;
	std::string commandName = SplitStringOnDelimiter(command, ' ')[0];

	for (int bannedCmdInd = 0; bannedCmdInd < s_invalidRCCmds.size(); bannedCmdInd++) {
		if (AreStringsEqualCaseInsensitive(commandName, s_invalidRCCmds[bannedCmdInd])) {
			if (isReceiving) {
				std::string commandExecution = "Detected ban command: " + commandName;
				currentRemoteConsole->SendEcho(connectionIndex, commandExecution);

				return true;
			}
		}
	}




	if (isReceiving) {
		bool executedCmd = currentRemoteConsole->m_devConsole->Execute(command);
		if (executedCmd) {
			std::string commandExecution = "Executing command: " + commandName;
			currentRemoteConsole->SendEcho(connectionIndex, commandExecution);
		}
		else {
			std::string commandExecution = "Command not recognized: " + commandName;

			currentRemoteConsole->SendEcho(connectionIndex, commandExecution);
		}
	}
	else {
		currentRemoteConsole->SendCommand(connectionIndex, command);
	}

	return true;
}

bool RemoteConsole::RCA(EventArgs& args)
{
	if (!currentRemoteConsole) return true;

	std::string command = args.GetValue("cmd", "");

	if (command.empty()) return true;

	Strings commandSplitBySpace = SplitStringOnDelimiter(command, ' ');

	if (commandSplitBySpace.size() == 0)return true;
	std::string commandName = SplitStringOnDelimiter(command, ' ')[0];

	for (int bannedCmdInd = 0; bannedCmdInd < s_invalidRCCmds.size(); bannedCmdInd++) {
		if (AreStringsEqualCaseInsensitive(commandName, s_invalidRCCmds[bannedCmdInd])) return true;
	}

	currentRemoteConsole->SendCommand(command);

	return true;
}

bool RemoteConsole::RCEcho(EventArgs& args)
{
	if (!currentRemoteConsole) return true;

	std::string message = args.GetValue("msg", "");
	int connectionIndex = args.GetValue("idx", 0);
	bool isReceiving = args.GetValue("isReceiving", false);
	bool isBroadCast = args.GetValue("broadcast", false);
	std::string networkAddr = args.GetValue("addr", "localhost");

	if (message.empty()) return true;

	if (isReceiving) {
		std::string fullMessage = Stringf("[Echo: %s]: ", networkAddr.c_str());
		fullMessage += message;
		currentRemoteConsole->m_devConsole->AddLine(DevConsole::INFO_MINOR_COLOR, fullMessage);
	}
	else {
		if (isBroadCast) {
			currentRemoteConsole->SendEcho(message);
		}
		else {
			currentRemoteConsole->SendEcho(connectionIndex, message);
		}
	}

	return true;
}

bool RemoteConsole::RCKick(EventArgs& args)
{
	if (!currentRemoteConsole) return true;

	int kickInd = args.GetValue("idx", 0);

	currentRemoteConsole->BanConnection(kickInd);

	return true;
}

bool RemoteConsole::RCLeave(EventArgs& args)
{
	UNUSED(args);

	if (!currentRemoteConsole) return true;

	currentRemoteConsole->Quit();


	return false;
}

bool RemoteConsole::RCBan(EventArgs& args)
{
	std::string cmdName = args.GetValue("cmd", "");

	if (cmdName.empty()) return true;

	if (!currentRemoteConsole) return true;

	s_invalidRCCmds.push_back(cmdName);

	currentRemoteConsole->m_devConsole->AddLine(DevConsole::INFO_MAJOR_COLOR, Stringf("Banned command: %s", cmdName.c_str()));

	return false;
}

void RemoteConsole::ClearConnections()
{
	for (int connInd = 0; connInd < m_connections.size(); connInd++) {
		TCPConnection*& conn = m_connections[connInd];
		if (conn) {
			conn->Close();
			delete conn;
			conn = nullptr;
		}
	}
}


