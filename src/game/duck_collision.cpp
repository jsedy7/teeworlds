#include "duck_collision.h"
#include <base/system.h>
#include <game/mapitems.h>
#include <game/gamecore.h>
#include <game/layers.h>

const float MinStaticPhysSize = 30; // actually the smallest boject right now is a map tile (32 x 32)

void CDuckCollision::Init(CLayers* pLayers)
{
	m_pLayers = pLayers;
	m_Width = m_pLayers->GameLayer()->m_Width;
	m_Height = m_pLayers->GameLayer()->m_Height;
	m_pTiles = static_cast<CTile *>(m_pLayers->Map()->GetData(m_pLayers->GameLayer()->m_Data));

	//Reset();
}

/*bool CDuckCollision::CheckPoint(float x, float y, int Flag) const
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
*/

void CDuckCollision::Reset()
{
	/*m_aStaticBlocks.hint_size(1024);
	m_aStaticBlocks.set_size(0); // clear

	for(int i = 0; i < MAX_STATICBLOCK_FETCH_IDS; i++)
	{
		m_aStaticBlockDataID[i] = -1;
	}*/
}

void CDuckCollision::SetTileCollisionFlags(int Tx, int Ty, int Flags)
{
	if(!(Tx >= 0 && Tx < m_Width) || !(Ty >= 0 && Ty < m_Height))
		return;
	m_pTiles[Ty*m_Width+Tx].m_Index = Flags;
}

bool CDuckCollision::TestBoxBig(vec2 Pos, vec2 Size, int Flag) const
{
	if(TestBox(Pos, Size, Flag))
		return true;

	// multi sample the rest
	const int MsCount = (int)(Size.x / MinStaticPhysSize);
	const float MsGap = Size.x / MsCount;

	if(!MsCount)
		return false;

	Size *= 0.5;

	// top
	for(int i = 0; i < MsCount; i++)
	{
		if(CheckPoint(Pos.x-Size.x + (i+1) * MsGap, Pos.y-Size.y, Flag))
			return true;
	}

	// bottom
	for(int i = 0; i < MsCount; i++)
	{
		if(CheckPoint(Pos.x-Size.x + (i+1) * MsGap, Pos.y+Size.y, Flag))
			return true;
	}

	// left
	for(int i = 0; i < MsCount; i++)
	{
		if(CheckPoint(Pos.x-Size.x, Pos.y-Size.y + (i+1) * MsGap, Flag))
			return true;
	}

	// right
	for(int i = 0; i < MsCount; i++)
	{
		if(CheckPoint(Pos.x+Size.x, Pos.y-Size.y + (i+1) * MsGap, Flag))
			return true;
	}
	return 0;
}

void CDuckCollision::MoveBoxBig(vec2 *pInoutPos, vec2 *pInoutVel, vec2 Size, float Elasticity) const
{
	// do the move
	vec2 Pos = *pInoutPos;
	vec2 Vel = *pInoutVel;

	float Distance = length(Vel);
	int Max = (int)Distance;

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

			if(TestBoxBig(vec2(NewPos.x, NewPos.y), Size))
			{
				int Hits = 0;

				if(TestBoxBig(vec2(Pos.x, NewPos.y), Size))
				{
					NewPos.y = Pos.y;
					Vel.y *= -Elasticity;
					Hits++;
				}

				if(TestBoxBig(vec2(NewPos.x, Pos.y), Size))
				{
					NewPos.x = Pos.x;
					Vel.x *= -Elasticity;
					Hits++;
				}

				// neither of the tests got a collision.
				// this is a real _corner case_!
				if(Hits == 0)
				{
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

bool CDuckCollision::IsBoxGrounded(vec2 Pos, vec2 Size)
{
	// multi sample
	const int MsCount = (int)(Size.x / MinStaticPhysSize);
	const float MsGap = Size.x / MsCount;

	Size *= 0.5;

	if(MsCount)
	{
		// bottom
		for(int i = 0; i < MsCount; i++)
		{
			if(CheckPoint(Pos.x-Size.x + (i+1) * MsGap, Pos.y+Size.y+5, COLFLAG_SOLID))
				return true;
		}
	}

	if(CheckPoint(Pos.x-Size.x, Pos.y+Size.y+5, COLFLAG_SOLID))
		return true;
	if(CheckPoint(Pos.x+Size.x, Pos.y+Size.y+5, COLFLAG_SOLID))
		return true;
	return false;
}
