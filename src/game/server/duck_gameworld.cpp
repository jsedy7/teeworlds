#include "duck_gameworld.h"
#include "entity.h"
#include "gamecontext.h"
#include "gamecontroller.h"
#include <game/duck_collision.h>

void CDuckGameWorld::SetGameServer(CGameContext *pGameServer)
{
	CGameWorld::SetGameServer(pGameServer);
	m_DuckCore.Init(&m_Core, pGameServer->Collision());
}

void CDuckGameWorld::Reset()
{
	m_DuckCore.Reset();
	CGameWorld::Reset();
}

void CDuckGameWorld::Tick()
{
	// copied from gameworld.cpp

	if(m_ResetRequested)
		Reset();

	if(!m_Paused)
	{
		// update all objects
		for(int i = 0; i < NUM_ENTTYPES; i++)
			for(CEntity *pEnt = m_apFirstEntityTypes[i]; pEnt; )
			{
				m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;
				pEnt->Tick();
				pEnt = m_pNextTraverseEntity;
			}

		m_DuckCore.Tick();

		for(int i = 0; i < NUM_ENTTYPES; i++)
			for(CEntity *pEnt = m_apFirstEntityTypes[i]; pEnt; )
			{
				m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;
				pEnt->TickDefered();
				pEnt = m_pNextTraverseEntity;
			}

		m_DuckCore.TickDefered();
	}
	else if(GameServer()->m_pController->IsGamePaused())
	{
		// update all objects
		for(int i = 0; i < NUM_ENTTYPES; i++)
			for(CEntity *pEnt = m_apFirstEntityTypes[i]; pEnt; )
			{
				m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;
				pEnt->TickPaused();
				pEnt = m_pNextTraverseEntity;
			}
	}

	RemoveEntities();
}

void CDuckGameWorld::Snap(int SnappingClient)
{
	CGameWorld::Snap(SnappingClient);
	m_DuckCore.Snap(m_pGameServer, SnappingClient);
}
