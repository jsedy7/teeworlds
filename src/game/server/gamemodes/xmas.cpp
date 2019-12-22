#include "xmas.h"
#include <engine/server.h>
#include <engine/shared/protocol.h>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/entities/character.h>
#include <base/color.h>

inline float randFloat01()
{
	return (random_int()%100000)/100000.f;
}

struct CNetObj_Santa
{
	enum { NET_ID = 0x1 };
	float m_PosX;
	float m_PosY;
};

struct CNetObj_Explosion
{
	enum { NET_ID = 0x2 };
	float m_PosX;
	float m_PosY;
	float m_Radius;
	int m_Color;
};

struct CNetObj_Present
{
	enum { NET_ID = 0x3 };
	int m_CoreID;
	int m_Tick;
	int m_Color1;
	int m_Color2;
};

struct CSanta
{
	CDuckWorldCore* m_pWorld;
	CGameContext* m_pGameServer;
	int m_SantaCoreUID;
	int m_BagCoreUID;
	int m_PlgFlyUID;
	int m_BagHealth;

	struct CPresent
	{
		int m_CoreUID;
		int m_StartTick;
		int m_Color1;
		int m_Color2;
	};

	enum { MAX_PRESENTS = 128 };
	CPresent m_aPresents[MAX_PRESENTS];
	int m_PresentCount;

	void Init(CDuckWorldCore* pWorld, CGameContext* pGameServer)
	{
		m_pWorld = pWorld;
		m_pGameServer = pGameServer;

		CCustomCore* pBagCore = m_pWorld->AddCustomCore(50);
		CCustomCore* pSantaCore = m_pWorld->AddCustomCore(20);

		m_BagCoreUID = pBagCore->m_UID;
		m_SantaCoreUID = pSantaCore->m_UID;

		CPhysicsLawsGroup* pPlgFly = m_pWorld->AddPhysicLawsGroup();
		pPlgFly->m_AirAccel = 1;
		pPlgFly->m_AirFriction = 1;
		pPlgFly->m_Gravity = 0;

		m_PlgFlyUID = pPlgFly->m_UID;
		pSantaCore->SetPhysicLawGroup(pPlgFly);
		pBagCore->SetPhysicLawGroup(pPlgFly);

		pSantaCore->m_Pos.x = 5280;
		pSantaCore->m_Pos.y = 1696;
		pSantaCore->m_Vel = vec2(0, 0);

		m_PresentCount = 0;

		for(int i = 0; i < MAX_PRESENTS; i++)
		{
			m_aPresents[i].m_CoreUID = -1;
		}
	}

	void Start()
	{
		CCustomCore* pBagCore = m_pWorld->FindCustomCoreFromUID(m_BagCoreUID);
		CCustomCore* pSantaCore = m_pWorld->FindCustomCoreFromUID(m_SantaCoreUID);

		pSantaCore->m_Pos.x = 5280;
		pSantaCore->m_Pos.y = 1696;
		pSantaCore->m_Vel = vec2(-10, 0);
		pBagCore->m_PlgUID = m_PlgFlyUID;

		m_BagHealth = 5;

		for(int i = 0; i < m_PresentCount; i++)
		{
			int ID;
			m_pWorld->FindCustomCoreFromUID(m_aPresents[i].m_CoreUID, &ID);
			m_pWorld->RemoveCustomCore(ID);
			m_aPresents[i].m_CoreUID = -1;
		}

		m_PresentCount = 0;
	}

	void Tick()
	{
		CCustomCore* pBagCore = m_pWorld->FindCustomCoreFromUID(m_BagCoreUID);
		CCustomCore* pSantaCore = m_pWorld->FindCustomCoreFromUID(m_SantaCoreUID);

		if(pSantaCore->m_Pos.x < 3300)
		{
			if(pBagCore->m_PlgUID != -1)
			{
				dbg_msg("xmas", "DROP");
				pBagCore->m_PlgUID = -1;
				pBagCore->m_Vel = vec2(0,-10);
			}
		}
		else
		{
			pBagCore->m_Pos = pSantaCore->m_Pos + vec2(120, -20);
			pBagCore->m_Vel = pSantaCore->m_Vel;
		}

	}

	void Snap(CGameContext* pServer, int SnappinClient)
	{
		//CNetObj_Santa* pSanta = pServer->DuckSnapNewItem<CNetObj_Santa>(0);
		//pSanta->m_PosX = m_pSantaCore->m_Pos.x;
		//pSanta->m_PosY = m_pSantaCore->m_Pos.y;

		for(int i = 0; i < m_PresentCount; i++)
		{
			CNetObj_Present* pPresent = pServer->DuckSnapNewItem<CNetObj_Present>(i);
			m_pWorld->FindCustomCoreFromUID(m_aPresents[i].m_CoreUID, &pPresent->m_CoreID);
			pPresent->m_CoreID++; // lua index
			pPresent->m_Tick = m_pGameServer->Server()->Tick() - m_aPresents[i].m_StartTick;
			pPresent->m_Color1 = m_aPresents[i].m_Color1;
			pPresent->m_Color2 = m_aPresents[i].m_Color2;
		}
	}

	bool TryHitBag(vec2 HitPos)
	{
		CCustomCore* pBagCore = m_pWorld->FindCustomCoreFromUID(m_BagCoreUID);

		if(distance(pBagCore->m_Pos, HitPos) < pBagCore->m_Radius + 16)
		{
			vec2 Dir = normalize(pBagCore->m_Pos - HitPos);
			pBagCore->m_Vel.x += Dir.x * 5;
			pBagCore->m_Vel.y -= 5;

			m_BagHealth -= 1;
			if(m_BagHealth <= 0)
			{
				Explode(pBagCore->m_Pos);

				// hide the bag
				pBagCore->m_PlgUID = m_PlgFlyUID;
				pBagCore->m_Vel = vec2(0,0);
				pBagCore->m_Pos = vec2(0,0);
			}

			return true;
		}

		return false;
	}

	void Explode(vec2 Pos)
	{
		CNetObj_Explosion Explosion;
		Explosion.m_PosX = Pos.x;
		Explosion.m_PosY = Pos.y;
		Explosion.m_Radius = 100;
		Explosion.m_Color = 0xFF0000FF; // red

		m_pGameServer->SendDuckNetObj(Explosion, -1);

		const int Radius = 30;
		const int Size = Radius*2;
		const int Line = 4;
		const int Rows = 10;

		m_PresentCount = Line*Rows;

		for(int i = 0; i < m_PresentCount; i++)
		{
			CCustomCore* pCore = m_pWorld->AddCustomCore(Radius);
			pCore->m_Pos = Pos + vec2(-Line*Size/2 + (i%Line)*Size, 50-Rows*Size + i/Line*Size);
			pCore->m_Vel.x = (15 + random_int()%10) * (random_int()%2 * 2 - 1);
			pCore->m_Vel.y = -(10 + random_int()%10);
			//pCore->m_Vel = vec2(0,0);

			m_aPresents[i].m_CoreUID = pCore->m_UID;
			m_aPresents[i].m_StartTick = m_pGameServer->Server()->Tick();

			vec3 hsl(randFloat01(), randFloat01()*0.5 + 0.5f, 0.2f + randFloat01() * 0.8f);
			vec3 hsl2 = vec3(fmod(hsl.h + randFloat01()*0.6f + 0.2f, 1.0f), randFloat01()*0.5 + 0.5f, 0.2f + randFloat01() * 0.8f);

			vec3 rgb = HslToRgb(hsl);
			vec3 rgb2 = HslToRgb(hsl2);

			m_aPresents[i].m_Color1 = 0xFF000000 | int(rgb.b * 255) << 16 | int(rgb.g * 255) << 8 | int(rgb.r * 255);
			m_aPresents[i].m_Color2 = 0xFF000000 | int(rgb2.b * 255) << 16 | int(rgb2.g * 255) << 8 | int(rgb2.r * 255);
		}
	}
};

static CSanta santa;

CGameControllerXmas::CGameControllerXmas(class CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "HOHO";
	str_copy(g_Config.m_SvMap, "xmas", sizeof(g_Config.m_SvMap)); // force dm1

	// load duck mod
	if(!Server()->LoadDuckMod("", "", "mods/xmas"))
	{
		dbg_msg("server", "failed to load duck mod");
	}

	CDuckCollision* pCollision = (CDuckCollision*)GameServer()->Collision();
	m_DuckWorldCore.Init(&GameServer()->m_World.m_Core, pCollision);

	santa.Init(&m_DuckWorldCore, GameServer());
}

void CGameControllerXmas::OnPlayerConnect(CPlayer* pPlayer)
{
}

void CGameControllerXmas::Tick()
{
	IGameController::Tick();

	m_DuckWorldCore.Tick();
	santa.Tick();
}

void CGameControllerXmas::Snap(int SnappingClient)
{
	IGameController::Snap(SnappingClient);
	m_DuckWorldCore.Snap(GameServer(), SnappingClient);
	santa.Snap(GameServer(), SnappingClient);
}

void CGameControllerXmas::OnDuckMessage(int MsgID, CUnpacker *pUnpacker, int ClientID)
{
	if(MsgID == 0x1)
	{
		dbg_msg("xmas", "START");
		santa.Start();
	}
}

void CGameControllerXmas::OnCharacterHammerHit(vec2 Pos)
{
	if(santa.TryHitBag(Pos))
	{
		GameServer()->CreateHammerHit(Pos);
	}
}
