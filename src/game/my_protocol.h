#pragma once
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

struct DukNetObjID
{
	enum Enum {
		TEST=0,
		DEBUG_RECT,
		MAP_RECT_SET_SOLID,
		_COUNT
	};
};

struct CNetObj_Test
{
	enum { NET_ID = DukNetObjID::TEST };

	u16 ClientID;
	float Value1;
};

struct CNetObj_DebugRect
{
	enum { NET_ID = DukNetObjID::DEBUG_RECT };

	float x;
	float y;
	float w;
	float h;
	u32 color;
};

struct CNetObj_MapRectSetSolid
{
	enum { NET_ID = DukNetObjID::MAP_RECT_SET_SOLID };

	u8 solid;
	u16 x;
	u16 y;
	u16 w;
	u16 h;
};
