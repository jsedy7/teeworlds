#pragma once
#include <stdint.h>

typedef uint32_t u32;
typedef int32_t i32;

struct DukNetObjID
{
	enum Enum {
		TEST=0,
		DEBUG_RECT,
		MAP_RECT_SET_SOLID,

		HOOK_BLOCK,
		DYNAMIC_DISK,

		_COUNT
	};
};

struct CNetObj_Test
{
	enum { NET_ID = DukNetObjID::TEST };

	i32 ClientID;
	float Value1;
};

struct CNetObj_DebugRect
{
	enum { NET_ID = DukNetObjID::DEBUG_RECT };

	int id;
	float x;
	float y;
	float w;
	float h;
	u32 color;
};

struct CNetObj_MapRectSetSolid
{
	enum { NET_ID = DukNetObjID::MAP_RECT_SET_SOLID };

	i32 solid;
	i32 hookable;
	i32 x;
	i32 y;
	i32 w;
	i32 h;
};

struct CNetObj_HookBlock
{
	enum { NET_ID = DukNetObjID::HOOK_BLOCK };

	int m_Id;
	int m_Flags;
	float m_PosX;
	float m_PosY;
	float m_VelX;
	float m_VelY;
	float m_Width;
	float m_Height;
};

struct CNetObj_DynamicDisk
{
	enum { NET_ID = DukNetObjID::DYNAMIC_DISK };

	int m_Id;
	int m_Flags;
	float m_PosX;
	float m_PosY;
	float m_VelX;
	float m_VelY;
	float m_Radius;
	float m_HookForce;
};
