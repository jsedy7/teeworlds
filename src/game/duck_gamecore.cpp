#include "duck_gamecore.h"
#include "duck_collision.h"
#include <game/server/gamecontext.h>
#include <game/server/player.h> // is dummy


void CDuckWorldCore::Init(CWorldCore *pBaseWorldCore, CDuckCollision *pDuckCollison)
{
	m_pBaseWorldCore = pBaseWorldCore;
	m_pCollision = pDuckCollison;

	m_aCoreExtras.set_size(MAX_CLIENTS);

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		m_aCoreExtras[i].Reset();
	}
}

void CDuckWorldCore::Reset()
{
	m_aCustomCores.clear();
	m_aCoreExtras.set_size(MAX_CLIENTS);

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		m_aCoreExtras[i].Reset();
	}
}

void CDuckWorldCore::Tick()
{
	CCharacterCore** aBaseCores = m_pBaseWorldCore->m_apCharacters;

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!aBaseCores[i])
			continue;
		CharacterCore_ExtraTick(aBaseCores[i], &m_aCoreExtras[i], true);
	}

	const int AdditionnalCount = m_aCustomCores.size();
	for(int i = 0; i < AdditionnalCount; i++)
	{
		m_aCustomCores[i].m_pWorld = m_pBaseWorldCore;
		m_aCustomCores[i].m_pCollision = m_pCollision;
		CustomCore_Tick(&m_aCustomCores[i], &m_aCoreExtras[i+MAX_CLIENTS], true);
	}

	for(int i = 0; i < AdditionnalCount; i++)
	{
		CustomCore_Move(&m_aCustomCores[i], &m_aCoreExtras[i+MAX_CLIENTS]);
	}

	// base characters will Move() elsewhere
}

void CDuckWorldCore::CharacterCore_ExtraTick(CCharacterCore *pThis, CCoreExtra* pThisExtra, bool UseInput)
{
	const float PhysSize = 28.f;
	const int AdditionnalCount = m_aCustomCores.size();

	if(UseInput)
	{
		if(!pThis->m_Input.m_Hook)
		{
			pThisExtra->m_HookedAddCharChore = -1;
		}
		else
		{
			pThisExtra->m_OldHookState = HOOK_FLYING;
		}
	}

	if(pThisExtra->m_OldHookState == HOOK_FLYING)
	{
		vec2 OldPos = pThisExtra->m_OldHookPos;
		vec2 NewPos = pThis->m_HookPos; // we're already at the new position

		// Check against other players first
		if(m_pBaseWorldCore && m_pBaseWorldCore->m_Tuning.m_PlayerHooking)
		{
			float Distance = 0.0f;
			for(int i = 0; i < AdditionnalCount; i++)
			{
				CCharacterCore *pCharCore = &m_aCustomCores[i];

				if(pCharCore == pThis)
					continue; // make sure that we don't nudge our self

				vec2 ClosestPoint = closest_point_on_line(OldPos, NewPos, pCharCore->m_Pos);
				if(distance(pCharCore->m_Pos, ClosestPoint) < PhysSize+2.0f)
				{
					if((pThis->m_HookedPlayer == -1 && pThisExtra->m_HookedAddCharChore == -1) || distance(OldPos, pCharCore->m_Pos) < Distance)
					{
						pThis->m_TriggeredEvents |= COREEVENTFLAG_HOOK_ATTACH_PLAYER;
						pThis->m_TriggeredEvents &= ~COREEVENTFLAG_HOOK_ATTACH_GROUND;
						pThis->m_TriggeredEvents &= ~COREEVENTFLAG_HOOK_HIT_NOHOOK;
						pThis->m_HookState = HOOK_GRABBED;
						pThis->m_HookedPlayer = -1;
						pThisExtra->m_HookedAddCharChore = i; // we grabbed an additonnal core
						Distance = distance(OldPos, pCharCore->m_Pos);
					}
				}
			}
		}
	}

	if(pThis->m_HookState == HOOK_GRABBED)
	{
		if(pThis->m_HookedPlayer == -1 && pThisExtra->m_HookedAddCharChore != -1)
		{
			CCharacterCore *pCharCore = &m_aCustomCores[pThisExtra->m_HookedAddCharChore];
			pThis->m_HookPos = pCharCore->m_Pos;
			// TODO: Check if pThisAddInfo->m_HookedAddCharChore still exists!
		}
	}

	for(int i = 0; i < AdditionnalCount; i++)
	{
		CCharacterCore *pCharCore = &m_aCustomCores[i];
		CCoreExtra *pExtra = &m_aCoreExtras[i+MAX_CLIENTS];

		// handle player <-> player collision
		float Distance = distance(pThis->m_Pos, pCharCore->m_Pos);
		vec2 Dir = normalize(pThis->m_Pos - pCharCore->m_Pos);
		float MinDist = pExtra->m_Radius + PhysSize * 0.5f;
		if(m_pBaseWorldCore->m_Tuning.m_PlayerCollision && Distance < MinDist+7 && Distance > 0.0f)
		{
			float a = (MinDist+10 - Distance);
			float Velocity = 0.5f;

			// make sure that we don't add excess force by checking the
			// direction against the current velocity. if not zero.
			if (length(pThis->m_Vel) > 0.0001)
				Velocity = 1-(dot(normalize(pThis->m_Vel), Dir)+1)/2;

			pThis->m_Vel += Dir*a*(Velocity*0.75f);
			pThis->m_Vel *= 0.85f;
		}

		// handle hook influence
		if(pThis->m_HookedPlayer == -1 && pThisExtra->m_HookedAddCharChore == i && m_pBaseWorldCore->m_Tuning.m_PlayerHooking)
		{
			if(Distance > MinDist+14) // TODO: fix tweakable variable
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

	pThisExtra->m_OldHookPos = pThis->m_HookPos;
	pThisExtra->m_OldHookState = pThis->m_HookState;
}

void CDuckWorldCore::CustomCore_Tick(CCharacterCore *pThis, CCoreExtra *pThisExtra, bool UseInput)
{
	pThis->m_TriggeredEvents = 0;

	// get ground state
	bool Grounded = false;
	if(m_pCollision->CheckPoint(pThis->m_Pos.x+pThisExtra->m_Radius/2, pThis->m_Pos.y+pThisExtra->m_Radius/2+5))
		Grounded = true;
	if(m_pCollision->CheckPoint(pThis->m_Pos.x-pThisExtra->m_Radius/2, pThis->m_Pos.y+pThisExtra->m_Radius/2+5))
		Grounded = true;

	pThis->m_Vel.y += m_pBaseWorldCore->m_Tuning.m_Gravity;

	float MaxSpeed = Grounded ? m_pBaseWorldCore->m_Tuning.m_GroundControlSpeed : m_pBaseWorldCore->m_Tuning.m_AirControlSpeed;
	float Accel = Grounded ? m_pBaseWorldCore->m_Tuning.m_GroundControlAccel : m_pBaseWorldCore->m_Tuning.m_AirControlAccel;
	float Friction = Grounded ? m_pBaseWorldCore->m_Tuning.m_GroundFriction : m_pBaseWorldCore->m_Tuning.m_AirFriction;

	const float PhysSize = pThisExtra->m_Radius;
	const int AdditionnalCount = m_aCustomCores.size();

	if(pThis->m_HookState == HOOK_GRABBED)
	{
		if(pThis->m_HookedPlayer == -1 && pThisExtra->m_HookedAddCharChore != -1)
		{
			CCharacterCore *pCharCore = &m_aCustomCores[pThisExtra->m_HookedAddCharChore];
			pThis->m_HookPos = pCharCore->m_Pos;
			// TODO: Check if pThisAddInfo->m_HookedAddCharChore still exists!
		}
	}

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		CCharacterCore *pCharCore = m_pBaseWorldCore->m_apCharacters[i];
		if(!pCharCore)
			continue;

		// handle player <-> player collision
		float Distance = distance(pThis->m_Pos, pCharCore->m_Pos);
		vec2 Dir = normalize(pThis->m_Pos - pCharCore->m_Pos);
		float MinDist = 28.0f * 0.5f + pThisExtra->m_Radius;
		if(m_pBaseWorldCore->m_Tuning.m_PlayerCollision && Distance < MinDist+7 && Distance > 0.0f)
		{
			float a = (MinDist+10 - Distance);
			float Velocity = 0.5f;

			// make sure that we don't add excess force by checking the
			// direction against the current velocity. if not zero.
			if (length(pThis->m_Vel) > 0.0001)
				Velocity = 1-(dot(normalize(pThis->m_Vel), Dir)+1)/2;

			pThis->m_Vel += Dir*a*(Velocity*0.75f);
			pThis->m_Vel *= 0.85f;
		}

		// handle hook influence
		/*if(pThis->m_HookedPlayer == i && m_pBaseWorldCore->m_Tuning.m_PlayerHooking)
		{
			if(Distance > PhysSize*1.50f) // TODO: fix tweakable variable
			{
				float Accel = m_pBaseWorldCore->m_Tuning.m_HookDragAccel * (Distance/m_pWorld->m_Tuning.m_HookLength);
				float DragSpeed = m_pBaseWorldCore->m_Tuning.m_HookDragSpeed;

				// add force to the hooked player
				pCharCore->m_Vel.x = SaturatedAdd(-DragSpeed, DragSpeed, pCharCore->m_Vel.x, Accel*Dir.x*1.5f);
				pCharCore->m_Vel.y = SaturatedAdd(-DragSpeed, DragSpeed, pCharCore->m_Vel.y, Accel*Dir.y*1.5f);

				// add a little bit force to the guy who has the grip
				m_Vel.x = SaturatedAdd(-DragSpeed, DragSpeed, m_Vel.x, -Accel*Dir.x*0.25f);
				m_Vel.y = SaturatedAdd(-DragSpeed, DragSpeed, m_Vel.y, -Accel*Dir.y*0.25f);
			}
		}*/
	}

	for(int i = 0; i < AdditionnalCount; i++)
	{
		CCharacterCore *pCharCore = &m_aCustomCores[i];
		CCoreExtra *pExtra = &m_aCoreExtras[i+MAX_CLIENTS];

		if(pCharCore == pThis)
			continue; // make sure that we don't nudge our self

		// handle player <-> player collision
		float Distance = distance(pThis->m_Pos, pCharCore->m_Pos);
		vec2 Dir = normalize(pThis->m_Pos - pCharCore->m_Pos);
		float MinDist = pExtra->m_Radius + pThisExtra->m_Radius;
		if(m_pBaseWorldCore->m_Tuning.m_PlayerCollision && Distance < MinDist+7 && Distance > 0.0f)
		{
			float a = (MinDist+10 - Distance);
			float Velocity = 0.5f;

			// make sure that we don't add excess force by checking the
			// direction against the current velocity. if not zero.
			if (length(pThis->m_Vel) > 0.0001)
				Velocity = 1-(dot(normalize(pThis->m_Vel), Dir)+1)/2;

			pThis->m_Vel += Dir*a*(Velocity*0.75f);
			pThis->m_Vel *= 0.85f;
		}

		// handle hook influence
		if(pThis->m_HookedPlayer == -1 && pThisExtra->m_HookedAddCharChore == i && m_pBaseWorldCore->m_Tuning.m_PlayerHooking)
		{
			if(Distance > MinDist+14) // TODO: fix tweakable variable
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

	pThisExtra->m_OldHookPos = pThis->m_HookPos;
	pThisExtra->m_OldHookState = pThis->m_HookState;

	// clamp the velocity to something sane
	if(length(pThis->m_Vel) > 6000)
		pThis->m_Vel = normalize(pThis->m_Vel) * 6000;
}

void CDuckWorldCore::CustomCore_Move(CCharacterCore *pThis, CCoreExtra *pThisExtra)
{
	const float PhysSize = pThisExtra->m_Radius * 2;
	float RampValue = VelocityRamp(length(pThis->m_Vel)*50, m_pBaseWorldCore->m_Tuning.m_VelrampStart, m_pBaseWorldCore->m_Tuning.m_VelrampRange, m_pBaseWorldCore->m_Tuning.m_VelrampCurvature);

	pThis->m_Vel.x = pThis->m_Vel.x*RampValue;

	vec2 NewPos = pThis->m_Pos;

	// split into several boxes if size is >= TileSize
	const float MaxPhysSize = 31;
	if(PhysSize > MaxPhysSize)
	{
		float MinDeltaX = 100000000000.0f;
		float MinDeltaY = 100000000000.0f;
		float MinVelX = 100000000000.0f;
		float MinVelY = 100000000000.0f;

		for(float byi = -PhysSize * 0.5; byi < PhysSize * 0.5; byi += MaxPhysSize * 0.5)
		{
			for(float bxi = -PhysSize * 0.5; bxi < PhysSize * 0.5; bxi += MaxPhysSize * 0.5)
			{
				vec2 StartPos = pThis->m_Pos + vec2(bxi, byi) + vec2(MaxPhysSize * 0.5, MaxPhysSize * 0.5);
				if(bxi + MaxPhysSize > PhysSize * 0.5)
					StartPos.x -= MaxPhysSize - (PhysSize * 0.5 - bxi);
				if(byi + MaxPhysSize > PhysSize * 0.5)
					StartPos.y -= MaxPhysSize - (PhysSize * 0.5 - byi);

				vec2 TestPos = StartPos;
				vec2 StartVel = pThis->m_Vel;
				vec2 TestVel = StartVel;
				m_pCollision->MoveBox(&TestPos, &TestVel, vec2(MaxPhysSize, MaxPhysSize), 0, &pThis->m_Death);

				float DeltaX = TestPos.x - StartPos.x;
				float DeltaY = TestPos.y - StartPos.y;

				/*if(TestVel.x == 0 && TestVel.y == 0 && (StartVel.x != 0 || StartVel.y != 0))
					continue;*/

				if(abs(DeltaX) < abs(MinDeltaX))
				{
					MinDeltaX = DeltaX;
					MinVelX = TestVel.x;
				}
				if(abs(DeltaY) < abs(MinDeltaY))
				{
					MinDeltaY = DeltaY;
					MinVelY = TestVel.y;
				}
			}
		}

		NewPos += vec2(MinDeltaX, MinDeltaY);
		pThis->m_Vel = vec2(MinVelX, MinVelY);
	}
	else
	{
		m_pCollision->MoveBox(&NewPos, &pThis->m_Vel, vec2(PhysSize, PhysSize), 0, &pThis->m_Death);
	}

	pThis->m_Vel.x = pThis->m_Vel.x*(1.0f/RampValue);

	const int AdditionnalCount = m_aCustomCores.size();

	if(m_pBaseWorldCore->m_Tuning.m_PlayerCollision)
	{
		// check player collision
		float Distance = distance(pThis->m_Pos, NewPos);
		int End = Distance+1;
		vec2 LastPos = pThis->m_Pos;
		for(int i = 0; i < End; i++)
		{
			float a = i/Distance;
			vec2 Pos = mix(pThis->m_Pos, NewPos, a);
			for(int p = 0; p < MAX_CLIENTS; p++)
			{
				CCharacterCore *pCharCore = m_pBaseWorldCore->m_apCharacters[p];
				if(!pCharCore)
					continue;
				float D = distance(Pos, pCharCore->m_Pos);
				float MinDist = pThisExtra->m_Radius + 28.0f * 0.5f;
				if(D < MinDist && D > 0.0f)
				{
					if(a > 0.0f)
						pThis->m_Pos = LastPos;
					else if(distance(NewPos, pCharCore->m_Pos) > D)
						pThis->m_Pos = NewPos;
					return;
				}
			}

			// check against custom cores
			for(int p = 0; p < AdditionnalCount; p++)
			{
				CCharacterCore *pCharCore = &m_aCustomCores[p];
				CCoreExtra *pExtra = &m_aCoreExtras[p+MAX_CLIENTS];
				if(pCharCore == pThis)
					continue;

				float D = distance(Pos, pCharCore->m_Pos);
				float MinDist = pThisExtra->m_Radius + pExtra->m_Radius;
				if(D < MinDist && D > 0.0f)
				{
					if(a > 0.0f)
						pThis->m_Pos = LastPos;
					else if(distance(NewPos, pCharCore->m_Pos) > D)
						pThis->m_Pos = NewPos;
					return;
				}
			}
			LastPos = Pos;
		}
	}

	pThis->m_Pos = NewPos;
}

int CDuckWorldCore::AddCustomCore(float Radius)
{
	CCharacterCore Core;
	mem_zero(&Core, sizeof(Core));
	Core.m_pWorld = m_pBaseWorldCore;
	Core.m_pCollision = m_pCollision;

	CCoreExtra Extra;
	Extra.Reset();
	if(Radius > 0)
		Extra.m_Radius = Radius;
	m_aCoreExtras.add(Extra);
	return m_aCustomCores.add(Core);
}

void CDuckWorldCore::SendAllCoreData(CGameContext *pGameServer)
{
	const int AdditionnalCount = m_aCustomCores.size();

	for(int p = 0; p < MAX_CLIENTS; p++)
	{
		if(!pGameServer->m_apPlayers[p])
			continue;
		if(pGameServer->m_apPlayers[p]->IsDummy())
			continue;

		for(int i = 0; i < AdditionnalCount; i++)
		{
			CNetCoreCustomData Data;
			Data.m_ID = i;
			Data.m_Core = m_aCustomCores[i];
			Data.m_Extra = m_aCoreExtras[i + MAX_CLIENTS];
			pGameServer->SendDuckNetObj(Data, pGameServer->m_apPlayers[p]->GetCID());
		}

		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			// FIXME: fix GetCharacter() linker error
			if(!pGameServer->m_apPlayers[i] || pGameServer->m_apPlayers[p]->IsDummy()/* || !pGameServer->m_apPlayers[p]->GetCharacter()*/)
				continue;

			CNetCoreBaseExtraData Data;
			Data.m_ID = i;
			Data.m_Extra = m_aCoreExtras[i];
			pGameServer->SendDuckNetObj(Data, pGameServer->m_apPlayers[p]->GetCID());
		}
	}
}

void CDuckWorldCore::RecvCoreCustomData(const CDuckWorldCore::CNetCoreCustomData &CoreData)
{
	// TODO: this is not great
	// having the 2 differently sized arrays is prone to error
	const int ID = CoreData.m_ID;
	if(ID < 0 || ID > 4096) // reasonable limit
		return;

	if(ID >= m_aCustomCores.size())
	{
		m_aCustomCores.set_size(ID + 1);
		m_aCoreExtras.set_size(ID + 1 + MAX_CLIENTS);
	}

	m_aCustomCores[ID] = CoreData.m_Core;
	m_aCoreExtras[MAX_CLIENTS + ID] = CoreData.m_Extra;
}

void CDuckWorldCore::RecvCoreBaseExtraData(const CDuckWorldCore::CNetCoreBaseExtraData &CoreData)
{
	const int ID = CoreData.m_ID;
	if(ID < 0 || ID > MAX_CLIENTS)
		return;

	m_aCoreExtras[ID] = CoreData.m_Extra;
}

void CDuckWorldCore::Copy(const CDuckWorldCore *pOther)
{
	m_aCustomCores = pOther->m_aCustomCores;
	m_aCoreExtras = pOther->m_aCoreExtras;
}
