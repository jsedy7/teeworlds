#pragma once

#include "gameworld.h"
#include <game/duck_gamecore.h>

struct CDuckGameWorld : CGameWorld
{
	CDuckWorldCore m_DuckCore;

	void SetGameServer(CGameContext *pGameServer); // override
	void Reset(); // override
	void Tick(); // override
	void Snap(int SnappingClient); // override
};
