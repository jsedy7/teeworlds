#pragma once
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

struct DukNetObjID
{
	enum Enum {
		TEST=0,
		RECT,
		_COUNT
	};
};

struct CNetObj_Test
{
	enum { NET_ID = DukNetObjID::TEST };

	uint16_t ClientID;
	float Value1;
};

struct CNetObj_Rect
{
	enum { NET_ID = DukNetObjID::RECT };

	float x;
	float y;
	float w;
	float h;
};
