#pragma once
#include "collision.h"
#include <base/tl/array.h>
#include <engine/shared/protocol.h>
#include <stdint.h>

class CWorldCore;

class CDuckCollision: public CCollision
{
public:
	void Init(class CLayers *pLayers);
	//bool CheckPoint(float x, float y, int Flag=COLFLAG_SOLID) const;
	//int GetCollisionAt(float x, float y) const;
	void SetTileCollisionFlags(int Tx, int Ty, int Flags);

	/*struct CStaticBlock
	{
		vec2 m_Pos;
		vec2 m_Size;
		int16_t m_Flags;
		int16_t m_FetchID;
	};*/

	enum {
		MAX_STATICBLOCK_FETCH_IDS=10000,
		MAX_DYNDISK_FETCH_IDS=10000,
	};

	/*int16_t m_aStaticBlockDataID[MAX_STATICBLOCK_FETCH_IDS];
	int16_t m_aDynDiskDataID[MAX_DYNDISK_FETCH_IDS];
	array<CStaticBlock> m_aStaticBlocks;
	array<CDynamicDisk> m_aDynamicDisks;
	int16_t m_aCharacterHookDiskFetchId[MAX_CLIENTS];
	vec2 m_aCharacterHookDiskOffset[MAX_CLIENTS];*/

	/*void SetStaticBlock(int BlockId, CStaticBlock Block);
	void ClearStaticBlock(int BlockId);
	void SetDynamicDisk(int DiskId, CDynamicDisk Disk);
	CDynamicDisk* GetDynamicDisk(int DiskId);
	void ClearDynamicDisk(int DiskId);*/

	//void Tick(CWorldCore* pWorld);

	void Reset();
	bool TestBoxBig(vec2 Pos, vec2 Size, int Flag=COLFLAG_SOLID) const;
	void MoveBoxBig(vec2 *pInoutPos, vec2 *pInoutVel, vec2 Size, float Elasticity) const;
	bool IsBoxGrounded(vec2 Pos, vec2 Size);
};
