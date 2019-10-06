#include "duck_gamecore.h"
#include "duck_collision.h"
#include <game/server/gamecontext.h>
#include <game/server/player.h> // is dummy


void CDuckWorldCore::Init(CWorldCore *pBaseWorldCore, CDuckCollision *pDuckCollison)
{
	m_pBaseWorldCore = pBaseWorldCore;
	m_pCollision = pDuckCollison;
	m_aCustomCores.hint_size(64);

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		m_aBaseCoreExtras[i].Reset();
	}

	m_NextUID = 0;
}

void CDuckWorldCore::Reset()
{
	m_aCustomCores.clear();

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		m_aBaseCoreExtras[i].Reset();
	}
}

void CDuckWorldCore::Tick()
{
	CCharacterCore** aBaseCores = m_pBaseWorldCore->m_apCharacters;

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!aBaseCores[i])
			continue;
		CharacterCore_ExtraTick(aBaseCores[i], &m_aBaseCoreExtras[i], true);
	}

	const int AdditionnalCount = m_aCustomCores.size();
	for(int i = 0; i < AdditionnalCount; i++)
	{
		if(m_aCustomCores[i].m_UID < 0)
			continue;
		CustomCore_Tick(&m_aCustomCores[i], &m_aCustomCores[i].m_Extra);
	}

	for(int i = 0; i < AdditionnalCount; i++)
	{
		if(m_aCustomCores[i].m_UID < 0)
			continue;
		CustomCore_Move(&m_aCustomCores[i], &m_aCustomCores[i].m_Extra);
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
			pThisExtra->m_HookedCustomCoreUID = -1;
		}
		else
		{
			pThisExtra->m_OldHookState = HOOK_FLYING;
		}
	}

	int HookedCustomRealID = -1;
	if(pThisExtra->m_HookedCustomCoreUID != -1)
	{
		HookedCustomRealID = FindCustomCoreFromUID(pThisExtra->m_HookedCustomCoreUID);
		if(HookedCustomRealID == -1)
		{
			pThisExtra->m_HookedCustomCoreUID = -1;
			pThis->m_HookState = HOOK_RETRACT_START;
		}
	}

	// cancel ground hooking when hooking a custom core
	if(pThis->m_HookedPlayer == -1 && pThis->m_HookState == HOOK_GRABBED && distance(pThis->m_HookPos, pThis->m_Pos) > 46.0f && pThisExtra->m_HookedCustomCoreUID != -1)
	{
		vec2 HookVel = normalize(pThis->m_HookPos-pThis->m_Pos)*m_pBaseWorldCore->m_Tuning.m_HookDragAccel;
		if(HookVel.y > 0)
			HookVel.y *= 0.3f;

		if((HookVel.x < 0 && pThis->m_Direction < 0) || (HookVel.x > 0 && pThis->m_Direction > 0))
			HookVel.x *= 0.95f;
		else
			HookVel.x *= 0.75f;

		pThis->m_Vel -= HookVel;
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
				CCustomCore *pCore = &m_aCustomCores[i];

				vec2 ClosestPoint = closest_point_on_line(OldPos, NewPos, pCore->m_Pos);
				if(distance(pCore->m_Pos, ClosestPoint) < PhysSize+2.0f)
				{
					if((pThis->m_HookedPlayer == -1 && pThisExtra->m_HookedCustomCoreUID == -1) || distance(OldPos, pCore->m_Pos) < Distance)
					{
						pThis->m_TriggeredEvents |= COREEVENTFLAG_HOOK_ATTACH_PLAYER;
						pThis->m_TriggeredEvents &= ~COREEVENTFLAG_HOOK_ATTACH_GROUND;
						pThis->m_TriggeredEvents &= ~COREEVENTFLAG_HOOK_HIT_NOHOOK;
						pThis->m_HookState = HOOK_GRABBED;
						pThis->m_HookedPlayer = -1;
						pThisExtra->m_HookedCustomCoreUID = pCore->m_UID; // we grabbed an custom core
						HookedCustomRealID = i;
						Distance = distance(OldPos, pCore->m_Pos);
					}
				}
			}
		}
	}

	if(pThis->m_HookState == HOOK_GRABBED)
	{
		if(pThis->m_HookedPlayer == -1 && pThisExtra->m_HookedCustomCoreUID != -1)
		{
			CCustomCore *pCore = &m_aCustomCores[HookedCustomRealID];
			pThis->m_HookPos = pCore->m_Pos;
		}
	}

	for(int i = 0; i < AdditionnalCount; i++)
	{
		CCustomCore *pCore = &m_aCustomCores[i];

		// handle player <-> player collision
		float Distance = distance(pThis->m_Pos, pCore->m_Pos);
		vec2 Dir = normalize(pThis->m_Pos - pCore->m_Pos);
		float MinDist = pCore->m_Extra.m_Radius + PhysSize * 0.5f;
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
		if(pThis->m_HookedPlayer == -1 && pThisExtra->m_HookedCustomCoreUID == pCore->m_UID && m_pBaseWorldCore->m_Tuning.m_PlayerHooking)
		{
			if(Distance > MinDist+14) // TODO: fix tweakable variable
			{
				float Accel = m_pBaseWorldCore->m_Tuning.m_HookDragAccel * (Distance/m_pBaseWorldCore->m_Tuning.m_HookLength);
				float DragSpeed = m_pBaseWorldCore->m_Tuning.m_HookDragSpeed;

				// add force to the hooked player
				pCore->m_Vel.x = SaturatedAdd(-DragSpeed, DragSpeed, pCore->m_Vel.x, Accel*Dir.x*1.5f);
				pCore->m_Vel.y = SaturatedAdd(-DragSpeed, DragSpeed, pCore->m_Vel.y, Accel*Dir.y*1.5f);

				// add a little bit force to the guy who has the grip
				pThis->m_Vel.x = SaturatedAdd(-DragSpeed, DragSpeed, pThis->m_Vel.x, -Accel*Dir.x*0.25f);
				pThis->m_Vel.y = SaturatedAdd(-DragSpeed, DragSpeed, pThis->m_Vel.y, -Accel*Dir.y*0.25f);
			}
		}
	}

	pThisExtra->m_OldHookPos = pThis->m_HookPos;
	pThisExtra->m_OldHookState = pThis->m_HookState;
}

void CDuckWorldCore::CustomCore_Tick(CCustomCore *pThis, CCoreExtra *pThisExtra)
{
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

	int HookedCustomRealID = -1;
	if(pThisExtra->m_HookedCustomCoreUID != -1)
	{
		HookedCustomRealID = FindCustomCoreFromUID(pThisExtra->m_HookedCustomCoreUID);
		if(HookedCustomRealID == -1)
		{
			pThisExtra->m_HookedCustomCoreUID = -1;
			pThis->m_HookState = HOOK_RETRACT_START;
		}
	}

	if(pThis->m_HookState == HOOK_GRABBED)
	{
		if(pThis->m_HookedPlayer == -1 && pThisExtra->m_HookedCustomCoreUID != -1)
		{
			CCustomCore *pCore = &m_aCustomCores[HookedCustomRealID];
			pThis->m_HookPos = pCore->m_Pos;
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
		CCustomCore *pCore = &m_aCustomCores[i];

		if(pCore == pThis)
			continue; // make sure that we don't nudge our self

		// handle player <-> player collision
		float Distance = distance(pThis->m_Pos, pCore->m_Pos);
		vec2 Dir = normalize(pThis->m_Pos - pCore->m_Pos);
		float MinDist = pCore->m_Extra.m_Radius + pThisExtra->m_Radius;
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
		if(pThis->m_HookedPlayer == -1 && pThisExtra->m_HookedCustomCoreUID == pCore->m_UID && m_pBaseWorldCore->m_Tuning.m_PlayerHooking)
		{
			if(Distance > MinDist+14) // TODO: fix tweakable variable
			{
				float Accel = m_pBaseWorldCore->m_Tuning.m_HookDragAccel * (Distance/m_pBaseWorldCore->m_Tuning.m_HookLength);
				float DragSpeed = m_pBaseWorldCore->m_Tuning.m_HookDragSpeed;

				// add force to the hooked player
				pCore->m_Vel.x = SaturatedAdd(-DragSpeed, DragSpeed, pCore->m_Vel.x, Accel*Dir.x*1.5f);
				pCore->m_Vel.y = SaturatedAdd(-DragSpeed, DragSpeed, pCore->m_Vel.y, Accel*Dir.y*1.5f);

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

void CDuckWorldCore::CustomCore_Move(CCustomCore *pThis, CCoreExtra *pThisExtra)
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
		float MinVelX   = 100000000000.0f;
		float MinVelY   = 100000000000.0f;

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

				// FIXME: this is supposed to be MoveBox(), but the corner case causes issues
				bool Corner = false;
				m_pCollision->MoveBoxCornerSignal(&TestPos, &TestVel, vec2(MaxPhysSize, MaxPhysSize), 0, &Corner);
				if(Corner)
					continue;

				float DeltaX = TestPos.x - StartPos.x;
				float DeltaY = TestPos.y - StartPos.y;

				if(fabs(DeltaX) < fabs(MinDeltaX))
					MinDeltaX = DeltaX;
				if(fabs(DeltaY) < fabs(MinDeltaY))
					MinDeltaY = DeltaY;

				if(fabs(TestVel.x) < fabs(MinVelX))
					MinVelX = TestVel.x;
				if(fabs(TestVel.y) < fabs(MinVelY))
					MinVelY = TestVel.y;
			}
		}

		NewPos += vec2(MinDeltaX, MinDeltaY);
		pThis->m_Vel = vec2(MinVelX, MinVelY);
	}
	else
	{
		m_pCollision->MoveBox(&NewPos, &pThis->m_Vel, vec2(PhysSize, PhysSize), 0, NULL);
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
				CCustomCore *pCore = &m_aCustomCores[p];
				if(pCore == pThis)
					continue;

				float D = distance(Pos, pCore->m_Pos);
				float MinDist = pThisExtra->m_Radius + pCore->m_Extra.m_Radius;
				if(D < MinDist && D > 0.0f)
				{
					if(a > 0.0f)
						pThis->m_Pos = LastPos;
					else if(distance(NewPos, pCore->m_Pos) > D)
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
	CCustomCore Core;
	mem_zero(&Core, sizeof(Core));

	CCoreExtra Extra;
	Extra.Reset();
	if(Radius > 0)
		Extra.m_Radius = Radius;
	Core.m_Extra = Extra;
	Core.m_UID = m_NextUID++;
	return m_aCustomCores.add(Core);
}

void CDuckWorldCore::RemoveCustomCore(int ID)
{
	dbg_assert(ID >= 0 && ID <= m_aCustomCores.size(), "ID out of bounds");

	m_aCustomCores.remove_index_fast(ID);
}

void CDuckWorldCore::SendAllCoreData(CGameContext *pGameServer)
{
	const int AdditionnalCount = m_aCustomCores.size();

	for(int PlayerID = 0; PlayerID < MAX_CLIENTS; PlayerID++)
	{
		if(!pGameServer->m_apPlayers[PlayerID])
			continue;
		if(pGameServer->m_apPlayers[PlayerID]->IsDummy())
			continue;

		CNetClear NetClear;
		pGameServer->SendDuckNetObj(NetClear, pGameServer->m_apPlayers[PlayerID]->GetCID());

		for(int i = 0; i < AdditionnalCount; i++)
		{
			CNetCoreCustomData Data;
			Data.m_ID = i;
			Data.m_Core = m_aCustomCores[i];
			pGameServer->SendDuckNetObj(Data, pGameServer->m_apPlayers[PlayerID]->GetCID());
		}

		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			// FIXME: fix GetCharacter() linker error
			if(!pGameServer->m_apPlayers[i] || pGameServer->m_apPlayers[PlayerID]->IsDummy()/* || !pGameServer->m_apPlayers[p]->GetCharacter()*/)
				continue;

			CNetCoreBaseExtraData Data;
			Data.m_ID = i;
			Data.m_Extra = m_aBaseCoreExtras[i];
			pGameServer->SendDuckNetObj(Data, pGameServer->m_apPlayers[PlayerID]->GetCID());
		}
	}
}

void CDuckWorldCore::RecvCoreCustomData(const CDuckWorldCore::CNetCoreCustomData &CoreData)
{
	const int ID = CoreData.m_ID;
	if(ID < 0 || ID > 4096) // reasonable limit
		return;

	/*if(ID >= m_aCustomCores.size())
	{
		m_aCustomCores.set_size(ID + 1);
	}

	m_aCustomCores[ID] = CoreData.m_Core;*/
	m_aCustomCores.add(CoreData.m_Core);
}

void CDuckWorldCore::RecvCoreBaseExtraData(const CDuckWorldCore::CNetCoreBaseExtraData &CoreData)
{
	const int ID = CoreData.m_ID;
	if(ID < 0 || ID > MAX_CLIENTS)
		return;

	m_aBaseCoreExtras[ID] = CoreData.m_Extra;
}

void CDuckWorldCore::RecvClear(const CDuckWorldCore::CNetClear &NetClear)
{
	m_aCustomCores.set_size(0); // clear
}

void CDuckWorldCore::Copy(const CDuckWorldCore *pOther)
{
	*this = *pOther;
}

int CDuckWorldCore::FindCustomCoreFromUID(int UID)
{
	// slow
	const int Count = m_aCustomCores.size();
	for(int i = 0; i < Count; i++)
	{
		if(m_aCustomCores[i].m_UID == UID)
			return i;
	}

	return -1;
}
