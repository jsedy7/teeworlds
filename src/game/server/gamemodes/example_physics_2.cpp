#include "example_physics_2.h"
#include <stdint.h>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/server/entities/character.h>
#include <game/server/player.h>

typedef uint32_t u32;
typedef int32_t i32;

struct CNetObj_DebugRect
{
	enum { NET_ID = 0x1 };

	int id;
	float x;
	float y;
	float w;
	float h;
	u32 color;
};

struct CNetObj_MapRectSetSolid
{
	enum { NET_ID = 0x2 };

	i32 solid;
	i32 hookable;
	i32 x;
	i32 y;
	i32 w;
	i32 h;
};

inline bool IsInsideRect(vec2 Pos, vec2 RectPos, vec2 RectSize)
{
	return (Pos.x >= RectPos.x && Pos.x < (RectPos.x+RectSize.x) &&
			Pos.y >= RectPos.y && Pos.y < (RectPos.y+RectSize.y));
}

void CGameControllerExamplePhys2::FlipSolidRect(float Rx, float Ry, float Rw, float Rh, bool Solid, bool IsHookable)
{
	CNetObj_MapRectSetSolid Flip;
	Flip.solid = Solid;
	Flip.hookable = IsHookable;
	Flip.x = Rx;
	Flip.y = Ry;
	Flip.w = Rw;
	Flip.h = Rh;

	CDuckCollision* pCollision = GameServer()->Collision();

	if(Solid)
	{
		for(int y = 0; y < Flip.h; y++)
		{
			for(int x = 0; x < Flip.w; x++)
			{
				pCollision->SetTileCollisionFlags(Flip.x + x, Flip.y + y, CCollision::COLFLAG_SOLID | (CCollision::COLFLAG_NOHOOK * !IsHookable));
			}
		}
	}
	else
	{
		for(int y = 0; y < Flip.h; y++)
		{
			for(int x = 0; x < Flip.w; x++)
			{
				pCollision->SetTileCollisionFlags(Flip.x + x, Flip.y + y, 0);
			}
		}
	}

	for(int i = 0; i < MAX_PLAYERS; i++)
	{
		if(GameServer()->m_apPlayers[i])
		{
			GameServer()->SendDuckNetObj(Flip, i);
		}
	}
}

CGameControllerExamplePhys2::CGameControllerExamplePhys2(class CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "EXPHYS2";
	str_copy(g_Config.m_SvMap, "phys2", sizeof(g_Config.m_SvMap)); // force dm1

	str_copy(m_aDuckMod, "example_physics_2", sizeof(m_aDuckMod));

	//m_GameFlags = GAMEFLAG_TEAMS; // GAMEFLAG_TEAMS makes it a two-team gamemode

	ButtonLaserLinePair Pair;
	Pair.m_ButtonPos = vec2(2, 43);
	Pair.m_ButtonSize = vec2(2, 1);
	Pair.m_LinePos = vec2(23, 44);
	Pair.m_LineSize = vec2(40, 1);
	Pair.m_IsButtonActive = false;
	Pair.m_LineFlip = false;
	Pair.m_IsLineHookable = true;
	m_aButtonLinePairs[0] = Pair;

	Pair.m_ButtonPos = vec2(17, 30);
	Pair.m_ButtonSize = vec2(2, 1);
	Pair.m_LinePos = vec2(2, 33);
	Pair.m_LineSize = vec2(11, 1);
	Pair.m_IsButtonActive = false;
	Pair.m_LineFlip = true;
	Pair.m_IsLineHookable = false;
	m_aButtonLinePairs[1] = Pair;

	Pair.m_ButtonPos = vec2(64, 10);
	Pair.m_ButtonSize = vec2(2, 1);
	Pair.m_LinePos = vec2(87, 14);
	Pair.m_LineSize = vec2(4, 10);
	Pair.m_IsButtonActive = false;
	Pair.m_LineFlip = false;
	Pair.m_IsLineHookable = true;
	m_aButtonLinePairs[2] = Pair;

	Pair.m_ButtonPos = vec2(103, 12);
	Pair.m_ButtonSize = vec2(2, 1);
	Pair.m_LinePos = vec2(69, 30);
	Pair.m_LineSize = vec2(37, 2);
	Pair.m_IsButtonActive = false;
	Pair.m_LineFlip = false;
	Pair.m_IsLineHookable = false;
	m_aButtonLinePairs[3] = Pair;
}

void CGameControllerExamplePhys2::Tick()
{
	IGameController::Tick();

	static int CooldownTick = 0;
	CooldownTick++;

	for(int bpi = 0; bpi < BUTTON_PAIR_COUNT; bpi++)
	{
		ButtonLaserLinePair& Pair = m_aButtonLinePairs[bpi];
		bool NewActive = false;

		for(int p = 0; p < MAX_PLAYERS; p++)
		{
			CCharacter* pChar = GameServer()->GetPlayerChar(p);
			if(pChar && IsInsideRect(pChar->GetPos(), Pair.m_ButtonPos * 32, Pair.m_ButtonSize * 32))
			{
				NewActive = true;
				break;
			}
		}

		if(NewActive != Pair.m_IsButtonActive)
		{
			bool Solid = NewActive;
			if(Pair.m_LineFlip)
				Solid ^= 1;

			FlipSolidRect(Pair.m_LinePos.x, Pair.m_LinePos.y, Pair.m_LineSize.x, Pair.m_LineSize.y, Solid, Pair.m_IsLineHookable);
		}

		Pair.m_IsButtonActive = NewActive;

		for(int p = 0; p < MAX_PLAYERS; p++)
		{
			if(GameServer()->m_apPlayers[p])
			{
				if(GameServer()->m_apPlayers[p]->IsDummy())
					continue;

				CNetObj_DebugRect Rect;
				Rect.id = bpi;
				Rect.x = Pair.m_ButtonPos.x * 32;
				Rect.y = Pair.m_ButtonPos.y * 32;
				Rect.w = Pair.m_ButtonSize.x * 32;
				Rect.h = Pair.m_ButtonSize.y * 32;
				Rect.color = Pair.m_IsButtonActive ? 0x7f00ff00 : 0x7f0000ff;
				GameServer()->SendDuckNetObj(Rect, p);
			}
		}
	}
}

void CGameControllerExamplePhys2::OnPlayerConnect(CPlayer* pPlayer)
{
	IGameController::OnPlayerConnect(pPlayer);

	for(int bpi = 0; bpi < BUTTON_PAIR_COUNT; bpi++)
	{
		const ButtonLaserLinePair& Pair = m_aButtonLinePairs[bpi];
		bool Solid = Pair.m_IsButtonActive;
		if(Pair.m_LineFlip)
			Solid ^= 1;

		FlipSolidRect(Pair.m_LinePos.x, Pair.m_LinePos.y, Pair.m_LineSize.x, Pair.m_LineSize.y, Solid, Pair.m_IsLineHookable);
	}
}
