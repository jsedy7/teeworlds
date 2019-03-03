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
void CGameControllerDUCK::SendDukNetObj(const T& NetObj)
{
	char aPackedStr[256];
	PackNetObjAsStr((int)T::NET_ID, &NetObj, sizeof(NetObj), aPackedStr, sizeof(aPackedStr));
	GameServer()->SendBroadcast(aPackedStr, 0);
}

CGameControllerDUCK::CGameControllerDUCK(class CGameContext *pGameServer)
: IGameController(pGameServer)
{
	// Exchange this to a string that identifies your game mode.
	// DM, TDM and CTF are reserved for teeworlds original modes.
	m_pGameType = "DUCK";

	//m_GameFlags = GAMEFLAG_TEAMS; // GAMEFLAG_TEAMS makes it a two-team gamemode
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

	if(GameServer()->GetPlayerChar(0))
	{
		CNetObj_Rect Rect;
		vec2 Pos = GameServer()->GetPlayerChar(0)->GetPos();

		Rect.x = Pos.x + 10.0f;
		Rect.y = Pos.y - 10.0f;
		Rect.w = 100.0f;
		Rect.h = 50.0f;

		SendDukNetObj(Rect);
	}
}
