#include "duck_collision.h"
#include <base/system.h>
#include <game/mapitems.h>

vec2 HookBlockPos = vec2(800, 512);
vec2 HookBlockSize = vec2(96, 64);
vec2 HookBlockVel = vec2(0, 0);

bool CDuckCollision::CheckPoint(float x, float y) const
{
	if((x > HookBlockPos.x && x < (HookBlockPos.x+HookBlockSize.x) &&
		y > HookBlockPos.y && y < (HookBlockPos.y+HookBlockSize.y)))
	{
		return true;
	}

	return CCollision::CheckPoint(x, y);
}

int CDuckCollision::GetCollisionAt(float x, float y) const
{
	if((x > HookBlockPos.x && x < (HookBlockPos.x+HookBlockSize.x) &&
		y > HookBlockPos.y && y < (HookBlockPos.y+HookBlockSize.y)))
	{
		return COLFLAG_SOLID;
	}

	return CCollision::GetCollisionAt(x, y);
}


void CDuckCollision::SetTileCollisionFlags(int Tx, int Ty, int Flags)
{
	dbg_assert(Tx >= 0 && Tx < m_Width, "Tx out of bounds");
	dbg_assert(Ty >= 0 && Ty < m_Height, "Ty out of bounds");
	m_pTiles[Ty*m_Width+Tx].m_Index = Flags;
}
