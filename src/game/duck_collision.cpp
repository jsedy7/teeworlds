#include "duck_collision.h"
#include <base/system.h>
#include <game/mapitems.h>

vec2 HookBlockPos = vec2(800, 512);
vec2 HookBlockSize = vec2(96, 64);
vec2 HookBlockVel = vec2(0, 0);

void CDuckCollision::Init(CLayers* pLayers)
{
	CCollision::Init(pLayers);

	m_aSolidBlocks.hint_size(1024);
	for(int i = 0; i < MAX_SOLIDBLOCK_FETCH_IDS; i++)
	{
		m_aSolidBlockDataID[i] = -1;
	}
}

bool CDuckCollision::CheckPoint(float x, float y, int Flag) const
{
	const int SolidBlockCount = m_aSolidBlocks.size();
	const CSolidBlock* pBlocks = m_aSolidBlocks.base_ptr();
	for(int i = 0; i < SolidBlockCount; i++)
	{
		const CSolidBlock& Block = pBlocks[i];

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
	const int SolidBlockCount = m_aSolidBlocks.size();
	const CSolidBlock* pBlocks = m_aSolidBlocks.base_ptr();
	for(int i = 0; i < SolidBlockCount; i++)
	{
		const CSolidBlock& Block = pBlocks[i];

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

void CDuckCollision::SetSolidBlock(int BlockId, CSolidBlock Block)
{
	dbg_assert(BlockId >= 0 && BlockId < MAX_SOLIDBLOCK_FETCH_IDS, "BlockId out of bounds");

	Block.m_FetchID = BlockId;
	int DataID = m_aSolidBlockDataID[BlockId];

	if(DataID != -1)
		m_aSolidBlocks[DataID] = Block;
	else
	{
		DataID = m_aSolidBlocks.add(Block);
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
		int LastFetchID = m_aSolidBlocks[m_aSolidBlocks.size()-1].m_FetchID;

		m_aSolidBlocks.remove_index_fast(DataID);

		if(m_aSolidBlocks.size() > 0)
			m_aSolidBlockDataID[LastFetchID] = DataID;

		m_aSolidBlockDataID[BlockId] = -1;
	}
}
