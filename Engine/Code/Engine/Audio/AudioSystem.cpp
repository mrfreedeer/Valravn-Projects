#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"

//-----------------------------------------------------------------------------------------------
// To disable audio entirely (and remove requirement for fmod.dll / fmod64.dll) for any game,
//	#define ENGINE_DISABLE_AUDIO in your game's Code/Game/EngineBuildPreferences.hpp file.
//
// Note that this #include is an exception to the rule "engine code doesn't know about game code".
//	Purpose: Each game can now direct the engine via #defines to build differently for that game.
//	Downside: ALL games must now have this Code/Game/EngineBuildPreferences.hpp file.
//
// SD1 NOTE: THIS MEANS *EVERY* GAME MUST HAVE AN EngineBuildPreferences.hpp FILE IN ITS CODE/GAME FOLDER!!
#include "Game/EngineBuildPreferences.hpp"
#if !defined( ENGINE_DISABLE_AUDIO )


//-----------------------------------------------------------------------------------------------
// Link in the appropriate FMOD static library (32-bit or 64-bit)
//
#if defined( _WIN64 )
#pragma comment( lib, "ThirdParty/fmod/fmod64_vc.lib" )
#else
#pragma comment( lib, "ThirdParty/fmod/fmod_vc.lib" )
#endif


//-----------------------------------------------------------------------------------------------
// Initialization code based on example from "FMOD Studio Programmers API for Windows"
//
AudioSystem::AudioSystem(AudioSystemConfig const& config)
	: m_fmodSystem(nullptr),
	m_config(config)
{
}


//-----------------------------------------------------------------------------------------------
AudioSystem::~AudioSystem()
{
}


//------------------------------------------------------------------------------------------------
void AudioSystem::Startup()
{
	FMOD_RESULT result;
	result = FMOD::System_Create(&m_fmodSystem);
	ValidateResult(result);

	result = m_fmodSystem->init(512, FMOD_INIT_3D_RIGHTHANDED, nullptr);
	ValidateResult(result);
}


//------------------------------------------------------------------------------------------------
void AudioSystem::Shutdown()
{
	FMOD_RESULT result = m_fmodSystem->release();
	ValidateResult(result);

	m_fmodSystem = nullptr; // #Fixme: do we delete/free the object also, or just do this?
}


//-----------------------------------------------------------------------------------------------
void AudioSystem::BeginFrame()
{
	m_fmodSystem->update();
}


//-----------------------------------------------------------------------------------------------
void AudioSystem::EndFrame()
{
}


void AudioSystem::SetNumListeners(int numListeners)
{
	m_fmodSystem->set3DNumListeners(numListeners);
}

void AudioSystem::UpdateListener(int listenerIndex, Vec3 const& listenerPosition, Vec3 const& listenerVelocity, Vec3 const& listenerForward, Vec3 const& listenerUp)
{
	FMOD_VECTOR position = { -listenerPosition.y, listenerPosition.z, -listenerPosition.x };
	FMOD_VECTOR velocity = { -listenerVelocity.y, listenerVelocity.z, -listenerVelocity.x };
	FMOD_VECTOR forward = { -listenerForward.y, listenerForward.z, -listenerForward.x };
	FMOD_VECTOR up = { -listenerUp.y, listenerUp.z, -listenerUp.x };

	m_fmodSystem->set3DListenerAttributes(listenerIndex, &position, &velocity, &forward, &up);
}

//-----------------------------------------------------------------------------------------------
SoundID AudioSystem::CreateOrGetSound(const std::string& soundFilePath)
{
	std::map< std::string, SoundID >::iterator found = m_registeredSoundIDs.find(soundFilePath);
	if (found != m_registeredSoundIDs.end())
	{
		return found->second;
	}
	else
	{
		FMOD::Sound* newSound = nullptr;
		m_fmodSystem->createSound(soundFilePath.c_str(), FMOD_3D, nullptr, &newSound);
		if (newSound)
		{
			SoundID newSoundID = m_registeredSounds.size();
			m_registeredSoundIDs[soundFilePath] = newSoundID;
			m_registeredSounds.push_back(newSound);
			return newSoundID;
		}
	}

	return MISSING_SOUND_ID;
}


//-----------------------------------------------------------------------------------------------
SoundPlaybackID AudioSystem::StartSound(SoundID soundID, bool isLooped, float volume, float balance, float speed, bool isPaused)
{
	size_t numSounds = m_registeredSounds.size();
	if (soundID < 0 || soundID >= numSounds)
		return MISSING_SOUND_ID;

	FMOD::Sound* sound = m_registeredSounds[soundID];
	if (!sound)
		return MISSING_SOUND_ID;

	FMOD::Channel* channelAssignedToSound = nullptr;
	m_fmodSystem->playSound(sound, nullptr, true, &channelAssignedToSound);
	if (channelAssignedToSound)
	{
		int loopCount = isLooped ? -1 : 0;
		unsigned int playbackMode = isLooped ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;
		playbackMode |= FMOD_2D;
		float frequency;
		channelAssignedToSound->setMode(playbackMode);
		channelAssignedToSound->getFrequency(&frequency);
		channelAssignedToSound->setFrequency(frequency * speed);
		channelAssignedToSound->setVolume(volume);
		channelAssignedToSound->setPan(balance);
		channelAssignedToSound->setLoopCount(loopCount);
		channelAssignedToSound->setPaused(isPaused);
	}

	return (SoundPlaybackID)channelAssignedToSound;
}

//-----------------------------------------------------------------------------------------------
SoundPlaybackID AudioSystem::StartSoundAt(SoundID soundID, Vec3 const& soundPosition, Vec3 const& soundVelocity, bool isLooped, float volume, float balance, float speed, bool isPaused)
{
	size_t numSounds = m_registeredSounds.size();
	if (soundID < 0 || soundID >= numSounds)
		return MISSING_SOUND_ID;

	FMOD::Sound* sound = m_registeredSounds[soundID];
	if (!sound)
		return MISSING_SOUND_ID;

	FMOD::Channel* channelAssignedToSound = nullptr;
	m_fmodSystem->playSound(sound, nullptr, true, &channelAssignedToSound);
	if (channelAssignedToSound)
	{
		int loopCount = isLooped ? -1 : 0;
		unsigned int playbackMode = isLooped ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;
		float frequency;

		FMOD_VECTOR position = { -soundPosition.y, soundPosition.z, -soundPosition.x };
		FMOD_VECTOR velocity = { -soundVelocity.y, soundVelocity.z, -soundVelocity.x };

		channelAssignedToSound->set3DAttributes(&position, &velocity);
		channelAssignedToSound->setMode(playbackMode);
		channelAssignedToSound->getFrequency(&frequency);
		channelAssignedToSound->setFrequency(frequency * speed);
		channelAssignedToSound->setVolume(volume);
		channelAssignedToSound->setPan(balance);
		channelAssignedToSound->setLoopCount(loopCount);
		channelAssignedToSound->setPaused(isPaused);
	}

	return (SoundPlaybackID)channelAssignedToSound;
}


//-----------------------------------------------------------------------------------------------
void AudioSystem::StopSound(SoundPlaybackID soundPlaybackID)
{
	if (soundPlaybackID == MISSING_SOUND_ID)
	{
		ERROR_RECOVERABLE("WARNING: attempt to stop sound on missing sound playback ID!");
		return;
	}

	FMOD::Channel* channelAssignedToSound = (FMOD::Channel*)soundPlaybackID;
	channelAssignedToSound->stop();
}


//-----------------------------------------------------------------------------------------------
// Volume is in [0,1]
//
void AudioSystem::SetSoundPlaybackVolume(SoundPlaybackID soundPlaybackID, float volume)
{
	if (soundPlaybackID == MISSING_SOUND_ID)
	{
		ERROR_RECOVERABLE("WARNING: attempt to set volume on missing sound playback ID!");
		return;
	}

	FMOD::Channel* channelAssignedToSound = (FMOD::Channel*)soundPlaybackID;
	channelAssignedToSound->setVolume(volume);
}


//-----------------------------------------------------------------------------------------------
// Balance is in [-1,1], where 0 is L/R centered
//
void AudioSystem::SetSoundPlaybackBalance(SoundPlaybackID soundPlaybackID, float balance)
{
	if (soundPlaybackID == MISSING_SOUND_ID)
	{
		ERROR_RECOVERABLE("WARNING: attempt to set balance on missing sound playback ID!");
		return;
	}

	FMOD::Channel* channelAssignedToSound = (FMOD::Channel*)soundPlaybackID;
	channelAssignedToSound->setPan(balance);
}


//-----------------------------------------------------------------------------------------------
// Speed is frequency multiplier (1.0 == normal)
//	A speed of 2.0 gives 2x frequency, i.e. exactly one octave higher
//	A speed of 0.5 gives 1/2 frequency, i.e. exactly one octave lower
//
void AudioSystem::SetSoundPlaybackSpeed(SoundPlaybackID soundPlaybackID, float speed)
{
	if (soundPlaybackID == MISSING_SOUND_ID)
	{
		ERROR_RECOVERABLE("WARNING: attempt to set speed on missing sound playback ID!");
		return;
	}

	FMOD::Channel* channelAssignedToSound = (FMOD::Channel*)soundPlaybackID;
	float frequency;
	FMOD::Sound* currentSound = nullptr;
	channelAssignedToSound->getCurrentSound(&currentSound);
	if (!currentSound)
		return;

	int ignored = 0;
	currentSound->getDefaults(&frequency, &ignored);
	channelAssignedToSound->setFrequency(frequency * speed);
}

void AudioSystem::SetSound3DProperties(SoundPlaybackID soundPlaybackID, Vec3 const& soundPosition, Vec3 const& soundVelocity)
{
	if (soundPlaybackID == MISSING_SOUND_ID)
	{
		ERROR_RECOVERABLE("WARNING: attempt to set 3D sound properties sound playback ID!");
		return;
	}
	FMOD_VECTOR position = { -soundPosition.y, soundPosition.z, -soundPosition.x };
	FMOD_VECTOR velocity = { -soundVelocity.y, soundVelocity.z, -soundVelocity.x };

	FMOD::Channel* channelAssignedToSound = (FMOD::Channel*)soundPlaybackID;
	channelAssignedToSound->set3DAttributes(&position, &velocity);

}


//-----------------------------------------------------------------------------------------------
void AudioSystem::ValidateResult(FMOD_RESULT result)
{
	if (result != FMOD_OK)
	{
		ERROR_RECOVERABLE(Stringf("Engine/Audio SYSTEM ERROR: Got error result code %i - error codes listed in fmod_common.h\n", (int)result));
	}
}

void AudioSystem::SetPause(SoundPlaybackID soundID, bool isPaused)
{
	FMOD::Channel* channelAssignedToSound = (FMOD::Channel*)soundID;
	channelAssignedToSound->setPaused(isPaused);
}


#endif // !defined( ENGINE_DISABLE_AUDIO )
