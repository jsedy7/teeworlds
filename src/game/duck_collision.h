#pragma once
#include "collision.h"

class CDuckCollision: public CCollision
{
public:
	bool CheckPoint(float x, float y) const;
	int GetCollisionAt(float x, float y) const;

	void SetTileCollisionFlags(int Tx, int Ty, int Flags);
};
