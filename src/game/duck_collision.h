#pragma once
#include "collision.h"
#include <base/tl/array.h>
#include <engine/shared/protocol.h>
#include <stdint.h>

class CWorldCore;

class CDuckCollision: public CCollision
{
public:
	~CDuckCollision();
	void Init(class CLayers *pLayers);
	void SetTileCollisionFlags(int Tx, int Ty, int Flags);

	bool TestBoxBig(vec2 Pos, vec2 Size, int Flag=COLFLAG_SOLID) const;
	void MoveBoxBig(vec2 *pInoutPos, vec2 *pInoutVel, vec2 Size, float Elasticity) const;
	bool IsBoxGrounded(vec2 Pos, vec2 Size);
};
