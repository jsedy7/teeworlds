#include "duck_collision.h"
#include <base/system.h>
#include <game/mapitems.h>
#include <game/gamecore.h>
#include <game/layers.h>

void CDuckCollision::Init(CLayers* pLayers)
{
	m_pLayers = pLayers;
	m_Width = m_pLayers->GameLayer()->m_Width;
	m_Height = m_pLayers->GameLayer()->m_Height;
	m_pTiles = static_cast<CTile *>(m_pLayers->Map()->GetData(m_pLayers->GameLayer()->m_Data));

	Reset();
}

bool CDuckCollision::CheckPoint(float x, float y, int Flag) const
{
	const int SolidBlockCount = m_aStaticBlocks.size();
	const CStaticBlock* pBlocks = m_aStaticBlocks.base_ptr();
	for(int i = 0; i < SolidBlockCount; i++)
	{
		const CStaticBlock& Block = pBlocks[i];

		if((x > Block.m_Pos.x && x < (Block.m_Pos.x+Block.m_Size.x) &&
			y > Block.m_Pos.y && y < (Block.m_Pos.y+Block.m_Size.y)))
		{
			return Block.m_Flags&Flag;
		}
	}

	return CCollision::CheckPoint(x, y, Flag);
}

int CDuckCollision::GetCollisionAt(float x, float y) const
{
	const int SolidBlockCount = m_aStaticBlocks.size();
	const CStaticBlock* pBlocks = m_aStaticBlocks.base_ptr();
	for(int i = 0; i < SolidBlockCount; i++)
	{
		const CStaticBlock& Block = pBlocks[i];

		if((x > Block.m_Pos.x && x < (Block.m_Pos.x+Block.m_Size.x) &&
			y > Block.m_Pos.y && y < (Block.m_Pos.y+Block.m_Size.y)))
		{
			return Block.m_Flags;
		}
	}

	return CCollision::GetCollisionAt(x, y);
}


void CDuckCollision::SetTileCollisionFlags(int Tx, int Ty, int Flags)
{
	if(!(Tx >= 0 && Tx < m_Width) || !(Ty >= 0 && Ty < m_Height))
		return;
	m_pTiles[Ty*m_Width+Tx].m_Index = Flags;
}

void CDuckCollision::SetStaticBlock(int BlockId, CStaticBlock Block)
{
	dbg_assert(BlockId >= 0 && BlockId < MAX_STATICBLOCK_FETCH_IDS, "BlockId out of bounds");

	Block.m_FetchID = BlockId;
	int DataID = m_aStaticBlockDataID[BlockId];

	if(DataID != -1)
		m_aStaticBlocks[DataID] = Block;
	else
	{
		DataID = m_aStaticBlocks.add(Block);
		m_aStaticBlockDataID[BlockId] = DataID;
	}
}

void CDuckCollision::ClearStaticBlock(int BlockId)
{
	dbg_assert(BlockId >= 0 && BlockId < MAX_STATICBLOCK_FETCH_IDS, "BlockId out of bounds");

	int DataID = m_aStaticBlockDataID[BlockId];
	if(DataID != -1)
	{
		// swap remove
		int LastFetchID = m_aStaticBlocks[m_aStaticBlocks.size()-1].m_FetchID;

		m_aStaticBlocks.remove_index_fast(DataID);

		if(m_aStaticBlocks.size() > 0)
			m_aStaticBlockDataID[LastFetchID] = DataID;

		m_aStaticBlockDataID[BlockId] = -1;
	}
}

void CDuckCollision::SetDynamicDisk(int DiskId, CDynamicDisk Disk)
{
	dbg_assert(DiskId >= 0 && DiskId < MAX_DYNDISK_FETCH_IDS, "DiskId out of bounds");

	Disk.m_FetchID = DiskId;
	int DataID = m_aDynDiskDataID[DiskId];

	if(DataID != -1)
		m_aDynamicDisks[DataID] = Disk;
	else
	{
		DataID = m_aDynamicDisks.add(Disk);
		m_aDynDiskDataID[DiskId] = DataID;
	}
}

CDuckCollision::CDynamicDisk *CDuckCollision::GetDynamicDisk(int DiskId)
{
	dbg_assert(DiskId >= 0 && DiskId < MAX_DYNDISK_FETCH_IDS, "DiskId out of bounds");

	int DataID = m_aDynDiskDataID[DiskId];

	if(DataID != -1)
		return &m_aDynamicDisks[DataID];
	return 0x0;
}

void CDuckCollision::ClearDynamicDisk(int DiskId)
{
	dbg_assert(DiskId >= 0 && DiskId < MAX_DYNDISK_FETCH_IDS, "DiskId out of bounds");

	int DataID = m_aDynDiskDataID[DiskId];
	if(DataID != -1)
	{
		// swap remove
		int LastFetchID = m_aDynamicDisks[m_aDynamicDisks.size()-1].m_FetchID;

		m_aDynamicDisks.remove_index_fast(DataID);

		if(m_aDynamicDisks.size() > 0)
			m_aDynDiskDataID[LastFetchID] = DataID;

		m_aDynDiskDataID[DiskId] = -1;
	}
}

void CDuckCollision::Tick(CWorldCore *pWorld)
{
	const int DiskCount = m_aDynamicDisks.size();
	for(int i = 0; i < DiskCount; i++)
	{
		CDuckCollision::CDynamicDisk& Disk = m_aDynamicDisks[i];
		Disk.Move(this, pWorld);
	}
}

void CDuckCollision::Reset()
{
	m_aStaticBlocks.hint_size(1024);
	m_aStaticBlocks.set_size(0); // clear
	m_aDynamicDisks.hint_size(1024);
	m_aDynamicDisks.set_size(0); // clear

	for(int i = 0; i < MAX_STATICBLOCK_FETCH_IDS; i++)
	{
		m_aStaticBlockDataID[i] = -1;
	}
	for(int i = 0; i < MAX_DYNDISK_FETCH_IDS; i++)
	{
		m_aDynDiskDataID[i] = -1;
	}
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		m_aCharacterHookDiskFetchId[i] = -1;
	}
}

// Same as CCollision::MoveBox, but signal if the corner case is reached
void CDuckCollision::MoveBoxCornerSignal(vec2 *pInoutPos, vec2 *pInoutVel, vec2 Size, float Elasticity, bool *pCorner) const
{
	// do the move
	vec2 Pos = *pInoutPos;
	vec2 Vel = *pInoutVel;

	float Distance = length(Vel);
	int Max = (int)Distance;

	*pCorner = false;

	if(Distance > 0.00001f)
	{
		//vec2 old_pos = pos;
		float Fraction = 1.0f/(float)(Max+1);
		for(int i = 0; i <= Max; i++)
		{
			//float amount = i/(float)max;
			//if(max == 0)
				//amount = 0;

			vec2 NewPos = Pos + Vel*Fraction; // TODO: this row is not nice

			if(TestBox(vec2(NewPos.x, NewPos.y), Size))
			{
				int Hits = 0;

				if(TestBox(vec2(Pos.x, NewPos.y), Size))
				{
					NewPos.y = Pos.y;
					Vel.y *= -Elasticity;
					Hits++;
				}

				if(TestBox(vec2(NewPos.x, Pos.y), Size))
				{
					NewPos.x = Pos.x;
					Vel.x *= -Elasticity;
					Hits++;
				}

				// neither of the tests got a collision.
				// this is a real _corner case_!
				if(Hits == 0)
				{
					*pCorner = true;
					NewPos.y = Pos.y;
					Vel.y *= -Elasticity;
					NewPos.x = Pos.x;
					Vel.x *= -Elasticity;
				}
			}

			Pos = NewPos;
		}
	}

	*pInoutPos = Pos;
	*pInoutVel = Vel;
}

void CDuckCollision::CDynamicDisk::Tick(CDuckCollision *pCollision, CWorldCore *pWorld)
{
	const float PhysSize = 28;
	const int DiskCount = pCollision->m_aDynamicDisks.size();
	const CDuckCollision::CDynamicDisk* pDisks = pCollision->m_aDynamicDisks.base_ptr();

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		CCharacterCore *pCharCore = pWorld->m_apCharacters[i];
		if(!pCharCore)
			continue;

		// handle player <-> player collision
		float Distance = distance(m_Pos, pCharCore->m_Pos);
		vec2 Dir = normalize(m_Pos - pCharCore->m_Pos);
		if(pWorld->m_Tuning.m_PlayerCollision && Distance < PhysSize*1.25f && Distance > 0.0f)
		{
			float a = (PhysSize*1.45f - Distance);
			float Velocity = 0.5f;

			// make sure that we don't add excess force by checking the
			// direction against the current velocity. if not zero.
			if (length(m_Vel) > 0.0001)
				Velocity = 1-(dot(normalize(m_Vel), Dir)+1)/2;

			m_Vel += Dir*a*(Velocity*0.75f);
			m_Vel *= 0.85f;
		}

		// handle hook influence
		/*if(m_HookedPlayer == i && m_pWorld->m_Tuning.m_PlayerHooking)
		{
			if(Distance > PhysSize*1.50f) // TODO: fix tweakable variable
			{
				float Accel = m_pWorld->m_Tuning.m_HookDragAccel * (Distance/m_pWorld->m_Tuning.m_HookLength);
				float DragSpeed = m_pWorld->m_Tuning.m_HookDragSpeed;

				// add force to the hooked player
				pCharCore->m_Vel.x = SaturatedAdd(-DragSpeed, DragSpeed, pCharCore->m_Vel.x, Accel*Dir.x*1.5f);
				pCharCore->m_Vel.y = SaturatedAdd(-DragSpeed, DragSpeed, pCharCore->m_Vel.y, Accel*Dir.y*1.5f);

				// add a little bit force to the guy who has the grip
				m_Vel.x = SaturatedAdd(-DragSpeed, DragSpeed, m_Vel.x, -Accel*Dir.x*0.25f);
				m_Vel.y = SaturatedAdd(-DragSpeed, DragSpeed, m_Vel.y, -Accel*Dir.y*0.25f);
			}
		}*/
	}

	for(int DiskId = 0; DiskId < DiskCount; DiskId++)
	{
		const CDuckCollision::CDynamicDisk& Disk = pDisks[DiskId];
		if(this == &Disk)
			continue;

		float Distance = distance(m_Pos, Disk.m_Pos);
		vec2 Dir = normalize(m_Pos - Disk.m_Pos);
		float MinDist = Disk.m_Radius + PhysSize * 0.5f;
		if(Distance < MinDist*1.25f && Distance > 0.0f)
		{
			float a = (MinDist*1.45f - Distance);
			float Velocity = 0.5f;

			// make sure that we don't add excess force by checking the
			// direction against the current velocity. if not zero.
			if (length(m_Vel) > 0.0001)
				Velocity = 1-(dot(normalize(m_Vel), Dir)+1)/2;

			m_Vel += Dir*a*(Velocity*0.75f);
			m_Vel *= 0.85f;
		}
	}

	// clamp the velocity to something sane
	if(length(m_Vel) > 6000)
		m_Vel = normalize(m_Vel) * 6000;
}

void CDuckCollision::CDynamicDisk::Move(CDuckCollision *pCollision, CWorldCore* pWorld)
{
	vec2 NewPos = m_Pos;
	pCollision->MoveBox(&NewPos, &m_Vel, vec2(m_Radius, m_Radius), 0, 0x0);

	const float PhysSize = 28;
	const float CollisionRadius = PhysSize*0.5+m_Radius;

	//if(FLAG collide with players)
	{
		// check player collision
		CCharacterCore** pWorldCharacters = pWorld->m_apCharacters;
		float Distance = distance(m_Pos, NewPos);
		int End = Distance+1;
		vec2 LastPos = m_Pos;

		for(int i = 0; i < End; i++)
		{
			float a = i/Distance;
			vec2 Pos = mix(m_Pos, NewPos, a);
			for(int p = 0; p < MAX_CLIENTS; p++)
			{
				CCharacterCore *pCharCore = pWorldCharacters[p];
				if(!pCharCore)
					continue;
				float D = distance(Pos, pCharCore->m_Pos);
				if(D < CollisionRadius && D > 0.0f)
				{
					if(a > 0.0f)
						m_Pos = LastPos;
					else if(distance(NewPos, pCharCore->m_Pos) > D)
						m_Pos = NewPos;
					return;
				}
			}
			LastPos = Pos;
		}
	}

	// TODO: if duck?
	const int DiskCount = pCollision->m_aDynamicDisks.size();
	const CDuckCollision::CDynamicDisk* pDisks = pCollision->m_aDynamicDisks.base_ptr();

	float Distance = distance(m_Pos, NewPos);
	int End = Distance+1;
	vec2 LastPos = m_Pos;
	for(int i = 0; i < End; i++)
	{
		float a = i/Distance;
		vec2 Pos = mix(m_Pos, NewPos, a);

		for(int DiskId = 0; DiskId < DiskCount; DiskId++)
		{
			const CDuckCollision::CDynamicDisk& Disk = pDisks[DiskId];
			if(this == &Disk)
				continue;

			float D = distance(Pos, Disk.m_Pos);
			if(D < CollisionRadius && D > 0.0f)
			{
				if(a > 0.0f)
					m_Pos = LastPos;
				else if(distance(NewPos, Disk.m_Pos) > D)
					m_Pos = NewPos;
				return;
			}
		}
	}

	m_Pos = NewPos;
}
