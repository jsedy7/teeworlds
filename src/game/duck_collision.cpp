#include "duck_collision.h"
#include <base/system.h>
#include <game/mapitems.h>

vec2 HookBlockPos = vec2(800, 512);
vec2 HookBlockSize = vec2(96, 64);
vec2 HookBlockVel = vec2(0, 0);

void CDuckCollision::Init(CLayers* pLayers)
{
	CCollision::Init(pLayers);

	m_aStaticBlocks.hint_size(1024);
	m_aDynamicDisks.hint_size(1024);
	m_aDynamicDisksPredicted.hint_size(1024);
	for(int i = 0; i < MAX_SOLIDBLOCK_FETCH_IDS; i++)
	{
		m_aSolidBlockDataID[i] = -1;
	}

	CDynamicDisk Disk;
	Disk.m_Pos = vec2(50, 50);
	Disk.m_Vel = vec2(0,0);
	Disk.m_Radius = 30;
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
	dbg_assert(Tx >= 0 && Tx < m_Width, "Tx out of bounds");
	dbg_assert(Ty >= 0 && Ty < m_Height, "Ty out of bounds");
	m_pTiles[Ty*m_Width+Tx].m_Index = Flags;
}

void CDuckCollision::SetSolidBlock(int BlockId, CStaticBlock Block)
{
	dbg_assert(BlockId >= 0 && BlockId < MAX_SOLIDBLOCK_FETCH_IDS, "BlockId out of bounds");

	Block.m_FetchID = BlockId;
	int DataID = m_aSolidBlockDataID[BlockId];

	if(DataID != -1)
		m_aStaticBlocks[DataID] = Block;
	else
	{
		DataID = m_aStaticBlocks.add(Block);
		m_aSolidBlockDataID[BlockId] = DataID;
	}
}

void CDuckCollision::ClearSolidBlock(int BlockId)
{
	dbg_assert(BlockId >= 0 && BlockId < MAX_SOLIDBLOCK_FETCH_IDS, "BlockId out of bounds");

	int DataID = m_aSolidBlockDataID[BlockId];
	if(DataID != -1)
	{
		// swap remove
		int LastFetchID = m_aStaticBlocks[m_aStaticBlocks.size()-1].m_FetchID;

		m_aStaticBlocks.remove_index_fast(DataID);

		if(m_aStaticBlocks.size() > 0)
			m_aSolidBlockDataID[LastFetchID] = DataID;

		m_aSolidBlockDataID[BlockId] = -1;
	}
}

void CDuckCollision::OnPredictStart()
{
	const int DiskCount = m_aDynamicDisks.size();
	m_aDynamicDisksPredicted.set_size(DiskCount);
	mem_move(m_aDynamicDisksPredicted.base_ptr(), m_aDynamicDisks.base_ptr(), sizeof(m_aDynamicDisks.base_ptr()[0]) * DiskCount);
}

void CDuckCollision::OnPredictTick()
{
	const int DiskCount = m_aDynamicDisks.size();
	for(int i = 0; i < DiskCount; i++)
	{
		CDynamicDisk& Disk = m_aDynamicDisks[i];
		Disk.m_Pos += Disk.m_Vel;
		// TODO: Disk.Move() like CCharacterCore::Move()
	}
}
