#pragma once

struct NetworkSystemConfig {

};


class NetworkSystem {
public:
	NetworkSystem(NetworkSystemConfig const& soysConfig);
	~NetworkSystem();

	void Startup();
	void BeginFrame();
	void EndFrame();
	void Shutdown();


private:
	NetworkSystemConfig m_config;
};
