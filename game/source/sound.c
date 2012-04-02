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

static inline void pan( float* pLeft, float* pRight, float sample, float panning )
{
    *pLeft = sample * sqrt( 1.0f - panning );
    *pRight = sample * sqrt( panning );
}

void sound_fillBuffer( float* pBuffer, uint count )
{
    const double sampleRateFactor = 2 * PI / (double)SoundSampleRate;

    double samplePos = s_sound.samplePos;
    double engineFrequency = s_sound.currentEngineFrequency;

    double engineFrequencyShiftSpeed = 1.0f / ( double)SoundSampleRate;

    for( uint i = 0u; i < count; ++i )
    {
        double engineShift = double_clamp( 
            ( s_sound.targetEngineFrequency - engineFrequency ),
            -4.0f * engineFrequencyShiftSpeed,
            engineFrequencyShiftSpeed );
        engineFrequency += engineShift;

        const double x = samplePos * sampleRateFactor;

        // very crude engine sound:
        const double sample0 = cos( 4.0f * x ) * sin( 120.0f * x );
        const double sample1 = cos( 23.0f * x ) * sin( 80.0f * x );

        const double sample = double_lerp( sample0, sample1, engineFrequency );
        
        const float panning = fabsf( 0.1f * cosf( (float)x ) );

        float left, right;
        pan( &left, &right, (float)sample, panning );

        // left:
        *pBuffer++ = left;
        // right:
        *pBuffer++ = right;

        samplePos += 1.0;
        s_sound.samplePos++;
    }
    s_sound.currentEngineFrequency = (float)engineFrequency;
}

