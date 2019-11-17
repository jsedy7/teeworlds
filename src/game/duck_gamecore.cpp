#include "duck_gamecore.h"
#include "duck_collision.h"
#include <game/server/gamecontext.h>
#include <game/server/player.h> // is dummy

const CPhysicsLawsGroup g_PlgDefault = {
	-1, // m_UID
	0.95f, // m_AirFriction
	0.5f, // m_GroundFriction
	250.0f / SERVER_TICK_SPEED, // m_AirMaxSpeed
	10, // m_GroundMaxSpeed
	1.5f, // m_AirAccel
	100.0f / SERVER_TICK_SPEED, // m_GroundAccel
	0.5f, // m_Gravity
	0, // m_Elasticity
};

const float g_CoreOuterRadius = 5;

void CDuckWorldCore::Init(CWorldCore *pBaseWorldCore, CDuckCollision *pDuckCollison)
{
	m_pBaseWorldCore = pBaseWorldCore;
	m_pCollision = pDuckCollison;

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		m_aBaseCoreExtras[i].Reset();
	}

	m_NextUID = 0;

	m_aCustomCores.hint_size(256);
	m_aJoints.hint_size(1024);
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
	const int JointCount = m_aJoints.size();
	for(int i = 0; i < JointCount; i++)
	{
		Joint_Tick(&m_aJoints[i]);
	}

	// base characters will Tick() and Move() before
	// TODO: we need to tick before Move() somehow

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
		CustomCore_Tick(&m_aCustomCores[i]);
	}

	for(int i = 0; i < AdditionnalCount; i++)
	{
		if(m_aCustomCores[i].m_UID < 0)
			continue;
		CustomCore_Move(&m_aCustomCores[i]);
		m_aCustomCores[i].Quantize();
	}
}

void CDuckWorldCore::CharacterCore_ExtraTick(CCharacterCore *pThis, CCharCoreExtra* pThisExtra, bool UseInput)
{
	const float PhysSize = 28.f;
	const int AdditionnalCount = m_aCustomCores.size();

	if(UseInput)
	{
		if(!pThis->m_Input.m_Hook)
		{
			pThisExtra->m_HookedCustomCoreUID = -1;
		}
	}

	CCustomCore* pHookedCore = FindCustomCoreFromUID(pThisExtra->m_HookedCustomCoreUID);
	if(pThisExtra->m_HookedCustomCoreUID != -1 && !pHookedCore)
	{
		pThisExtra->m_HookedCustomCoreUID = -1;
		pThis->m_HookState = HOOK_RETRACT_START;
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

	if(pThisExtra->m_OldHookState == HOOK_FLYING || pThis->m_HookState == HOOK_FLYING) // FIXME: sometimes we don't hook the core even though we should
	{
		vec2 OldPos = pThis->m_HookPos-pThis->m_HookDir*m_pBaseWorldCore->m_Tuning.m_HookFireSpeed;
		vec2 NewPos = pThis->m_HookPos;

		float Distance = 0.0f;
		for(int i = 0; i < AdditionnalCount; i++)
		{
			CCustomCore *pCore = &m_aCustomCores[i];
			if(!(pCore->m_Flags&CCustomCore::FLAG_CHAR_HOOK))
				continue;

			vec2 ClosestPoint = closest_point_on_line(OldPos, NewPos, pCore->m_Pos);
			if(distance(pCore->m_Pos, ClosestPoint) < pCore->m_Radius+2)
			{
				if((pThis->m_HookedPlayer == -1 && pThisExtra->m_HookedCustomCoreUID == -1) || distance(OldPos, pCore->m_Pos) < Distance)
				{
					pThis->m_TriggeredEvents |= COREEVENTFLAG_HOOK_ATTACH_PLAYER;
					pThis->m_TriggeredEvents &= ~COREEVENTFLAG_HOOK_ATTACH_GROUND;
					pThis->m_TriggeredEvents &= ~COREEVENTFLAG_HOOK_HIT_NOHOOK;
					pThis->m_HookState = HOOK_GRABBED;
					pThis->m_HookedPlayer = -1;
					pThisExtra->m_HookedCustomCoreUID = pCore->m_UID; // we grabbed an custom core
					pHookedCore = pCore;
					Distance = distance(OldPos, pCore->m_Pos);
				}
			}
		}
	}

	if(pThis->m_HookState == HOOK_GRABBED)
	{
		if(pThis->m_HookedPlayer == -1 && pThisExtra->m_HookedCustomCoreUID != -1)
		{
			pThis->m_HookPos = pHookedCore->m_Pos;
		}
	}

	for(int i = 0; i < AdditionnalCount; i++)
	{
		CCustomCore *pCore = &m_aCustomCores[i];

		// handle player <-> player collision
		float Distance = distance(pThis->m_Pos, pCore->m_Pos);
		vec2 Dir = normalize(pThis->m_Pos - pCore->m_Pos);
		float MinDist = pCore->m_Radius + PhysSize * 0.5f;
		if(pCore->m_Flags&CCustomCore::FLAG_CHAR_COLLIDE && Distance < MinDist+7 && Distance > 0.0f)
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
		if(pThis->m_HookedPlayer == -1 && pThisExtra->m_HookedCustomCoreUID == pCore->m_UID)
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

	pThisExtra->m_OldHookState = pThis->m_HookState;
}

void CDuckWorldCore::CustomCore_Tick(CCustomCore *pThis)
{
	CPhysicsLawsGroup* pPlgFind = FindPhysicLawsGroupFromUID(pThis->m_PlgUID);
	const CPhysicsLawsGroup& Plg = pPlgFind ? *pPlgFind : g_PlgDefault;

	// get ground state
	const float PhysSize = pThis->m_Radius * 2;
	bool Grounded = m_pCollision->IsBoxGrounded(pThis->m_Pos, vec2(PhysSize, PhysSize));

	pThis->m_Vel.y += Plg.m_Gravity;

	const float MaxSpeed = Grounded ? Plg.m_GroundMaxSpeed : Plg.m_AirMaxSpeed;
	const float Accel = Grounded ? Plg.m_GroundAccel : Plg.m_AirAccel;
	const float Friction = Grounded ? Plg.m_GroundFriction : Plg.m_AirFriction;

	pThis->m_Vel.x *= Friction;

	const int AdditionnalCount = m_aCustomCores.size();

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		CCharacterCore *pCharCore = m_pBaseWorldCore->m_apCharacters[i];
		if(!pCharCore)
			continue;

		// handle player <-> player collision
		float Distance = distance(pThis->m_Pos, pCharCore->m_Pos);
		vec2 Dir = normalize(pThis->m_Pos - pCharCore->m_Pos);
		float MinDist = 28.0f * 0.5f + pThis->m_Radius;
		if(pThis->m_Flags&CCustomCore::FLAG_CHAR_COLLIDE && Distance < MinDist+7 && Distance > 0.0f)
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
	}

#if 1
	for(int i = 0; i < AdditionnalCount; i++)
	{
		CCustomCore *pCore = &m_aCustomCores[i];

		if(pCore == pThis)
			continue; // make sure that we don't nudge our self

		if(!(pCore->m_Flags&CCustomCore::FLAG_CORE_COLLIDE))
			continue;

		// handle player <-> player collision
		float Distance = distance(pThis->m_Pos, pCore->m_Pos);
		vec2 Dir = normalize(pThis->m_Pos - pCore->m_Pos);
		float MinDist = pCore->m_Radius + pThis->m_Radius;
		if(Distance < MinDist && Distance > 0.0f)
		{
			/*float a = (MinDist+g_CoreOuterRadius - Distance);
			float Velocity = 0.5f;

			// make sure that we don't add excess force by checking the
			// direction against the current velocity. if not zero.
			if(length(pThis->m_Vel) > 0.0001)
				Velocity = 1-(dot(normalize(pThis->m_Vel), Dir)+1)/2;

			pThis->m_Vel += Dir*a*(Velocity*0.75f);
			pThis->m_Vel *= 0.85f;*/
			pThis->m_Vel += Dir * (MinDist-Distance) * 1.01f;
		}
	}
#endif

	// clamp the velocity to something sane
	if(length(pThis->m_Vel) > 6000)
		pThis->m_Vel = normalize(pThis->m_Vel) * 6000;
}

void CDuckWorldCore::CustomCore_Move(CCustomCore *pThis)
{
	CPhysicsLawsGroup* pPlgFind = FindPhysicLawsGroupFromUID(pThis->m_PlgUID);
	const CPhysicsLawsGroup& Plg = pPlgFind ? *pPlgFind : g_PlgDefault;

	const float PhysSize = pThis->m_Radius * 2;
	float RampValue = VelocityRamp(length(pThis->m_Vel)*50, m_pBaseWorldCore->m_Tuning.m_VelrampStart, m_pBaseWorldCore->m_Tuning.m_VelrampRange, m_pBaseWorldCore->m_Tuning.m_VelrampCurvature);

	pThis->m_Vel.x = pThis->m_Vel.x*RampValue;

	vec2 NewPos = pThis->m_Pos;

	// NOTE: this is MoveBox() with more sampling
	m_pCollision->MoveBoxBig(&NewPos, &pThis->m_Vel, vec2(PhysSize, PhysSize), Plg.m_Elasticity);

	pThis->m_Vel.x = pThis->m_Vel.x*(1.0f/RampValue);

	const int AdditionnalCount = m_aCustomCores.size();

	float Distance = distance(pThis->m_Pos, NewPos);
	int End = Distance+1;
	vec2 LastPos = pThis->m_Pos;
	for(int i = 0; i < End; i++)
	{
		float a = i/Distance;
		vec2 Pos = mix(pThis->m_Pos, NewPos, a);

		// check player collision
		if(pThis->m_Flags&CCustomCore::FLAG_CHAR_COLLIDE)
		{
			for(int p = 0; p < MAX_CLIENTS; p++)
			{
				CCharacterCore *pCharCore = m_pBaseWorldCore->m_apCharacters[p];
				if(!pCharCore)
					continue;
				float D = distance(Pos, pCharCore->m_Pos);
				float MinDist = pThis->m_Radius + 28.0f * 0.5f;
				if(D < MinDist && D > 0.0f)
				{
					if(a > 0.0f)
						pThis->m_Pos = LastPos;
					else if(distance(NewPos, pCharCore->m_Pos) > D)
						pThis->m_Pos = NewPos;
					return;
				}
			}
		}

		// check against custom cores
		if(pThis->m_Flags&CCustomCore::FLAG_CORE_COLLIDE)
		{
			for(int p = 0; p < AdditionnalCount; p++)
			{
				CCustomCore *pCore = &m_aCustomCores[p];
				if(pCore == pThis)
					continue;

				float D = distance(Pos, pCore->m_Pos);
				float MinDist = pThis->m_Radius + pCore->m_Radius;
				if(D < MinDist)
				{
					if(distance(NewPos, pCore->m_Pos) > D || distance(Pos, pCore->m_Pos + pCore->m_Vel) > D) // we are moving away from the core
					{
						continue;
					}

					if(a > 0.0f)
					{
						pThis->m_Pos = LastPos;
						// negate velocity vector based on impact
						vec2 ImpactDir = normalize(pCore->m_Pos - Pos);
						float ProjLength = dot(ImpactDir, pThis->m_Vel);
						if(ProjLength > 0)
							pThis->m_Vel -= ImpactDir * ProjLength;
						return;
					}
					else
					{
						pThis->m_Pos = NewPos;
						return;
					}
				}
			}
		}

		LastPos = Pos;
	}

	pThis->m_Pos = NewPos;
}

void CDuckWorldCore::Joint_Tick(CDuckPhysJoint *pThis)
{
	CCustomCore* pCore1 = FindCustomCoreFromUID(pThis->m_CustomCoreUID1);
	CCustomCore* pCore2 = FindCustomCoreFromUID(pThis->m_CustomCoreUID2);

	if(pCore1 && pCore2)
	{
		float Distance = distance(pCore1->m_Pos, pCore2->m_Pos);
		if(pThis->m_Force1 != 0 && Distance > (pCore1->m_Radius + pCore2->m_Radius + g_CoreOuterRadius))
		{
			/*float Accel = m_pBaseWorldCore->m_Tuning.m_HookDragAccel * (Distance/m_pBaseWorldCore->m_Tuning.m_HookLength);
			float DragSpeed = m_pBaseWorldCore->m_Tuning.m_HookDragSpeed;

			// add force to the hooked player
			pCore->m_Vel.x = SaturatedAdd(-DragSpeed, DragSpeed, pCore->m_Vel.x, Accel*Dir.x*1.5f);
			pCore->m_Vel.y = SaturatedAdd(-DragSpeed, DragSpeed, pCore->m_Vel.y, Accel*Dir.y*1.5f);

			// add a little bit force to the guy who has the grip
			pThis->m_Vel.x = SaturatedAdd(-DragSpeed, DragSpeed, pThis->m_Vel.x, -Accel*Dir.x*0.25f);
			pThis->m_Vel.y = SaturatedAdd(-DragSpeed, DragSpeed, pThis->m_Vel.y, -Accel*Dir.y*0.25f);*/

			vec2 Dir = normalize(pCore2->m_Pos - pCore1->m_Pos);
			pCore1->m_Vel += Dir * pThis->m_Force1;
			pCore2->m_Vel += Dir * -pThis->m_Force1;
		}
		if(pThis->m_MaxDist > 0 && Distance > pThis->m_MaxDist)
		{
			vec2 Dir = normalize(pCore2->m_Pos - pCore1->m_Pos);
			float Diff = (Distance - pThis->m_MaxDist);
			pCore1->m_Vel += Dir * Diff;
			pCore2->m_Vel += Dir * -Diff;
		}
	}
}

CCustomCore *CDuckWorldCore::AddCustomCore(float Radius)
{
	CCustomCore Core;
	Core.Reset();
	Core.m_Radius = Radius;
	Core.m_UID = m_NextUID++;
	int ID = m_aCustomCores.add(Core);
	return &m_aCustomCores[ID];
}

void CDuckWorldCore::RemoveCustomCore(int ID)
{
	dbg_assert(ID >= 0 && ID <= m_aCustomCores.size(), "ID out of bounds");

	m_aCustomCores.remove_index_fast(ID);
}

CPhysicsLawsGroup *CDuckWorldCore::AddPhysicLawsGroup()
{
	CPhysicsLawsGroup Plg = {
		-1, // m_UID
		m_pBaseWorldCore->m_Tuning.m_AirFriction, // m_AirFriction
		m_pBaseWorldCore->m_Tuning.m_GroundFriction, // m_GroundFriction
		m_pBaseWorldCore->m_Tuning.m_AirControlSpeed, // m_AirMaxSpeed
		m_pBaseWorldCore->m_Tuning.m_GroundControlSpeed, // m_GroundMaxSpeed
		m_pBaseWorldCore->m_Tuning.m_AirControlAccel, // m_AirAccel
		m_pBaseWorldCore->m_Tuning.m_GroundControlAccel, // m_GroundAccel
		m_pBaseWorldCore->m_Tuning.m_Gravity, // m_Gravity
		0, // m_Elasticity
	};
	Plg.m_UID = m_NextUID++;
	int ID = m_aPhysicsLawsGroups.add(Plg);
	return &m_aPhysicsLawsGroups[ID];
}

void CDuckWorldCore::Snap(CGameContext *pGameServer, int SnappingClient)
{
	const int CoreCount = m_aCustomCores.size();
	const int JointCount = m_aJoints.size();
	const int PlgCount = m_aPhysicsLawsGroups.size();


	if(!pGameServer->m_apPlayers[SnappingClient])
		return;
	if(pGameServer->m_apPlayers[SnappingClient]->IsDummy())
		return;

	for(int i = 0; i < CoreCount; i++)
	{
		CNetObj_DuckCustomCore* pData = pGameServer->DuckSnapNewItem<CNetObj_DuckCustomCore>(m_aCustomCores[i].m_UID);
		m_aCustomCores[i].Write(pData);
	}

	for(int i = 0; i < JointCount; i++)
	{
		CNetObj_DuckPhysJoint* pData = pGameServer->DuckSnapNewItem<CNetObj_DuckPhysJoint>(i);
		m_aJoints[i].Write(pData);
	}

	for(int i = 0; i < PlgCount; i++)
	{
		CNetObj_DuckPhysicsLawsGroup* pData = pGameServer->DuckSnapNewItem<CNetObj_DuckPhysicsLawsGroup>(i);
		m_aPhysicsLawsGroups[i].Write(pData);
	}

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		// FIXME: fix GetCharacter() linker error
		if(!pGameServer->m_apPlayers[i] || pGameServer->m_apPlayers[i]->IsDummy()/* || !pGameServer->m_apPlayers[p]->GetCharacter()*/)
			continue;

		CNetObj_DuckCharCoreExtra* pData = pGameServer->DuckSnapNewItem<CNetObj_DuckCharCoreExtra>(pGameServer->m_apPlayers[i]->GetCID());
		m_aBaseCoreExtras[i].Write(pData);
	}
}

void CDuckWorldCore::Copy(const CDuckWorldCore *pOther)
{
	*this = *pOther;
}

CCustomCore* CDuckWorldCore::FindCustomCoreFromUID(int UID, int* pID)
{
	if(UID < 0)
		return NULL;

	// slow
	const int Count = m_aCustomCores.size();
	for(int i = 0; i < Count; i++)
	{
		if(m_aCustomCores[i].m_UID == UID)
		{
			if(pID) *pID = i;
			return &m_aCustomCores[i];
		}
	}

	if(pID) *pID = -1;
	return NULL;
}

CPhysicsLawsGroup *CDuckWorldCore::FindPhysicLawsGroupFromUID(int UID)
{
	if(UID < 0)
		return NULL;

	// slow
	const int Count = m_aPhysicsLawsGroups.size();
	for(int i = 0; i < Count; i++)
	{
		if(m_aPhysicsLawsGroups[i].m_UID == UID)
			return &m_aPhysicsLawsGroups[i];
	}

	return NULL;
}
