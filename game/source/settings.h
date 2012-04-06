#ifndef SETTINGS_H_INCLUDED
#define SETTINGS_H_INCLUDED

enum
{
	MaxPlayer		= 4u,
	MaxBombs		= 16u,
	StartBombs		= 2u,
	MaxExplosions	= 16u,
	MaxItems		= 4u,
	NetworkPort		= 2357u,
};	

static const float s_bombTime			= 2.0f;
static const float s_explosionTime		= 1.0f;
static const float s_itemMinTime		= 2.0f;
static const float s_itemMaxTime		= 3.0f; 
static const float s_carRadius			= 1.5f;
static const float s_itemRadius			= 1.0f;
static const float s_bombRadius			= 1.0f;
static const float s_itemBombLength		= 2.0f;
static const float s_startBombLength	= 6.0f;
static const float s_explosionRadius	= 1.0f;
static const float s_bombCarOffset		= 2.0f;
static const float s_burnRadius			= 2.0f;

#endif

