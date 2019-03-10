#pragma once
#include <game/server/gamecontroller.h>

class CGameControllerDUCK : public IGameController
{
	template<typename T>
	void SendDukNetObj(const T& NetObj, int CID);

	void FlipSolidRect(float Rx, float Ry, float Rw, float Rh, bool Solid);

	struct ButtonLaserLinePair
	{
		vec2 m_ButtonPos;
		vec2 m_ButtonSize;
		vec2 m_LinePos;
		vec2 m_LineSize;
		bool m_IsButtonActive;
		bool m_LineFlip;
	};

	enum { BUTTON_PAIR_COUNT = 1 };
	ButtonLaserLinePair m_aButtonLinePairs[BUTTON_PAIR_COUNT];

public:
	CGameControllerDUCK(class CGameContext *pGameServer);
	virtual void Tick();
	// add more virtual functions here if you wish
};
