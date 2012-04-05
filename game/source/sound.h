#ifndef SOUND_H_INCLUDED
#define SOUND_H_INCLUDED

#include "types.h"

enum 
{
    SoundBufferSampleCount = 16384,
	SoundBufferSampleHalfCount = SoundBufferSampleCount / 2u,
    SoundSampleRate = 44100,
    SoundChannelCount = 2,
	SoundSampleSize = 2
};

void sound_init();
void sound_done();

void sound_fillBuffer( float2* pBuffer, uint count );

void sound_setEngineFrequency( float frequency );

#endif

