#include "duck_collision.h"
#include <base/system.h>
#include <game/mapitems.h>

vec2 HookBlockPos = vec2(800, 512);
vec2 HookBlockSize = vec2(96, 64);
vec2 HookBlockVel = vec2(0, 0);

void CDuckCollision::Init(CLayers* pLayers)
{
	CCollision::Init(pLayers);

	m_aSolidBlocks.set_size(1024);
	const int SolidBlockCount = m_aSolidBlocks.size();
	for(int i = 0; i < SolidBlockCount; i++)
	{
		m_aSolidBlocks[i].m_Flags = -1;
	}
}

bool CDuckCollision::CheckPoint(float x, float y) const
{
	const int SolidBlockCount = m_aSolidBlocks.size();
	for(int i = 0; i < SolidBlockCount; i++)
	{
		const CSolidBlock& Block = m_aSolidBlocks[i];
		if(Block.m_Flags == -1)
			continue;

		if((x > Block.m_Pos.x && x < (Block.m_Pos.x+Block.m_Size.x) &&
			y > Block.m_Pos.y && y < (Block.m_Pos.y+Block.m_Size.y)))
		{
			return true;
		}
	}

	return CCollision::CheckPoint(x, y);
}

int CDuckCollision::GetCollisionAt(float x, float y) const
{
	const int SolidBlockCount = m_aSolidBlocks.size();
	for(int i = 0; i < SolidBlockCount; i++)
	{
		const CSolidBlock& Block = m_aSolidBlocks[i];
		if(Block.m_Flags == -1)
			continue;

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
