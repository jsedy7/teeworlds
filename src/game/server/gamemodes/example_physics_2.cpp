#include "example_physics_2.h"
#include <stdint.h>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/server/entities/character.h>
#include <game/server/player.h>

typedef uint32_t u32;
typedef int32_t i32;

struct ModNetID
{
	enum Enum {
		TEST=0,
		DEBUG_RECT,
		MAP_RECT_SET_SOLID,

		_COUNT
	};
};

struct CNetObj_Test
{
	enum { NET_ID = ModNetID::TEST };

	i32 ClientID;
	float Value1;
};

struct CNetObj_DebugRect
{
	enum { NET_ID = ModNetID::DEBUG_RECT };

	int id;
	float x;
	float y;
	float w;
	float h;
	u32 color;
};

struct CNetObj_MapRectSetSolid
{
	enum { NET_ID = ModNetID::MAP_RECT_SET_SOLID };

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

	CDuckCollision* pCollision = (CDuckCollision*)GameServer()->Collision();

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

	// load duck mod
	if(!Server()->LoadDuckMod("", "", "mods/example_physics_2"))
	{
		dbg_msg("server", "failed to load duck mod");
	}

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

	/*if(CooldownTick >= SERVER_TICK_SPEED)
	{
		CooldownTick = 0;
		CNetObj_Test Test;
		Test.ClientID = random_int();
		Test.Value1 = (double)random_int() / (int)0x7FFFFFFF;

		SendDukNetObj(Test);
		dbg_msg("duck_mod", "sending ClientID=%d Value1=%g", Test.ClientID, Test.Value1);
	}*/

	/*if(GameServer()->GetPlayerChar(0))
	{
		CNetObj_Rect Rect;
		vec2 Pos = GameServer()->GetPlayerChar(0)->GetPos();

		Rect.x = Pos.x + 10.0f;
		Rect.y = Pos.y - 10.0f;
		Rect.w = 100.0f;
		Rect.h = 50.0f;

		SendDukNetObj(Rect, 0);
	}*/

	/*static bool s_SetSolid = false;
	if(CooldownTick >= SERVER_TICK_SPEED * 5)
	{
		CooldownTick = 0;
		s_SetSolid ^= 1;
		FlipSolidRect(23, 44, 40, 1, s_SetSolid);
	}*/

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
