#pragma once
#include <game/server/gamecontroller.h>

class CGameControllerDUCK : public IGameController
{
	void FlipSolidRect(float Rx, float Ry, float Rw, float Rh, bool Solid, bool IsHookable);

	struct ButtonLaserLinePair
	{
		vec2 m_ButtonPos;
		vec2 m_ButtonSize;
		vec2 m_LinePos;
		vec2 m_LineSize;
		bool m_IsButtonActive;
		bool m_LineFlip;
		bool m_IsLineHookable;
	};

	enum { BUTTON_PAIR_COUNT = 4 };
	ButtonLaserLinePair m_aButtonLinePairs[BUTTON_PAIR_COUNT];

public:
	CGameControllerDUCK(class CGameContext *pGameServer);

	virtual void Tick();
	void OnPlayerConnect(class CPlayer *pPlayer);
};
