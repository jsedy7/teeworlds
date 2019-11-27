// NOTE: Fonctions arguments start at 1 when using the Lua back-end (0 when using duktape)

#include "duck_lua.h"
#include "duck_bridge.h"

extern "C" {
	#include <lua.h>
	#include <lualib.h>
	#include <lauxlib.h>
}

static CDuckLua* s_DuckLua = 0;
inline CDuckLua* This() { return s_DuckLua; }

#define LUA_WARN(fmt, ...) This()->Bridge()->ScriptError(JsErrorLvl::WARNING, fmt, __VA_ARGS__)
#define LUA_ERR(fmt, ...) This()->Bridge()->ScriptError(JsErrorLvl::ERROR, fmt, __VA_ARGS__)
#define LUA_CRIT(fmt, ...) This()->Bridge()->ScriptError(JsErrorLvl::CRITICAL, fmt, __VA_ARGS__)

#define GetFunctionRef(F) GetFunctionFromRef(m_FuncRef##F, #F)

#ifdef CONF_DEBUG
	#define DBG_DETECTSTACKLEAK() if(IsStackLeaking()) {\
		volatile int StackTop = lua_gettop(L());\
		dbg_assert(0, "stack leak"); }
#else
	#define DBG_DETECTSTACKLEAK()
#endif

bool CDuckLua::_CheckArgumentCountImp(lua_State* L, int NumArgs, const char* pFuncName)
{
	int n = lua_gettop(L);
	if(n != NumArgs)
	{
		LUA_ERR("Tw%s() wrong argument count (%d), expected %d", pFuncName, n, NumArgs);
		return false;
	}

	for(int i = 0; i < n; i++)
	{
		if(lua_isnil(L, i+1))
		{
			LUA_ERR("Tw%s() wrong argument count (%d), expected %d (%d argument is nil)", pFuncName, n, NumArgs, i);
			return false;
		}
	}

	return true;
}

#define CheckArgumentCount(L, num) if(!This()->_CheckArgumentCountImp(L, num, __FUNCTION__ + 16)) { return 0; }


static void LuaGetPropString(lua_State *L, int Index, const char *pPropName, char *pOut, int OutSize)
{
	lua_getfield(L, Index, pPropName);
	if(!lua_isnil(L, -1))
	{
		const char* pStr = lua_tostring(L, -1);
		str_copy(pOut, pStr, OutSize);
	}
	lua_pop(L, 1);
}

static void LuaGetPropInteger(lua_State *L, int Index, const char *pPropName, int64_t *pOut)
{
	lua_getfield(L, Index, pPropName);
	if(!lua_isnil(L, -1))
		*pOut = lua_tointeger(L, -1);
	lua_pop(L, 1);
}

static void LuaGetPropNumber(lua_State *L, int Index, const char *pPropName, double *pOut)
{
	lua_getfield(L, Index, pPropName);
	if(!lua_isnil(L, -1))
		*pOut = lua_tonumber(L, -1);
	lua_pop(L, 1);
}

static void LuaGetPropBool(lua_State *L, int Index, const char *pPropName, bool *pOut)
{
	lua_getfield(L, Index, pPropName);
	if(!lua_isnil(L, -1))
		*pOut = lua_toboolean(L, -1);
	lua_pop(L, 1);
}

static void LuaSetPropInteger(lua_State *L, int Index, const char *pPropName, int Num)
{
	lua_pushstring(L, pPropName);
	lua_pushinteger(L, Num);
	lua_rawset(L, Index - 2);
}

static void LuaSetPropNumber(lua_State *L, int Index, const char *pPropName, double Num)
{
	lua_pushstring(L, pPropName);
	lua_pushnumber(L, Num);
	lua_rawset(L, Index - 2);
}

static void LuaSetPropNil(lua_State *L, int Index, const char *pPropName)
{
	lua_pushstring(L, pPropName);
	lua_pushnil(L);
	lua_rawset(L, Index - 2);
}

static void LuaSetPropBool(lua_State *L, int Index, const char *pPropName, bool Val)
{
	lua_pushstring(L, pPropName);
	lua_pushboolean(L, Val);
	lua_rawset(L, Index - 2);
}

static void LuaSetPropString(lua_State *L, int Index, const char *pPropName, const char *pStr)
{
	lua_pushstring(L, pPropName);
	lua_pushstring(L, pStr);
	lua_rawset(L, Index - 2);
}

static void LuaSetPropStringN(lua_State *L, int Index, const char *pPropName, const char *pStr, int Len)
{
	lua_pushstring(L, pPropName);
	lua_pushlstring(L, pStr, Len);
	lua_rawset(L, Index - 2);
}

void CDuckLua::PrintToConsole(const char* pStr, int Len)
{
	const int MaxLen = 128;
	char aBuff[MaxLen+1];

	for(int i = 0; i < Len; i += MaxLen)
	{
		str_copy(aBuff, pStr + i, min(Len-i+1, MaxLen+1));
		Bridge()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "mod", aBuff);
	}
}

static void TableToStr(lua_State *L, char* pStr, int MaxLen)
{
	char* pCur = pStr;

#define APPEND(STR, STRLEN) if(pCur-pStr < MaxLen) { str_append(pCur, STR, MaxLen - (pCur-pStr)); pCur += STRLEN; }

	APPEND("{ ", 2);

	lua_pushnil(L);  /* first key */
	while(lua_next(L, -2) != 0)
	{
		APPEND("[", 1);
		if(lua_type(L, -2) == LUA_TSTRING)
		{
			size_t Len;
			const char* pKey = lua_tolstring(L, -2, &Len);
			APPEND(pKey, Len);

		}
		else
		{
			char aNum[16];
			str_format(aNum, sizeof(aNum), "%d", lua_tointeger(L, -2));
			APPEND(aNum, str_length(aNum));
		}
		APPEND("] = ", 4);

		if(lua_istable(L, -1))
		{
			TableToStr(L, pCur, MaxLen - (pCur-pStr));
		}
		else
		{
			const bool IsString = lua_type(L, -1) == LUA_TSTRING;
			if(IsString)
				APPEND("\"", 1);

			size_t Len;
			const char* pVal = lua_tolstring(L, -1, &Len);
			APPEND(pVal, Len);

			if(IsString)
				APPEND("\"", 1);
		}

		APPEND(", ", 2);

		lua_pop(L, 1);
	}

	APPEND("}", 1);

#undef APPEND
}

/*#
`print(arg1)`

Prints arg1 to console.

**Parameters**

* **arg1**: any

**Returns**

* None
#*/
int CDuckLua::NativePrint(lua_State *L)
{
	CheckArgumentCount(L, 1);

	if(lua_istable(L, 1))
	{
		static char aBuff[1024*1024]; // 1MB
		aBuff[0] = 0;
		TableToStr(L, aBuff, sizeof(aBuff));
		This()->PrintToConsole(aBuff, str_length(aBuff));
		return 0;
	}

	const char* pStr = lua_tostring(L, 1);
	if(!pStr)
		return 0;

	This()->PrintToConsole(pStr, str_length(pStr));
	return 0;
}

/*#
`require(file)`

Includes lua script file.

**Parameters**

* **file**: string

**Returns**

* None
#*/
int CDuckLua::NativeRequire(lua_State *L)
{
	CheckArgumentCount(L, 1);

	const char* pStr = lua_tostring(L, 1);
	char aBuff[128];
	str_copy(aBuff, pStr, sizeof(aBuff));

	int ScriptFileID = This()->FindScriptFileFromName(aBuff);
	if(ScriptFileID != -1)
	{
		This()->LuaLoadScriptFileData(ScriptFileID);
	}

	return 0;
}

/*#
`cos(angle)`

Returns cosine of angle (radians).

**Parameters**

* **angle**: float

**Returns**

* **cosine**: float
#*/
int CDuckLua::NativeCos(lua_State *L)
{
	CheckArgumentCount(L, 1);
	double a = lua_tonumber(L, 1);
	lua_pushnumber(L, cos(a));
	return 1;
}

/*#
`sin(angle)`

Returns sine of angle (radians).

**Parameters**

* **angle**: float

**Returns**

* **sine**: float
#*/
int CDuckLua::NativeSin(lua_State *L)
{
	CheckArgumentCount(L, 1);
	double a = lua_tonumber(L, 1);
	lua_pushnumber(L, sin(a));
	return 1;
}

/*#
`sqrt(value)`

Returns square root of value.

**Parameters**

* **value**: float

**Returns**

* **result**: float
#*/
int CDuckLua::NativeSqrt(lua_State *L)
{
	CheckArgumentCount(L, 1);
	double a = lua_tonumber(L, 1);
	lua_pushnumber(L, sqrt(a));
	return 1;
}

/*#
`atan2(y, x)`

Returns atan2 of y, x.

**Parameters**

* **y**: float
* **x**: float

**Returns**

* **result**: float
#*/
int CDuckLua::NativeAtan2(lua_State *L)
{
	CheckArgumentCount(L, 2);
	double a = lua_tonumber(L, 1);
	double b = lua_tonumber(L, 2);
	lua_pushnumber(L, atan2(a, b));
	return 1;
}

/*#
`floor(num)`

Returns floor of num.

**Parameters**

* **num**: float

**Returns**

* **result**: int
#*/
int CDuckLua::NativeFloor(lua_State *L)
{
	CheckArgumentCount(L, 1);
	double a = lua_tonumber(L, 1);
	lua_pushinteger(L, floor(a));
	return 1;
}

/*#
`TwRenderQuad(x, y, width, height)`

Draws a quad.

**Parameters**

* **x**: float
* **y**: float
* **width**: float
* **height**: float

**Returns**

* None
#*/
int CDuckLua::NativeRenderQuad(lua_State *L)
{
	CheckArgumentCount(L, 4);

	double x = lua_tonumber(L, 1);
	double y = lua_tonumber(L, 2);
	double Width = lua_tonumber(L, 3);
	double Height = lua_tonumber(L, 4);

	IGraphics::CQuadItem Quad(x, y, Width, Height);
	This()->Bridge()->QueueDrawQuad(Quad);
	return 0;
}

/*#
`TwRenderQuadCentered(x, y, width, height)`

Draws a quad centered.

**Parameters**

* **x**: float
* **y**: float
* **width**: float
* **height**: float

**Returns**

* None
#*/
int CDuckLua::NativeRenderQuadCentered(lua_State *L)
{
	CheckArgumentCount(L, 4);

	double x = lua_tonumber(L, 1);
	double y = lua_tonumber(L, 2);
	double Width = lua_tonumber(L, 3);
	double Height = lua_tonumber(L, 4);

	IGraphics::CQuadItem Quad(x, y, Width, Height);
	This()->Bridge()->QueueDrawQuadCentered(Quad);
	return 0;
}

/*#
`TwRenderSetColorU32(color)`

| Set render color state as uint32 RGBA.
| Example: ``0x7F0000FF`` is half transparent red.

**Parameters**

* **color**: int

**Returns**

* None
#*/
int CDuckLua::NativeRenderSetColorU32(lua_State *L)
{
	CheckArgumentCount(L, 1);

	uint32_t x = (uint32_t)lua_tointeger(L, 1); // NOTE: this returns a int64

	float aColor[4];
	aColor[0] = (x & 0xFF) / 255.f;
	aColor[1] = ((x >> 8) & 0xFF) / 255.f;
	aColor[2] = ((x >> 16) & 0xFF) / 255.f;
	aColor[3] = ((x >> 24) & 0xFF) / 255.f;

	This()->Bridge()->QueueSetColor(aColor);
	return 0;
}

/*#
`TwRenderSetColorF4(r, g, b, a)`

| Set render color state as float RGBA.

**Parameters**

* **r**: float (0.0 - 1.0)
* **g**: float (0.0 - 1.0)
* **b**: float (0.0 - 1.0)
* **a**: float (0.0 - 1.0)

**Returns**

* None
#*/
int CDuckLua::NativeRenderSetColorF4(lua_State *L)
{
	CheckArgumentCount(L, 4);

	float aColor[4] = {1, 1, 1, 1};
	aColor[0] = lua_tonumber(L, 1);
	aColor[1] = lua_tonumber(L, 2);
	aColor[2] = lua_tonumber(L, 3);
	aColor[3] = lua_tonumber(L, 4);

	This()->Bridge()->QueueSetColor(aColor);
	return 0;
}


/*#
`TwRenderSetTexture(texture_id)`

| Set texture for subsequent draws.
| Example: ``TwRenderSetTexture(TwGetModTexture("duck_butt"));``

**Parameters**

* **texture_id**: int

**Returns**

* None
#*/
int CDuckLua::NativeRenderSetTexture(lua_State* L)
{
	CheckArgumentCount(L, 1);

	This()->Bridge()->QueueSetTexture((int)lua_tointeger(L, 1));
	return 0;
}

/*#
`TwRenderSetQuadSubSet(x1, y1, x2, y2)`

| Set quad texture coordinates. ``0, 0, 1, 1`` is default.

**Parameters**

* **x1**: float
* **y1**: float
* **x2**: float
* **y2**: float

**Returns**

* None
#*/
int CDuckLua::NativeRenderSetQuadSubSet(lua_State* L)
{
	CheckArgumentCount(L, 4);

	float aSubSet[4];
	aSubSet[0] = lua_tonumber(L, 1);
	aSubSet[1] = lua_tonumber(L, 2);
	aSubSet[2] = lua_tonumber(L, 3);
	aSubSet[3] = lua_tonumber(L, 4);

	This()->Bridge()->QueueSetQuadSubSet(aSubSet);
	return 0;
}

/*#
`TwRenderSetQuadRotation(angle)`

| Set quad rotation.

**Parameters**

* **angle**: float (radians)

**Returns**

* None
#*/
int CDuckLua::NativeRenderSetQuadRotation(lua_State* L)
{
	CheckArgumentCount(L, 1);

	float Angle = lua_tonumber(L, 1);
	This()->Bridge()->QueueSetQuadRotation(Angle);
	return 0;
}


/*#
`TwRenderSetTeeSkin(skin)`

| **DEPRECATED**
| Set tee skin for next tee draw call.

**Parameters**

* **skin**

.. code-block:: js

	var skin = {
		textures: [
			texid_body,
			texid_marking,
			texid_decoration,
			texid_hands,
			texid_feet,
			texid_eyes
		],

		colors: [
			color_body,
			color_marking,
			color_decoration,
			color_hands,
			color_feet,
			color_eyes
		]
	};

| texid_*: Use TwGetSkinPartTexture() to get this texture id.
| color_*: ``var color = {r: 1, g: 1, b: 1, a: 1};``

**Returns**

* None
#*/
int CDuckLua::NativeRenderSetTeeSkin(lua_State* L)
{
	LUA_ERR("TwRenderSetTeeSkin is deprecated");

#if 0
	CheckArgumentCount(L, 1);

	CDuckBridge::CTeeSkinInfo SkinInfo;

	for(int i = 0; i < NUM_SKINPARTS; i++)
	{
		SkinInfo.m_aTextures[i]	= -1;
		SkinInfo.m_aColors[i][0] = 1;
		SkinInfo.m_aColors[i][1] = 1;
		SkinInfo.m_aColors[i][2] = 1;
		SkinInfo.m_aColors[i][3] = 1;
	}

	if(duk_get_prop_string(ctx, 0, "textures"))
	{
		for(int i = 0; i < NUM_SKINPARTS; i++)
		{
			if(duk_get_prop_index(ctx, -1, i))
			{
				if(duk_is_null_or_undefined(ctx, -1)) {
					SkinInfo.m_aTextures[i] = -1;
				}
				else {
					SkinInfo.m_aTextures[i] = (int)duk_to_int(ctx, -1);
				}
			}
			duk_pop(ctx);
		}
	}
	duk_pop(ctx);

	if(duk_get_prop_string(ctx, 0, "colors"))
	{
		for(int i = 0; i < NUM_SKINPARTS; i++)
		{
			// TODO: make colors simple arrays? instead of rgba objects
			if(duk_get_prop_index(ctx, -1, i))
			{
				duk_to_object(ctx, -1);
				DukGetFloatProp(ctx, -1, "r", &SkinInfo.m_aColors[i][0]);
				DukGetFloatProp(ctx, -1, "g", &SkinInfo.m_aColors[i][1]);
				DukGetFloatProp(ctx, -1, "b", &SkinInfo.m_aColors[i][2]);
				DukGetFloatProp(ctx, -1, "a", &SkinInfo.m_aColors[i][3]);
			}
			duk_pop(ctx);
		}
	}
	duk_pop(ctx);

	This()->Bridge()->QueueSetTeeSkin(SkinInfo);
#endif
	return 0;
}

/*#
`TwRenderSetFreeform(array_vertices)`

| Set free form quad (custom vertices).
| Every 8 number produces a quad.
| Example: ``0,0, 1,0, 1,1, 0,1``.

**Parameters**

* **array_vertices**: float[]

**Returns**

* None
#*/
int CDuckLua::NativeRenderSetFreeform(lua_State* L)
{
	CheckArgumentCount(L, 1);

	static IGraphics::CFreeformItem aFreeformBuffer[CDuckBridge::CRenderSpace::FREEFORM_MAX_COUNT];
	int FreeformCount = 0;
	int VertCount = 0;
	float CurrentFreeform[sizeof(IGraphics::CFreeformItem)/sizeof(float)];
	const int FfFloatCount = sizeof(CurrentFreeform)/sizeof(CurrentFreeform[0]);

	if(!lua_istable(L, 1))
	{
		LUA_ERR("TwRenderSetFreeform(arg1): arg1 is not a table");
		return 0;
	}

	const int ArrayLength = min((int)lua_objlen(L, 1), (int)CDuckBridge::CRenderSpace::FREEFORM_MAX_COUNT * FfFloatCount);

	for(int i = 0; i < ArrayLength; i++)
	{
		lua_rawgeti(L, 1, i+1);

		CurrentFreeform[VertCount++] = lua_tonumber(L, -1);

		if(VertCount >= FfFloatCount)
		{
			VertCount = 0;
			aFreeformBuffer[FreeformCount++] = *(IGraphics::CFreeformItem*)CurrentFreeform;
		}

		lua_pop(L, 1);
	}

	if(VertCount > 0)
	{
		mem_zero(CurrentFreeform+VertCount, sizeof(CurrentFreeform)-sizeof(float)*VertCount);
		aFreeformBuffer[FreeformCount++] = *(IGraphics::CFreeformItem*)CurrentFreeform;
	}

	This()->Bridge()->QueueSetFreeform(aFreeformBuffer, FreeformCount);
	return 0;
}

/*#
`TwRenderSetDrawSpace(draw_space_id)`

| Select draw space to draw on.
| Example: ``TwRenderSetDrawSpace(Teeworlds.DRAW_SPACE_HUD)``.

**Parameters**

* **draw_space_id**: int

**Returns**

* None
#*/
int CDuckLua::NativeRenderSetDrawSpace(lua_State* L)
{
	CheckArgumentCount(L, 1);

	int ds = lua_tointeger(L, 1);

	if(!This()->Bridge()->RenderSetDrawSpace(ds))
	{
		LUA_ERR("TwRenderSetDrawSpace(arg1): Draw space undefined");
	}
	return 0;
}

/*#
`TwRenderDrawTeeBodyAndFeet(tee)`

| **DEPRECATED**
| Draws a tee without hands.

**Parameters**

* **tee**:

.. code-block:: js

	var tee = {
		size: float,
		angle: float,
		pos_x: float,
		pos_y: float,
		is_walking: bool,
		is_grounded: bool,
		got_air_jump: bool,
		emote: int,
	};

**Returns**

* None
#*/
int CDuckLua::NativeRenderDrawTeeBodyAndFeet(lua_State* L)
{
	LUA_ERR("TwRenderDrawTeeBodyAndFeet is deprecated");

#if 0
	CheckArgumentCount(L, 1);

	/*
	 * tee = {
	 *	size: float,
	 *	angle: float,
	 *	pos_x: float,
	 *	pos_y: float,
	 *	is_walking: bool,
	 *	is_grounded: bool,
	 *	got_air_jump: bool,
	 *	emote: int,
	 * }
	 *
	 */

	float Size = 64;
	float Angle = 0;
	float PosX = 0;
	float PosY = 0;
	bool IsWalking = false;
	bool IsGrounded = true;
	bool GotAirJump = true;
	int Emote = 0;

	DukGetFloatProp(ctx, 0, "size", &Size);
	DukGetFloatProp(ctx, 0, "angle", &Angle);
	DukGetFloatProp(ctx, 0, "pos_x", &PosX);
	DukGetFloatProp(ctx, 0, "pos_y", &PosY);
	DukGetBoolProp(ctx, 0, "is_walking", &IsWalking);
	DukGetBoolProp(ctx, 0, "is_grounded", &IsGrounded);
	DukGetBoolProp(ctx, 0, "got_air_jump", &GotAirJump);
	DukGetIntProp(ctx, 0, "emote", &Emote);

	CDuckBridge::CTeeDrawBodyAndFeetInfo TeeDrawInfo;
	TeeDrawInfo.m_Size = Size;
	TeeDrawInfo.m_Angle = Angle;
	TeeDrawInfo.m_Pos[0] = PosX;
	TeeDrawInfo.m_Pos[1] = PosY;
	TeeDrawInfo.m_IsWalking = IsWalking;
	TeeDrawInfo.m_IsGrounded = IsGrounded;
	TeeDrawInfo.m_GotAirJump = GotAirJump;
	TeeDrawInfo.m_Emote = Emote;
	//dbg_msg("duk", "DrawTeeBodyAndFeet( tee = { size: %g, pos_x: %g, pos_y: %g }", Size, PosX, PosY);
	This()->Bridge()->QueueDrawTeeBodyAndFeet(TeeDrawInfo);
#endif
	return 0;
}

/*#
`TwRenderDrawTeeHand(tee)`

| **DEPRECATED**
| Draws a tee hand.

**Parameters**

* **tee**:

.. code-block:: js

	var hand = {
		size: float,
		angle_dir: float,
		angle_off: float,
		pos_x: float,
		pos_y: float,
		off_x: float,
		off_y: float,
	};

**Returns**

* None
#*/
int CDuckLua::NativeRenderDrawTeeHand(lua_State* L)
{
	LUA_ERR("TwRenderDrawTeeHand is deprecated");

#if 0
	CheckArgumentCount(L, 1);

	/*
	 * hand = {
	 *	size: float,
	 *	angle_dir: float,
	 *	angle_off: float,
	 *	pos_x: float,
	 *	pos_y: float,
	 *	off_x: float,
	 *	off_y: float,
	 * }
	 *
	 */

	float Size = 64;
	float AngleDir = 0;
	float AngleOff = 0;
	float PosX = 0;
	float PosY = 0;
	float OffX = 0;
	float OffY = 0;

	DukGetFloatProp(ctx, 0, "size", &Size);
	DukGetFloatProp(ctx, 0, "angle_dir", &AngleDir);
	DukGetFloatProp(ctx, 0, "angle_off", &AngleOff);
	DukGetFloatProp(ctx, 0, "pos_x", &PosX);
	DukGetFloatProp(ctx, 0, "pos_y", &PosY);
	DukGetFloatProp(ctx, 0, "off_x", &OffX);
	DukGetFloatProp(ctx, 0, "off_y", &OffY);

	CDuckBridge::CTeeDrawHand TeeHandInfo;
	TeeHandInfo.m_Size = Size;
	TeeHandInfo.m_AngleDir = AngleDir;
	TeeHandInfo.m_AngleOff = AngleOff;
	TeeHandInfo.m_Pos[0] = PosX;
	TeeHandInfo.m_Pos[1] = PosY;
	TeeHandInfo.m_Offset[0] = OffX;
	TeeHandInfo.m_Offset[1] = OffY;
	//dbg_msg("duk", "NativeRenderDrawTeeHand( hand = { size: %g, angle_dir: %g, angle_off: %g, pos_x: %g, pos_y: %g, off_x: %g, off_y: %g }", Size, AngleDir, AngleOff, PosX, PosY, OffX, OffY);
	This()->Bridge()->QueueDrawTeeHand(TeeHandInfo);
#endif
	return 0;
}

/*#
`TwRenderDrawFreeform(x, y)`

| Draws the previously defined free form quad at position **x, y**.

**Parameters**

* **x**: float
* **y**: float

**Returns**

* None
#*/
int CDuckLua::NativeRenderDrawFreeform(lua_State* L)
{
	CheckArgumentCount(L, 2);

	vec2 Pos;
	Pos.x = lua_tonumber(L, 1);
	Pos.y = lua_tonumber(L, 2);

	This()->Bridge()->QueueDrawFreeform(Pos);
	return 0;
}

/*#
`TwRenderDrawText(text)`

| Draw text.
| Example:

.. code-block:: lua

	TwRenderDrawText({
		text = "This a text",
		font_size = 10,
		line_width = -1,
		pos = {10, 20},
		color = {1, 0, 1, 1}, // rgba (0.0 - 1.0)
		clip = {100, 25, 200, 100}, // x y width height
	});

**Parameters**

* **text**:

.. code-block:: lua

	local text = {
		text: string,
		font_size: float,
		line_width: float,
		pos: float[2],
		color: float[4],
		clip: float[4],
	};

**Returns**

* None
#*/
int CDuckLua::NativeRenderDrawText(lua_State* L)
{
	CheckArgumentCount(L, 1);

	if(!lua_istable(L, 1))
	{
		LUA_ERR("TwRenderDrawText(arg1): arg1 is not a table");
		return 0;
	}

	char* pText = 0;
	double FontSize = 10.0;
	double LineWidth = -1;
	float aPos[2] = { 0, 0 };
	float aColors[4] = {1, 1, 1, 1};
	float aRect[4] = { 0, 0, -1, -1};

	lua_getfield(L, 1, "text");
	const char* pStr = lua_tostring(L, -1);
	if(pStr)
	{
		int Len = min(str_length(pStr), 1024*1024); // 1MB max
		pText = (char*)This()->Bridge()->m_FrameAllocator.Alloc(Len+1);
		str_copy(pText, pStr, Len+1);
	}
	else
	{
		LUA_ERR("TwRenderDrawText(arg1) arg1.text is null");
		return 0;
	}
	lua_pop(L, 1);

	LuaGetPropNumber(L, 1, "font_size", &FontSize);
	LuaGetPropNumber(L, 1, "line_width", &LineWidth);

	// get position
	lua_getfield(L, 1, "pos");
	if(!lua_istable(L, -1))
	{
		LUA_ERR("TwRenderDrawText(arg1) arg1.pos is not a table");
		return 0;
	}

	const int PosLen = min(2, (int)lua_objlen(L, -1));
	for(int i = 0; i < PosLen; i++)
	{
		lua_rawgeti(L, -1, i+1);
		aPos[i] = lua_tonumber(L, -1);
		lua_pop(L, 1);
	}
	lua_pop(L, 1);

	// get colors
	lua_getfield(L, 1, "color");
	if(!lua_isnil(L, -1))
	{
		if(!lua_istable(L, -1))
		{
			LUA_ERR("TwRenderDrawText(arg1) arg1.color is not a table");
			return 0;
		}

		const int ColorLen = min(4, (int)lua_objlen(L, -1));
		for(int i = 0; i < ColorLen; i++)
		{
			lua_rawgeti(L, -1, i+1);
			aColors[i] = lua_tonumber(L, -1);
			lua_pop(L, 1);
		}
		lua_pop(L, 1);
	}

	// get clip
	lua_getfield(L, 1, "clip");
	if(!lua_isnil(L, -1))
	{
		if(!lua_istable(L, -1))
		{
			LUA_ERR("TwRenderDrawText(arg1) arg1.clip is not a table");
			return 0;
		}

		const int Len = min(4, (int)lua_objlen(L, -1));
		for(int i = 0; i < Len; i++)
		{
			lua_rawgeti(L, -1, i+1);
			aRect[i] = lua_tonumber(L, -1);
			lua_pop(L, 1);
		}

		lua_pop(L, 1);
	}

	This()->Bridge()->QueueDrawText(pText, FontSize, LineWidth, aPos, aRect, aColors);
	return 0;
}

int CDuckLua::NativeRenderDrawCircle(lua_State* L)
{
	CheckArgumentCount(L, 3);

	float x = lua_tonumber(L, 1);
	float y = lua_tonumber(L, 2);
	float Radius = lua_tonumber(L, 3);

	This()->Bridge()->QueueDrawCircle(vec2(x, y), Radius);
	return 0;
}

int CDuckLua::NativeRenderDrawLine(lua_State* L)
{
	CheckArgumentCount(L, 5);

	float Pos1X = lua_tonumber(L, 1);
	float Pos1Y = lua_tonumber(L, 2);
	float Pos2X = lua_tonumber(L, 3);
	float Pos2Y = lua_tonumber(L, 4);
	float Thickness = lua_tonumber(L, 5);

	This()->Bridge()->QueueDrawLine(vec2(Pos1X, Pos1Y), vec2(Pos2X, Pos2Y), Thickness);
	return 0;
}


/*#
`TwGetBaseTexture(image_id)`

| Get vanilla teeworlds texture id.
| Example: ``TwGetBaseTexture(Teeworlds.IMAGE_GAME)``

**Parameters**

* **image_id**: int

**Returns**

* **texture_id**: int
#*/
int CDuckLua::NativeGetBaseTexture(lua_State* L)
{
	CheckArgumentCount(L, 1);

	int ImgID = lua_tointeger(L, 1);
	lua_pushinteger(L, This()->Bridge()->GetBaseTextureHandle(ImgID));
	return 1;
}

/*#
`TwGetSpriteSubSet(sprite_id)`

| Get sprite texture coordinates.
| TODO: example

**Parameters**

* **sprite_id**: int

**Returns**

* **subset**:

.. code-block:: lua

	local subset = {
		x1: float,
		y1: float,
		x2: float,
		y2: float,
	};
#*/
int CDuckLua::NativeGetSpriteSubSet(lua_State* L)
{
	CheckArgumentCount(L, 1);

	int SpriteID = lua_tointeger(L, 1);
	float aSubSet[4];
	This()->Bridge()->GetBaseSpritSubset(SpriteID, aSubSet);

	lua_newtable(L);
	LuaSetPropNumber(L, -1, "x1", aSubSet[0]);
	LuaSetPropNumber(L, -1, "y1", aSubSet[1]);
	LuaSetPropNumber(L, -1, "x2", aSubSet[2]);
	LuaSetPropNumber(L, -1, "y2", aSubSet[3]);
	return 1;
}

/*#
`TwGetSpriteScale(sprite_id)`

| Get vanilla teeworlds sprite scale.
| TODO: example

**Parameters**

* **sprite_id**: int

**Returns**

* **scale**: {w: float, w: float}
#*/
int CDuckLua::NativeGetSpriteScale(lua_State* L)
{
	CheckArgumentCount(L, 1);

	int SpriteID = lua_tointeger(L, 1);
	float aScale[2];
	This()->Bridge()->GetBaseSpritScale(SpriteID, aScale);

	lua_newtable(L);
	LuaSetPropNumber(L, -1, "w", aScale[0]);
	LuaSetPropNumber(L, -1, "h", aScale[0]);
	return 1;
}

/*#
`TwGetWeaponSpec(weapon_id)`

| **DEPRECATED**
| Get vanilla teeworlds weapon specifications.
| TODO: example

**Parameters**

* **weapon_id**: int

**Returns**

* **TODO**
#*/
int CDuckLua::NativeGetWeaponSpec(lua_State* L)
{
	LUA_ERR("TwGetWeaponSpec is deprecated");
	// TODO: find a better way to expose client data
	return 0;

#if 0
	CheckArgumentCount(L, 1);

	int WeaponID = clamp((int)duk_to_int(ctx, 0), 0, NUM_WEAPONS-1);
	const CDataWeaponspec BaseSpec = g_pData->m_Weapons.m_aId[WeaponID];

	This()->PushObject();
	This()->ObjectSetMemberInt("sprite_body_texid", *(int*)&BaseSpec.m_pSpriteBody->m_pSet->m_pImage->m_Id);

	/*
	 * sprite_***_subset = {
	 *	x1: float,
	 *	y1: float,
	 *	x2: float,
	 *	y2: float,
	 * }
	 *
	 * sprite_***_scale = {
	 *	w: float,
	 *	h: float,
	 * }
	 */

	{
	float aSubSet[4];
	GetSpriteSubSet(*BaseSpec.m_pSpriteBody, aSubSet);
	int SpriteSubsetObjIdx = duk_push_object(ctx);
		duk_push_number(ctx, aSubSet[0]);
		duk_put_prop_string(ctx, SpriteSubsetObjIdx, "x1");
		duk_push_number(ctx, aSubSet[1]);
		duk_put_prop_string(ctx, SpriteSubsetObjIdx, "y1");
		duk_push_number(ctx, aSubSet[2]);
		duk_put_prop_string(ctx, SpriteSubsetObjIdx, "x2");
		duk_push_number(ctx, aSubSet[3]);
		duk_put_prop_string(ctx, SpriteSubsetObjIdx, "y2");
	This()->ObjectSetMember("sprite_body_subset"); // should pop obj

	int w = BaseSpec.m_pSpriteBody->m_W;
	int h = BaseSpec.m_pSpriteBody->m_H;
	float f = sqrtf(h*h + w*w);
	float ScaleW = w/f;
	float ScaleH = h/f;

	int SpriteScaleObjIdx = duk_push_object(ctx);
		duk_push_number(ctx, ScaleW);
		duk_put_prop_string(ctx, SpriteScaleObjIdx, "w");
		duk_push_number(ctx, ScaleH);
		duk_put_prop_string(ctx, SpriteScaleObjIdx, "h");
	This()->ObjectSetMember("sprite_body_scale"); // should pop obj
	}

	This()->ObjectSetMemberInt("sprite_cursor_texid", *(int*)&BaseSpec.m_pSpriteCursor->m_pSet->m_pImage->m_Id);

	{
	float aSubSet[4];
	GetSpriteSubSet(*BaseSpec.m_pSpriteCursor, aSubSet);
	int SpriteSubsetObjIdx = duk_push_object(ctx);
		duk_push_number(ctx, aSubSet[0]);
		duk_put_prop_string(ctx, SpriteSubsetObjIdx, "x1");
		duk_push_number(ctx, aSubSet[1]);
		duk_put_prop_string(ctx, SpriteSubsetObjIdx, "y1");
		duk_push_number(ctx, aSubSet[2]);
		duk_put_prop_string(ctx, SpriteSubsetObjIdx, "x2");
		duk_push_number(ctx, aSubSet[3]);
		duk_put_prop_string(ctx, SpriteSubsetObjIdx, "y2");
	This()->ObjectSetMember("sprite_cursor_subset"); // should pop obj

	int w = BaseSpec.m_pSpriteCursor->m_W;
	int h = BaseSpec.m_pSpriteCursor->m_H;
	float f = sqrtf(h*h + w*w);
	float ScaleW = w/f;
	float ScaleH = h/f;

	int SpriteScaleObjIdx = duk_push_object(ctx);
		duk_push_number(ctx, ScaleW);
		duk_put_prop_string(ctx, SpriteScaleObjIdx, "w");
		duk_push_number(ctx, ScaleH);
		duk_put_prop_string(ctx, SpriteScaleObjIdx, "h");
	This()->ObjectSetMember("sprite_cursor_scale"); // should pop obj
	}

	This()->ObjectSetMemberInt("sprite_proj_texid", *(int*)&BaseSpec.m_pSpriteProj->m_pSet->m_pImage->m_Id);

	{
	float aSubSet[4];
	GetSpriteSubSet(*BaseSpec.m_pSpriteProj, aSubSet);
	int SpriteSubsetObjIdx = duk_push_object(ctx);
		duk_push_number(ctx, aSubSet[0]);
		duk_put_prop_string(ctx, SpriteSubsetObjIdx, "x1");
		duk_push_number(ctx, aSubSet[1]);
		duk_put_prop_string(ctx, SpriteSubsetObjIdx, "y1");
		duk_push_number(ctx, aSubSet[2]);
		duk_put_prop_string(ctx, SpriteSubsetObjIdx, "x2");
		duk_push_number(ctx, aSubSet[3]);
		duk_put_prop_string(ctx, SpriteSubsetObjIdx, "y2");
	This()->ObjectSetMember("sprite_proj_subset"); // should pop obj

	int w = BaseSpec.m_pSpriteProj->m_W;
	int h = BaseSpec.m_pSpriteProj->m_H;
	float f = sqrtf(h*h + w*w);
	float ScaleW = w/f;
	float ScaleH = h/f;

	int SpriteScaleObjIdx = duk_push_object(ctx);
		duk_push_number(ctx, ScaleW);
		duk_put_prop_string(ctx, SpriteScaleObjIdx, "w");
		duk_push_number(ctx, ScaleH);
		duk_put_prop_string(ctx, SpriteScaleObjIdx, "h");
	This()->ObjectSetMember("sprite_proj_scale"); // should pop obj
	}

	This()->ObjectSetMemberInt("num_sprite_muzzles", BaseSpec.m_NumSpriteMuzzles);


	// sprite muzzles array
	duk_idx_t ArrayIdx;
	ArrayIdx = duk_push_array(ctx);

	for(int i = 0; i < BaseSpec.m_NumSpriteMuzzles; i++)
	{
		duk_push_int(ctx, *(int*)&BaseSpec.m_aSpriteMuzzles[i]->m_pSet->m_pImage->m_Id);
		duk_put_prop_index(ctx, ArrayIdx, i);
	}

	This()->ObjectSetMember("sprite_muzzles_texids"); // should pop array


	This()->ObjectSetMemberInt("visual_size", BaseSpec.m_VisualSize);
	This()->ObjectSetMemberInt("fire_delay", BaseSpec.m_Firedelay);
	This()->ObjectSetMemberInt("max_ammo", BaseSpec.m_Maxammo);
	This()->ObjectSetMemberInt("ammo_regen_time", BaseSpec.m_Ammoregentime);
	This()->ObjectSetMemberInt("damage", BaseSpec.m_Damage);

	This()->ObjectSetMemberFloat("offset_x", BaseSpec.m_Offsetx);
	This()->ObjectSetMemberFloat("offset_y", BaseSpec.m_Offsety);
	This()->ObjectSetMemberFloat("muzzle_offset_x", BaseSpec.m_Muzzleoffsetx);
	This()->ObjectSetMemberFloat("muzzle_offset_y", BaseSpec.m_Muzzleoffsety);
	This()->ObjectSetMemberFloat("muzzle_duration", BaseSpec.m_Muzzleduration);

	// TODO: extended weapon spec
	/*switch(WeaponID)
	{
		case WEAPON_GUN:

	}*/
#endif
	return 1;
}

/*#
`TwGetModTexture(image_name)`

| Get a mod texture based on its name.
| Example: ``TwGetModTexture("duck_burger")``

**Parameters**

* **image_name**: string

**Returns**

* **texture_id**: int
#*/
int CDuckLua::NativeGetModTexture(lua_State* L)
{
	CheckArgumentCount(L, 1);

	const char* pTextureName = lua_tostring(L, 1);
	if(pTextureName)
	{
		IGraphics::CTextureHandle Handle = This()->Bridge()->GetTextureFromName(pTextureName);
		lua_pushinteger(L, *(int*)&Handle);
	}
	else
	{
		lua_pushinteger(L, -1);
	}
	return 1;
}

/*#
`TwGetClientSkinInfo(client_id)`

| Returns the client's skin info

**Parameters**

* **client_id**: int

**Returns**

* **skin**

.. code-block:: lua

	local skin = {
		textures = {
			texid_body: int,
			texid_marking: int,
			texid_decoration: int,
			texid_hands: int,
			texid_feet: int,
			texid_eyes: int
		},

		colors = {
			color_body: {r, g, b ,a},
			color_marking: {r, g, b ,a},
			color_decoration: {r, g, b ,a},
			color_hands: {r, g, b ,a},
			color_feet: {r, g, b ,a},
			color_eyes
		}
	}
#*/
int CDuckLua::NativeGetClientSkinInfo(lua_State* L)
{
	CheckArgumentCount(L, 1);

	const int ClientID = clamp((int)lua_tointeger(L, 1), 0, MAX_CLIENTS-1);
	if(!This()->Bridge()->m_pClient->m_aClients[ClientID].m_Active)
	{
		lua_pushnil(L);
		dbg_msg("duck", "nil");
		return 1; // client not active, return null
	}

	const CTeeRenderInfo& RenderInfo = This()->Bridge()->m_pClient->m_aClients[ClientID].m_RenderInfo;

	lua_newtable(L);

	lua_pushstring(L, "textures");
	lua_createtable(L, NUM_SKINPARTS, 0);
	for(int i = 0; i < NUM_SKINPARTS; i++)
	{
		lua_pushinteger(L, *(int*)&RenderInfo.m_aTextures[i]);
		lua_rawseti(L, -2, i+1);
	}
	lua_settable(L, -3);

	lua_pushstring(L, "colors");
	lua_createtable(L, NUM_SKINPARTS, 0);
	for(int i = 0; i < NUM_SKINPARTS; i++)
	{
		lua_createtable(L, 4, 0);

		const vec4 Color = RenderInfo.m_aColors[i];
		lua_pushnumber(L, Color.r);
		lua_rawseti(L, -2, 1);
		lua_pushnumber(L, Color.g);
		lua_rawseti(L, -2, 2);
		lua_pushnumber(L, Color.b);
		lua_rawseti(L, -2, 3);
		lua_pushnumber(L, Color.a);
		lua_rawseti(L, -2, 4);

		lua_rawseti(L, -2, i+1);
	}
	lua_settable(L, -3);

	return 1;
}


/*#
`TwGetClientCharacterCores()`

| Get interpolated player character cores.

**Parameters**

* None

**Returns**

* **cores**

.. code-block:: js

	var cores = [
		{
			tick: int,
			vel_x: float,
			vel_y: float,
			angle: float,
			direction: int,
			jumped: int,
			hooked_player: int,
			hook_state: int,
			hook_tick: int,
			hook_x: float,
			hook_y: float,
			hook_dx: float,
			hook_dy: float,
			pos_x: float,
			pos_y: float,
		},
		...
	];

#*/
int CDuckLua::NativeGetClientCharacterCores(lua_State* L)
{
	CheckArgumentCount(L, 0);

	IClient* pClient = This()->Bridge()->Client();
	CGameClient* pGameClient = This()->Bridge()->m_pClient;

	float IntraTick = pClient->IntraGameTick();

	lua_createtable(L, MAX_CLIENTS, 0);
	const CGameClient::CSnapState::CCharacterInfo* pSnapCharacters = pGameClient->m_Snap.m_aCharacters;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		const CGameClient::CSnapState::CCharacterInfo& CharInfo = pSnapCharacters[i];
		if(CharInfo.m_Active)
		{
			CNetObj_Character Prev = CharInfo.m_Prev;
			CNetObj_Character Cur = CharInfo.m_Cur;

			float IntraTick = pClient->IntraGameTick();
			if(i == pGameClient->m_LocalClientID)
			{
				IntraTick = This()->Bridge()->Client()->PredIntraGameTick();
				pGameClient->m_PredictedChar.Write(&Cur);
				pGameClient->m_PredictedPrevChar.Write(&Prev);
			}

			if(Prev.m_Angle < pi*-128 && Cur.m_Angle > pi*128)
				Prev.m_Angle += 2*pi*256;
			else if(Prev.m_Angle > pi*128 && Cur.m_Angle < pi*-128)
				Cur.m_Angle += 2*pi*256;
			float Angle = mix((float)Prev.m_Angle, (float)Cur.m_Angle, IntraTick)/256.0f;

			vec2 Position = mix(vec2(Prev.m_X, Prev.m_Y), vec2(Cur.m_X, Cur.m_Y), IntraTick);
			vec2 Vel = mix(vec2(Prev.m_VelX/256.0f, Prev.m_VelY/256.0f), vec2(Cur.m_VelX/256.0f, Cur.m_VelY/256.0f), IntraTick);
			vec2 HookPos = mix(vec2(Prev.m_HookX, Prev.m_HookY), vec2(Cur.m_HookX, Cur.m_HookY), IntraTick);

			lua_newtable(L);
			LuaSetPropInteger(L, -1, "tick", Cur.m_Tick);
			LuaSetPropNumber(L, -1, "vel_x", Vel.x);
			LuaSetPropNumber(L, -1, "vel_y", Vel.y);
			LuaSetPropNumber(L, -1, "angle", Angle);
			LuaSetPropInteger(L, -1, "direction", Cur.m_Direction);
			LuaSetPropInteger(L, -1, "jumped", Cur.m_Jumped);
			LuaSetPropInteger(L, -1, "hooked_player", Cur.m_HookedPlayer);
			LuaSetPropInteger(L, -1, "hook_state", Cur.m_HookState);
			LuaSetPropInteger(L, -1, "hook_tick", Cur.m_HookTick);
			LuaSetPropNumber(L, -1, "hook_x", HookPos.x);
			LuaSetPropNumber(L, -1, "hook_y", HookPos.y);
			LuaSetPropNumber(L, -1, "hook_dx", Cur.m_HookDx/256.f);
			LuaSetPropNumber(L, -1, "hook_dy", Cur.m_HookDy/256.f);
			LuaSetPropNumber(L, -1, "pos_x", Position.x);
			LuaSetPropNumber(L, -1, "pos_y", Position.y);
		}
		else
			lua_pushnil(L);

		lua_rawseti(L, -2, i + 1);
	}

	return 1;
}

/*#
`TwGetStandardSkinInfo()`

| **DEPRECATED**
| Get the standard skin info.

**Parameters**

* None

**Returns**

* **skin**

.. code-block:: js

	var skin = {
		textures: [
			texid_body: int,
			texid_marking: int,
			texid_decoration: int,
			texid_hands: int,
			texid_feet: int,
			texid_eyes: int
		],

		colors: [
			color_body: {r, g, b ,a},
			color_marking: {r, g, b ,a},
			color_decoration: {r, g, b ,a},
			color_hands: {r, g, b ,a},
			color_feet: {r, g, b ,a},
			color_eyes
		]
	};

#*/
int CDuckLua::NativeGetStandardSkinInfo(lua_State* L)
{
	LUA_ERR("TwGetStandardSkinInfo is deprecated");
	// TODO: find a better way to expose client data
	return 0;

#if 0
	CheckArgumentCount(L, 0);

	// TODO: make an actual standard skin info
	const CTeeRenderInfo& RenderInfo = This()->Bridge()->m_pClient->m_aClients[This()->Bridge()->m_pClient->m_LocalClientID].m_RenderInfo;

	This()->PushObject();

	duk_idx_t ArrayIdx = duk_push_array(ctx);
	for(int i = 0; i < NUM_SKINPARTS; i++)
	{
		duk_push_int(ctx, *(int*)&RenderInfo.m_aTextures[i]);
		duk_put_prop_index(ctx, ArrayIdx, i);
	}
	This()->ObjectSetMember("textures");

	ArrayIdx = duk_push_array(ctx);
	for(int i = 0; i < NUM_SKINPARTS; i++)
	{
		const vec4 Color = RenderInfo.m_aColors[i];
		duk_idx_t ColorObj = duk_push_object(ctx);
		duk_push_number(ctx, Color.r);
		duk_put_prop_string(ctx, ColorObj, "r");
		duk_push_number(ctx, Color.g);
		duk_put_prop_string(ctx, ColorObj, "g");
		duk_push_number(ctx, Color.b);
		duk_put_prop_string(ctx, ColorObj, "b");
		duk_push_number(ctx, Color.a);
		duk_put_prop_string(ctx, ColorObj, "a");

		duk_put_prop_index(ctx, ArrayIdx, i);
	}
	This()->ObjectSetMember("colors");
#endif

	return 1;
}


/*#
`TwGetSkinPartTexture(part_id, part_name)`

| Get a skin part texture. Vanilla and mod skin parts both work here.
| Example: ``TwGetSkinPartTexture(Teeworlds.SKINPART_BODY, "zombie")``

**Parameters**

* **part_id**: int
* **part_name**: string

**Returns**

* **texture_id**: int

#*/
int CDuckLua::NativeGetSkinPartTexture(lua_State* L)
{
	CheckArgumentCount(L, 2);

	int Part = clamp((int)lua_tointeger(L, 1), 0, NUM_SKINPARTS-1);
	const char* PartName = lua_tostring(L, 2);
	if(!PartName)
	{
		LUA_ERR("TwGetSkinPartTexture(part_id, part_name): part_name is invalid");
		return 0;
	}

	IGraphics::CTextureHandle OrgText;
	IGraphics::CTextureHandle ColorText;
	bool Found = This()->Bridge()->GetSkinPart(Part, PartName, &OrgText, &ColorText);

	if(!Found)
	{
		lua_pushnil(L);
		return 1;
	}

	lua_createtable(L, 2, 0);
	lua_pushinteger(L, *(int*)&OrgText);
	lua_rawseti(L, -2, 1);
	lua_pushinteger(L, *(int*)&ColorText);
	lua_rawseti(L, -2, 2);
	return 1;
}

/*#
`TwGetCursorPosition()`

| Get weapon cursor position in world space.

**Parameters**

* None

**Returns**

* **cursor_pos**: { x: float, y: float }

#*/
int CDuckLua::NativeGetCursorPosition(lua_State* L)
{
	CheckArgumentCount(L, 0);

	const vec2 CursorPos = This()->Bridge()->GetLocalCursorPos();

	lua_createtable(L, 0, 2);
	LuaSetPropNumber(L, -1, "x", CursorPos.x);
	LuaSetPropNumber(L, -1, "y", CursorPos.y);
	return  1;
}

/*#
`TwGetUiScreenRect()`

| Get UI screen rect. Useful to draw in UI space.

**Parameters**

* None

**Returns**

* **rect**: { x: float, y: float, w: float, h: float }

#*/
int CDuckLua::NativeGetUiScreenRect(lua_State* L)
{
	CheckArgumentCount(L, 0);
	CUIRect Rect = This()->Bridge()->GetUiScreenRect();

	lua_createtable(L, 0, 4);
	LuaSetPropNumber(L, -1, "x", Rect.x);
	LuaSetPropNumber(L, -1, "y", Rect.y);
	LuaSetPropNumber(L, -1, "w", Rect.w);
	LuaSetPropNumber(L, -1, "h", Rect.h);
	return 1;
}

/*#
`TwGetScreenSize()`

| Get screen size.

**Parameters**

* None

**Returns**

* **size**: { w: float, h: float }

#*/
int CDuckLua::NativeGetScreenSize(lua_State* L)
{
	CheckArgumentCount(L, 0);
	vec2 Size = This()->Bridge()->GetScreenSize();

	lua_createtable(L, 0, 2);
	LuaSetPropNumber(L, -1, "w", Size.x);
	LuaSetPropNumber(L, -1, "h", Size.y);
	return 1;
}

/*#
`TwGetCamera()`

| Get camera position and zoom.

**Parameters**

* None

**Returns**

* **camera**: { x: float, y: float, zoom: float }

#*/
int CDuckLua::NativeGetCamera(lua_State* L)
{
	CheckArgumentCount(L, 0);
	vec2 Pos = This()->Bridge()->GetCameraPos();

	lua_createtable(L, 0, 3);
	LuaSetPropNumber(L, -1, "x", Pos.x);
	LuaSetPropNumber(L, -1, "y", Pos.y);
	LuaSetPropNumber(L, -1, "zoom", This()->Bridge()->GetCameraZoom());
	return 1;
}

/*#
`TwGetUiMousePos()`

| Get screen mouse position in UI coordinates.

**Parameters**

* None

**Returns**

* **pos**: { x: float, y: float }

#*/
int CDuckLua::NativeGetUiMousePos(lua_State* L)
{
	CheckArgumentCount(L, 0);
	vec2 Pos = This()->Bridge()->GetUiMousePos();

	lua_createtable(L, 0, 2);
	LuaSetPropNumber(L, -1, "x", Pos.x);
	LuaSetPropNumber(L, -1, "y", Pos.y);
	return 1;
}

/*#
`TwGetPixelScale()`

| Get current draw plane pixel scale. Useful to draw using pixel size.
| Example:

.. code-block:: js

	const pixelScale = TwGetPixelScale();
	var wdith = widthInPixel * pixelScale.x;
	TwRenderQuad(0, 0, width, 50);


**Parameters**

* None

**Returns**

* **scale**: { x: float, y: float }

#*/
int CDuckLua::NativeGetPixelScale(lua_State* L)
{
	CheckArgumentCount(L, 0);
	vec2 Scale = This()->Bridge()->GetPixelScale();

	lua_createtable(L, 0, 2);
	LuaSetPropNumber(L, -1, "x", Scale.x);
	LuaSetPropNumber(L, -1, "y", Scale.y);
	return 1;
}


/*#
`TwPhysGetCores()`

| Get predicted physical cores.


**Parameters**

* None

**Returns**

* **cores**:

.. code-block:: lua

	local cores = {
		{
			x: float,
			y: float,
		},
		...
	}

#*/
int CDuckLua::NativePhysGetCores(lua_State* L)
{
	CheckArgumentCount(L, 0);


	IClient* pClient = This()->Bridge()->Client();
	float IntraTick = pClient->IntraGameTick();
	float PredIntraTick = pClient->PredIntraGameTick();
	const CDuckBridge& Bridge = *This()->Bridge();

	const int Count = Bridge.m_WorldCorePredicted.m_aCustomCores.size();
	const CCustomCore* Cores = Bridge.m_WorldCorePredicted.m_aCustomCores.base_ptr();
	const int CountPrev = Bridge.m_WorldCorePredictedPrev.m_aCustomCores.size();
	const CCustomCore* CoresPrev = Bridge.m_WorldCorePredictedPrev.m_aCustomCores.base_ptr();

	lua_createtable(L, Count, 0);
	for(int CurI = 0; CurI < Count; CurI++)
	{
		lua_createtable(L, 0, 6);

		int PrevID = -1;
		for(int PrevI = 0; PrevI < CountPrev; PrevI++)
		{
			if(CoresPrev[PrevI].m_UID == Cores[CurI].m_UID)
			{
				PrevID = PrevI;
				break;
			}
		}

		vec2 CurPos = Cores[CurI].m_Pos;
		vec2 PrevPos;
		vec2 CurVel = Cores[CurI].m_Vel;
		vec2 PrevVel;

		// don't interpolate this one
		if(PrevID == -1)
		{
			PrevPos = CurPos;
			PrevVel = CurVel;
		}
		else
		{
			PrevPos = CoresPrev[PrevID].m_Pos;
			PrevVel = CoresPrev[PrevID].m_Vel;
		}

		vec2 Position = mix(PrevPos, CurPos, PredIntraTick);
		vec2 Velocity = mix(PrevVel, CurVel, PredIntraTick);

		LuaSetPropInteger(L, -1, "id", CurI);
		LuaSetPropNumber(L, -1, "x", Position.x);
		LuaSetPropNumber(L, -1, "y", Position.y);
		LuaSetPropNumber(L, -1, "vel_x", Velocity.x);
		LuaSetPropNumber(L, -1, "vel_y", Velocity.y);
		LuaSetPropNumber(L, -1, "radius", Cores[CurI].m_Radius);

		lua_rawseti(L, -2, CurI + 1);
	}

	return 1;
}


/*#
`TwPhysGetJoints()`

| Get physical joints.
| *Work in progress*


**Parameters**

* None

**Returns**

* **joints**:

.. code-block:: lua

	local joints = {
		{
			core1_id: int or nil,
			core2_id: int or nil,
		},
		...
	}

#*/
int CDuckLua::NativePhysGetJoints(lua_State* L)
{
	CheckArgumentCount(L, 0);

	IClient* pClient = This()->Bridge()->Client();
	float IntraTick = pClient->IntraGameTick();
	float PredIntraTick = pClient->PredIntraGameTick();
	CDuckBridge& Bridge = *This()->Bridge();

	const int Count = Bridge.m_WorldCorePredicted.m_aJoints.size();
	const CDuckPhysJoint* aJoints = Bridge.m_WorldCorePredicted.m_aJoints.base_ptr();
	const CCustomCore* aCores = Bridge.m_WorldCorePredicted.m_aCustomCores.base_ptr();

	lua_createtable(L, Count, 0);
	for(int i = 0; i < Count; i++)
	{
		lua_createtable(L, 0, 2);

		const CDuckPhysJoint& Joint = aJoints[i];

		CCustomCore* pCore1 = Bridge.m_WorldCorePredicted.FindCustomCoreFromUID(Joint.m_CustomCoreUID1);
		CCustomCore* pCore2 = Bridge.m_WorldCorePredicted.FindCustomCoreFromUID(Joint.m_CustomCoreUID2);

		if(pCore1)
			LuaSetPropInteger(L, -1, "core1_id", pCore1 - aCores + 1);
		else
			LuaSetPropNil(L, -1, "core1_id");

		if(pCore2)
			LuaSetPropInteger(L, -1, "core2_id", pCore2 - aCores + 1);
		else
			LuaSetPropNil(L, -1, "core2_id");

		lua_rawseti(L, -2, i + 1);
	}

	return 1; // pop array
}


/*#
`TwPhysSetTileCollisionFlags(tile_x, tile_y, flags)`

| Modify a map tile's collision flags.
| Example: ``TwMapSetTileCollisionFlags(tx, ty, 0); // air``
| See collision.h for flags.
| TODO: give easy access to flags?

**Parameters**

* **tile_x**: int
* **tile_y**: int
* **flags**: int

**Returns**

* None

#*/
int CDuckLua::NativePhysSetTileCollisionFlags(lua_State* L)
{
	CheckArgumentCount(L, 3);

	int Tx = lua_tointeger(L, 1);
	int Ty = lua_tointeger(L, 2);
	int Flags = lua_tointeger(L, 3);

	This()->Bridge()->m_Collision.SetTileCollisionFlags(Tx, Ty, Flags);

	return 0;
}

/*#
`TwDirectionFromAngle(angle)`

| Get direction vector from angle.

**Parameters**

* **angle**: float

**Returns**

* **dir**: { x: float, y: float }

#*/
int CDuckLua::NativeDirectionFromAngle(lua_State* L)
{
	CheckArgumentCount(L, 1);

	float Angle = (float)lua_tonumber(L, 1);
	vec2 Dir = direction(Angle);

	lua_createtable(L, 0, 2);
	LuaSetPropNumber(L, -1, "x", Dir.x);
	LuaSetPropNumber(L, -1, "y", Dir.y);
	return 1;
}


/*#
`TwSetHudPartsShown(hud)`

| Show/hide parts of the hud.
| Not specified parts are unchanged.
| Example:

.. code-block:: js

	var hud = {
		health: 0,
		armor: 0,
		ammo: 1,
		time: 0,
		killfeed: 1,
		score: 1,
		chat: 1,
		scoreboard: 1,
		weapon_cursor: 1,
	};

	TwSetHudPartsShown(hud);

**Parameters**

* **hud**

.. code-block:: js

	var hud = {
		health: bool,
		armor: bool,
		ammo: bool,
		time: bool,
		killfeed: bool,
		score: bool,
		chat: bool,
		scoreboard: bool,
		weapon_cursor: bool,
	};

**Returns**

* None

#*/
int CDuckLua::NativeSetHudPartsShown(lua_State* L)
{
	CheckArgumentCount(L, 1);

	if(!lua_istable(L, 1)) {
		LUA_ERR("TwSetHudPartsShown(hud): hud is not a table");
		return 0;
	}

	CDuckBridge::CHudPartsShown hps = This()->Bridge()->m_HudPartsShown;

	LuaGetPropBool(L, 1, "health", &hps.m_Health);
	LuaGetPropBool(L, 1, "armor", &hps.m_Armor);
	LuaGetPropBool(L, 1, "ammo", &hps.m_Ammo);
	LuaGetPropBool(L, 1, "time", &hps.m_Time);
	LuaGetPropBool(L, 1, "killfeed", &hps.m_KillFeed);
	LuaGetPropBool(L, 1, "score", &hps.m_Score);
	LuaGetPropBool(L, 1, "chat", &hps.m_Chat);
	LuaGetPropBool(L, 1, "scoreboard", &hps.m_Scoreboard);
	LuaGetPropBool(L, 1, "weapon_cursor", &hps.m_WeaponCursor);

	This()->Bridge()->SetHudPartsShown(hps);

	return 0;
}

/*#
`TwNetSendPacket(packet)`

| Send a packet.
| Packet object needs to be formatted to add type information, example:

.. code-block:: lua

	local packet = {
		net_id = 1478,
		force_send_now = 0,

		i32_blockID = 1,
		i32_flags = 5,
		float_pos_x = 180,
		float_pos_y = 20,
		float_vel_x = 0,
		float_vel_y = 0,
		float_width = 1000,
		float_height = 200,
	}


| The first 2 fields are required, the rest are in the form type_name: value.
| Supported types are:

* i32
* u32
* float
* str* (str32_something is a 32 length string)

**Parameters**

* **packet**: user edited object based on:

.. code-block:: lua

	local packet = {
		net_id: int,
		force_send_now: int (0 or 1),
		...
	}

**Returns**

* None

#*/
int CDuckLua::NativeNetSendPacket(lua_State* L)
{
	CheckArgumentCount(L, 1);

	if(!lua_istable(L, 1))
	{
		LUA_ERR("TwNetSendPacket(packet) packet is not an object");
		return 0;
	}

	const int TableIndex = 1;
	int64_t NetID = -1;
	int64_t SendNow = 0;
	LuaGetPropInteger(L, TableIndex, "net_id", &NetID);
	LuaGetPropInteger(L, TableIndex, "force_send_now", &SendNow);

	if(NetID <= 0)
	{
		LUA_ERR("TwNetSendPacket(packet) net_id is invalid (%d)", NetID);
		return 0;
	}

	int Flags = 0;
	Flags |= MSGFLAG_VITAL;
	if(SendNow > 0) Flags |= MSGFLAG_FLUSH;
	This()->Bridge()->PacketCreate(NetID, Flags);

	// list properties
	lua_pushnil(L); // first key
	while(lua_next(L, TableIndex))
	{
		const int KeyIdx = -2;
		const int ValIdx = -1;

		// key is a string
		if(lua_isstring(L, KeyIdx))
		{
			const char* pKey = lua_tostring(L, KeyIdx);

			if(str_startswith(pKey, "i32_"))
			{
				This()->Bridge()->PacketPackInt(lua_tointeger(L, ValIdx));
			}
			else if(str_startswith(pKey, "u32_"))
			{
				This()->Bridge()->PacketPackInt(lua_tointeger(L, ValIdx));
			}
			else if(str_startswith(pKey, "float_"))
			{
				This()->Bridge()->PacketPackFloat(lua_tonumber(L, ValIdx));
			}
			else if(str_startswith(pKey, "str"))
			{
				u32 Size;
				if(sscanf(pKey+3, "%u_", &Size) == 1)
				{
					char aStr[2048];

					if(Size > sizeof(aStr))
					{
						LUA_ERR("TwNetSendPacket(packet) string is too large (%u)", Size);
						return 0;
					}

					This()->Bridge()->PacketPackString(lua_tostring(L, ValIdx), Size);
				}
			}
		}

		/* removes 'value'; keeps 'key' for next iteration */
		lua_pop(L, 1);
	}

	This()->Bridge()->SendPacket();
	return 0;
}

/*#
`TwNetPacketUnpack(packet, template)`

| Unpack packet based on template.
| Each template field will be filled based on the specified type, for example this code:

.. code-block:: lua

	local block = TwNetPacketUnpack(packet, {
		"i32_blockID",
		"i32_flags",
		"float_pos_x",
		"float_pos_y",
		"float_vel_x",
		"float_vel_y",
		"float_width",
		"float_height"
	}

| Will fill the first field (blockID) with the first int in the packet. Same with flags, pos_x will get a float and so on.
| The type is removed on return, so the resulting object looks like this:

.. code-block:: lua

	local block = {
		blockID: int,
		flags: int,
		pos_x: float,
		pos_y: float,
		vel_x: float,
		vel_y: float,
		width: float,
		height:float,
	}

| Supported types are:

* i32
* u32
* float
* str* (str32_something is a 32 length string)

**Parameters**

* **packet**: Packet from OnMessage(packet)
* **template**: user object

**Returns**

* **unpacked**: object

#*/
int CDuckLua::NativeNetPacketUnpack(lua_State* L)
{
	CheckArgumentCount(L, 2);

	const int PacketIdx = 1;
	const int TemplateIdx = 2;

	if(!lua_istable(L, PacketIdx))
	{
		LUA_ERR("TwNetPacketUnpack(packet, template) packet is not an object");
		return 0;
	}

	if(!lua_istable(L, TemplateIdx))
	{
		LUA_ERR("TwNetPacketUnpack(packet, template) template is not an object");
		return 0;
	}

	lua_getfield(L, PacketIdx, "raw");
	if(!lua_isstring(L, -1))
	{
		lua_pop(L, 1);
		LUA_ERR("TwNetPacketUnpack(packet, template) can't unpack");
		return 0;
	}

	size_t RawBufferSize;
	const u8* pStr = (u8*)lua_tolstring(L, -1, &RawBufferSize);

	u8 aRawBuff[2048];
	if(RawBufferSize > sizeof(aRawBuff))
	{
		lua_pop(L, 1);
		LUA_ERR("TwNetPacketUnpack(packet, template) packet too large");
		return 0;
	}

	mem_move(aRawBuff, pStr, RawBufferSize);

	lua_pop(L, 1);

	int Cursor = 0;

	enum {
		T_INT32=0,
		T_UINT32,
		T_FLOAT,
		T_STRING
	};

	struct CKeyValuePair {
		char aKey[64];
		const void* pValue;
		int Type;
		int Size;
	};

	array<CKeyValuePair> aProperties;

#define ADD_PROP(NAMEOFF, TYPE, SIZE) CKeyValuePair vp;\
	str_copy(vp.aKey, pKey + NAMEOFF, sizeof(vp.aKey));\
	vp.pValue = aRawBuff + Cursor;\
	vp.Type = TYPE;\
	vp.Size = SIZE;\
	Cursor += SIZE;\
	if(Cursor > RawBufferSize) {\
		LUA_ERR("TwNetPacketUnpack(net_obj, template) template is too large (%d > %d)", Cursor, RawBufferSize);\
		return 0;\
	}\
	aProperties.add(vp)

	// list properties
	const int count = lua_objlen(L, TemplateIdx); // first key
	for(int i = 1; i <= count ; i++)
	{
		const int ValIdx = -1;
		lua_rawgeti(L, TemplateIdx, i);

		// key is a string
		if(lua_isstring(L, ValIdx))
		{
			const char* pKey = lua_tostring(L, ValIdx);

			if(str_startswith(pKey, "i32_"))
			{
				ADD_PROP(4, T_INT32, 4);
			}
			else if(str_startswith(pKey, "u32_"))
			{
				ADD_PROP(4, T_UINT32, 4);
			}
			else if(str_startswith(pKey, "float_"))
			{
				ADD_PROP(6, T_FLOAT, 4);
			}
			else if(str_startswith(pKey, "str"))
			{
				int Size;
				if(sscanf(pKey+3, "%d_", &Size) == 1)
				{
					int Offset = str_find(pKey, "_") - pKey + 1;
					ADD_PROP(Offset, T_STRING, Size);
				}
			}
		}

		lua_pop(L, 1);
	}

#undef ADD_PROP

	lua_createtable(L, 0, aProperties.size());
	for(int i = 0; i < aProperties.size(); i++)
	{
		const CKeyValuePair& vp = aProperties[i];
		//dbg_msg("duck", "type=%d size=%d key='%s'", aProperties[i].Type, aProperties[i].Size, aProperties[i].aKey);

		switch(vp.Type)
		{
			case T_INT32:
				LuaSetPropInteger(L, -1, vp.aKey, *(int32_t*)vp.pValue);
				break;

			case T_UINT32:
				LuaSetPropInteger(L, -1, vp.aKey, *(uint32_t*)vp.pValue);
				break;

			case T_FLOAT:
				LuaSetPropNumber(L, -1, vp.aKey, *(float*)vp.pValue);
				break;

			case T_STRING:
			{
				char aString[2048];
				str_copy(aString, (const char*)vp.pValue, sizeof(aString));
				LuaSetPropString(L, -1, vp.aKey, aString);
			} break;

			default:
				LUA_CRIT("Type not handled");
				break;
		}
	}

	if(Cursor < RawBufferSize)
		LUA_WARN("TwNetPacketUnpack() packet only partially unpacked (%d / %d)", Cursor, RawBufferSize);

	return 1;
}


/*#
`TwAddWeapon(weapon)`

| Simple helper to add a custom weapon.

**Parameters**

* **weapon**

.. code-block:: js

	var weapon = {
		id: int,
		tex_weapon: int, // can be null
		tex_cursor: int, // can be null
		weapon_x: float,
		weapon_y: float,
		hand_x,: float,
		hand_y,: float,
		hand_angle,: float,
		recoil,: float,
	};

**Returns**

* None

#*/
int CDuckLua::NativeAddWeapon(lua_State* L)
{
	LUA_ERR("TwRenderSetTeeSkin is deprecated");
	return 0;

	/*
	CheckArgumentCount(L, 1);

	CDuckBridge::CWeaponCustomJs Wc;

	DukGetIntProp(ctx, 0, "id", &Wc.WeaponID);

	if(DukIsPropNull(ctx, 0, "tex_weapon"))
		Wc.aTexWeapon[0] = 0;
	else
		DukGetStringProp(ctx, 0, "tex_weapon", Wc.aTexWeapon, sizeof(Wc.aTexWeapon));

	if(DukIsPropNull(ctx, 0, "tex_cursor"))
		Wc.aTexCursor[0] = 0;
	else
		DukGetStringProp(ctx, 0, "tex_cursor", Wc.aTexCursor, sizeof(Wc.aTexCursor));

	DukGetFloatProp(ctx, 0, "weapon_x", &Wc.WeaponX);
	DukGetFloatProp(ctx, 0, "weapon_y", &Wc.WeaponY);
	DukGetFloatProp(ctx, 0, "weapon_sx", &Wc.WeaponSizeX);
	DukGetFloatProp(ctx, 0, "weapon_sy", &Wc.WeaponSizeY);
	DukGetFloatProp(ctx, 0, "hand_x", &Wc.HandX);
	DukGetFloatProp(ctx, 0, "hand_y", &Wc.HandY);
	DukGetFloatProp(ctx, 0, "hand_angle", &Wc.HandAngle);
	DukGetFloatProp(ctx, 0, "recoil", &Wc.Recoil);

	This()->Bridge()->AddWeapon(Wc);
	return 0;
	*/
}

/*#
`TwPlaySoundAt(sound_name, x, y)`

| Play a mod sound at position x,y.

**Parameters**

* **sound_name**: string
* **x**: float
* **y**: float

**Returns**

* None

#*/
int CDuckLua::NativePlaySoundAt(lua_State* L)
{
	CheckArgumentCount(L, 3);

	const char* pStr = lua_tostring(L, 1);
	float x = lua_tonumber(L, 2);
	float y = lua_tonumber(L, 3);

	This()->Bridge()->PlaySoundAt(pStr, x, y);
	return 0;
}

/*#
`TwPlaySoundGlobal(sound_name)`

| Play a mod sound globally.

**Parameters**

* **sound_name**: string

**Returns**

* None

#*/
int CDuckLua::NativePlaySoundGlobal(lua_State* L)
{
	CheckArgumentCount(L, 1);

	const char* pStr = lua_tostring(L, 1);
	This()->Bridge()->PlaySoundGlobal(pStr);
	return 0;
}

/*#
`TwPlayMusic(sound_name)`

| Play a mod music (will loop).

**Parameters**

* **sound_name**: string

**Returns**

* None

#*/
int CDuckLua::NativePlayMusic(lua_State* L)
{
	CheckArgumentCount(L, 1);

	const char* pStr = lua_tostring(L, 1);
	This()->Bridge()->PlayMusic(pStr);
	return 0;
}

/*#
`TwRandomInt(min, max)`

| Get a random int between min and max, included.
| Example for getting a float instead: ``TwRandomInt(0, 1000000)/1000000.0``

**Parameters**

* **min**: int
* **max**: int

**Returns**

* **rand_int**: int

#*/
int CDuckLua::NativeRandomInt(lua_State* L)
{
	CheckArgumentCount(L, 2);

	int Min = lua_tointeger(L, 1);
	int Max = lua_tointeger(L, 2);
	if(Min > Max)
		tl_swap(Min, Max);

	// xor shift random
	static uint32_t XorShiftState = time(0);
	uint32_t x = XorShiftState;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	XorShiftState = x;

	int Val = x % (Max - Min + 1) + Min;
	lua_pushinteger(L, Val);
	return 1;
}


/*#
`TwCalculateTextSize(text)`

| Calculate text size for the current draw space.
| Example:

.. code-block:: lua

	local size = TwCalculateTextSize({
		text: "Some text",
		font_size: 13,
		line_width: 240
	}

| Note: this is not 100% accurate for now unfortunately...

**Parameters**

* **text**:

.. code-block:: lua

	local text = {
		str: string,
		font_size: float,
		line_width: float
	}

**Returns**

* **size**: { x: float, y: float }

#*/
int CDuckLua::NativeCalculateTextSize(lua_State* L)
{
	CheckArgumentCount(L, 1);

	const int TableIdx = 1;
	if(!lua_istable(L, TableIdx))
	{
		LUA_ERR("TwCalculateTextSize(text) text is not an object");
		return 0;
	}

	char* pText = 0;
	double FontSize = 10.0f;
	double LineWidth = -1;

	lua_getfield(L, TableIdx, "text");
	if(lua_isstring(L, -1))
	{
		const char* pStr = lua_tostring(L, -1);
		int Len = min(str_length(pStr), 1024*1024); // 1MB max
		pText = (char*)This()->Bridge()->m_FrameAllocator.Alloc(Len+1);
		str_copy(pText, pStr, Len+1);
	}
	else
	{
		lua_pop(L, 1);
		LUA_ERR("TwCalculateTextSize(text) text.text is null");
		return 0;
	}
	lua_pop(L, 1);

	LuaGetPropNumber(L, TableIdx, "font_size", &FontSize);
	LuaGetPropNumber(L, TableIdx, "line_width", &LineWidth);

	vec2 Size = This()->Bridge()->CalculateTextSize(pText, FontSize, LineWidth);

	lua_createtable(L, 0, 2);
	LuaSetPropNumber(L, -1, "w", Size.x);
	LuaSetPropNumber(L, -1, "h", Size.y);
	return 1;
}

/*#
`TwSetMenuModeActive(active)`

| Activate or deactivate menu mode.
| All game input is deactivated (tee does not move, shoot or anything else).

**Parameters**

* **active**: bool

**Returns**

* None

#*/
int CDuckLua::NativeSetMenuModeActive(lua_State* L)
{
	CheckArgumentCount(L, 1);

	bool Active = lua_toboolean(L, 1);
	This()->Bridge()->SetMenuModeActive(Active);
	return 0;
}

// authorized base lib snippet copied from "minilua.c"
namespace BaseLuaLib {

static int luaB_type(lua_State*L)
{
	luaL_checkany(L,1);
	lua_pushstring(L,luaL_typename(L,1));
	return 1;
}

static int luaB_next(lua_State*L)
{
	luaL_checktype(L,1,5);
	lua_settop(L,2);
	if(lua_next(L,1))
		return 2;
	else {
		lua_pushnil(L);
		return 1;
	}
}

static int luaB_pairs(lua_State*L)
{
	luaL_checktype(L,1,5);
	lua_pushvalue(L,lua_upvalueindex(1));
	lua_pushvalue(L,1);
	lua_pushnil(L);
	return 3;
}

static int ipairsaux(lua_State*L)
{
	int i = luaL_checkint(L, 2);
	luaL_checktype(L, 1, 5);
	i++;
	lua_pushinteger(L, i);
	lua_rawgeti(L, 1, i);
	return (lua_isnil(L, -1)) ? 0:2;
}

static int luaB_ipairs(lua_State* L)
{
	luaL_checktype(L, 1, 5);
	lua_pushvalue(L, lua_upvalueindex(1));
	lua_pushvalue(L, 1);
	lua_pushinteger(L, 0);
	return 3;
}

static void auxopen(lua_State* L, const char*name, lua_CFunction f, lua_CFunction u)
{
	lua_pushcfunction(L, u);
	lua_pushcclosure(L, f, 1);
	lua_setfield(L, -2, name);
}

static const luaL_Reg base_funcs[]={
	//{"assert",luaB_assert},
	//{"error",luaB_error},
	//{"loadfile",luaB_loadfile},
	//{"loadstring",luaB_loadstring},
	//{"next",luaB_next},
	//{"pcall",luaB_pcall},
	//{"rawget",luaB_rawget},
	//{"setfenv",luaB_setfenv},
	//{"setmetatable",luaB_setmetatable},
	//{"tonumber",luaB_tonumber},
	{"type", luaB_type},
	//{"unpack",luaB_unpack},
	{NULL,NULL}
};

static void base_open(lua_State* L)
{
	lua_pushvalue(L, (-10002));
	lua_setglobal(L, "_G");
	luaL_register(L, "_G", base_funcs);
	lua_pushliteral(L, "Lua 5.1");
	lua_setglobal(L, "_VERSION");
	auxopen(L, "ipairs", luaB_ipairs, ipairsaux);
	auxopen(L, "pairs", luaB_pairs, luaB_next);
	lua_settop(L, 0); // pop stack
}

}

bool CDuckLua::LoadScriptFile(const char *pFilePath, const char *pRelFilePath)
{
	IOHANDLE ScriptFile = io_open(pFilePath, IOFLAG_READ);
	if(!ScriptFile)
	{
		dbg_msg("duck", "could not open '%s'", pFilePath);
		return false;
	}

	const int FileSize = (int)io_length(ScriptFile);
	dbg_assert(FileSize < (10*1024*1024), "File too large");

	char *pFileData = (char *)mem_alloc(FileSize + 1, 1);
	dbg_assert(pFileData != NULL, "Failed to allocate");

	io_read(ScriptFile, pFileData, FileSize);
	pFileData[FileSize] = 0;
	io_close(ScriptFile);

	AddScriptFileItem(pRelFilePath, pFileData, FileSize);

	return true;
}

void CDuckLua::AddScriptFileItem(const char *pScriptFilename, const char *pFileData, int FileSize)
{
	CScriptFile Script;
	const int NameLen = str_length(pScriptFilename);
	Script.m_NameHash = hash_fnv1a(pScriptFilename, NameLen);
	str_copy(Script.m_aName, pScriptFilename, sizeof(Script.m_aName));
	Script.m_pFileData = pFileData;
	Script.m_FileSize = FileSize;

	dbg_assert(m_ScriptFileCount < SCRIPTFILE_MAXCOUNT, "Too many script files");
	m_aScriptFiles[m_ScriptFileCount++] = Script;
}

int CDuckLua::FindScriptFileFromName(const char *pScriptFilename)
{
	const int NameLen = str_length(pScriptFilename);
	const uint32_t FindHash = hash_fnv1a(pScriptFilename, NameLen);

	const int Count = m_ScriptFileCount;
	for(int i = 0; i < Count; i++)
	{
		if(m_aScriptFiles[i].m_NameHash == FindHash)
		{
			return i;
		}
	}

	return -1;
}

bool CDuckLua::LuaLoadScriptFileData(int ScriptFileID)
{
	dbg_assert(ScriptFileID >= 0 && ScriptFileID < m_ScriptFileCount, "ScriptFileID out of bounds");
	CScriptFile& Script = m_aScriptFiles[ScriptFileID];

	if(m_aIsScriptLuaLoaded[ScriptFileID])
		return true;
	m_aIsScriptLuaLoaded[ScriptFileID] = true;

	// load script
	dbg_msg("duck", "luaload '%s'", Script.m_aName);

	int r = luaL_loadbuffer(L(), Script.m_pFileData, Script.m_FileSize, Script.m_aName);
	dbg_assert(r != LUA_ERRMEM, "Lua memory error");

	if(r != 0)
	{
		LUA_ERR("'%s': \n   %s", Script.m_aName, lua_tostring(L(), -1));
		lua_pop(L(), 1);
		return false;
	}

	if(lua_pcall(L(), 0, 0, 0) != 0)
	{
		LUA_ERR("'%s': \n   %s", Script.m_aName, lua_tostring(L(), -1));
		lua_pop(L(), 1);
		return false;
	}

	dbg_msg("duck", "'%s' loaded (%d)", Script.m_aName, Script.m_FileSize);
	return true;
}

void CDuckLua::FreeAllScriptFileData()
{
	const int Count = m_ScriptFileCount;
	for(int i = 0; i < Count; i++)
	{
		mem_free((void*)m_aScriptFiles[i].m_pFileData);
	}
}

void CDuckLua::ResetLuaState()
{
	if(m_pLuaState)
		lua_close(m_pLuaState);

	m_pLuaState = luaL_newstate();
	BaseLuaLib::base_open(L()); // open base lib

	lua_pushnumber(L(), 3.14159265359);
	lua_setglobal(L(), "pi");

	lua_newtable(L());
	GetContentEnumsAsLua();
	lua_setglobal(L(), "Teeworlds");

#define REGISTER_FUNC_PLAIN(fname, luaname) \
	lua_pushcfunction(L(), Native##fname);\
	lua_setglobal(L(), #luaname)

#define REGISTER_FUNC(fname, arg_count) \
	lua_pushcfunction(L(), Native##fname);\
	lua_setglobal(L(), "Tw" #fname)

	REGISTER_FUNC_PLAIN(Print, print);
	REGISTER_FUNC_PLAIN(Require, require);
	REGISTER_FUNC_PLAIN(Cos, cos);
	REGISTER_FUNC_PLAIN(Sin, sin);
	REGISTER_FUNC_PLAIN(Sqrt, sqrt);
	REGISTER_FUNC_PLAIN(Atan2, atan2);
	REGISTER_FUNC_PLAIN(Floor, floor);

	REGISTER_FUNC(RenderQuad, 4);
	REGISTER_FUNC(RenderQuadCentered, 4);
	REGISTER_FUNC(RenderSetColorU32, 1);
	REGISTER_FUNC(RenderSetColorF4, 4);
	REGISTER_FUNC(RenderSetTexture, 1);
	REGISTER_FUNC(RenderSetQuadSubSet, 4);
	REGISTER_FUNC(RenderSetQuadRotation, 1);
	REGISTER_FUNC(RenderSetTeeSkin, 1);
	REGISTER_FUNC(RenderSetFreeform, 1);
	REGISTER_FUNC(RenderSetDrawSpace, 1);
	REGISTER_FUNC(RenderDrawTeeBodyAndFeet, 1);
	REGISTER_FUNC(RenderDrawTeeHand, 1);
	REGISTER_FUNC(RenderDrawFreeform, 2);
	REGISTER_FUNC(RenderDrawText, 1);
	REGISTER_FUNC(RenderDrawCircle, 3);
	REGISTER_FUNC(RenderDrawLine, 5);
	REGISTER_FUNC(GetBaseTexture, 1);
	REGISTER_FUNC(GetSpriteSubSet, 1);
	REGISTER_FUNC(GetSpriteScale, 1);
	REGISTER_FUNC(GetWeaponSpec, 1);
	REGISTER_FUNC(GetModTexture, 1);
	REGISTER_FUNC(GetClientSkinInfo, 1);
	REGISTER_FUNC(GetClientCharacterCores, 0);
	REGISTER_FUNC(GetStandardSkinInfo, 0);
	REGISTER_FUNC(GetSkinPartTexture, 2);
	REGISTER_FUNC(GetCursorPosition, 0);
	REGISTER_FUNC(GetUiScreenRect, 0);
	REGISTER_FUNC(GetScreenSize, 0);
	REGISTER_FUNC(GetCamera, 0);
	REGISTER_FUNC(GetUiMousePos, 0);
	REGISTER_FUNC(GetPixelScale, 0);
	REGISTER_FUNC(PhysGetCores, 0);
	REGISTER_FUNC(PhysGetJoints, 0);
	REGISTER_FUNC(PhysSetTileCollisionFlags, 3);
	REGISTER_FUNC(DirectionFromAngle, 1);
	REGISTER_FUNC(SetHudPartsShown, 1);
	REGISTER_FUNC(NetSendPacket, 1);
	REGISTER_FUNC(NetPacketUnpack, 2);
	REGISTER_FUNC(AddWeapon, 1);
	REGISTER_FUNC(PlaySoundAt, 3);
	REGISTER_FUNC(PlaySoundGlobal, 1);
	REGISTER_FUNC(PlayMusic, 1);
	REGISTER_FUNC(RandomInt, 2);
	REGISTER_FUNC(CalculateTextSize, 1);
	REGISTER_FUNC(SetMenuModeActive, 1);

#undef REGISTER_FUNC_PLAIN
#undef REGISTER_FUNC
}

void CDuckLua::Reset()
{
	ResetLuaState();

	m_ScriptFileCount = 0;
	mem_zero(m_aIsScriptLuaLoaded, sizeof(m_aIsScriptLuaLoaded));
}

bool CDuckLua::GetFunction(const char *Name)
{
	lua_getglobal(L(), Name);
	if(lua_isfunction(L(), -1) == 0)
	{
		lua_pop(L(), 1);
		aLastCalledFunction[0] = 0;
		return false;
	}

	str_copy(aLastCalledFunction, Name, sizeof(aLastCalledFunction));
	return true;
}

bool CDuckLua::GetFunctionFromRef(int Ref, const char* Name)
{
	lua_rawgeti(L(), LUA_REGISTRYINDEX, Ref);
	if(lua_isfunction(L(), -1) == 0)
	{
		lua_pop(L(), 1);
		aLastCalledFunction[0] = 0;
		return false;
	}

	str_copy(aLastCalledFunction, Name, sizeof(aLastCalledFunction));
	return true;
}

void CDuckLua::CallFunction(int ArgCount, int ReturnCount)
{
	dbg_assert(lua_gettop(L()) == ArgCount + 1, "Wrong argument count pushed on stack");

	int ret = lua_pcall(L(), ArgCount, ReturnCount, 0);
	if(ret != 0)
	{
		LUA_ERR("%s(): \n    %s", aLastCalledFunction, lua_tostring(L(), -1));
		lua_pop(L(), 1);
	}

	if(lua_gettop(L()) > ReturnCount)
	{
		DBG_DETECTSTACKLEAK();
	}
}

CDuckLua::CDuckLua()
{
	s_DuckLua = this;
	m_pLuaState = NULL;
}

void CDuckLua::Shutdown()
{
	if(m_pLuaState)
	{
		lua_close(m_pLuaState);
		m_pLuaState = NULL;
	}
}

void CDuckLua::OnMessage(int Msg, void *pRawMsg)
{
	if(GetFunctionRef(OnMessage))
	{
		if(MakeVanillaLuaNetMessage(Msg, pRawMsg))
		{
		}
		else if(Msg == NETMSG_DUCK_NETOBJ)
		{
			CUnpacker* pUnpacker = (CUnpacker*)pRawMsg;
			const int ObjID = pUnpacker->GetInt();
			const int ObjSize = pUnpacker->GetInt();
			const u8* pObjRawData = (u8*)pUnpacker->GetRaw(ObjSize);
			//dbg_msg("duck", "DUK packed netobj, id=0x%x size=%d", ObjID, ObjSize);

			// make netObj
			lua_createtable(L(), 0, 2);
			LuaSetPropInteger(L(), -1, "mod_id", ObjID);
			LuaSetPropStringN(L(), -1, "raw", (const char*)pObjRawData, ObjSize);
		}
		else
		{
			// make netObj
			lua_createtable(L(), 0, 2);
			LuaSetPropInteger(L(), -1, "tw_id", Msg);
			LuaSetPropString(L(), -1, "error", "Unknown message type");
		}

		// call OnMessage(netObj)
		CallFunction(1, 0);
	}
}

void CDuckLua::OnSnapItem(int Msg, int SnapID, void *pRawMsg)
{
	if(GetFunctionRef(OnSnap))
	{
		if(MakeVanillaLuaNetObj(Msg, pRawMsg))
		{
			lua_pushinteger(L(), SnapID);

			CallFunction(2, 0);
		}
	}
}

void CDuckLua::OnDuckSnapItem(int Msg, int SnapID, void *pRawMsg, int Size)
{
	if(GetFunctionRef(OnSnap))
	{
		// make netObj
		lua_createtable(L(), 0, 2);
		LuaSetPropInteger(L(), -1, "mod_id", Msg);
		LuaSetPropStringN(L(), -1, "raw", (const char*)pRawMsg, Size);

		lua_pushinteger(L(), SnapID);

		CallFunction(2, 0);
	}
}

void CDuckLua::OnInput(IInput::CEvent e)
{
	if(GetFunctionRef(OnInput))
	{
		// make event
		lua_createtable(L(), 0, 3);
		LuaSetPropInteger(L(), -1, "key", e.m_Key);
		if(e.m_Flags&IInput::FLAG_PRESS)
			LuaSetPropBool(L(), -1, "pressed", true);
		if(e.m_Flags&IInput::FLAG_RELEASE)
			LuaSetPropBool(L(), -1, "released", true);
		LuaSetPropString(L(), -1, "text", e.m_aText);


		// call OnInput(event)
		CallFunction(1, 0);
	}
}

void CDuckLua::OnModLoaded()
{
	// Actually load the lua files now
	int MainLuaID = FindScriptFileFromName("main.lua");
	dbg_assert(MainLuaID != -1, "Main script file not found");

	bool r = LuaLoadScriptFileData(MainLuaID);
	if(!r)
	{
		LUA_CRIT("Could not load 'main.lua'");
		return;
	}

	FreeAllScriptFileData();

	// Get functions references for fast access

#define SEARCH_FUNCTION(F)\
	lua_getglobal(L(), #F);\
	m_FuncRef##F = luaL_ref(L(), LUA_REGISTRYINDEX)

	SEARCH_FUNCTION(OnLoad);
	SEARCH_FUNCTION(OnMessage);
	SEARCH_FUNCTION(OnSnap);
	SEARCH_FUNCTION(OnInput);
	SEARCH_FUNCTION(OnRender);
	SEARCH_FUNCTION(OnRenderPlayer);
	SEARCH_FUNCTION(OnUpdate);

#undef SEARCH_FUNCTION

	if(GetFunctionRef(OnLoad))
	{
		CallFunction(0, 0);
	}
}

static void AnimKeyframeLuaPush(lua_State* L, const CAnimKeyframe& Kf)
{
	lua_createtable(L, 0, 4);
	LuaSetPropNumber(L, -1, "time", Kf.m_Time);
	LuaSetPropNumber(L, -1, "x", Kf.m_X);
	LuaSetPropNumber(L, -1, "y", Kf.m_Y);
	LuaSetPropNumber(L, -1, "angle", Kf.m_Time);
}

bool CDuckLua::OnRenderPlayer(CAnimState* pState, vec2 Pos, int ClientID)
{
	if(GetFunctionRef(OnRenderPlayer))
	{
		// Argument 1 : AnimState
		lua_createtable(L(), 0, 4);

		lua_pushstring(L(), "body");
		AnimKeyframeLuaPush(L(), *pState->GetBody());
		lua_rawset(L(), -3);

		lua_pushstring(L(), "backfoot");
		AnimKeyframeLuaPush(L(), *pState->GetBackFoot());
		lua_rawset(L(), -3);

		lua_pushstring(L(), "frontfoot");
		AnimKeyframeLuaPush(L(), *pState->GetFrontFoot());
		lua_rawset(L(), -3);

		lua_pushstring(L(), "attach");
		AnimKeyframeLuaPush(L(), *pState->GetFrontFoot());
		lua_rawset(L(), -3);

		// Argument 2 : Position
		lua_createtable(L(), 0, 2);
		LuaSetPropNumber(L(), -1, "x", Pos.x);
		LuaSetPropNumber(L(), -1, "y", Pos.y);

		lua_pushinteger(L(), ClientID);
		CallFunction(3, 1);

		bool r = lua_toboolean(L(), -1);
		lua_pop(L(), 1);
		return r;
	}

	return false;
}

void CDuckLua::OnRender(float LocalTime, float IntraGameTick)
{
	if(GetFunctionRef(OnRender))
	{
		lua_pushnumber(L(), LocalTime);
		lua_pushnumber(L(), IntraGameTick);

		CallFunction(2, 0);
	}
}

void CDuckLua::OnUpdate(float LocalTime, float IntraGameTick)
{
	if(GetFunctionRef(OnUpdate))
	{
		lua_pushnumber(L(), LocalTime);

		CallFunction(1, 0);
	}
}

bool CDuckLua::IsStackLeaking()
{
	return lua_gettop(L()) != 0;
}

#include <generated/netobj_lua.cpp>
