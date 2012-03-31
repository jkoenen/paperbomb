#include "sound.h"
#include "debug.h"

#include <math.h>

typedef struct
{
    uint    samplePos;
    float   currentEngineFrequency;
    float   targetEngineFrequency;
} Sound;

static Sound s_sound;


void sound_init()
{
    s_sound.currentEngineFrequency = 40.0f;
    s_sound.targetEngineFrequency = 40.0f;
    s_sound.samplePos = 0u;
}

void sound_done()
{
}

void sound_setEngineFrequency( float frequency )
{
    s_sound.targetEngineFrequency = frequency;
}

void sound_fillBuffer( float* pBuffer, uint count )
{
    const double sampleRateFactor = 2 * PI / (double)SoundSampleRate;

    double samplePos = s_sound.samplePos;
    double engineFrequency = s_sound.currentEngineFrequency;

    double engineFrequencyShiftSpeed = 1.0f / ( double)SoundSampleRate;

    for( uint i = 0u; i < count; ++i )
    {
        double engineShift = float_clamp( 
            ( s_sound.targetEngineFrequency - engineFrequency ),
            -4.0f * engineFrequencyShiftSpeed,
            engineFrequencyShiftSpeed );
        engineFrequency += engineShift;

        const double x = samplePos * sampleRateFactor;

        // very crude engine sound:
        const double sample0 = cos( 4.0f * x ) * sin( 40.0f * x );
        const double sample1 = cos( 23.0f * x ) * sin( 80.0f * x );

        const double sample = 0.8 * float_lerp( sample0, sample1, engineFrequency );

        // left:
        *pBuffer++ = (float)sample;
        // right:
        *pBuffer++ = (float)sample;

        samplePos += 1.0;
        s_sound.samplePos++;
    }
    s_sound.currentEngineFrequency = engineFrequency;
}

