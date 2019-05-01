#pragma once
#include "collision.h"
#include <base/tl/array.h>

class CDuckCollision: public CCollision
{
public:
	void Init(class CLayers *pLayers);
	bool CheckPoint(float x, float y) const;
	int GetCollisionAt(float x, float y) const;
	void SetTileCollisionFlags(int Tx, int Ty, int Flags);

	struct CSolidBlock
	{
		vec2 m_Pos;
		vec2 m_Size;
		int m_Flags;
	};

	array<CSolidBlock> m_aSolidBlocks;
};
