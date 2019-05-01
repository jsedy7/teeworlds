#pragma once
#include "collision.h"

class CDuckCollision: public CCollision
{
public:
	bool CheckPoint(float x, float y) const;
	int GetCollisionAt(float x, float y) const;
};
