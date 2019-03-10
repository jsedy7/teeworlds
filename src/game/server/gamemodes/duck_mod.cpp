#include "duck_mod.h"
#include <stdint.h>
#include <game/server/gamecontext.h>
#include <game/server/entities/character.h>
#include <game/my_protocol.h>

inline u16 toHexStr(u8 val)
{
	u16 hex = (val & 15) << 8 | (val >> 4);
	u16 mask = ((hex + 0x0606) >> 4) & 0x0101;
	hex |= 0x3030;
	hex += 0x07 * mask;
	return hex;
}

static void PackNetObjAsStr(u8 NetObjID, const void* pNetObj, int NetObjSize, char* pOutStr, int StrBuffSize)
{
	dbg_assert(StrBuffSize >= (NetObjSize*2+4+1), "String buffer too small");
	pOutStr[0] = 0x44;
	pOutStr[1] = 0x55;
	pOutStr[2] = 0x4B;
	pOutStr[3] = NetObjID + 0x21;

	int c = 4;
	for(int i = 0; i < NetObjSize; i++)
	{
		u8 Hex = ((u8*)pNetObj)[i];
		u16 HexStr = toHexStr(Hex);
		pOutStr[c++] = HexStr & 0xFF;
		pOutStr[c++] = HexStr >> 8;
	}

	pOutStr[c] = 0; // null terminate
}

template<typename T>
void CGameControllerDUCK::SendDukNetObj(const T& NetObj, int CID)
{
	char aPackedStr[256];
	PackNetObjAsStr((int)T::NET_ID, &NetObj, sizeof(NetObj), aPackedStr, sizeof(aPackedStr));
	CNetMsg_Sv_Broadcast Msg;
	Msg.m_pMessage = aPackedStr;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, CID);
}

void CGameControllerDUCK::FlipSolidRect(float Rx, float Ry, float Rw, float Rh, bool Solid)
{
	CNetObj_MapRectSetSolid Flip;
	Flip.solid = Solid;
	Flip.x = Rx;
	Flip.y = Ry;
	Flip.w = Rw;
	Flip.h = Rh;

	if(Solid)
	{
		for(int y = 0; y < Flip.h; y++)
		{
			for(int x = 0; x < Flip.w; x++)
			{
				GameServer()->Collision()->SetTileCollisionFlags(Flip.x + x, Flip.y + y, 1);
			}
		}
	}
	else
	{
		for(int y = 0; y < Flip.h; y++)
		{
			for(int x = 0; x < Flip.w; x++)
			{
				GameServer()->Collision()->SetTileCollisionFlags(Flip.x + x, Flip.y + y, 0);
			}
		}
	}

	for(int i = 0; i < MAX_PLAYERS; i++)
	{
		if(GameServer()->m_apPlayers[i])
		{
			SendDukNetObj(Flip, i);
		}
	}
}

CGameControllerDUCK::CGameControllerDUCK(class CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "DUCK";
	//m_GameFlags = GAMEFLAG_TEAMS; // GAMEFLAG_TEAMS makes it a two-team gamemode

	ButtonLaserLinePair Pair1;
	Pair1.m_ButtonPos = vec2(2, 43);
	Pair1.m_ButtonSize = vec2(2, 1);
	Pair1.m_LinePos = vec2(23, 44);
	Pair1.m_LineSize = vec2(40, 1);
	Pair1.m_IsLineActive = false;
	Pair1.m_IsLineActiveDefault = false;
	m_aButtonLinePairs[0] = Pair1;
}

void CGameControllerDUCK::Tick()
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

	static bool s_SetSolid = false;
	if(CooldownTick >= SERVER_TICK_SPEED * 5)
	{
		CooldownTick = 0;
		s_SetSolid ^= 1;
		FlipSolidRect(23, 44, 40, 1, s_SetSolid);
	}

	for(int i = 0; i < BUTTON_PAIR_COUNT; i++)
	{
		ButtonLaserLinePair& Pair = m_aButtonLinePairs[i];

		for(int p = 0; p < MAX_PLAYERS; p++)
		{
			if(GameServer()->m_apPlayers[p])
			{
				CNetObj_DebugRect Rect;
				Rect.x = Pair.m_ButtonPos.x * 32;
				Rect.y = Pair.m_ButtonPos.y * 32;
				Rect.w = Pair.m_ButtonSize.x * 32;
				Rect.h = Pair.m_ButtonSize.y * 32;
				Rect.color = Pair.m_IsLineActive ? 0x7f00ff00 : 0x7f0000ff;
				SendDukNetObj(Rect, p);
			}
		}
	}
}
