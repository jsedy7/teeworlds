#include "duck_collision.h"

bool CDuckCollision::CheckPoint(float x, float y) const
{
	vec2 HookBlockPos = vec2(800, 500);
	vec2 HookBlockSize = vec2(100, 50);
	vec2 HookBlockVel = vec2(0, 0);

	if((x >= HookBlockPos.x && x < (HookBlockPos.x+HookBlockSize.x) &&
		y >= HookBlockPos.y && y < (HookBlockPos.y+HookBlockSize.y)))
	{
		return true;
	}

	return CCollision::CheckPoint(x, y);
}

int CDuckCollision::GetCollisionAt(float x, float y) const
{
	vec2 HookBlockPos = vec2(800, 500);
	vec2 HookBlockSize = vec2(100, 50);
	vec2 HookBlockVel = vec2(0, 0);

	if((x >= HookBlockPos.x && x < (HookBlockPos.x+HookBlockSize.x) &&
		y >= HookBlockPos.y && y < (HookBlockPos.y+HookBlockSize.y)))
	{
		return COLFLAG_SOLID;
	}

	return CCollision::GetCollisionAt(x, y);
}
