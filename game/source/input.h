#ifndef INPUT_H_INCLUDED
#define INPUT_H_INCLUDED

enum
{
	ButtonMask_Left					= 1u << 0u,
	ButtonMask_Right				= 1u << 1u,
	ButtonMask_Up					= 1u << 2u,
	ButtonMask_Down					= 1u << 3u,
	ButtonMask_PlaceBomb			= 1u << 4u,

	ButtonMask_Player2Left			= 1u << 5u,
	ButtonMask_Player2Right			= 1u << 6u,
	ButtonMask_Player2Up			= 1u << 7u,
	ButtonMask_Player2Down			= 1u << 8u,
	ButtonMask_Player2PlaceBomb		= 1u << 9u,

	ButtonMask_CtrlLeft				= 1u << 10u,
	ButtonMask_CtrlRight			= 1u << 11u,
	ButtonMask_CtrlUp				= 1u << 12u,
	ButtonMask_CtrlDown				= 1u << 13u,

	ButtonMask_Client				= 1u << 14u,
	ButtonMask_Server				= 1u << 15u,
	ButtonMask_Leave				= 1u << 16u,

	Button_PlayerMask				= ( 1u << 5u ) - 1u,
	Button_PlayerShift				= 5u,
};

#endif

