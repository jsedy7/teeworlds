#pragma once
#include "collision.h"
#include <base/tl/array.h>
#include <stdint.h>

class CDuckCollision: public CCollision
{
public:
	void Init(class CLayers *pLayers);
	bool CheckPoint(float x, float y, int Flag=COLFLAG_SOLID) const;
	int GetCollisionAt(float x, float y) const;
	void SetTileCollisionFlags(int Tx, int Ty, int Flags);

	struct CStaticBlock
	{
		vec2 m_Pos;
		vec2 m_Size;
		int16_t m_Flags;
		int16_t m_FetchID;
	};

	struct CDynamicDisk
	{
		vec2 m_Pos;
		vec2 m_Vel;
		float m_Radius;
	};

	enum {
		MAX_SOLIDBLOCK_FETCH_IDS=10000
	};

	int16_t m_aSolidBlockDataID[MAX_SOLIDBLOCK_FETCH_IDS];
	array<CStaticBlock> m_aStaticBlocks;
	array<CDynamicDisk> m_aDynamicDisks;

	void SetSolidBlock(int BlockId, CStaticBlock Block);
	void ClearSolidBlock(int BlockId);
};
