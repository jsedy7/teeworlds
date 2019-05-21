#include "duck_collision.h"
#include <base/system.h>
#include <game/mapitems.h>

void CDuckCollision::Init(CLayers* pLayers)
{
	CCollision::Init(pLayers);

	m_aStaticBlocks.hint_size(1024);
	m_aDynamicDisks.hint_size(1024);
	for(int i = 0; i < MAX_STATICBLOCK_FETCH_IDS; i++)
	{
		m_aStaticBlockDataID[i] = -1;
	}
	for(int i = 0; i < MAX_DYNDISK_FETCH_IDS; i++)
	{
		m_aDynDiskDataID[i] = -1;
	}
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

void CDuckCollision::Tick()
{
	const int DiskCount = m_aDynamicDisks.size();
	for(int i = 0; i < DiskCount; i++)
	{
		CDuckCollision::CDynamicDisk& Disk = m_aDynamicDisks[i];
		Disk.m_Pos += Disk.m_Vel;
		// TODO: Disk.Move() like CCharacterCore::Move()
	}
}
