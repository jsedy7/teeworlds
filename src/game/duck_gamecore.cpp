#include "duck_gamecore.h"
#include "duck_collision.h"
#include <game/server/gamecontext.h>
#include <game/server/player.h> // is dummy


void CDuckWorldCore::Init(CWorldCore *pBaseWorldCore, CDuckCollision *pDuckCollison)
{
	m_pBaseWorldCore = pBaseWorldCore;
	m_pCollision = pDuckCollison;

	m_aAdditionalCharCoreInfos.set_size(MAX_CLIENTS);

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		m_aAdditionalCharCoreInfos[i].Reset();
	}
}

void CDuckWorldCore::Reset()
{
	m_aAdditionalCharCores.clear();
	m_aAdditionalCharCoreInfos.set_size(MAX_CLIENTS);

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		m_aAdditionalCharCoreInfos[i].Reset();
	}
}

void CDuckWorldCore::Tick()
{
	CCharacterCore** aBaseCores = m_pBaseWorldCore->m_apCharacters;

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!aBaseCores[i])
			continue;
		CCharacterCore_Tick(aBaseCores[i], &m_aAdditionalCharCoreInfos[i], true);
	}

	const int AdditionnalCount = m_aAdditionalCharCores.size();
	for(int i = 0; i < AdditionnalCount; i++)
	{
		m_aAdditionalCharCores[i].m_pWorld = m_pBaseWorldCore;
		m_aAdditionalCharCores[i].m_pCollision = m_pCollision;
		m_aAdditionalCharCores[i].Tick(true);
		CCharacterCore_Tick(&m_aAdditionalCharCores[i], &m_aAdditionalCharCoreInfos[i+MAX_CLIENTS], true);
	}

	for(int i = 0; i < AdditionnalCount; i++)
	{
		m_aAdditionalCharCores[i].Move();
	}

	// base characters will Move() elsewhere
}

void CDuckWorldCore::CCharacterCore_Tick(CCharacterCore *pThis, CCharacterCoreAddInfo* pThisAddInfo, bool UseInput)
{
	const float PhysSize = 28.f;
	const int AdditionnalCount = m_aAdditionalCharCores.size();

	if(UseInput)
	{
		if(!pThis->m_Input.m_Hook)
		{
			pThisAddInfo->m_HookedAddCharChore = -1;
		}
		else
		{
			pThisAddInfo->m_OldHookState = HOOK_FLYING;
		}
	}

	if(pThisAddInfo->m_OldHookState == HOOK_FLYING)
	{
		vec2 OldPos = pThisAddInfo->m_OldHookPos;
		vec2 NewPos = pThis->m_HookPos; // we're already at the new position

		// Check against other players first
		if(m_pBaseWorldCore && m_pBaseWorldCore->m_Tuning.m_PlayerHooking)
		{
			float Distance = 0.0f;
			for(int i = 0; i < AdditionnalCount; i++)
			{
				CCharacterCore *pCharCore = &m_aAdditionalCharCores[i];

				if(pCharCore == pThis)
					continue; // make sure that we don't nudge our self

				vec2 ClosestPoint = closest_point_on_line(OldPos, NewPos, pCharCore->m_Pos);
				if(distance(pCharCore->m_Pos, ClosestPoint) < PhysSize+2.0f)
				{
					if((pThis->m_HookedPlayer == -1 && pThisAddInfo->m_HookedAddCharChore == -1) || distance(OldPos, pCharCore->m_Pos) < Distance)
					{
						pThis->m_TriggeredEvents |= COREEVENTFLAG_HOOK_ATTACH_PLAYER;
						pThis->m_TriggeredEvents &= ~COREEVENTFLAG_HOOK_ATTACH_GROUND;
						pThis->m_TriggeredEvents &= ~COREEVENTFLAG_HOOK_HIT_NOHOOK;
						pThis->m_HookState = HOOK_GRABBED;
						pThis->m_HookedPlayer = -1;
						pThisAddInfo->m_HookedAddCharChore = i; // we grabbed an additonnal core
						Distance = distance(OldPos, pCharCore->m_Pos);
					}
				}
			}
		}
	}

	if(pThis->m_HookState == HOOK_GRABBED)
	{
		if(pThis->m_HookedPlayer == -1 && pThisAddInfo->m_HookedAddCharChore != -1)
		{
			CCharacterCore *pCharCore = &m_aAdditionalCharCores[pThisAddInfo->m_HookedAddCharChore];
			pThis->m_HookPos = pCharCore->m_Pos;
			// TODO: Check if pThisAddInfo->m_HookedAddCharChore still exists!
		}
	}

	for(int i = 0; i < AdditionnalCount; i++)
	{
		CCharacterCore *pCharCore = &m_aAdditionalCharCores[i];

		if(pCharCore == pThis)
			continue; // make sure that we don't nudge our self

		// handle player <-> player collision
		float Distance = distance(pThis->m_Pos, pCharCore->m_Pos);
		vec2 Dir = normalize(pThis->m_Pos - pCharCore->m_Pos);
		if(m_pBaseWorldCore->m_Tuning.m_PlayerCollision && Distance < PhysSize*1.25f && Distance > 0.0f)
		{
			float a = (PhysSize*1.45f - Distance);
			float Velocity = 0.5f;

			// make sure that we don't add excess force by checking the
			// direction against the current velocity. if not zero.
			if (length(pThis->m_Vel) > 0.0001)
				Velocity = 1-(dot(normalize(pThis->m_Vel), Dir)+1)/2;

			pThis->m_Vel += Dir*a*(Velocity*0.75f);
			pThis->m_Vel *= 0.85f;
		}

		// handle hook influence
		if(pThis->m_HookedPlayer == -1 && pThisAddInfo->m_HookedAddCharChore == i && m_pBaseWorldCore->m_Tuning.m_PlayerHooking)
		{
			if(Distance > PhysSize*1.50f) // TODO: fix tweakable variable
			{
				float Accel = m_pBaseWorldCore->m_Tuning.m_HookDragAccel * (Distance/m_pBaseWorldCore->m_Tuning.m_HookLength);
				float DragSpeed = m_pBaseWorldCore->m_Tuning.m_HookDragSpeed;

				// add force to the hooked player
				pCharCore->m_Vel.x = SaturatedAdd(-DragSpeed, DragSpeed, pCharCore->m_Vel.x, Accel*Dir.x*1.5f);
				pCharCore->m_Vel.y = SaturatedAdd(-DragSpeed, DragSpeed, pCharCore->m_Vel.y, Accel*Dir.y*1.5f);

				// add a little bit force to the guy who has the grip
				pThis->m_Vel.x = SaturatedAdd(-DragSpeed, DragSpeed, pThis->m_Vel.x, -Accel*Dir.x*0.25f);
				pThis->m_Vel.y = SaturatedAdd(-DragSpeed, DragSpeed, pThis->m_Vel.y, -Accel*Dir.y*0.25f);
			}
		}
	}

	pThisAddInfo->m_OldHookPos = pThis->m_HookPos;
	pThisAddInfo->m_OldHookState = pThis->m_HookState;
}

int CDuckWorldCore::AddCharCore()
{
	CCharacterCore Core;
	mem_zero(&Core, sizeof(Core));
	Core.m_pWorld = m_pBaseWorldCore;
	Core.m_pCollision = m_pCollision;
	CCharacterCoreAddInfo AddInfo;
	AddInfo.Reset();
	m_aAdditionalCharCoreInfos.add(AddInfo);
	return m_aAdditionalCharCores.add(Core);
}

void CDuckWorldCore::SendAllCoreData(CGameContext *pGameServer)
{
	const int AdditionnalCount = m_aAdditionalCharCores.size();

	for(int p = 0; p < MAX_CLIENTS; p++)
	{
		if(!pGameServer->m_apPlayers[p])
			continue;
		if(pGameServer->m_apPlayers[p]->IsDummy())
			continue;

		for(int i = 0; i < AdditionnalCount; i++)
		{
			CNetCoreAddiData Data;
			Data.m_ID = i;
			Data.m_Core = m_aAdditionalCharCores[i];
			Data.m_AddInfo = m_aAdditionalCharCoreInfos[i + MAX_CLIENTS];
			pGameServer->SendDuckNetObj(Data, pGameServer->m_apPlayers[p]->GetCID());
		}

		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			// FIXME: fix GetCharacter() linker error
			if(!pGameServer->m_apPlayers[i] || pGameServer->m_apPlayers[p]->IsDummy()/* || !pGameServer->m_apPlayers[p]->GetCharacter()*/)
				continue;

			CNetCoreBaseExtraData Data;
			Data.m_ID = i;
			Data.m_AddInfo = m_aAdditionalCharCoreInfos[i];
			pGameServer->SendDuckNetObj(Data, pGameServer->m_apPlayers[p]->GetCID());
		}
	}
}

void CDuckWorldCore::RecvCoreAddiData(const CDuckWorldCore::CNetCoreAddiData &CoreData)
{
	// TODO: this is not great
	// having the 2 differently sized arrays is prone to error
	const int ID = CoreData.m_ID;
	if(ID < 0 || ID > 4096) // reasonable limit
		return;

	if(ID >= m_aAdditionalCharCores.size())
	{
		m_aAdditionalCharCores.set_size(ID + 1);
		m_aAdditionalCharCoreInfos.set_size(ID + 1 + MAX_CLIENTS);
	}

	m_aAdditionalCharCores[ID] = CoreData.m_Core;
	m_aAdditionalCharCoreInfos[MAX_CLIENTS + ID] = CoreData.m_AddInfo;
}

void CDuckWorldCore::RecvCoreBaseExtraData(const CDuckWorldCore::CNetCoreBaseExtraData &CoreData)
{
	const int ID = CoreData.m_ID;
	if(ID < 0 || ID > MAX_CLIENTS)
		return;

	m_aAdditionalCharCoreInfos[ID] = CoreData.m_AddInfo;
}

void CDuckWorldCore::Copy(const CDuckWorldCore *pOther)
{
	m_aAdditionalCharCores = pOther->m_aAdditionalCharCores;
	m_aAdditionalCharCoreInfos = pOther->m_aAdditionalCharCoreInfos;
}
