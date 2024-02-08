#include "Game/Framework/App.hpp"
#include "Game/Framework/GameCommon.hpp"

#define WIN32_LEAN_AND_MEAN		// Always #define this before #including <windows.h>
#include <windows.h>			// #include this (massive, platform-specific) header in very few places
#include <cmath>
#include <cassert>
#include <crtdbg.h>

App* g_theApp = nullptr;


//-----------------------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE applicationInstanceHandle, HINSTANCE, LPSTR commandLineString, int)
{
	UNUSED(commandLineString);
	UNUSED(applicationInstanceHandle);

	g_theApp = new App();
	g_theApp->Startup();

	g_theApp->RunFrame(); 

	g_theApp->Shutdown();
	delete g_theApp;
	g_theApp = nullptr;

	return 0;
}


