//-----------------------------------------------------------------------------------------------
// ErrorWarningAssert.cpp
//

//-----------------------------------------------------------------------------------------------
#ifdef _WIN32
#define PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

//-----------------------------------------------------------------------------------------------
#include "ErrorWarningAssert.hpp"
#include <stdarg.h>
#include <iostream>


//-----------------------------------------------------------------------------------------------
bool IsDebuggerAvailable()
{
#if defined( PLATFORM_WINDOWS )
	typedef BOOL (CALLBACK IsDebuggerPresentFunc)();

	// Get a handle to KERNEL32.DLL
	static HINSTANCE hInstanceKernel32 = GetModuleHandle( TEXT( "KERNEL32" ) );
	if( !hInstanceKernel32 )
		return false;

	// Get a handle to the IsDebuggerPresent() function in KERNEL32.DLL
	static IsDebuggerPresentFunc* isDebuggerPresentFunc = (IsDebuggerPresentFunc*) GetProcAddress( hInstanceKernel32, "IsDebuggerPresent" );
	if( !isDebuggerPresentFunc )
		return false;

	// Now CALL that function and return its result
	static BOOL isDebuggerAvailable = isDebuggerPresentFunc();
	return( isDebuggerAvailable == TRUE );
#else
	return false;
#endif
}


//-----------------------------------------------------------------------------------------------
void DebuggerPrintf( char const* messageFormat, ... )
{
	const int MESSAGE_MAX_LENGTH = 2048;
	char messageLiteral[ MESSAGE_MAX_LENGTH ];
	va_list variableArgumentList;
	va_start( variableArgumentList, messageFormat );
	vsnprintf_s( messageLiteral, MESSAGE_MAX_LENGTH, _TRUNCATE, messageFormat, variableArgumentList );
	va_end( variableArgumentList );
	messageLiteral[ MESSAGE_MAX_LENGTH - 1 ] = '\0'; // In case vsnprintf overran (doesn't auto-terminate)

#if defined( PLATFORM_WINDOWS )
	if( IsDebuggerAvailable() )
	{
		OutputDebugStringA( messageLiteral );
	}
#endif

	std::cout << messageLiteral;
}


//-----------------------------------------------------------------------------------------------
// Converts a SeverityLevel to a Windows MessageBox icon type (MB_etc)
//
#if defined( PLATFORM_WINDOWS )
UINT GetWindowsMessageBoxIconFlagForSeverityLevel( MsgSeverityLevel severity )
{
	switch( severity )
	{
		case MsgSeverityLevel::INFORMATION:		return MB_ICONASTERISK;		// blue circle with 'i' in Windows 7
		case MsgSeverityLevel::QUESTION:		return MB_ICONQUESTION;		// blue circle with '?' in Windows 7
		case MsgSeverityLevel::WARNING:			return MB_ICONEXCLAMATION;	// yellow triangle with '!' in Windows 7
		case MsgSeverityLevel::FATAL:			return MB_ICONHAND;			// red circle with 'x' in Windows 7
		default:								return MB_ICONEXCLAMATION;
	}
}
#endif


//-----------------------------------------------------------------------------------------------
char const* FindStartOfFileNameWithinFilePath( char const* filePath )
{
	if( filePath == nullptr )
		return nullptr;

	size_t pathLen = strlen( filePath );
	char const* scan = filePath + pathLen; // start with null terminator after last character
	while( scan > filePath )
	{
		-- scan;

		if( *scan == '/' || *scan == '\\' )
		{
			++ scan;
			break;
		}
	}

	return scan;
}


//-----------------------------------------------------------------------------------------------
void SystemDialogue_Okay( std::string const& messageTitle, std::string const& messageText, MsgSeverityLevel severity )
{
	#if defined( PLATFORM_WINDOWS )
	{
		ShowCursor( TRUE );
		UINT dialogueIconTypeFlag = GetWindowsMessageBoxIconFlagForSeverityLevel( severity );
		MessageBoxA( NULL, messageText.c_str(), messageTitle.c_str(), MB_OK | dialogueIconTypeFlag | MB_TOPMOST );
		ShowCursor( FALSE );
	}
	#endif
}


//-----------------------------------------------------------------------------------------------
// Returns true if OKAY was chosen, false if CANCEL was chosen.
//
bool SystemDialogue_OkayCancel( std::string const& messageTitle, std::string const& messageText, MsgSeverityLevel severity )
{
	bool isAnswerOkay = true;

	#if defined( PLATFORM_WINDOWS )
	{
		ShowCursor( TRUE );
		UINT dialogueIconTypeFlag = GetWindowsMessageBoxIconFlagForSeverityLevel( severity );
		int buttonClicked = MessageBoxA( NULL, messageText.c_str(), messageTitle.c_str(), MB_OKCANCEL | dialogueIconTypeFlag | MB_TOPMOST );
		isAnswerOkay = (buttonClicked == IDOK);
		ShowCursor( FALSE );
	}
	#endif

	return isAnswerOkay;
}


//-----------------------------------------------------------------------------------------------
// Returns true if YES was chosen, false if NO was chosen.
//
bool SystemDialogue_YesNo( std::string const& messageTitle, std::string const& messageText, MsgSeverityLevel severity )
{
	bool isAnswerYes = true;

	#if defined( PLATFORM_WINDOWS )
	{
		ShowCursor( TRUE );
		UINT dialogueIconTypeFlag = GetWindowsMessageBoxIconFlagForSeverityLevel( severity );
		int buttonClicked = MessageBoxA( NULL, messageText.c_str(), messageTitle.c_str(), MB_YESNO | dialogueIconTypeFlag | MB_TOPMOST );
		isAnswerYes = (buttonClicked == IDYES);
		ShowCursor( FALSE );
	}
	#endif

	return isAnswerYes;
}


//-----------------------------------------------------------------------------------------------
// Returns 1 if YES was chosen, 0 if NO was chosen, -1 if CANCEL was chosen.
//
int SystemDialogue_YesNoCancel( std::string const& messageTitle, std::string const& messageText, MsgSeverityLevel severity )
{
	int answerCode = 1;

	#if defined( PLATFORM_WINDOWS )
	{
		ShowCursor( TRUE );
		UINT dialogueIconTypeFlag = GetWindowsMessageBoxIconFlagForSeverityLevel( severity );
		int buttonClicked = MessageBoxA( NULL, messageText.c_str(), messageTitle.c_str(), MB_YESNOCANCEL | dialogueIconTypeFlag | MB_TOPMOST );
		answerCode = (buttonClicked == IDYES ? 1 : (buttonClicked == IDNO ? 0 : -1) );
		ShowCursor( FALSE );
	}
	#endif

	return answerCode;
}


