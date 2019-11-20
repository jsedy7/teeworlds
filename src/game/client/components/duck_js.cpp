#include "duck_js.h"
#include <stdint.h>
#include <engine/storage.h>
#include <engine/client/http.h>
#include <engine/shared/config.h>

#include <generated/protocol.h>
#include <generated/client_data.h>
#include <game/client/components/skins.h>
#include <game/client/components/controls.h>

#include "duck_bridge.h"

static CDuckJs* s_DuckJs = 0;
inline CDuckJs* This() { return s_DuckJs; }

#define JS_WARN(fmt, ...) This()->Bridge()->ScriptError(JsErrorLvl::WARNING, fmt, __VA_ARGS__)
#define JS_ERR(fmt, ...) This()->Bridge()->ScriptError(JsErrorLvl::ERROR, fmt, __VA_ARGS__)
#define JS_CRIT(fmt, ...) This()->Bridge()->ScriptError(JsErrorLvl::CRITICAL, fmt, __VA_ARGS__)

bool CDuckJs::_CheckArgumentCountImp(duk_context* pCtx, int NumArgs, const char* pFuncName)
{
	int n = duk_get_top(pCtx);
	if(n != NumArgs)
	{
		JS_ERR("Tw%s() wrong argument count, expected %d", pFuncName, NumArgs);
		return false;
	}

	for(int i = 0; i < n; i++)
	{
		if(duk_is_undefined(pCtx, i))
		{
			JS_ERR("Tw%s() wrong argument count, expected %d", pFuncName, NumArgs);
			return false;
		}
	}

	return true;
}

#define CheckArgumentCount(ctx, num) if(!This()->_CheckArgumentCountImp(ctx, num, __FUNCTION__ + 15)) { return 0; }

/*#
`print(str)`

Print a string to console.

**Parameters**

* **str**: string

**Returns**

* None
#*/
duk_ret_t CDuckJs::NativePrint(duk_context *ctx)
{
	CheckArgumentCount(ctx, 1);

	const char* pStr = duk_to_string(ctx, 0);
	const int Len = str_length(pStr);
	const int MaxLen = 128;
	char aBuff[MaxLen+1];

	for(int i = 0; i < Len; i += MaxLen)
	{
		str_copy(aBuff, pStr + i, min(Len-i+1, MaxLen+1));
		This()->Bridge()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "mod", aBuff);
	}
	return 0;  /* no return value (= undefined) */
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
duk_ret_t CDuckJs::NativeRenderQuad(duk_context *ctx)
{
	CheckArgumentCount(ctx, 4);

	double x = duk_to_number(ctx, 0);
	double y = duk_to_number(ctx, 1);
	double Width = duk_to_number(ctx, 2);
	double Height = duk_to_number(ctx, 3);

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
duk_ret_t CDuckJs::NativeRenderQuadCentered(duk_context* ctx)
{
	CheckArgumentCount(ctx, 4);

	double x = duk_to_number(ctx, 0);
	double y = duk_to_number(ctx, 1);
	double Width = duk_to_number(ctx, 2);
	double Height = duk_to_number(ctx, 3);

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
duk_ret_t CDuckJs::NativeRenderSetColorU32(duk_context *ctx)
{
	CheckArgumentCount(ctx, 1);

	uint32_t x = duk_to_uint(ctx, 0);

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
duk_ret_t CDuckJs::NativeRenderSetColorF4(duk_context *ctx)
{
	CheckArgumentCount(ctx, 4);

	float aColor[4] = {1, 1, 1, 1};
	aColor[0] = duk_to_number(ctx, 0);
	aColor[1] = duk_to_number(ctx, 1);
	aColor[2] = duk_to_number(ctx, 2);
	aColor[3] = duk_to_number(ctx, 3);

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
duk_ret_t CDuckJs::NativeRenderSetTexture(duk_context *ctx)
{
	CheckArgumentCount(ctx, 1);

	This()->Bridge()->QueueSetTexture((int)duk_to_int(ctx, 0));
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
duk_ret_t CDuckJs::NativeRenderSetQuadSubSet(duk_context* ctx)
{
	CheckArgumentCount(ctx, 4);

	float aSubSet[4];
	aSubSet[0] = duk_to_number(ctx, 0);
	aSubSet[1] = duk_to_number(ctx, 1);
	aSubSet[2] = duk_to_number(ctx, 2);
	aSubSet[3] = duk_to_number(ctx, 3);

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
duk_ret_t CDuckJs::NativeRenderSetQuadRotation(duk_context* ctx)
{
	CheckArgumentCount(ctx, 1);

	float Angle = duk_to_number(ctx, 0);
	This()->Bridge()->QueueSetQuadRotation(Angle);
	return 0;
}


/*#
`TwRenderSetTeeSkin(skin)`

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
duk_ret_t CDuckJs::NativeRenderSetTeeSkin(duk_context* ctx)
{
	CheckArgumentCount(ctx, 1);

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
duk_ret_t CDuckJs::NativeRenderSetFreeform(duk_context *ctx)
{
	CheckArgumentCount(ctx, 1);

	static IGraphics::CFreeformItem aFreeformBuffer[CDuckBridge::CRenderSpace::FREEFORM_MAX_COUNT];
	int FreeformCount = 0;
	int VertCount = 0;
	float CurrentFreeform[sizeof(IGraphics::CFreeformItem)/sizeof(float)];
	const int FfFloatCount = sizeof(CurrentFreeform)/sizeof(CurrentFreeform[0]);

	const int ArrayLength = min((int)duk_get_length(ctx, 0), (int)CDuckBridge::CRenderSpace::FREEFORM_MAX_COUNT * FfFloatCount);
	for(int i = 0; i < ArrayLength; i++)
	{
		if(duk_get_prop_index(ctx, 0, i))
		{
			CurrentFreeform[VertCount++] = duk_to_number(ctx, -1);

			if(VertCount >= FfFloatCount)
			{
				VertCount = 0;
				aFreeformBuffer[FreeformCount++] = *(IGraphics::CFreeformItem*)CurrentFreeform;
			}
		}
		duk_pop(ctx);
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
duk_ret_t CDuckJs::NativeRenderSetDrawSpace(duk_context *ctx)
{
	CheckArgumentCount(ctx, 1);

	int ds = duk_to_int(ctx, 0);
	if(!(ds >= 0 && ds < CDuckBridge::DrawSpace::_COUNT))
	{
		JS_ERR("TwRenderSetDrawSpace(): Draw space undefined");
		return 0;
	}

	This()->Bridge()->m_CurrentDrawSpace = ds;
	return 0;
}

/*#
`TwRenderDrawTeeBodyAndFeet(tee)`

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
duk_ret_t CDuckJs::NativeRenderDrawTeeBodyAndFeet(duk_context *ctx)
{
	CheckArgumentCount(ctx, 1);

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
	return 0;
}

/*#
`TwRenderDrawTeeHand(tee)`

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
duk_ret_t CDuckJs::NativeRenderDrawTeeHand(duk_context* ctx)
{
	CheckArgumentCount(ctx, 1);

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
duk_ret_t CDuckJs::NativeRenderDrawFreeform(duk_context *ctx)
{
	CheckArgumentCount(ctx, 2);

	vec2 Pos;
	Pos.x = duk_to_number(ctx, 0);
	Pos.y = duk_to_number(ctx, 1);

	This()->Bridge()->QueueDrawFreeform(Pos);
	return 0;
}

/*#
`TwRenderDrawText(text)`

| Draw text.
| Example:

.. code-block:: js

	TwRenderDrawText({
		str: "This a text",
		font_size: 10,
		color: [1, 0, 1, 1], // rgba (0.0 - 1.0)
		rect: [100, 25, 200, 100], // x y width height
	});

**Parameters**

* **text**:

.. code-block:: js

	var text = {
		str: string,
		font_size: float,
		color: float[4],
		rect: float[4],
	};

**Returns**

* None
#*/
duk_ret_t CDuckJs::NativeRenderDrawText(duk_context *ctx)
{
	CheckArgumentCount(ctx, 1);

	dbg_assert(0, "update implementation");

#if 0
	if(!duk_is_object(ctx, 0))
	{
		JS_ERR("TwRenderDrawText(text) text is not an object");
		return 0;
	}

	char* pText = 0;
	float FontSize = 10.0f;
	float aColors[4] = {1, 1, 1, 1};
	float aRect[4] = { 0, 0, -1, -1};

	if(duk_get_prop_string(ctx, 0, "str"))
	{
		const char* pStr = duk_to_string(ctx, -1);
		int Len = min(str_length(pStr), 1024*1024); // 1MB max
		pText = (char*)This()->Bridge()->m_FrameAllocator.Alloc(Len+1);(Len+1);
		str_copy(pText, pStr, Len+1);
	}
	else
	{
		duk_pop(ctx);
		JS_ERR("TwRenderDrawText(text) text.str is null");
		return 0;
	}
	duk_pop(ctx);

	DukGetFloatProp(ctx, 0, "font_size", &FontSize);

	// get colors
	if(duk_get_prop_string(ctx, 0, "color"))
	{
		for(int i = 0; i < 4; i++)
		{
			if(duk_get_prop_index(ctx, -1, i))
				aColors[i] = duk_to_number(ctx, -1);
			duk_pop(ctx);
		}
	}
	duk_pop(ctx);

	// get rect
	if(duk_get_prop_string(ctx, 0, "rect"))
	{
		for(int i = 0; i < 4; i++)
		{
			if(duk_get_prop_index(ctx, -1, i))
				aRect[i] = duk_to_number(ctx, -1);
			duk_pop(ctx);
		}
	}
	else
	{
		duk_pop(ctx);
		JS_ERR("TwRenderDrawText(text) text.rect is undefined");
		return 0;
	}
	duk_pop(ctx);

	This()->Bridge()->QueueDrawText(pText, FontSize, aRect, aColors);
#endif
	return 0;
}

duk_ret_t CDuckJs::NativeRenderDrawCircle(duk_context *ctx)
{
	CheckArgumentCount(ctx, 3);

	float x = duk_to_number(ctx, 0);
	float y = duk_to_number(ctx, 1);
	float Radius = duk_to_number(ctx, 2);

	This()->Bridge()->QueueDrawCircle(vec2(x, y), Radius);
	return 0;
}

duk_ret_t CDuckJs::NativeRenderDrawLine(duk_context *ctx)
{
	CheckArgumentCount(ctx, 5);

	float Pos1X = duk_to_number(ctx, 0);
	float Pos1Y = duk_to_number(ctx, 1);
	float Pos2X = duk_to_number(ctx, 2);
	float Pos2Y = duk_to_number(ctx, 3);
	float Thickness = duk_to_number(ctx, 4);

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
duk_ret_t CDuckJs::NativeGetBaseTexture(duk_context* ctx)
{
	CheckArgumentCount(ctx, 1);

	int ImgID = duk_to_int(ctx, 0);

	duk_push_int(ctx, *(int*)&g_pData->m_aImages[ImgID % NUM_IMAGES].m_Id);
	return 1;
}

static void GetSpriteSubSet(const CDataSprite& Spr, float* pOutSubSet)
{
	int x = Spr.m_X;
	int y = Spr.m_Y;
	int w = Spr.m_W;
	int h = Spr.m_H;
	int cx = Spr.m_pSet->m_Gridx;
	int cy = Spr.m_pSet->m_Gridy;

	float x1 = x/(float)cx;
	float x2 = (x+w-1/32.0f)/(float)cx;
	float y1 = y/(float)cy;
	float y2 = (y+h-1/32.0f)/(float)cy;

	pOutSubSet[0] = x1;
	pOutSubSet[1] = y1;
	pOutSubSet[2] = x2;
	pOutSubSet[3] = y2;
}

/*#
`TwGetSpriteSubSet(sprite_id)`

| Get sprite texture coordinates.
| TODO: example

**Parameters**

* **sprite_id**: int

**Returns**

* **subset**:

.. code-block:: js

	var subset = {
		x1: float,
		y1: float,
		x2: float,
		y2: float,
	};
#*/
duk_ret_t CDuckJs::NativeGetSpriteSubSet(duk_context* ctx)
{
	CheckArgumentCount(ctx, 1);

	int SpriteID = duk_to_int(ctx, 0);

	CDataSprite Spr = g_pData->m_aSprites[SpriteID % NUM_SPRITES];
	float aSubSet[4];
	GetSpriteSubSet(Spr, aSubSet);

	This()->PushObject();
	This()->ObjectSetMemberFloat("x1", aSubSet[0]);
	This()->ObjectSetMemberFloat("y1", aSubSet[1]);
	This()->ObjectSetMemberFloat("x2", aSubSet[2]);
	This()->ObjectSetMemberFloat("y2", aSubSet[3]);
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
duk_ret_t CDuckJs::NativeGetSpriteScale(duk_context* ctx)
{
	CheckArgumentCount(ctx, 1);

	int SpriteID = duk_to_int(ctx, 0);

	CDataSprite Spr = g_pData->m_aSprites[SpriteID % NUM_SPRITES];
	int x = Spr.m_X;
	int y = Spr.m_Y;
	int w = Spr.m_W;
	int h = Spr.m_H;

	float f = sqrtf(h*h + w*w);
	float ScaleW = w/f;
	float ScaleH = h/f;

	This()->PushObject();
	This()->ObjectSetMemberFloat("w", ScaleW);
	This()->ObjectSetMemberFloat("h", ScaleH);
	return 1;
}

/*#
`TwGetWeaponSpec(weapon_id)`

| Get vanilla teeworlds weapon specifications.
| TODO: example

**Parameters**

* **weapon_id**: int

**Returns**

* **TODO**
#*/
duk_ret_t CDuckJs::NativeGetWeaponSpec(duk_context* ctx)
{
	CheckArgumentCount(ctx, 1);

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
duk_ret_t CDuckJs::NativeGetModTexture(duk_context *ctx)
{
	CheckArgumentCount(ctx, 1);

	const char* pTextureName = duk_get_string(ctx, 0);
	IGraphics::CTextureHandle Handle = This()->Bridge()->GetTextureFromName(pTextureName);
	duk_push_int(ctx, *(int*)&Handle);
	return 1;
}

/*#
`TwGetClientSkinInfo(client_id)`

| Returns the client's skin info

**Parameters**

* **client_id**: int

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
duk_ret_t CDuckJs::NativeGetClientSkinInfo(duk_context* ctx)
{
	CheckArgumentCount(ctx, 1);

	const int ClientID = clamp((int)duk_to_int(ctx, 0), 0, MAX_CLIENTS-1);
	if(!This()->Bridge()->m_pClient->m_aClients[ClientID].m_Active)
	{
		duk_push_null(ctx);
		return 1; // client not active, return null
	}

	const CTeeRenderInfo& RenderInfo = This()->Bridge()->m_pClient->m_aClients[ClientID].m_RenderInfo;

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
duk_ret_t CDuckJs::NativeGetClientCharacterCores(duk_context* ctx)
{
	CheckArgumentCount(ctx, 0);

	IClient* pClient = This()->Bridge()->Client();
	CGameClient* pGameClient = This()->Bridge()->m_pClient;

	float IntraTick = pClient->IntraGameTick();

	duk_idx_t ArrayIdx = duk_push_array(ctx);
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

			duk_idx_t ObjIdx = duk_push_object(ctx);
			DukSetIntProp(ctx, ObjIdx, "tick", Cur.m_Tick);
			DukSetFloatProp(ctx, ObjIdx, "vel_x", Vel.x);
			DukSetFloatProp(ctx, ObjIdx, "vel_y", Vel.y);
			DukSetFloatProp(ctx, ObjIdx, "angle", Angle);
			DukSetIntProp(ctx, ObjIdx, "direction", Cur.m_Direction);
			DukSetIntProp(ctx, ObjIdx, "jumped", Cur.m_Jumped);
			DukSetIntProp(ctx, ObjIdx, "hooked_player", Cur.m_HookedPlayer);
			DukSetIntProp(ctx, ObjIdx, "hook_state", Cur.m_HookState);
			DukSetIntProp(ctx, ObjIdx, "hook_tick", Cur.m_HookTick);
			DukSetFloatProp(ctx, ObjIdx, "hook_x", HookPos.x);
			DukSetFloatProp(ctx, ObjIdx, "hook_y", HookPos.y);
			DukSetFloatProp(ctx, ObjIdx, "hook_dx", Cur.m_HookDx/256.f);
			DukSetFloatProp(ctx, ObjIdx, "hook_dy", Cur.m_HookDy/256.f);
			DukSetFloatProp(ctx, ObjIdx, "pos_x", Position.x);
			DukSetFloatProp(ctx, ObjIdx, "pos_y", Position.y);
		}
		else
			duk_push_null(ctx);
		duk_put_prop_index(ctx, ArrayIdx, i);
	}

	return 1;
}

/*#
`TwGetStandardSkinInfo()`

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
duk_ret_t CDuckJs::NativeGetStandardSkinInfo(duk_context* ctx)
{
	CheckArgumentCount(ctx, 0);

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
duk_ret_t CDuckJs::NativeGetSkinPartTexture(duk_context* ctx)
{
	CheckArgumentCount(ctx, 2);

	int Part = clamp((int)duk_to_int(ctx, 0), 0, NUM_SKINPARTS-1);
	const char* PartName = duk_to_string(ctx, 1);

	int SkinPartID = This()->Bridge()->m_pClient->m_pSkins->FindSkinPart(Part, PartName, true);
	if(SkinPartID < 0)
	{
		duk_push_null(ctx);
		return 1;
	}

	duk_idx_t ArrayIdx = duk_push_array(ctx);

	duk_push_int(ctx, *(int*)&This()->Bridge()->m_pClient->m_pSkins->GetSkinPart(Part, SkinPartID)->m_OrgTexture);
	duk_put_prop_index(ctx, ArrayIdx, 0);
	duk_push_int(ctx, *(int*)&This()->Bridge()->m_pClient->m_pSkins->GetSkinPart(Part, SkinPartID)->m_ColorTexture);
	duk_put_prop_index(ctx, ArrayIdx, 1);
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
duk_ret_t CDuckJs::NativeGetCursorPosition(duk_context *ctx)
{
	CheckArgumentCount(ctx, 0);

	const vec2 CursorPos = This()->Bridge()->m_pClient->m_pControls->m_TargetPos;

	This()->PushObject();
	DukSetFloatProp(ctx, 0, "x", CursorPos.x);
	DukSetFloatProp(ctx, 0, "y", CursorPos.y);
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
duk_ret_t CDuckJs::NativeGetUiScreenRect(duk_context *ctx)
{
	CheckArgumentCount(ctx, 0);
	CUIRect Rect = This()->Bridge()->GetUiScreenRect();

	This()->PushObject();
	This()->ObjectSetMemberFloat("x", Rect.x);
	This()->ObjectSetMemberFloat("y", Rect.y);
	This()->ObjectSetMemberFloat("w", Rect.w);
	This()->ObjectSetMemberFloat("h", Rect.h);
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
duk_ret_t CDuckJs::NativeGetScreenSize(duk_context *ctx)
{
	CheckArgumentCount(ctx, 0);
	vec2 Size = This()->Bridge()->GetScreenSize();

	This()->PushObject();
	This()->ObjectSetMemberFloat("w", Size.x);
	This()->ObjectSetMemberFloat("h", Size.y);
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
duk_ret_t CDuckJs::NativeGetCamera(duk_context *ctx)
{
	CheckArgumentCount(ctx, 0);
	vec2 Pos = This()->Bridge()->GetCameraPos();

	This()->PushObject();
	This()->ObjectSetMemberFloat("x", Pos.x);
	This()->ObjectSetMemberFloat("y", Pos.y);
	This()->ObjectSetMemberFloat("zoom", This()->Bridge()->GetCameraZoom());
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
duk_ret_t CDuckJs::NativeGetUiMousePos(duk_context *ctx)
{
	CheckArgumentCount(ctx, 0);
	vec2 Pos = This()->Bridge()->GetUiMousePos();

	This()->PushObject();
	This()->ObjectSetMemberFloat("x", Pos.x);
	This()->ObjectSetMemberFloat("y", Pos.y);
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
duk_ret_t CDuckJs::NativeGetPixelScale(duk_context *ctx)
{
	CheckArgumentCount(ctx, 0);
	vec2 Scale = This()->Bridge()->GetPixelScale();

	This()->PushObject();
	This()->ObjectSetMemberFloat("x", Scale.x);
	This()->ObjectSetMemberFloat("y", Scale.y);
	return 1;
}


/*#
`TwPhysGetCores()`

| Get predicted physical cores.


**Parameters**

* None

**Returns**

* **cores**:

.. code-block:: js

	var cores = [
		{
			x: float,
			y: float,
		},
		...
	];

#*/
duk_ret_t CDuckJs::NativePhysGetCores(duk_context *ctx)
{
	CheckArgumentCount(ctx, 0);


	IClient* pClient = This()->Bridge()->Client();
	float IntraTick = pClient->IntraGameTick();
	float PredIntraTick = pClient->PredIntraGameTick();
	const CDuckBridge& Bridge = *This()->Bridge();

	const int Count = Bridge.m_WorldCorePredicted.m_aCustomCores.size();
	const CCustomCore* Cores = Bridge.m_WorldCorePredicted.m_aCustomCores.base_ptr();
	const int CountPrev = Bridge.m_WorldCorePredictedPrev.m_aCustomCores.size();
	const CCustomCore* CoresPrev = Bridge.m_WorldCorePredictedPrev.m_aCustomCores.base_ptr();

	duk_idx_t Array = duk_push_array(ctx);
	for(int CurI = 0; CurI < Count; CurI++)
	{
		duk_idx_t ObjIdx = duk_push_object(ctx);

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

		// don't interpolate this one
		if(PrevID == -1)
			PrevPos = CurPos;
		else
			PrevPos = CoresPrev[PrevID].m_Pos;

		vec2 Position = mix(PrevPos, CurPos, PredIntraTick);

		DukSetIntProp(ctx, ObjIdx, "id", CurI);
		DukSetFloatProp(ctx, ObjIdx, "x", Position.x);
		DukSetFloatProp(ctx, ObjIdx, "y", Position.y);
		DukSetFloatProp(ctx, ObjIdx, "radius", Cores[CurI].m_Radius);

		duk_put_prop_index(ctx, Array, CurI);
	}

	/*const int Count = Bridge.m_Snap.m_aCustomCores.size();
	const CNetObj_DuckCustomCore* Cores = Bridge.m_Snap.m_aCustomCores.base_ptr();
	const int CountPrev = Bridge.m_SnapPrev.m_aCustomCores.size();
	const CNetObj_DuckCustomCore* CoresPrev = Bridge.m_SnapPrev.m_aCustomCores.base_ptr();

	duk_idx_t Array = duk_push_array(ctx);
	for(int CurI = 0; CurI < Count; CurI++)
	{
		duk_idx_t ObjIdx = duk_push_object(ctx);

		int PrevID = -1;
		for(int PrevI = 0; PrevI < CountPrev; PrevI++)
		{
			if(CoresPrev[PrevI].m_UID == Cores[CurI].m_UID)
			{
				PrevID = PrevI;
				break;
			}
		}

		vec2 CurPos = vec2(Cores[CurI].m_PosX / 256.f, Cores[CurI].m_PosY / 256.f);
		vec2 PrevPos;

		// don't interpolate this one
		if(PrevID == -1)
			PrevPos = CurPos;
		else
			PrevPos = vec2(CoresPrev[PrevID].m_PosX / 256.f, CoresPrev[PrevID].m_PosY / 256.f);;

		vec2 Position = mix(PrevPos, CurPos, IntraTick);

		DukSetIntProp(ctx, ObjIdx, "id", CurI);
		DukSetFloatProp(ctx, ObjIdx, "x", Position.x);
		DukSetFloatProp(ctx, ObjIdx, "y", Position.y);
		DukSetFloatProp(ctx, ObjIdx, "radius", Cores[CurI].m_Radius / 256.f);

		duk_put_prop_index(ctx, Array, CurI);
	}*/

	return 1;
}

/*#
`TwPhysGetCores()`

| Get physical joints.
| *Work in progress*


**Parameters**

* None

**Returns**

* **joints**:

.. code-block:: js

	var joints = [
		{
			core1_id: int or null,
			core2_id: int or null,
		},
		...
	];

#*/
duk_ret_t CDuckJs::NativePhysGetJoints(duk_context *ctx)
{
	CheckArgumentCount(ctx, 0);

	IClient* pClient = This()->Bridge()->Client();
	float IntraTick = pClient->IntraGameTick();
	float PredIntraTick = pClient->PredIntraGameTick();
	CDuckBridge& Bridge = *This()->Bridge();

	const int Count = Bridge.m_WorldCorePredicted.m_aJoints.size();
	const CDuckPhysJoint* aJoints = Bridge.m_WorldCorePredicted.m_aJoints.base_ptr();
	const CCustomCore* aCores = Bridge.m_WorldCorePredicted.m_aCustomCores.base_ptr();

	duk_idx_t Array = duk_push_array(ctx);
	for(int i = 0; i < Count; i++)
	{
		const CDuckPhysJoint& Joint = aJoints[i];
		duk_idx_t ObjIdx = duk_push_object(ctx);

		CCustomCore* pCore1 = Bridge.m_WorldCorePredicted.FindCustomCoreFromUID(Joint.m_CustomCoreUID1);
		CCustomCore* pCore2 = Bridge.m_WorldCorePredicted.FindCustomCoreFromUID(Joint.m_CustomCoreUID2);

		if(pCore1)
			DukSetIntProp(ctx, ObjIdx, "core1_id", pCore1 - aCores);
		else
		{
			duk_push_null(ctx);
			duk_put_prop_string(ctx, ObjIdx, "core1_id");
		}

		if(pCore2)
			DukSetIntProp(ctx, ObjIdx, "core2_id", pCore2 - aCores);
		else
		{
			duk_push_null(ctx);
			duk_put_prop_string(ctx, ObjIdx, "core2_id");
		}

		duk_put_prop_index(ctx, Array, i);
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
duk_ret_t CDuckJs::NativePhysSetTileCollisionFlags(duk_context *ctx)
{
	CheckArgumentCount(ctx, 3);

	int Tx = duk_to_int(ctx, 0);
	int Ty = duk_to_int(ctx, 1);
	int Flags = duk_to_int(ctx, 2);

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
duk_ret_t CDuckJs::NativeDirectionFromAngle(duk_context* ctx)
{
	CheckArgumentCount(ctx, 1);

	float Angle = (float)duk_to_number(ctx, 0);
	vec2 Dir = direction(Angle);

	int DirObjIdx = duk_push_object(ctx);
		duk_push_number(ctx, Dir.x);
		duk_put_prop_string(ctx, DirObjIdx, "x");
		duk_push_number(ctx, Dir.y);
		duk_put_prop_string(ctx, DirObjIdx, "y");
		return 1;
}

/*#
`TwCollisionSetStaticBlock(block_id, block)`

| Creates or modify a collision block (rectangle).
| Note: these are supposed to stay the same size and not move much, if at all.

**Parameters**

* **block_id**: int
* **block**

.. code-block:: js

	var block = {
		flags: int, // collision flags
		pos_x, float,
		pos_y, float,
		width, float,
		height, float,
	};

**Returns**

* None

#*/
duk_ret_t CDuckJs::NativeCollisionSetStaticBlock(duk_context* ctx)
{
	CheckArgumentCount(ctx, 2);

	int BlockId = duk_to_int(ctx, 0);
	dbg_assert(0, "remove or implement");

	/*CDuckCollision::CStaticBlock SolidBlock;
	int Flags = -1;

	DukGetIntProp(ctx, 1, "flags", &Flags);
	SolidBlock.m_Flags = Flags;

	DukGetFloatProp(ctx, 1, "pos_x", &SolidBlock.m_Pos.x);
	DukGetFloatProp(ctx, 1, "pos_y", &SolidBlock.m_Pos.y);
	DukGetFloatProp(ctx, 1, "width", &SolidBlock.m_Size.x);
	DukGetFloatProp(ctx, 1, "height", &SolidBlock.m_Size.y);

	if(BlockId >= 0)
		This()->Bridge()->m_Collision.SetStaticBlock(BlockId, SolidBlock);*/
	return 0;
}

/*#
`TwCollisionClearStaticBlock(block_id)`

| Removes a collision block.

**Parameters**

* **block_id**: int

**Returns**

* None

#*/
duk_ret_t CDuckJs::NativeCollisionClearStaticBlock(duk_context* ctx)
{
	CheckArgumentCount(ctx, 1);

	dbg_assert(0, "remove or implement");

	/*int BlockId = duk_to_int(ctx, 0);
	if(BlockId >= 0)
		This()->Bridge()->m_Collision.ClearStaticBlock(BlockId);*/
	return 0;
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
duk_ret_t CDuckJs::NativeSetHudPartsShown(duk_context *ctx)
{
	CheckArgumentCount(ctx, 1);

	CDuckBridge::CHudPartsShown hps = This()->Bridge()->m_HudPartsShown;

	DukGetBoolProp(ctx, 0, "health", &hps.m_Health);
	DukGetBoolProp(ctx, 0, "armor", &hps.m_Armor);
	DukGetBoolProp(ctx, 0, "ammo", &hps.m_Ammo);
	DukGetBoolProp(ctx, 0, "time", &hps.m_Time);
	DukGetBoolProp(ctx, 0, "killfeed", &hps.m_KillFeed);
	DukGetBoolProp(ctx, 0, "score", &hps.m_Score);
	DukGetBoolProp(ctx, 0, "chat", &hps.m_Chat);
	DukGetBoolProp(ctx, 0, "scoreboard", &hps.m_Scoreboard);
	DukGetBoolProp(ctx, 0, "weapon_cursor", &hps.m_WeaponCursor);

	This()->Bridge()->SetHudPartsShown(hps);

	return 0;
}

/*#
`TwNetSendPacket(packet)`

| Send a packet.
| Packet object needs to be formatted to add type information, example:

.. code-block:: js

	var packet = {
		net_id: 1478,
		force_send_now: 0,

		i32_blockID: 1,
		i32_flags:   5,
		float_pos_x: 180,
		float_pos_y: 20,
		float_vel_x: 0,
		float_vel_y: 0,
		float_width: 1000,
		float_height:200,
	});


| The first 2 fields are required, the rest are in the form type_name: value.
| Supported types are:

* i32
* u32
* float
* str* (str32_something is a 32 length string)

**Parameters**

* **packet**: user edited object based on:

.. code-block:: js

	var packet = {
		net_id: int,
		force_send_now: int (0 or 1),
		...
	});

**Returns**

* None

#*/
duk_ret_t CDuckJs::NativeNetSendPacket(duk_context *ctx)
{
	CheckArgumentCount(ctx, 1);

	if(!duk_is_object(ctx, 0))
	{
		JS_ERR("TwNetSendPacket(packet) packet is not an object");
		return 0;
	}

	int NetID = -1;
	int SendNow = 0;
	DukGetIntProp(ctx, 0, "net_id", &NetID);
	DukGetIntProp(ctx, 0, "force_send_now", &SendNow);

	if(NetID <= 0)
	{
		JS_ERR("TwNetSendPacket(packet) net_id is invalid (%d)", NetID);
		return 0;
	}

	int Flags = 0;
	Flags |= MSGFLAG_VITAL;
	if(SendNow > 0) Flags |= MSGFLAG_FLUSH;
	This()->Bridge()->PacketCreate(NetID, Flags);

	// list properties
	duk_enum(ctx, 0, DUK_ENUM_OWN_PROPERTIES_ONLY);

	while(duk_next(ctx, -1 /*enum_idx*/, 0 /*get_value*/))
	{
		/* [ ... enum key ] */
		const char* pKey = duk_get_string(ctx, -1);

		if(str_startswith(pKey, "i32_"))
		{
			int Val;
			DukGetIntProp(ctx, 0, pKey, &Val);
			This()->Bridge()->PacketPackInt(Val);
		}
		else if(str_startswith(pKey, "u32_"))
		{
			int Val;
			DukGetIntProp(ctx, 0, pKey, &Val);
			This()->Bridge()->PacketPackInt(Val);
		}
		else if(str_startswith(pKey, "float_"))
		{
			float Val;
			DukGetFloatProp(ctx, 0, pKey, &Val);
			This()->Bridge()->PacketPackFloat(Val);
		}
		else if(str_startswith(pKey, "str"))
		{
			int Size;
			if(sscanf(pKey+3, "%d_", &Size) == 1)
			{
				char aStr[2048];

				if(Size > sizeof(aStr))
				{
					JS_ERR("TwNetSendPacket(packet) string is too large (%d)", Size);
					return 0;
				}

				DukGetStringProp(ctx, 0, pKey, aStr, sizeof(aStr));
				This()->Bridge()->PacketPackString(aStr, Size);
			}
		}

		duk_pop(ctx);  /* pop_key */
	}

#undef ADD_PROP
	duk_pop(ctx);  /* pop enum object */

	This()->Bridge()->SendPacket();
	return 0;
}

/*#
`TwNetPacketUnpack(packet, template)`

| Unpack packet based on template.
| Each template field will be filled based on the specified type, for example this code:

.. code-block:: js

	var block = TwNetPacketUnpack(packet, {
		i32_blockID: 0,
		i32_flags:   0,
		float_pos_x: 0,
		float_pos_y: 0,
		float_vel_x: 0,
		float_vel_y: 0,
		float_width: 0,
		float_height:0,
	});

| Will fill the first field (blockID) with the first int in the packet. Same with flags, pos_x will get a float and so on.
| The type is removed on return, so the resulting object looks like this:

.. code-block:: js

	var block = {
		blockID: int,
		flags: int,
		pos_x: float,
		pos_y: float,
		vel_x: float,
		vel_y: float,
		width: float,
		height:float,
	};

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
duk_ret_t CDuckJs::NativeNetPacketUnpack(duk_context *ctx)
{
	CheckArgumentCount(ctx, 2);

	if(!duk_is_object(ctx, 0))
	{
		JS_ERR("TwNetPacketUnpack(packet, template) packet is not an object");
		return 0;
	}

	if(!duk_is_object(ctx, 1))
	{
		JS_ERR("TwNetPacketUnpack(packet, template) template is not an object");
		return 0;
	}

	if(!duk_get_prop_string(ctx, 0, "raw"))
	{
		duk_pop(ctx);
		JS_ERR("TwNetPacketUnpack(packet, template) can't unpack");
		return 0;
	}

	duk_size_t RawBufferSize;
	const u8* pRawBuffer = (u8*)duk_get_buffer(ctx, -1, &RawBufferSize);
	duk_pop(ctx);

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
	vp.pValue = pRawBuffer + Cursor;\
	vp.Type = TYPE;\
	vp.Size = SIZE;\
	Cursor += SIZE;\
	if(Cursor > RawBufferSize) {\
		JS_ERR("TwNetPacketUnpack(net_obj, template) template is too large (%d > %d)", Cursor, RawBufferSize);\
		return 0;\
	}\
	aProperties.add(vp)

	// list properties
	duk_enum(ctx, 1, DUK_ENUM_OWN_PROPERTIES_ONLY);

	while(duk_next(ctx, -1 /*enum_idx*/, 0 /*get_value*/))
	{
		/* [ ... enum key ] */
		const char* pKey = duk_get_string(ctx, -1);

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

		duk_pop(ctx);  /* pop_key */
	}

#undef ADD_PROP
	duk_pop(ctx);  /* pop enum object */

	This()->PushObject();
	for(int i = 0; i < aProperties.size(); i++)
	{
		const CKeyValuePair& vp = aProperties[i];
		//dbg_msg("duck", "type=%d size=%d key='%s'", aProperties[i].Type, aProperties[i].Size, aProperties[i].aKey);

		switch(vp.Type)
		{
			case T_INT32:
				This()->ObjectSetMemberInt(vp.aKey, *(int32_t*)vp.pValue);
				break;

			case T_UINT32:
				This()->ObjectSetMemberUint(vp.aKey, *(uint32_t*)vp.pValue);
				break;

			case T_FLOAT:
				This()->ObjectSetMemberFloat(vp.aKey, *(float*)vp.pValue);
				break;

			case T_STRING:
			{
				char aString[2048];
				str_copy(aString, (const char*)vp.pValue, sizeof(aString));
				This()->ObjectSetMemberString(vp.aKey, aString);
			} break;

			default:
				JS_CRIT("Type not handled");
				break;
		}
	}

	if(Cursor < RawBufferSize)
		JS_WARN("TwNetPacketUnpack() packet only partially unpacked (%d / %d)", Cursor, RawBufferSize);

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
duk_ret_t CDuckJs::NativeAddWeapon(duk_context *ctx)
{
	CheckArgumentCount(ctx, 1);

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
duk_ret_t CDuckJs::NativePlaySoundAt(duk_context *ctx)
{
	CheckArgumentCount(ctx, 3);

	const char* pStr = duk_to_string(ctx, 0);
	float x = duk_to_number(ctx, 1);
	float y = duk_to_number(ctx, 2);

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
duk_ret_t CDuckJs::NativePlaySoundGlobal(duk_context *ctx)
{
	CheckArgumentCount(ctx, 1);

	const char* pStr = duk_to_string(ctx, 0);

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
duk_ret_t CDuckJs::NativePlayMusic(duk_context *ctx)
{
	CheckArgumentCount(ctx, 1);

	const char* pStr = duk_to_string(ctx, 0);

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
duk_ret_t CDuckJs::NativeRandomInt(duk_context *ctx)
{
	CheckArgumentCount(ctx, 2);

	int Min = duk_to_int(ctx, 0);
	int Max = duk_to_int(ctx, 1);
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
	duk_push_int(ctx, Val);
	return 1;
}

/*#
`TwCalculateTextSize(text)`

| Calculate text size for the current draw space.
| Example:

.. code-block:: js

	var size = TwCalculateTextSize({
		str: "Some text",
		font_size: 13,
		line_width: 240
	});

| Note: this is not 100% accurate for now unfortunately...

**Parameters**

* **text**:

.. code-block:: js

	var text = {
		str: string,
		font_size: float,
		line_width: float
	};

**Returns**

* **size**: { x: float, y: float }

#*/
duk_ret_t CDuckJs::NativeCalculateTextSize(duk_context *ctx)
{
	CheckArgumentCount(ctx, 1);

	if(!duk_is_object(ctx, 0))
	{
		JS_ERR("TwCalculateTextSize(text) text is not an object");
		return 0;
	}

	char* pText = 0;
	float FontSize = 10.0f;
	float LineWidth = -1;

	if(duk_get_prop_string(ctx, 0, "str"))
	{
		const char* pStr = duk_to_string(ctx, -1);
		int Len = min(str_length(pStr), 1024*1024); // 1MB max
		pText = (char*)This()->Bridge()->m_FrameAllocator.Alloc(Len+1);
		str_copy(pText, pStr, Len+1);
	}
	else
	{
		duk_pop(ctx);
		JS_ERR("TwCalculateTextSize(text) text.str is null");
		return 0;
	}
	duk_pop(ctx);

	DukGetFloatProp(ctx, 0, "font_size", &FontSize);
	DukGetFloatProp(ctx, 0, "line_width", &LineWidth);

	vec2 Size = This()->Bridge()->CalculateTextSize(pText, FontSize, LineWidth);

	This()->PushObject();
	This()->ObjectSetMemberFloat("w", Size.x);
	This()->ObjectSetMemberFloat("h", Size.y);
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
duk_ret_t CDuckJs::NativeSetMenuModeActive(duk_context *ctx)
{
	CheckArgumentCount(ctx, 1);

	bool Active = duk_to_boolean(ctx, 0);
	This()->Bridge()->SetMenuModeActive(Active);
	return 0;
}

void CDuckJs::PushObject()
{
	m_CurrentPushedObjID = duk_push_object(Ctx());
}

void CDuckJs::ObjectSetMemberInt(const char* MemberName, int Value)
{
	duk_push_int(Ctx(), Value);
	duk_put_prop_string(Ctx(), m_CurrentPushedObjID, MemberName);
}

void CDuckJs::ObjectSetMemberUint(const char* MemberName, uint32_t Value)
{
	duk_push_uint(Ctx(), Value);
	duk_put_prop_string(Ctx(), m_CurrentPushedObjID, MemberName);
}

void CDuckJs::ObjectSetMemberFloat(const char* MemberName, float Value)
{
	duk_push_number(Ctx(), Value);
	duk_put_prop_string(Ctx(), m_CurrentPushedObjID, MemberName);
}

void CDuckJs::ObjectSetMemberRawBuffer(const char* MemberName, const void* pRawBuffer, int RawBufferSize)
{
	duk_push_string(Ctx(), MemberName);
	duk_push_fixed_buffer(Ctx(), RawBufferSize);

	duk_size_t OutBufferSize;
	u8* OutBuffer = (u8*)duk_require_buffer_data(Ctx(), -1, &OutBufferSize);
	dbg_assert((int)OutBufferSize == RawBufferSize, "buffer size differs");
	mem_copy(OutBuffer, pRawBuffer, RawBufferSize);

	int rc = duk_put_prop(Ctx(), m_CurrentPushedObjID);
	dbg_assert(rc == 1, "could not put raw buffer prop");
}

void CDuckJs::ObjectSetMemberString(const char *MemberName, const char *pStr)
{
	duk_push_string(Ctx(), pStr);
	duk_put_prop_string(Ctx(), m_CurrentPushedObjID, MemberName);
}

void CDuckJs::ObjectSetMemberBool(const char *MemberName, bool Val)
{
	duk_push_boolean(Ctx(), Val);
	duk_put_prop_string(Ctx(), m_CurrentPushedObjID, MemberName);
}

void CDuckJs::ObjectSetMember(const char* MemberName)
{
	duk_put_prop_string(Ctx(), m_CurrentPushedObjID, MemberName);
}

bool CDuckJs::LoadScriptFile(const char* pJsFilePath, const char* pJsRelFilePath)
{
	IOHANDLE ScriptFile = io_open(pJsFilePath, IOFLAG_READ);
	if(!ScriptFile)
	{
		dbg_msg("duck", "could not open '%s'", pJsFilePath);
		return false;
	}

	int FileSize = (int)io_length(ScriptFile);
	char *pFileData = (char *)mem_alloc(FileSize + 1, 1);
	io_read(ScriptFile, pFileData, FileSize);
	pFileData[FileSize] = 0;
	io_close(ScriptFile);

	// eval script
	duk_push_string(Ctx(), pJsRelFilePath);
	if(duk_pcompile_string_filename(Ctx(), DUK_COMPILE_STRICT, pFileData) != 0)
	{
		JS_ERR("'%s': \n   %s", pJsRelFilePath, duk_safe_to_stacktrace(Ctx(), -1));
		mem_free(pFileData);
		return false;
	}
	if(duk_pcall(Ctx(), 0) != 0)
	{
		JS_ERR("'%s': \n   %s", pJsRelFilePath, duk_safe_to_stacktrace(Ctx(), -1));
		mem_free(pFileData);
		return false;
	}

	duk_pop(Ctx());
	dbg_msg("duck", "'%s' loaded (%d)", pJsRelFilePath, FileSize);
	mem_free(pFileData);
	return true;
}

void CDuckJs::ResetDukContext()
{
	if(m_pDukContext)
		duk_destroy_heap(m_pDukContext);

	// load ducktape, eval main.js
	m_pDukContext = duk_create_heap_default();

	// function binding
	// special functions
	duk_push_c_function(Ctx(), NativePrint, 1);
	duk_put_global_string(Ctx(), "print");

#define REGISTER_FUNC(fname, arg_count) \
	duk_push_c_function(Ctx(), Native##fname, arg_count);\
	duk_put_global_string(Ctx(), "Tw" #fname)

	REGISTER_FUNC(RenderQuad, 4);
	REGISTER_FUNC(RenderQuadCentered, 4);
	REGISTER_FUNC(RenderSetColorU32, 1);
	REGISTER_FUNC(RenderSetColorF4, 4);
	REGISTER_FUNC(RenderSetTexture, 1);
	REGISTER_FUNC(RenderSetQuadSubSet, 4);
	REGISTER_FUNC(RenderSetQuadRotation, 1);
	REGISTER_FUNC(RenderSetTeeSkin, 1);
	REGISTER_FUNC(RenderSetFreeform, 1);
	REGISTER_FUNC(RenderDrawTeeBodyAndFeet, 1);
	REGISTER_FUNC(RenderDrawTeeHand, 1);
	REGISTER_FUNC(RenderDrawFreeform, 2);
	REGISTER_FUNC(RenderDrawText, 1);
	REGISTER_FUNC(RenderDrawCircle, 3);
	REGISTER_FUNC(RenderDrawLine, 5);
	REGISTER_FUNC(RenderSetDrawSpace, 1);
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
	REGISTER_FUNC(CollisionSetStaticBlock, 2);
	REGISTER_FUNC(CollisionClearStaticBlock, 1);
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


#undef REGISTER_FUNC

	// Teeworlds global object
	char* aBuff = (char*)mem_alloc(1024*1024, 1);

	str_format(aBuff, 1024*1024,
		//"\"use strict\";\n"
		"const Teeworlds = {"
		"	DRAW_SPACE_GAME: 0,"
		"	DRAW_SPACE_GAME_FOREGROUND: 1,"
		"	DRAW_SPACE_HUD: 2,"
		"%s"
		"};", GetContentEnumsAsJs());

	//dbg_msg("duck", aBuff);

	//duk_eval_string(Ctx(), aBuff);
	if(duk_peval_string(Ctx(), aBuff) != DUK_EXEC_SUCCESS)
	{
		JS_ERR(duk_safe_to_stacktrace(Ctx(), -1));
		dbg_break();
	}

	duk_pop(Ctx());

	mem_free(aBuff);
}

void CDuckJs::Reset()
{
	ResetDukContext();
}

bool CDuckJs::GetJsFunction(const char *Name)
{
	duk_context* pCtx = Ctx();

	if(!duk_get_global_string(pCtx, Name))
	{
		duk_pop(pCtx);
		aLastCalledFunction[0] = 0;
		return false;
	}

	str_copy(aLastCalledFunction, Name, sizeof(aLastCalledFunction));
	return true;
}

void CDuckJs::CallJsFunction(int NumArgs)
{
	duk_context* pCtx = Ctx();

	if(duk_pcall(pCtx, NumArgs) != DUK_EXEC_SUCCESS)
	{
		JS_ERR("%s(): \n    %s", aLastCalledFunction, duk_safe_to_stacktrace(pCtx, -1));
	}
}

bool CDuckJs::HasJsFunctionReturned()
{
	duk_context* pCtx = Ctx();

	if(duk_is_undefined(pCtx, -1))
	{
		JS_WARN("%s() must return a value", aLastCalledFunction);
		duk_pop(pCtx);
		return false;
	}

	return true;
}

CDuckJs::CDuckJs()
{
	s_DuckJs = this;
	m_pDukContext = 0;
}

void CDuckJs::Shutdown()
{
	if(m_pDukContext)
	{
		duk_destroy_heap(m_pDukContext);
		m_pDukContext = 0;
	}
}

void CDuckJs::OnMessage(int Msg, void* pRawMsg)
{
	if(GetJsFunction("OnMessage"))
	{
		if(MakeVanillaJsNetMessage(Msg, pRawMsg))
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
			PushObject();
			ObjectSetMemberInt("mod_id", ObjID);
			ObjectSetMemberRawBuffer("raw", pObjRawData, ObjSize);
		}
		else
		{
			// make netObj
			PushObject();
			ObjectSetMemberInt("tw_id", Msg);
			ObjectSetMemberString("error", "Unknown message type");
		}

		// call OnMessage(netObj)
		CallJsFunction(1);
		duk_pop(Ctx());
	}
}

void CDuckJs::OnSnapItem(int Msg, int SnapID, void *pRawMsg)
{
	if(GetJsFunction("OnSnap"))
	{
		if(MakeVanillaJsNetObj(Msg, pRawMsg))
		{
			duk_push_int(Ctx(), SnapID);

			CallJsFunction(2);
			duk_pop(Ctx());
		}
	}
}

void CDuckJs::OnDuckSnapItem(int Msg, int SnapID, void *pRawMsg, int Size)
{
	if(GetJsFunction("OnSnap"))
	{
		// make netObj
		PushObject();
		ObjectSetMemberInt("mod_id", Msg);
		ObjectSetMemberRawBuffer("raw", pRawMsg, Size);

		duk_push_int(Ctx(), SnapID);

		CallJsFunction(2);
		duk_pop(Ctx());
	}
}

void CDuckJs::OnInput(IInput::CEvent e)
{
	if(GetJsFunction("OnInput")) {
		// make event
		PushObject();
		ObjectSetMemberInt("key", e.m_Key);
		ObjectSetMemberInt("pressed", e.m_Flags&IInput::FLAG_PRESS ? 1:0);
		ObjectSetMemberInt("released", e.m_Flags&IInput::FLAG_RELEASE ? 1:0);
		ObjectSetMemberString("text", e.m_aText);


		// call OnInput(event)
		CallJsFunction(1);

		duk_pop(Ctx());
	}
}

void CDuckJs::OnModLoaded()
{
	if(GetJsFunction("OnLoaded")) {
		// call OnLoaded()
		CallJsFunction(0);
		duk_pop(Ctx());
	}
}

void CDuckJs::OnRender(float LocalTime, float IntraGameTick)
{
	// Call OnRender(LocalTime, IntraGameTick)
	if(GetJsFunction("OnRender"))
	{
		duk_push_number(Ctx(), LocalTime);
		duk_push_number(Ctx(), IntraGameTick);

		CallJsFunction(2);

		duk_pop(Ctx());
	}
}

void CDuckJs::OnUpdate(float LocalTime, float IntraGameTick)
{
	if(GetJsFunction("OnUpdate"))
	{
		duk_push_number(Ctx(), LocalTime);
		duk_push_number(Ctx(), IntraGameTick);

		CallJsFunction(2);

		duk_pop(Ctx());
	}
}

bool CDuckJs::IsStackLeaking()
{
	return duk_get_top(Ctx()) != 0;
}

duk_idx_t DuktapePushCharacterCore(duk_context* pCtx, const CCharacterCore* pCharCore)
{
	duk_idx_t CharCoreObjIdx = duk_push_object(pCtx);
	DukSetFloatProp(pCtx, CharCoreObjIdx, "pos_x", pCharCore->m_Pos.x);
	DukSetFloatProp(pCtx, CharCoreObjIdx, "pos_y", pCharCore->m_Pos.y);
	DukSetFloatProp(pCtx, CharCoreObjIdx, "vel_x", pCharCore->m_Vel.x);
	DukSetFloatProp(pCtx, CharCoreObjIdx, "vel_y", pCharCore->m_Vel.y);
	DukSetFloatProp(pCtx, CharCoreObjIdx, "hook_pos_x", pCharCore->m_HookPos.x);
	DukSetFloatProp(pCtx, CharCoreObjIdx, "hook_pos_y", pCharCore->m_HookPos.y);
	DukSetFloatProp(pCtx, CharCoreObjIdx, "hook_dir_x", pCharCore->m_HookDir.x);
	DukSetFloatProp(pCtx, CharCoreObjIdx, "hook_dir_y", pCharCore->m_HookDir.y);

	DukSetIntProp(pCtx, CharCoreObjIdx, "hook_tick", pCharCore->m_HookTick);
	DukSetIntProp(pCtx, CharCoreObjIdx, "hook_state", pCharCore->m_HookState);
	DukSetIntProp(pCtx, CharCoreObjIdx, "hooked_player", pCharCore->m_HookedPlayer);
	DukSetIntProp(pCtx, CharCoreObjIdx, "jumped", pCharCore->m_Jumped);
	DukSetIntProp(pCtx, CharCoreObjIdx, "direction", pCharCore->m_Direction);
	DukSetIntProp(pCtx, CharCoreObjIdx, "angle", pCharCore->m_Angle);
	DukSetIntProp(pCtx, CharCoreObjIdx, "triggered_events", pCharCore->m_TriggeredEvents);
	return CharCoreObjIdx;
}

duk_idx_t DuktapePushNetObjPlayerInput(duk_context* pCtx, const CNetObj_PlayerInput* pInput)
{
	duk_idx_t InputObjIdx = duk_push_object(pCtx);
	DukSetIntProp(pCtx, InputObjIdx, "direction", pInput->m_Direction);
	DukSetIntProp(pCtx, InputObjIdx, "target_x", pInput->m_TargetX);
	DukSetIntProp(pCtx, InputObjIdx, "target_y", pInput->m_TargetY);
	DukSetIntProp(pCtx, InputObjIdx, "jump", pInput->m_Jump);
	DukSetIntProp(pCtx, InputObjIdx, "fire", pInput->m_Fire);
	DukSetIntProp(pCtx, InputObjIdx, "hook", pInput->m_Hook);
	DukSetIntProp(pCtx, InputObjIdx, "player_flags", pInput->m_PlayerFlags);
	DukSetIntProp(pCtx, InputObjIdx, "wanted_weapon", pInput->m_WantedWeapon);
	DukSetIntProp(pCtx, InputObjIdx, "next_weapon", pInput->m_NextWeapon);
	DukSetIntProp(pCtx, InputObjIdx, "prev_weapon", pInput->m_PrevWeapon);
	return InputObjIdx;
}

void DuktapeReadCharacterCore(duk_context* pCtx, duk_idx_t ObjIdx, CCharacterCore* pOutCharCore)
{
	DukGetFloatProp(pCtx, ObjIdx, "pos_x", &pOutCharCore->m_Pos.x);
	DukGetFloatProp(pCtx, ObjIdx, "pos_y", &pOutCharCore->m_Pos.y);
	DukGetFloatProp(pCtx, ObjIdx, "vel_x", &pOutCharCore->m_Vel.x);
	DukGetFloatProp(pCtx, ObjIdx, "vel_y", &pOutCharCore->m_Vel.y);
	DukGetFloatProp(pCtx, ObjIdx, "hook_pos_x", &pOutCharCore->m_HookPos.x);
	DukGetFloatProp(pCtx, ObjIdx, "hook_pos_y", &pOutCharCore->m_HookPos.y);
	DukGetFloatProp(pCtx, ObjIdx, "hook_dir_x", &pOutCharCore->m_HookDir.x);
	DukGetFloatProp(pCtx, ObjIdx, "hook_dir_y", &pOutCharCore->m_HookDir.y);

	DukGetIntProp(pCtx, ObjIdx, "hook_tick", &pOutCharCore->m_HookTick);
	DukGetIntProp(pCtx, ObjIdx, "hook_state", &pOutCharCore->m_HookState);
	DukGetIntProp(pCtx, ObjIdx, "hooked_player", &pOutCharCore->m_HookedPlayer);
	DukGetIntProp(pCtx, ObjIdx, "jumped", &pOutCharCore->m_Jumped);
	DukGetIntProp(pCtx, ObjIdx, "direction", &pOutCharCore->m_Direction);
	DukGetIntProp(pCtx, ObjIdx, "angle", &pOutCharCore->m_Angle);
	DukGetIntProp(pCtx, ObjIdx, "triggered_events", &pOutCharCore->m_TriggeredEvents);
}

void DuktapeReadNetObjPlayerInput(duk_context* pCtx, duk_idx_t ObjIdx, CNetObj_PlayerInput* pOutInput)
{
	DukGetIntProp(pCtx, ObjIdx, "direction", &pOutInput->m_Direction);
	DukGetIntProp(pCtx, ObjIdx, "target_x", &pOutInput->m_TargetX);
	DukGetIntProp(pCtx, ObjIdx, "target_y", &pOutInput->m_TargetY);
	DukGetIntProp(pCtx, ObjIdx, "jump", &pOutInput->m_Jump);
	DukGetIntProp(pCtx, ObjIdx, "fire", &pOutInput->m_Fire);
	DukGetIntProp(pCtx, ObjIdx, "hook", &pOutInput->m_Hook);
	DukGetIntProp(pCtx, ObjIdx, "player_flags", &pOutInput->m_PlayerFlags);
	DukGetIntProp(pCtx, ObjIdx, "wanted_weapon", &pOutInput->m_WantedWeapon);
	DukGetIntProp(pCtx, ObjIdx, "next_weapon", &pOutInput->m_NextWeapon);
	DukGetIntProp(pCtx, ObjIdx, "prev_weapon", &pOutInput->m_PrevWeapon);
}
