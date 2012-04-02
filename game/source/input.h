#ifndef INPUT_H_INCLUDED
#define INPUT_H_INCLUDED

enum
{
	ButtonMask_Left			= 1u << 0u,
	ButtonMask_Right		= 1u << 1u,
	ButtonMask_Up			= 1u << 2u,
	ButtonMask_Down			= 1u << 3u,
	ButtonMask_PlaceBomb	= 1u << 4u,

	ButtonMask_CtrlLeft		= 1u << 5u,
	ButtonMask_CtrlRight	= 1u << 6u,
	ButtonMask_CtrlUp		= 1u << 7u,
	ButtonMask_CtrlDown		= 1u << 8u
};

#endif

