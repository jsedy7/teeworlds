#include "duck_js.h"
#include <engine/storage.h>
#include <generated/protocol.h>
#include <generated/client_data.h>
#include <stdint.h>

#include <engine/client/http.h>
#include <engine/shared/growbuffer.h>
#include <engine/shared/config.h>
#include <engine/external/zlib/zlib.h>
#include <game/client/components/skins.h>
#include <game/client/components/controls.h>
#include <zip.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t i32;

// TODO: rename?
static CDuckJs* s_This = 0;
inline CDuckJs* This() { return s_This; }

static duk_ret_t NativePrint(duk_context *ctx)
{
	dbg_msg("duck", "%s", duk_to_string(ctx, 0));
	return 0;  /* no return value (= undefined) */
}

duk_ret_t CDuckJs::NativeRenderQuad(duk_context *ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 4, "Wrong argument count");
	double x = duk_to_number(ctx, 0);
	double y = duk_to_number(ctx, 1);
	double Width = duk_to_number(ctx, 2);
	double Height = duk_to_number(ctx, 3);

	IGraphics::CQuadItem Quad(x, y, Width, Height);
	This()->m_Bridge.QueueDrawQuad(Quad);
	return 0;
}

duk_ret_t CDuckJs::NativeRenderQuadCentered(duk_context* ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 4, "Wrong argument count");
	double x = duk_to_number(ctx, 0);
	double y = duk_to_number(ctx, 1);
	double Width = duk_to_number(ctx, 2);
	double Height = duk_to_number(ctx, 3);

	IGraphics::CQuadItem Quad(x, y, Width, Height);
	This()->m_Bridge.QueueDrawQuadCentered(Quad);
	return 0;
}

duk_ret_t CDuckJs::NativeRenderSetColorU32(duk_context *ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 1, "Wrong argument count");
	int x = duk_to_int(ctx, 0);

	float aColor[4];
	aColor[0] = (x & 0xFF) / 255.f;
	aColor[1] = ((x >> 8) & 0xFF) / 255.f;
	aColor[2] = ((x >> 16) & 0xFF) / 255.f;
	aColor[3] = ((x >> 24) & 0xFF) / 255.f;

	This()->m_Bridge.QueueSetColor(aColor);
	return 0;
}

duk_ret_t CDuckJs::NativeRenderSetColorF4(duk_context *ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 4, "Wrong argument count");

	float aColor[4];
	aColor[0] = duk_to_number(ctx, 0);
	aColor[1] = duk_to_number(ctx, 1);
	aColor[2] = duk_to_number(ctx, 2);
	aColor[3] = duk_to_number(ctx, 3);

	This()->m_Bridge.QueueSetColor(aColor);
	return 0;
}

duk_ret_t CDuckJs::NativeRenderSetTexture(duk_context *ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 1, "Wrong argument count");
	This()->m_Bridge.QueueSetTexture((int)duk_to_int(ctx, 0));
	return 0;
}

duk_ret_t CDuckJs::NativeRenderSetQuadSubSet(duk_context* ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 4, "Wrong argument count");

	float aSubSet[4];
	aSubSet[0] = duk_to_number(ctx, 0);
	aSubSet[1] = duk_to_number(ctx, 1);
	aSubSet[2] = duk_to_number(ctx, 2);
	aSubSet[3] = duk_to_number(ctx, 3);

	This()->m_Bridge.QueueSetQuadSubSet(aSubSet);
	return 0;
}

duk_ret_t CDuckJs::NativeRenderSetQuadRotation(duk_context* ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 1, "Wrong argument count");

	float Angle = duk_to_number(ctx, 0);
	This()->m_Bridge.QueueSetQuadRotation(Angle);
	return 0;
}

duk_ret_t CDuckJs::NativeRenderSetTeeSkin(duk_context* ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 1, "Wrong argument count");

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
				duk_pop(ctx);
			}
		}
		duk_pop(ctx);
	}

	if(duk_get_prop_string(ctx, 0, "colors"))
	{
		for(int i = 0; i < NUM_SKINPARTS; i++)
		{
			// TODO: make colors simple arrays? instead of rgba objects
			if(duk_get_prop_index(ctx, -1, i))
			{
				duk_to_object(ctx, -1);
				if(duk_get_prop_string(ctx, -1, "r"))
				{
					SkinInfo.m_aColors[i][0] = (float)duk_to_number(ctx, -1);
					duk_pop(ctx);
				}
				if(duk_get_prop_string(ctx, -1, "g"))
				{
					SkinInfo.m_aColors[i][1] = (float)duk_to_number(ctx, -1);
					duk_pop(ctx);
				}
				if(duk_get_prop_string(ctx, -1, "b"))
				{
					SkinInfo.m_aColors[i][2] = (float)duk_to_number(ctx, -1);
					duk_pop(ctx);
				}
				if(duk_get_prop_string(ctx, -1, "a"))
				{
					SkinInfo.m_aColors[i][3] = (float)duk_to_number(ctx, -1);
					duk_pop(ctx);
				}
				duk_pop(ctx);
			}
		}
		duk_pop(ctx);
	}

	This()->m_Bridge.QueueSetTeeSkin(SkinInfo);
	return 0;
}

duk_ret_t CDuckJs::NativeRenderSetFreeform(duk_context *ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 1, "Wrong argument count");

	IGraphics::CFreeformItem* pFreeformBuffer = (IGraphics::CFreeformItem*)This()->m_Bridge.m_FrameAllocator.Alloc(sizeof(IGraphics::CFreeformItem) * CDuckBridge::CRenderSpace::FREEFORM_MAX_COUNT );
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
			duk_pop(ctx);

			if(VertCount >= FfFloatCount)
			{
				VertCount = 0;
				pFreeformBuffer[FreeformCount++] = *(IGraphics::CFreeformItem*)CurrentFreeform;
			}
		}
	}

	if(VertCount > 0)
	{
		mem_zero(CurrentFreeform+VertCount, sizeof(CurrentFreeform)-sizeof(float)*VertCount);
		pFreeformBuffer[FreeformCount++] = *(IGraphics::CFreeformItem*)CurrentFreeform;
	}

	This()->m_Bridge.QueueSetFreeform(pFreeformBuffer, FreeformCount);
	return 0;
}

duk_ret_t CDuckJs::NativeRenderSetDrawSpace(duk_context *ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 1, "Wrong argument count");

	int ds = duk_to_int(ctx, 0);
	dbg_assert(ds >= 0 && ds < CDuckBridge::DrawSpace::_COUNT, "Draw space undefined");

	This()->m_Bridge.m_CurrentDrawSpace = ds;

	return 0;
}

duk_ret_t CDuckJs::NativeRenderDrawTeeBodyAndFeet(duk_context *ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 1, "Wrong argument count");

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

	if(duk_get_prop_string(ctx, 0, "size"))
	{
		Size = (float)duk_to_number(ctx, -1);
		duk_pop(ctx);
	}
	if(duk_get_prop_string(ctx, 0, "angle"))
	{
		Angle = (float)duk_to_number(ctx, -1);
		duk_pop(ctx);
	}
	if(duk_get_prop_string(ctx, 0, "pos_x"))
	{
		PosX = (float)duk_to_number(ctx, -1);
		duk_pop(ctx);
	}
	if(duk_get_prop_string(ctx, 0, "pos_y"))
	{
		PosY = (float)duk_to_number(ctx, -1);
		duk_pop(ctx);
	}
	if(duk_get_prop_string(ctx, 0, "is_walking"))
	{
		IsWalking = (bool)duk_to_boolean(ctx, -1);
		duk_pop(ctx);
	}
	if(duk_get_prop_string(ctx, 0, "is_grounded"))
	{
		IsGrounded = (bool)duk_to_boolean(ctx, -1);
		duk_pop(ctx);
	}
	if(duk_get_prop_string(ctx, 0, "got_air_jump"))
	{
		GotAirJump = (bool)duk_to_boolean(ctx, -1);
		duk_pop(ctx);
	}
	if(duk_get_prop_string(ctx, 0, "emote"))
	{
		Emote = (int)duk_to_int(ctx, -1);
		duk_pop(ctx);
	}

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
	This()->m_Bridge.QueueDrawTeeBodyAndFeet(TeeDrawInfo);
	return 0;
}

duk_ret_t CDuckJs::NativeRenderDrawTeeHand(duk_context* ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 1, "Wrong argument count");

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
	bool IsWalking = false;
	bool IsGrounded = true;
	bool GotAirJump = true;
	int Emote = 0;

	if(duk_get_prop_string(ctx, 0, "size"))
	{
		Size = (float)duk_to_number(ctx, -1);
		duk_pop(ctx);
	}
	if(duk_get_prop_string(ctx, 0, "angle_dir"))
	{
		AngleDir = (float)duk_to_number(ctx, -1);
		duk_pop(ctx);
	}
	if(duk_get_prop_string(ctx, 0, "angle_off"))
	{
		AngleOff = (float)duk_to_number(ctx, -1);
		duk_pop(ctx);
	}
	if(duk_get_prop_string(ctx, 0, "pos_x"))
	{
		PosX = (float)duk_to_number(ctx, -1);
		duk_pop(ctx);
	}
	if(duk_get_prop_string(ctx, 0, "pos_y"))
	{
		PosY = (float)duk_to_number(ctx, -1);
		duk_pop(ctx);
	}
	if(duk_get_prop_string(ctx, 0, "off_x"))
	{
		OffX = (float)duk_to_number(ctx, -1);
		duk_pop(ctx);
	}
	if(duk_get_prop_string(ctx, 0, "off_y"))
	{
		OffY = (float)duk_to_number(ctx, -1);
		duk_pop(ctx);
	}

	CDuckBridge::CTeeDrawHand TeeHandInfo;
	TeeHandInfo.m_Size = Size;
	TeeHandInfo.m_AngleDir = AngleDir;
	TeeHandInfo.m_AngleOff = AngleOff;
	TeeHandInfo.m_Pos[0] = PosX;
	TeeHandInfo.m_Pos[1] = PosY;
	TeeHandInfo.m_Offset[0] = OffX;
	TeeHandInfo.m_Offset[1] = OffY;
	//dbg_msg("duk", "NativeRenderDrawTeeHand( hand = { size: %g, angle_dir: %g, angle_off: %g, pos_x: %g, pos_y: %g, off_x: %g, off_y: %g }", Size, AngleDir, AngleOff, PosX, PosY, OffX, OffY);
	This()->m_Bridge.QueueDrawTeeHand(TeeHandInfo);
	return 0;
}

duk_ret_t CDuckJs::NativeRenderDrawFreeform(duk_context *ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 1, "Wrong argument count");

	vec2 Pos;
	if(duk_get_prop_index(ctx, 0, 0))
	{
		Pos.x = duk_to_number(ctx, -1);
		duk_pop(ctx);
	}
	if(duk_get_prop_index(ctx, 0, 1))
	{
		Pos.y = duk_to_number(ctx, -1);
		duk_pop(ctx);
	}

	This()->m_Bridge.QueueDrawFreeform(Pos);
	return 0;
}

duk_ret_t CDuckJs::NativeGetBaseTexture(duk_context* ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 1, "Wrong argument count");

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

duk_ret_t CDuckJs::NativeGetSpriteSubSet(duk_context* ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 1, "Wrong argument count");

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

duk_ret_t CDuckJs::NativeGetSpriteScale(duk_context* ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 1, "Wrong argument count");

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

duk_ret_t CDuckJs::NativeGetWeaponSpec(duk_context* ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 1, "Wrong argument count");

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

duk_ret_t CDuckJs::NativeGetModTexture(duk_context *ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 1, "Wrong argument count");

	const char* pTextureName = duk_get_string(ctx, 0);
	IGraphics::CTextureHandle Handle = This()->m_Bridge.GetTexture(pTextureName);
	duk_push_int(ctx, *(int*)&Handle);
	return 1;
}

duk_ret_t CDuckJs::NativeGetClientSkinInfo(duk_context* ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 1, "Wrong argument count");

	const int ClientID = clamp((int)duk_to_int(ctx, 0), 0, MAX_CLIENTS-1);
	if(!This()->m_pClient->m_aClients[ClientID].m_Active)
	{
		duk_push_null(ctx);
		return 1; // client not active, return null
	}

	const CTeeRenderInfo& RenderInfo = This()->m_pClient->m_aClients[ClientID].m_RenderInfo;

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

duk_ret_t CDuckJs::NativeGetClientCharacterCores(duk_context* ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 0, "Wrong argument count");

	IClient* pClient = This()->Client();
	CGameClient* pGameClient = This()->m_pClient;

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
				IntraTick = This()->Client()->PredIntraGameTick();
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

			duk_idx_t PosObjIdx = duk_push_object(ctx);
			DukSetIntProp(ctx, PosObjIdx, "tick", Cur.m_Tick);
			DukSetFloatProp(ctx, PosObjIdx, "vel_x", Vel.x);
			DukSetFloatProp(ctx, PosObjIdx, "vel_y", Vel.y);
			DukSetFloatProp(ctx, PosObjIdx, "angle", Angle);
			DukSetIntProp(ctx, PosObjIdx, "direction", Cur.m_Direction);
			DukSetIntProp(ctx, PosObjIdx, "jumped", Cur.m_Jumped);
			DukSetIntProp(ctx, PosObjIdx, "hooked_player", Cur.m_HookedPlayer);
			DukSetIntProp(ctx, PosObjIdx, "hook_state", Cur.m_HookState);
			DukSetIntProp(ctx, PosObjIdx, "hook_tick", Cur.m_HookTick);
			DukSetFloatProp(ctx, PosObjIdx, "hook_x", HookPos.x);
			DukSetFloatProp(ctx, PosObjIdx, "hook_y", HookPos.y);
			DukSetFloatProp(ctx, PosObjIdx, "hook_dx", Cur.m_HookDx/256.f);
			DukSetFloatProp(ctx, PosObjIdx, "hook_dy", Cur.m_HookDy/256.f);
			DukSetFloatProp(ctx, PosObjIdx, "pos_x", Position.x);
			DukSetFloatProp(ctx, PosObjIdx, "pos_y", Position.y);
		}
		else
			duk_push_null(ctx);
		duk_put_prop_index(ctx, ArrayIdx, i);
	}

	return 1;
}

duk_ret_t CDuckJs::NativeGetStandardSkinInfo(duk_context* ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 0, "Wrong argument count");

	// TODO: make an actual standard skin info
	const CTeeRenderInfo& RenderInfo = This()->m_pClient->m_aClients[This()->m_pClient->m_LocalClientID].m_RenderInfo;

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

duk_ret_t CDuckJs::NativeGetSkinPartTexture(duk_context* ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 2, "Wrong argument count");

	// TODO: bound check
	int Part = clamp((int)duk_to_int(ctx, 0), 0, NUM_SKINPARTS-1);
	const char* PartName = duk_to_string(ctx, 1);

	int SkinPartID = This()->m_pClient->m_pSkins->FindSkinPart(Part, PartName, true);
	if(SkinPartID < 0)
	{
		duk_push_null(ctx);
		return 1;
	}

	duk_idx_t ArrayIdx = duk_push_array(ctx);

	duk_push_int(ctx, *(int*)&This()->m_pClient->m_pSkins->GetSkinPart(Part, SkinPartID)->m_OrgTexture);
	duk_put_prop_index(ctx, ArrayIdx, 0);
	duk_push_int(ctx, *(int*)&This()->m_pClient->m_pSkins->GetSkinPart(Part, SkinPartID)->m_ColorTexture);
	duk_put_prop_index(ctx, ArrayIdx, 1);
	return 1;
}

duk_ret_t CDuckJs::NativeGetCursorPosition(duk_context *ctx)
{
	// Get position in world space

	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 0, "Wrong argument count");

	const vec2 CursorPos = This()->m_pClient->m_pControls->m_TargetPos;

	This()->PushObject();
	DukSetFloatProp(ctx, 0, "x", CursorPos.x);
	DukSetFloatProp(ctx, 0, "y", CursorPos.y);
	return  1;
}

duk_ret_t CDuckJs::NativeMapSetTileCollisionFlags(duk_context *ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 3, "Wrong argument count");

	// TODO: bound check
	int Tx = duk_to_int(ctx, 0);
	int Ty = duk_to_int(ctx, 1);
	int Flags = duk_to_int(ctx, 2);

	This()->m_Bridge.m_Collision.SetTileCollisionFlags(Tx, Ty, Flags);

	return 0;
}

duk_ret_t CDuckJs::NativeDirectionFromAngle(duk_context* ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 1, "Wrong argument count");

	float Angle = (float)duk_to_number(ctx, 0);
	vec2 Dir = direction(Angle);

	int DirObjIdx = duk_push_object(ctx);
		duk_push_number(ctx, Dir.x);
		duk_put_prop_string(ctx, DirObjIdx, "x");
		duk_push_number(ctx, Dir.y);
		duk_put_prop_string(ctx, DirObjIdx, "y");
		return 1;
}

duk_ret_t CDuckJs::NativeCollisionSetStaticBlock(duk_context* ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 2, "Wrong argument count");

	int BlockId = duk_to_int(ctx, 0);

	CDuckCollision::CStaticBlock SolidBlock;
	SolidBlock.m_Flags = -1;

	if(duk_get_prop_string(ctx, 1, "flags"))
	{
		SolidBlock.m_Flags = (int)duk_to_int(ctx, -1);
		duk_pop(ctx);
	}
	if(duk_get_prop_string(ctx, 1, "pos_x"))
	{
		SolidBlock.m_Pos.x = (float)duk_to_number(ctx, -1);
		duk_pop(ctx);
	}
	if(duk_get_prop_string(ctx, 1, "pos_y"))
	{
		SolidBlock.m_Pos.y = (float)duk_to_number(ctx, -1);
		duk_pop(ctx);
	}
	if(duk_get_prop_string(ctx, 1, "width"))
	{
		SolidBlock.m_Size.x = (float)duk_to_number(ctx, -1);
		duk_pop(ctx);
	}
	if(duk_get_prop_string(ctx, 1, "height"))
	{
		SolidBlock.m_Size.y = (float)duk_to_number(ctx, -1);
		duk_pop(ctx);
	}

	if(BlockId >= 0)
		This()->m_Bridge.m_Collision.SetStaticBlock(BlockId, SolidBlock);
	return 0;
}

duk_ret_t CDuckJs::NativeCollisionClearStaticBlock(duk_context* ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 1, "Wrong argument count");

	int BlockId = duk_to_int(ctx, 0);
	if(BlockId >= 0)
		This()->m_Bridge.m_Collision.ClearStaticBlock(BlockId);
	return 0;
}

duk_ret_t CDuckJs::NativeCollisionSetDynamicDisk(duk_context *ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 2, "Wrong argument count");

	int DiskId = duk_to_int(ctx, 0);

	CDuckCollision::CDynamicDisk Disk;
	Disk.m_Flags = -1;


	if(duk_get_prop_string(ctx, 1, "flags"))
	{
		Disk.m_Flags = (int)duk_to_int(ctx, -1);
		duk_pop(ctx);
	}

	DukGetFloatProp(ctx, 1, "pos_x", &Disk.m_Pos.x);
	DukGetFloatProp(ctx, 1, "pos_y", &Disk.m_Pos.y);
	DukGetFloatProp(ctx, 1, "vel_x", &Disk.m_Vel.x);
	DukGetFloatProp(ctx, 1, "vel_y", &Disk.m_Vel.y);
	DukGetFloatProp(ctx, 1, "radius", &Disk.m_Radius);
	DukGetFloatProp(ctx, 1, "hook_force", &Disk.m_HookForce);

	if(DiskId >= 0)
		This()->m_Bridge.m_Collision.SetDynamicDisk(DiskId, Disk);
	return 0;
}

duk_ret_t CDuckJs::NativeCollisionClearDynamicDisk(duk_context *ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 1, "Wrong argument count");

	int DiskId = duk_to_int(ctx, 0);
	if(DiskId >= 0)
		This()->m_Bridge.m_Collision.ClearDynamicDisk(DiskId);
	return 0;
}

duk_ret_t CDuckJs::NativeCollisionGetPredictedDynamicDisks(duk_context *ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 0, "Wrong argument count");

	const CDuckCollision::CDynamicDisk* pDisks = This()->m_Bridge.m_Collision.m_aDynamicDisks.base_ptr();
	const int DiskCount = This()->m_Bridge.m_Collision.m_aDynamicDisks.size();

	duk_idx_t ArrayIdx = duk_push_array(ctx);

	for(int d = 0; d < DiskCount; d++)
	{
		const CDuckCollision::CDynamicDisk& Disk = pDisks[d];
		duk_idx_t ObjIdx = duk_push_object(ctx);
		DukSetIntProp(ctx, ObjIdx, "id", Disk.m_FetchID);
		DukSetIntProp(ctx, ObjIdx, "flags", Disk.m_Flags);
		DukSetFloatProp(ctx, ObjIdx, "pos_x", Disk.m_Pos.x);
		DukSetFloatProp(ctx, ObjIdx, "pos_y", Disk.m_Pos.y);
		DukSetFloatProp(ctx, ObjIdx, "vel_x", Disk.m_Vel.x);
		DukSetFloatProp(ctx, ObjIdx, "vel_y", Disk.m_Vel.y);
		DukSetFloatProp(ctx, ObjIdx, "radius", Disk.m_Radius);
		DukSetFloatProp(ctx, ObjIdx, "hook_force", Disk.m_HookForce);

		duk_put_prop_index(ctx, ArrayIdx, d);
	}

	return 1;
}

duk_ret_t CDuckJs::NativeSetHudPartsShown(duk_context *ctx)
{
	/*
	var shown = {
		health: 1,
		armor: 1,
		ammo: 1,
		time: 1,
		killfeed: 1,
		score: 1,
		chat: 1,
	}
	*/

	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 1, "Wrong argument count");

	CDuckBridge::CHudPartsShown hps;

	DukGetIntProp(ctx, 0, "health", &hps.m_Health);
	DukGetIntProp(ctx, 0, "armor", &hps.m_Armor);
	DukGetIntProp(ctx, 0, "ammo", &hps.m_Ammo);
	DukGetIntProp(ctx, 0, "time", &hps.m_Time);
	DukGetIntProp(ctx, 0, "killfeed", &hps.m_KillFeed);
	DukGetIntProp(ctx, 0, "score", &hps.m_Score);
	DukGetIntProp(ctx, 0, "chat", &hps.m_Chat);

	This()->m_Bridge.SetHudPartsShown(hps);

	return 0;
}

duk_ret_t CDuckJs::NativeCreatePacket(duk_context *ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 1, "Wrong argument count");

	/*
	var info = {
		netid: 0x1,
		force_send_now: 0
	}
	*/

	int NetID = -1;
	int IsVital = -1;
	int SendNow = -1;
	DukGetIntProp(ctx, 0, "netid", &NetID);
	DukGetIntProp(ctx, 0, "force_send_now", &SendNow);

	if(NetID <= 0) {
		dbg_msg("duck", "WARNING: TwCreatePacket() >> NetID is invalid (%d)", NetID);
		return 0;
	}

	int Flags = 0;
	Flags |= MSGFLAG_VITAL;
	if(SendNow > 0) Flags |= MSGFLAG_FLUSH;
	This()->m_Bridge.PacketCreate(NetID, Flags);
	return 0;
}

duk_ret_t CDuckJs::NativePacketAddInt(duk_context *ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 1, "Wrong argument count");

	int i = duk_to_int(ctx, 0);
	This()->m_Bridge.PacketPackInt(i);
	return 0;
}

duk_ret_t CDuckJs::NativePacketAddFloat(duk_context *ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 1, "Wrong argument count");

	double f = duk_to_number(ctx, 0);
	This()->m_Bridge.PacketPackFloat((float)f);
	return 0;
}

duk_ret_t CDuckJs::NativePacketAddString(duk_context *ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 2, "Wrong argument count");

	const char* pStr = duk_to_string(ctx, 0);
	int SizeLimit = duk_to_int(ctx, 1);
	This()->m_Bridge.PacketPackString(pStr, SizeLimit);
	return 0;
}

duk_ret_t CDuckJs::NativeSendPacket(duk_context *ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 0, "Wrong argument count");

	This()->m_Bridge.SendPacket();
	return 0;
}

template<typename IntT>
duk_ret_t CDuckJs::NativeUnpackInteger(duk_context *ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 1, "Wrong argument count");

	// get cursor, raw buffer
	duk_get_prop_string(ctx, 0, "cursor");
	int Cursor = (int)duk_to_int(ctx, -1);
	duk_pop(ctx);

	duk_get_prop_string(ctx, 0, "raw");
	duk_size_t RawBufferSize;
	const u8* pRawBuffer = (u8*)duk_get_buffer(ctx, -1, &RawBufferSize);
	duk_pop(ctx);

	/*dbg_msg("duck", "netObj.cursor = %d", Cursor);
	dbg_msg("duck", "netObj.raw = %llx", pRawBuffer);
	dbg_msg("duck", "netObj.raw.length = %d", RawBufferSize);*/

	dbg_assert(Cursor + sizeof(IntT) <= RawBufferSize, "unpack: buffer overflow");

	IntT OutInt;
	mem_copy(&OutInt, pRawBuffer + Cursor, sizeof(IntT));
	Cursor += sizeof(IntT);

	// set new cursor
	duk_push_int(ctx, Cursor);
	duk_put_prop_string(ctx, 0, "cursor");

	// return int
	duk_push_int(ctx, (int)OutInt);
	return 1;
}

duk_ret_t CDuckJs::NativeUnpackFloat(duk_context *ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 1, "Wrong argument count");

	// get cursor, raw buffer
	duk_get_prop_string(ctx, 0, "cursor");
	int Cursor = (int)duk_to_int(ctx, -1);
	duk_pop(ctx);

	duk_get_prop_string(ctx, 0, "raw");
	duk_size_t RawBufferSize;
	const u8* pRawBuffer = (u8*)duk_get_buffer(ctx, -1, &RawBufferSize);
	duk_pop(ctx);

	dbg_assert(Cursor + sizeof(float) <= RawBufferSize, "unpack: buffer overflow");

	float OutFloat;
	mem_copy(&OutFloat, pRawBuffer + Cursor, sizeof(float));
	Cursor += sizeof(float);

	// set new cursor
	duk_push_int(ctx, Cursor);
	duk_put_prop_string(ctx, 0, "cursor");

	// return float
	duk_push_number(ctx, OutFloat);
	return 1;
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

void CDuckJs::ObjectSetMember(const char* MemberName)
{
	duk_put_prop_string(Ctx(), m_CurrentPushedObjID, MemberName);
}

bool CDuckJs::IsModAlreadyInstalled(const SHA256_DIGEST* pModSha256)
{
	char aSha256Str[SHA256_MAXSTRSIZE];
	sha256_str(*pModSha256, aSha256Str, sizeof(aSha256Str));

	char aModMainJsPath[512];
	str_copy(aModMainJsPath, "mods/", sizeof(aModMainJsPath));
	str_append(aModMainJsPath, aSha256Str, sizeof(aModMainJsPath));
	str_append(aModMainJsPath, "/main.js", sizeof(aModMainJsPath));

	IOHANDLE ModMainJs = Storage()->OpenFile(aModMainJsPath, IOFLAG_READ, IStorage::TYPE_SAVE);
	if(ModMainJs)
	{
		io_close(ModMainJs);
		dbg_msg("duck", "mod is already installed on disk");
		return true;
	}
	return false;
}

bool CDuckJs::ExtractAndInstallModZipBuffer(const CGrowBuffer* pHttpZipData, const SHA256_DIGEST* pModSha256)
{
	dbg_msg("unzip", "EXTRACTING AND INSTALLING MOD");

	char aUserModsPath[512];
	Storage()->GetCompletePath(IStorage::TYPE_SAVE, "mods", aUserModsPath, sizeof(aUserModsPath));
	fs_makedir(aUserModsPath); // Teeworlds/mods (user storage)

	// TODO: reduce folder hash string length?
	SHA256_DIGEST Sha256 = sha256(pHttpZipData->m_pData, pHttpZipData->m_Size);
	char aSha256Str[SHA256_MAXSTRSIZE];
	sha256_str(Sha256, aSha256Str, sizeof(aSha256Str));

	if(Sha256 != *pModSha256)
	{
		dbg_msg("duck", "mod url sha256 and server sent mod sha256 mismatch, received sha256=%s", aSha256Str);
		// TODO: display error message
		return false;
	}

	char aModRootPath[512];
	str_copy(aModRootPath, aUserModsPath, sizeof(aModRootPath));
	str_append(aModRootPath, "/", sizeof(aModRootPath));
	str_append(aModRootPath, aSha256Str, sizeof(aModRootPath));


	// FIXME: remove this
	/*
	str_append(aUserModsPath, "/temp.zip", sizeof(aUserModsPath));
	IOHANDLE File = io_open(aUserMoodsPath, IOFLAG_WRITE);
	io_write(File, pHttpZipData->m_pData, pHttpZipData->m_Size);
	io_close(File);

	zip *pZipArchive = zip_open(aUserMoodsPath, 0, &Error);
	if(pZipArchive == NULL)
	{
		char aErrorBuff[512];
		zip_error_to_str(aErrorBuff, sizeof(aErrorBuff), Error, errno);
		dbg_msg("unzip", "Error opening '%s' [%s]", aUserMoodsPath, aErrorBuff);
		return false;
	}*/

	zip_error_t ZipError;
	zip_error_init(&ZipError);
	zip_source_t* pZipSrc = zip_source_buffer_create(pHttpZipData->m_pData, pHttpZipData->m_Size, 1, &ZipError);
	if(!pZipSrc)
	{
		dbg_msg("unzip", "Error creating zip source [%s]", zip_error_strerror(&ZipError));
		zip_error_fini(&ZipError);
		return false;
	}

	dbg_msg("unzip", "OPENING zip source %s", aSha256Str);

	// int Error = 0;
	zip *pZipArchive = zip_open_from_source(pZipSrc, 0, &ZipError);
	if(pZipArchive == NULL)
	{
		dbg_msg("unzip", "Error opening source [%s]", zip_error_strerror(&ZipError));
		zip_source_free(pZipSrc);
		zip_error_fini(&ZipError);
		return false;
	}
	zip_error_fini(&ZipError);

	dbg_msg("unzip", "CREATE directory '%s'", aModRootPath);
	if(fs_makedir(aModRootPath) != 0)
	{
		dbg_msg("unzip", "Failed to create directory '%s'", aModRootPath);
		return false;
	}

	const int EntryCount = zip_get_num_entries(pZipArchive, 0);

	// find required files
	const char* aRequiredFiles[] = {
		"main.js",
		"mod_info.json"
	};
	const int RequiredFilesCount = sizeof(aRequiredFiles)/sizeof(aRequiredFiles[0]);

	int FoundRequiredFilesCount = 0;
	for(int i = 0; i < EntryCount && FoundRequiredFilesCount < RequiredFilesCount; i++)
	{
		zip_stat_t EntryStat;
		if(zip_stat_index(pZipArchive, i, 0, &EntryStat) != 0)
			continue;
    
		const int NameLen = str_length(EntryStat.name);
		if(EntryStat.name[NameLen-1] != '/')
		{
			for(int r = 0; r < RequiredFilesCount; r++)
			{
				 // TODO: can 2 files have the same name?
				if(str_comp(EntryStat.name, aRequiredFiles[r]) == 0)
					FoundRequiredFilesCount++;
			}
		}
	}

	if(FoundRequiredFilesCount != RequiredFilesCount)
	{
		dbg_msg("duck", "mod is missing a required file, required files are: ");
		for(int r = 0; r < RequiredFilesCount; r++)
		{
			dbg_msg("duck", "    - %s", aRequiredFiles[r]);
		}
		return false;
	}

	// walk zip file tree and extract
	for(int i = 0; i < EntryCount; i++)
	{
		zip_stat_t EntryStat;
		if(zip_stat_index(pZipArchive, i, 0, &EntryStat) != 0)
			continue;

		// TODO: remove
		dbg_msg("unzip", "- name: %s, size: %llu, mtime: [%u]", EntryStat.name, EntryStat.size, (unsigned int)EntryStat.mtime);

		// TODO: sanitize folder name
		const int NameLen = str_length(EntryStat.name);
		if(EntryStat.name[NameLen-1] == '/')
		{
			// create sub directory
			char aSubFolder[512];
			str_copy(aSubFolder, aModRootPath, sizeof(aSubFolder));
			str_append(aSubFolder, "/", sizeof(aSubFolder));
			str_append(aSubFolder, EntryStat.name, sizeof(aSubFolder));

			dbg_msg("unzip", "CREATE SUB directory '%s'", aSubFolder);
			if(fs_makedir(aSubFolder) != 0)
			{
				dbg_msg("unzip", "Failed to create directory '%s'", aSubFolder);
				return false;
			}
		}
		else
		{
			// filter by extension
			if(!(str_endswith(EntryStat.name, ".js") || str_endswith(EntryStat.name, ".json") || str_endswith(EntryStat.name, ".png") || str_endswith(EntryStat.name, ".wv")))
				continue;

			// TODO: verify file type? Might be very expensive to do so.
			zip_file_t* pFileZip = zip_fopen_index(pZipArchive, i, 0);
			if(!pFileZip)
			{
				dbg_msg("unzip", "Error reading file '%s'", EntryStat.name);
				return false;
			}

			// create file on disk
			char aFilePath[256];
			str_copy(aFilePath, aModRootPath, sizeof(aFilePath));
			str_append(aFilePath, "/", sizeof(aFilePath));
			str_append(aFilePath, EntryStat.name, sizeof(aFilePath));

			IOHANDLE FileExtracted = io_open(aFilePath, IOFLAG_WRITE);
			if(!FileExtracted)
			{
				dbg_msg("unzip", "Error creating file '%s'", aFilePath);
				return false;
			}

			// read zip file data and write to file on disk
			char aReadBuff[1024];
			unsigned ReadCurrentSize = 0;
			while(ReadCurrentSize != EntryStat.size)
			{
				const int ReadLen = zip_fread(pFileZip, aReadBuff, sizeof(aReadBuff));
				if(ReadLen < 0)
				{
					dbg_msg("unzip", "Error reading file '%s'", EntryStat.name);
					return false;
				}
				io_write(FileExtracted, aReadBuff, ReadLen);
				ReadCurrentSize += ReadLen;
			}

			io_close(FileExtracted);
			zip_fclose(pFileZip);
		}
	}

	zip_source_close(pZipSrc);
	// NOTE: no need to call zip_source_free(pZipSrc), HttpBuffer::Release() already frees up the buffer

	//zip_close(pZipArchive);

#if 0
	unzFile ZipFile = unzOpen64(aPath);
	unz_global_info GlobalInfo;
	int r = unzGetGlobalInfo(ZipFile, &GlobalInfo);
	if(r != UNZ_OK)
	{
		dbg_msg("unzip", "could not read file global info (%d)", r);
		unzClose(ZipFile);
		dbg_break();
		return false;
	}

	for(int i = 0; i < GlobalInfo.number_entry; i++)
	{
		// Get info about current file.
		unz_file_info file_info;
		char filename[256];
		if(unzGetCurrentFileInfo(ZipFile, &file_info, filename, sizeof(filename), NULL, 0, NULL, 0) != UNZ_OK)
		{
			dbg_msg("unzip", "could not read file info");
			unzClose(ZipFile);
			return false;
		}

		dbg_msg("unzip", "FILE_ENTRY %s", filename);

		/*// Check if this entry is a directory or file.
		const size_t filename_length = str_length(filename);
		if(filename[ filename_length-1 ] == '/')
		{
			// Entry is a directory, so create it.
			printf("dir:%s\n", filename);
			//mkdir(filename);
		}
		else
		{
			// Entry is a file, so extract it.
			printf("file:%s\n", filename);
			if(unzOpenCurrentFile(ZipFile) != UNZ_OK)
			{
				dbg_msg("unzip", "could not open file");
				unzClose(ZipFile);
				return false;
			}

			// Open a file to write out the data.
			FILE *out = fopen(filename, "wb");
			if(out == NULL)
			{
				dbg_msg("unzip", "could not open destination file");
				unzCloseCurrentFile(ZipFile);
				unzClose(ZipFile);
				return false;
			}

			int error = UNZ_OK;
			do
			{
			error = unzReadCurrentFile(zipfile, read_buffer, READ_SIZE);
			if(error < 0)
			{
			printf("error %d\n", error);
			unzCloseCurrentFile(zipfile);
			unzClose(zipfile);
			return -1;
			}

			// Write data to file.
			if(error > 0)
			{
			fwrite(read_buffer, error, 1, out); // You should check return of fwrite...
			}
			} while (error > 0);

			fclose(out);
		}*/

		unzCloseCurrentFile(ZipFile);

		// Go the the next entry listed in the zip file.
		if((i+1) < GlobalInfo.number_entry)
		{
			if(unzGoToNextFile(ZipFile) != UNZ_OK)
			{
				dbg_msg("unzip", "cound not read next file");
				unzClose(ZipFile);
				return false;
			}
		}
	}

	unzClose(ZipFile);
#endif

	return true;
}

bool CDuckJs::ExtractAndInstallModCompressedBuffer(const void* pCompBuff, int CompBuffSize, const SHA256_DIGEST* pModSha256)
{
	const bool IsConfigDebug = g_Config.m_Debug;

	if(IsConfigDebug)
		dbg_msg("unzip", "EXTRACTING AND INSTALLING *COMRPESSED* MOD");

	char aUserModsPath[512];
	Storage()->GetCompletePath(IStorage::TYPE_SAVE, "mods", aUserModsPath, sizeof(aUserModsPath));
	fs_makedir(aUserModsPath); // Teeworlds/mods (user storage)

	// TODO: reduce folder hash string length?
	SHA256_DIGEST Sha256 = sha256(pCompBuff, CompBuffSize);
	char aSha256Str[SHA256_MAXSTRSIZE];
	sha256_str(Sha256, aSha256Str, sizeof(aSha256Str));

	if(Sha256 != *pModSha256)
	{
		dbg_msg("duck", "mod url sha256 and server sent mod sha256 mismatch, received sha256=%s", aSha256Str);
		// TODO: display error message
		return false;
	}

	// mod folder where we're going to extract the files
	char aModRootPath[512];
	str_copy(aModRootPath, aUserModsPath, sizeof(aModRootPath));
	str_append(aModRootPath, "/", sizeof(aModRootPath));
	str_append(aModRootPath, aSha256Str, sizeof(aModRootPath));

	// uncompress
	CGrowBuffer FilePackBuff;
	FilePackBuff.Grow(CompBuffSize * 3);

	uLongf DestSize = FilePackBuff.m_Capacity;
	int UncompRet = uncompress((Bytef*)FilePackBuff.m_pData, &DestSize, (const Bytef*)pCompBuff, CompBuffSize);
	FilePackBuff.m_Size = DestSize;

	int GrowAttempts = 4;
	while(UncompRet == Z_BUF_ERROR && GrowAttempts--)
	{
		FilePackBuff.Grow(FilePackBuff.m_Capacity * 2);
		DestSize = FilePackBuff.m_Capacity;
		UncompRet = uncompress((Bytef*)FilePackBuff.m_pData, &DestSize, (const Bytef*)pCompBuff, CompBuffSize);
		FilePackBuff.m_Size = DestSize;
	}

	if(UncompRet != Z_OK)
	{
		switch(UncompRet)
		{
			case Z_MEM_ERROR:
				dbg_msg("duck", "DecompressMod: Error, not enough memory");
				break;
			case Z_BUF_ERROR:
				dbg_msg("duck", "DecompressMod: Error, not enough room in the output buffer");
				break;
			case Z_DATA_ERROR:
				dbg_msg("duck", "DecompressMod: Error, data incomplete or corrupted");
				break;
			default:
				dbg_break(); // should never be reached
		}
		return false;
	}

	// read decompressed pack file

	// header
	const char* pFilePack = FilePackBuff.m_pData;
	const int FilePackSize = FilePackBuff.m_Size;
	const char* const pFilePackEnd = pFilePack + FilePackSize;

	if(str_comp_num(pFilePack, "DUCK", 4) != 0)
	{
		dbg_msg("duck", "DecompressMod: Error, invalid pack file");
		return false;
	}

	pFilePack += 4;

	// find required files
	const char* aRequiredFiles[] = {
		"main.js",
		"mod_info.json"
	};
	const int RequiredFilesCount = sizeof(aRequiredFiles)/sizeof(aRequiredFiles[0]);
	int FoundRequiredFilesCount = 0;

	// TODO: use packer
	const char* pCursor = pFilePack;
	while(pCursor < pFilePackEnd)
	{
		const int FilePathLen = *(int*)pCursor;
		pCursor += 4;
		const char* pFilePath = pCursor;
		pCursor += FilePathLen;
		const int FileSize = *(int*)pCursor;
		pCursor += 4;
		const char* pFileData = pCursor;
		pCursor += FileSize;

		char aFilePath[512];
		dbg_assert(FilePathLen < sizeof(aFilePath)-1, "FilePathLen too large");
		mem_move(aFilePath, pFilePath, FilePathLen);
		aFilePath[FilePathLen] = 0;

		if(IsConfigDebug)
			dbg_msg("duck", "file path=%s size=%d", aFilePath, FileSize);

		for(int r = 0; r < RequiredFilesCount; r++)
		{
			// TODO: can 2 files have the same name?
			// TODO: not very efficient, but oh well
			const char* pFind = str_find(aFilePath, aRequiredFiles[r]);
			if(pFind && FilePathLen-(pFind-aFilePath) == str_length(aRequiredFiles[r]))
				FoundRequiredFilesCount++;
		}
	}

	if(FoundRequiredFilesCount != RequiredFilesCount)
	{
		dbg_msg("duck", "mod is missing a required file, required files are: ");
		for(int r = 0; r < RequiredFilesCount; r++)
		{
			dbg_msg("duck", "    - %s", aRequiredFiles[r]);
		}
		return false;
	}

	if(IsConfigDebug)
		dbg_msg("duck", "CREATE directory '%s'", aModRootPath);
	if(fs_makedir(aModRootPath) != 0)
	{
		dbg_msg("duck", "DecompressMod: Error, failed to create directory '%s'", aModRootPath);
		return false;
	}

	pCursor = pFilePack;
	while(pCursor < pFilePackEnd)
	{
		const int FilePathLen = *(int*)pCursor;
		pCursor += 4;
		const char* pFilePath = pCursor;
		pCursor += FilePathLen;
		const int FileSize = *(int*)pCursor;
		pCursor += 4;
		const char* pFileData = pCursor;
		pCursor += FileSize;

		char aFilePath[512];
		dbg_assert(FilePathLen < sizeof(aFilePath)-1, "FilePathLen too large");
		mem_move(aFilePath, pFilePath, FilePathLen);
		aFilePath[FilePathLen] = 0;

		// create directory to hold the file (if needed)
		const char* pSlashFound = aFilePath;
		do
		{
			pSlashFound = str_find(pSlashFound, "/");
			if(pSlashFound)
			{
				char aDir[512];
				str_format(aDir, sizeof(aDir), "%.*s", pSlashFound-aFilePath, aFilePath);

				if(IsConfigDebug)
					dbg_msg("duck", "CREATE SUBDIR dir=%s", aDir);

				char aDirPath[512];
				str_copy(aDirPath, aModRootPath, sizeof(aDirPath));
				str_append(aDirPath, "/", sizeof(aDirPath));
				str_append(aDirPath, aDir, sizeof(aDirPath));

				if(fs_makedir(aDirPath) != 0)
				{
					dbg_msg("duck", "DecompressMod: Error, failed to create directory '%s'", aDirPath);
					return false;
				}

				pSlashFound++;
			}
		}
		while(pSlashFound);

		// create file on disk
		char aDiskFilePath[256];
		str_copy(aDiskFilePath, aModRootPath, sizeof(aDiskFilePath));
		str_append(aDiskFilePath, "/", sizeof(aDiskFilePath));
		str_append(aDiskFilePath, aFilePath, sizeof(aDiskFilePath));

		IOHANDLE FileExtracted = io_open(aDiskFilePath, IOFLAG_WRITE);
		if(!FileExtracted)
		{
			dbg_msg("duck", "Error creating file '%s'", aDiskFilePath);
			return false;
		}

		io_write(FileExtracted, pFileData, FileSize);
		io_close(FileExtracted);
	}

	return true;
}

bool CDuckJs::LoadJsScriptFile(const char* pJsFilePath, const char* pJsRelFilePath)
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

	char aErrFuncBuff[1024];
	str_format(aErrFuncBuff, sizeof(aErrFuncBuff),
		"Duktape.errCreate = function(err) { \
			try { \
				if(typeof err === 'object' && \
				typeof err.stack !== 'undefined' && \
				typeof err.lineNumber === 'number') { \
					err.stack = err.stack + ' (%s:' + err.lineNumber + ')'; \
					err.message = err.stack; \
				} \
			} \
			catch(e) { \
			} \
			return err; \
	   }", pJsRelFilePath);

	duk_push_string(Ctx(), aErrFuncBuff);
	if(duk_peval(Ctx()) != 0)
	{
		dbg_msg("duck", "[JS ERROR] %s: %s", pJsRelFilePath, duk_safe_to_string(Ctx(), -1));
		return false;
	}
	duk_pop(Ctx());

	// eval script
	duk_push_string(Ctx(), pFileData);
	if(duk_peval(Ctx()) != 0)
	{
		dbg_msg("duck", "[JS ERROR] %s: %s", pJsFilePath, duk_safe_to_string(Ctx(), -1));
		return false;
	}
	duk_pop(Ctx());

	dbg_msg("duck", "'%s' loaded (%d)", pJsRelFilePath, FileSize);
	mem_free(pFileData);
	return true;
}

struct CPath
{
	char m_aBuff[512];
};

struct CFileSearch
{
	CPath m_BaseBath;
	array<CPath>* m_paFilePathList;
};

static int ListDirCallback(const char* pName, int IsDir, int Type, void *pUser)
{
	CFileSearch FileSearch = *(CFileSearch*)pUser; // copy

	if(IsDir)
	{
		if(pName[0] != '.')
		{
			//dbg_msg("duck", "ListDirCallback dir='%s'", pName);
			str_append(FileSearch.m_BaseBath.m_aBuff, "/", sizeof(FileSearch.m_BaseBath.m_aBuff));
			str_append(FileSearch.m_BaseBath.m_aBuff, pName, sizeof(FileSearch.m_BaseBath.m_aBuff));
			//dbg_msg("duck", "recursing... dir='%s'", DirPath.m_aBuff);
			fs_listdir(FileSearch.m_BaseBath.m_aBuff, ListDirCallback, Type + 1, &FileSearch);
		}
	}
	else
	{
		CPath FileStr;
		str_copy(FileStr.m_aBuff, pName, sizeof(FileStr.m_aBuff));

		CPath FilePath = FileSearch.m_BaseBath;
		str_append(FilePath.m_aBuff, "/", sizeof(FilePath.m_aBuff));
		str_append(FilePath.m_aBuff, pName, sizeof(FilePath.m_aBuff));
		FileSearch.m_paFilePathList->add(FilePath);
		//dbg_msg("duck", "ListDirCallback file='%s'", pName);
	}

	return 0;
}

bool CDuckJs::LoadModFilesFromDisk(const SHA256_DIGEST* pModSha256)
{
	char aModRootPath[512];
	char aSha256Str[SHA256_MAXSTRSIZE];
	Storage()->GetCompletePath(IStorage::TYPE_SAVE, "mods", aModRootPath, sizeof(aModRootPath));
	const int SaveDirPathLen = str_length(aModRootPath)-4;
	sha256_str(*pModSha256, aSha256Str, sizeof(aSha256Str));
	str_append(aModRootPath, "/", sizeof(aModRootPath));
	str_append(aModRootPath, aSha256Str, sizeof(aModRootPath));
	const int ModRootDirLen = str_length(aModRootPath);

	// get files recursively on disk
	array<CPath> aFilePathList;
	CFileSearch FileSearch;
	str_copy(FileSearch.m_BaseBath.m_aBuff, aModRootPath, sizeof(FileSearch.m_BaseBath.m_aBuff));
	FileSearch.m_paFilePathList = &aFilePathList;

	fs_listdir(aModRootPath, ListDirCallback, 1, &FileSearch);

	// reset mod
	OnModReset();

	const int FileCount = aFilePathList.size();
	const CPath* pFilePaths = aFilePathList.base_ptr();
	for(int i = 0; i < FileCount; i++)
	{
		//dbg_msg("duck", "file='%s'", pFilePaths[i].m_aBuff);
		if(str_endswith(pFilePaths[i].m_aBuff, ".js"))
		{
			const char* pRelPath = pFilePaths[i].m_aBuff+ModRootDirLen+1;
			const bool Loaded = LoadJsScriptFile(pFilePaths[i].m_aBuff, pRelPath);
			dbg_assert(Loaded, "error loading js script");
			// TODO: show error instead of breaking
		}
		else if(str_endswith(pFilePaths[i].m_aBuff, ".png"))
		{
			const char* pTextureName = pFilePaths[i].m_aBuff+ModRootDirLen+1;
			const char* pTextureRelPath = pFilePaths[i].m_aBuff+SaveDirPathLen;
			const bool Loaded = m_Bridge.LoadTexture(pTextureRelPath, pTextureName);
			dbg_assert(Loaded, "error loading png image");
			dbg_msg("duck", "image loaded '%s' (%x)", pTextureName, m_Bridge.m_aTextures[m_Bridge.m_aTextures.size()-1].m_Hash);
			// TODO: show error instead of breaking

			if(str_startswith(pTextureName, "skins/")) {
				pTextureName += 6;
				const char* pPartEnd = str_find(pTextureName, "/");
				if(!str_find(pPartEnd+1, "/")) {
					dbg_msg("duck", "skin part name = '%.*s'", pPartEnd-pTextureName, pTextureName);
					char aPart[256];
					str_format(aPart, sizeof(aPart), "%.*s", pPartEnd-pTextureName, pTextureName);
					m_Bridge.AddSkinPart(aPart, pPartEnd+1, m_Bridge.m_aTextures[m_Bridge.m_aTextures.size()-1].m_Handle);
				}
			}
		}
	}

	m_IsModLoaded = true;
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

	duk_push_c_function(Ctx(), NativeUnpackInteger<i32>, 1);
	duk_put_global_string(Ctx(), "TwUnpackInt32");

	duk_push_c_function(Ctx(), NativeUnpackInteger<u8>, 1);
	duk_put_global_string(Ctx(), "TwUnpackUint8");

	duk_push_c_function(Ctx(), NativeUnpackInteger<u16>, 1);
	duk_put_global_string(Ctx(), "TwUnpackUint16");

	duk_push_c_function(Ctx(), NativeUnpackInteger<u32>, 1);
	duk_put_global_string(Ctx(), "TwUnpackUint32");

#define REGISTER_FUNC(fname, arg_count) \
	duk_push_c_function(Ctx(), Native##fname, arg_count);\
	duk_put_global_string(Ctx(), "Tw" #fname)

	REGISTER_FUNC(UnpackFloat, 1);

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
	REGISTER_FUNC(RenderDrawFreeform, 1);
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
	REGISTER_FUNC(MapSetTileCollisionFlags, 3);
	REGISTER_FUNC(DirectionFromAngle, 1);
	REGISTER_FUNC(CollisionSetStaticBlock, 2);
	REGISTER_FUNC(CollisionClearStaticBlock, 1);
	REGISTER_FUNC(CollisionSetDynamicDisk, 2);
	REGISTER_FUNC(CollisionClearDynamicDisk, 1);
	REGISTER_FUNC(CollisionGetPredictedDynamicDisks, 0);
	REGISTER_FUNC(SetHudPartsShown, 1);
	REGISTER_FUNC(CreatePacket, 1);
	REGISTER_FUNC(PacketAddInt, 1);
	REGISTER_FUNC(PacketAddFloat, 1);
	REGISTER_FUNC(PacketAddString, 2);
	REGISTER_FUNC(SendPacket, 0);

#undef REGISTER_FUNC

	// Teeworlds global object
	duk_eval_string(Ctx(),
		"const Teeworlds = {"
		"	DRAW_SPACE_GAME: 0,"
		"	DRAW_SPACE_HUD: 1,"
		"};");
	duk_pop(Ctx());
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
		dbg_msg("duck", "[JS ERROR] %s() << %s >>", aLastCalledFunction, duk_safe_to_stacktrace(pCtx, -1));

		/*if(duk_is_error(pCtx, -1))
		{
			duk_get_prop_string(pCtx, -1, "stack");
			const char* pStack = duk_safe_to_string(pCtx, -1);
			duk_pop(pCtx);

			dbg_msg("duck", "[JS ERROR] OnCharacterCorePostTick(): %s", pStack);
		}
		else
		{
			dbg_msg("duck", "[JS ERROR] OnCharacterCorePostTick(): %s", duk_safe_to_string(pCtx, -1));
		}*/

		// TODO: exit more gracefully
		dbg_break();
	}
}

bool CDuckJs::HasJsFunctionReturned()
{
	duk_context* pCtx = Ctx();

	if(duk_is_undefined(pCtx, -1))
	{
		dbg_msg("duck", "[JS WARNING] %s() must return a value", aLastCalledFunction);
		duk_pop(pCtx);
		return false;
	}

	return true;
}

CDuckJs::CDuckJs()
{
	s_This = this;
	m_pDukContext = 0;
	m_IsModLoaded = false;
}

void CDuckJs::OnInit()
{
	m_Bridge.Init(this);
}

void CDuckJs::OnShutdown()
{
	if(m_pDukContext)
	{
		duk_destroy_heap(m_pDukContext);
		m_pDukContext = 0;
	}
}

void CDuckJs::OnRender()
{
	if(Client()->State() != IClient::STATE_ONLINE || !IsLoaded())
		return;

	m_Bridge.OnRender();

	// detect stack leak
	dbg_assert(duk_get_top(Ctx()) == 0, "stack leak");
}

void CDuckJs::OnMessage(int Msg, void* pRawMsg)
{
	if(Msg == NETMSG_DUCK_NETOBJ)
	{
		CUnpacker* pUnpacker = (CUnpacker*)pRawMsg;
		const int ObjID = pUnpacker->GetInt();
		const int ObjSize = pUnpacker->GetInt();
		const u8* pObjRawData = (u8*)pUnpacker->GetRaw(ObjSize);
		//dbg_msg("duck", "DUK packed netobj, id=0x%x size=%d", ObjID, ObjSize);

		if(GetJsFunction("OnMessage")) {
			// make netObj
			PushObject();
			ObjectSetMemberInt("netID", ObjID);
			ObjectSetMemberInt("cursor", 0);
			ObjectSetMemberRawBuffer("raw", pObjRawData, ObjSize);

			// call OnMessage(netObj)
			CallJsFunction(1);

			duk_pop(Ctx());
		}
	}
}

void CDuckJs::OnStateChange(int NewState, int OldState)
{
	if(OldState != IClient::STATE_OFFLINE && NewState == IClient::STATE_OFFLINE)
	{
		OnModReset();
	}
}

bool CDuckJs::OnInput(IInput::CEvent e)
{
	if(!IsLoaded()) {
		return false;
	}

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

	return false;
}

void CDuckJs::OnModReset()
{
	ResetDukContext();
	m_Bridge.Reset();
	m_IsModLoaded = false;
}

void CDuckJs::OnModUnload()
{
	if(m_pDukContext)
		duk_destroy_heap(m_pDukContext);
	m_pDukContext = 0;

	m_Bridge.Reset();
	m_IsModLoaded = false;
}

bool CDuckJs::StartDuckModHttpDownload(const char* pModUrl, const SHA256_DIGEST* pModSha256)
{
	dbg_assert(!IsModAlreadyInstalled(pModSha256), "mod is already installed, check it before calling this");

	CGrowBuffer Buff;
	HttpRequestPage(pModUrl, &Buff);

	bool IsUnzipped = ExtractAndInstallModZipBuffer(&Buff, pModSha256);
	dbg_assert(IsUnzipped, "Unzipped to disk: rip in peace");

	Buff.Release();

	if(!IsUnzipped)
		return false;

	bool IsLoaded = LoadModFilesFromDisk(pModSha256);
	dbg_assert(IsLoaded, "Loaded from disk: rip in peace");

	dbg_msg("duck", "mod loaded url='%s'", pModUrl);
	return IsLoaded;
}

bool CDuckJs::TryLoadInstalledDuckMod(const SHA256_DIGEST* pModSha256)
{
	if(!IsModAlreadyInstalled(pModSha256))
		return false;

	bool IsLoaded = LoadModFilesFromDisk(pModSha256);
	dbg_assert(IsLoaded, "Loaded from disk: rip in peace");

	char aSha256Str[SHA256_MAXSTRSIZE];
	sha256_str(*pModSha256, aSha256Str, sizeof(aSha256Str));
	dbg_msg("duck", "mod loaded (already installed) sha256='%s'", aSha256Str);
	return IsLoaded;
}

bool CDuckJs::InstallAndLoadDuckModFromZipBuffer(const void* pBuffer, int BufferSize, const SHA256_DIGEST* pModSha256)
{
	dbg_assert(!IsModAlreadyInstalled(pModSha256), "mod is already installed, check it before calling this");

	bool IsUnzipped = ExtractAndInstallModCompressedBuffer(pBuffer, BufferSize, pModSha256);
	dbg_assert(IsUnzipped, "Unzipped to disk: rip in peace");

	if(!IsUnzipped)
		return false;

	bool IsLoaded = LoadModFilesFromDisk(pModSha256);
	dbg_assert(IsLoaded, "Loaded from disk: rip in peace");

	char aSha256Str[SHA256_MAXSTRSIZE];
	sha256_str(*pModSha256, aSha256Str, sizeof(aSha256Str));
	dbg_msg("duck", "mod loaded from zip buffer sha256='%s'", aSha256Str);
	return IsLoaded;
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
