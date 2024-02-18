#include "Engine/Network/Network.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Network/NetworkCommon.hpp"

NetworkSystem* g_theNetwork = nullptr;

NetworkSystem::NetworkSystem(NetworkSystemConfig const& sysConfig):
	m_config(sysConfig)
{
}

NetworkSystem::~NetworkSystem()
{
}

void NetworkSystem::Startup()
{	
	WORD version = MAKEWORD(2,2);
	WSADATA data;

	int errorCode = ::WSAStartup(version, &data);
	GUARANTEE_OR_DIE(errorCode == 0, "COULD NOT INITIALIZE THE NETWORK SYSTEM");
}

void NetworkSystem::BeginFrame()
{
}

void NetworkSystem::EndFrame()
{
}

void NetworkSystem::Shutdown()
{
	::WSACleanup();	
}

