#include "duktape_comp.h"
#include <engine/storage.h>
#include <generated/protocol.h>
#include <generated/client_data.h>
#include <stdint.h>

#include <engine/client/http.h>
#include <engine/shared/growbuffer.h>
#include <engine/shared/config.h>
#include <engine/external/zlib/zlib.h>
#include <zip.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t i32;

// TODO: rename?
static CDuktape* s_This = 0;
inline CDuktape* This() { return s_This; }

static duk_ret_t NativePrint(duk_context *ctx)
{
	dbg_msg("duck", "%s", duk_to_string(ctx, 0));
	return 0;  /* no return value (= undefined) */
}

duk_ret_t CDuktape::NativeRenderQuad(duk_context *ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 4, "Wrong argument count");
	double x = duk_to_number(ctx, 0);
	double y = duk_to_number(ctx, 1);
	double Width = duk_to_number(ctx, 2);
	double Height = duk_to_number(ctx, 3);

	IGraphics::CQuadItem Quad(x, y, Width, Height);
	This()->m_DukEntry.QueueDrawQuad(Quad);
	return 0;
}

duk_ret_t CDuktape::NativeRenderSetColorU32(duk_context *ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 1, "Wrong argument count");
	int x = duk_to_int(ctx, 0);

	float aColor[4];
	aColor[0] = (x & 0xFF) / 255.f;
	aColor[1] = ((x >> 8) & 0xFF) / 255.f;
	aColor[2] = ((x >> 16) & 0xFF) / 255.f;
	aColor[3] = ((x >> 24) & 0xFF) / 255.f;

	This()->m_DukEntry.QueueSetColor(aColor);
	return 0;
}

duk_ret_t CDuktape::NativeRenderSetColorF4(duk_context *ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 4, "Wrong argument count");

	float aColor[4];
	aColor[0] = duk_to_number(ctx, 0);
	aColor[1] = duk_to_number(ctx, 1);
	aColor[2] = duk_to_number(ctx, 2);
	aColor[3] = duk_to_number(ctx, 3);

	This()->m_DukEntry.QueueSetColor(aColor);
	return 0;
}

duk_ret_t CDuktape::NativeRenderSetTexture(duk_context *ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 1, "Wrong argument count");
	This()->m_DukEntry.QueueSetTexture((int)duk_to_int(ctx, 0));
	return 0;
}

duk_ret_t CDuktape::NativeRenderSetQuadSubSet(duk_context* ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 4, "Wrong argument count");

	float aSubSet[4];
	aSubSet[0] = duk_to_number(ctx, 0);
	aSubSet[1] = duk_to_number(ctx, 1);
	aSubSet[2] = duk_to_number(ctx, 2);
	aSubSet[3] = duk_to_number(ctx, 3);

	This()->m_DukEntry.QueueSetQuadSubSet(aSubSet);
	return 0;
}

duk_ret_t CDuktape::NativeRenderSetQuadRotation(duk_context* ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 1, "Wrong argument count");

	float Angle = duk_to_number(ctx, 0);
	This()->m_DukEntry.QueueSetQuadRotation(Angle);
	return 0;
}

duk_ret_t CDuktape::NativeSetDrawSpace(duk_context *ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 1, "Wrong argument count");

	int ds = duk_to_int(ctx, 0);
	dbg_assert(ds >= 0 && ds < CDukEntry::DrawSpace::_COUNT, "Draw space undefined");

	This()->m_DukEntry.m_CurrentDrawSpace = ds;

	return 0;
}

duk_ret_t CDuktape::NativeRenderDrawTeeBodyAndFeet(duk_context *ctx)
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

	CDukEntry::CTeeDrawInfo TeeDrawInfo;
	TeeDrawInfo.m_Size = Size;
	TeeDrawInfo.m_Angle = Angle;
	TeeDrawInfo.m_Pos[0] = PosX;
	TeeDrawInfo.m_Pos[1] = PosY;
	TeeDrawInfo.m_IsWalking = IsWalking;
	TeeDrawInfo.m_IsGrounded = IsGrounded;
	TeeDrawInfo.m_GotAirJump = GotAirJump;
	TeeDrawInfo.m_Emote = Emote;
	//dbg_msg("duk", "DrawTeeBodyAndFeet( tee = { size: %g, pos_x: %g, pos_y: %g }", Size, PosX, PosY);
	This()->m_DukEntry.QueueDrawTeeBodyAndFeet(TeeDrawInfo);
	return 0;
}

duk_ret_t CDuktape::NativeGetBaseTexture(duk_context* ctx)
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

duk_ret_t CDuktape::NativeGetSpriteSubSet(duk_context* ctx)
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

duk_ret_t CDuktape::NativeGetSpriteScale(duk_context* ctx)
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

duk_ret_t CDuktape::NativeGetWeaponSpec(duk_context* ctx)
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

duk_ret_t CDuktape::NativeMapSetTileCollisionFlags(duk_context *ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 3, "Wrong argument count");

	// TODO: bound check
	int Tx = duk_to_int(ctx, 0);
	int Ty = duk_to_int(ctx, 1);
	int Flags = duk_to_int(ctx, 2);

	This()->Collision()->SetTileCollisionFlags(Tx, Ty, Flags);

	return 0;
}

template<typename IntT>
duk_ret_t CDuktape::NativeUnpackInteger(duk_context *ctx)
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

duk_ret_t CDuktape::NativeUnpackFloat(duk_context *ctx)
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

void CDuktape::PushObject()
{
	m_CurrentPushedObjID = duk_push_object(Duk());
}

void CDuktape::ObjectSetMemberInt(const char* MemberName, int Value)
{
	duk_push_int(Duk(), Value);
	duk_put_prop_string(Duk(), m_CurrentPushedObjID, MemberName);
}

void CDuktape::ObjectSetMemberFloat(const char* MemberName, float Value)
{
	duk_push_number(Duk(), Value);
	duk_put_prop_string(Duk(), m_CurrentPushedObjID, MemberName);
}

void CDuktape::ObjectSetMemberRawBuffer(const char* MemberName, const void* pRawBuffer, int RawBufferSize)
{
	duk_push_string(Duk(), MemberName);
	duk_push_fixed_buffer(Duk(), RawBufferSize);

	duk_size_t OutBufferSize;
	u8* OutBuffer = (u8*)duk_require_buffer_data(Duk(), -1, &OutBufferSize);
	dbg_assert((int)OutBufferSize == RawBufferSize, "buffer size differs");
	mem_copy(OutBuffer, pRawBuffer, RawBufferSize);

	int rc = duk_put_prop(Duk(), m_CurrentPushedObjID);
	dbg_assert(rc == 1, "could not put raw buffer prop");
}

void CDuktape::ObjectSetMember(const char* MemberName)
{
	duk_put_prop_string(Duk(), m_CurrentPushedObjID, MemberName);
}

bool CDuktape::IsModAlreadyInstalled(const SHA256_DIGEST* pModSha256)
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

bool CDuktape::ExtractAndInstallModZipBuffer(const HttpBuffer* pHttpZipData, const SHA256_DIGEST* pModSha256)
{
	dbg_msg("unzip", "EXTRACTING AND INSTALLING MOD");

	char aUserModsPath[512];
	Storage()->GetCompletePath(IStorage::TYPE_SAVE, "mods", aUserModsPath, sizeof(aUserModsPath));
	fs_makedir(aUserModsPath); // Teeworlds/mods (user storage)

	// TODO: reduce folder hash string length?
	SHA256_DIGEST Sha256 = sha256(pHttpZipData->m_pData, pHttpZipData->m_Cursor);
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
	zip_source_t* pZipSrc = zip_source_buffer_create(pHttpZipData->m_pData, pHttpZipData->m_Cursor, 1, &ZipError);
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
			if(!(str_ends_with(EntryStat.name, ".js") || str_ends_with(EntryStat.name, ".json") || str_ends_with(EntryStat.name, ".png") || str_ends_with(EntryStat.name, ".wv")))
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

bool CDuktape::ExtractAndInstallModCompressedBuffer(const void* pCompBuff, int CompBuffSize, const SHA256_DIGEST* pModSha256)
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

bool CDuktape::LoadJsScriptFile(const char* pJsFilePath)
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
	duk_push_string(Duk(), pFileData);
	if(duk_peval(Duk()) != 0)
	{
		/* Use duk_safe_to_string() to convert error into string.  This API
		 * call is guaranteed not to throw an error during the coercion.
		 */
		dbg_msg("duck", "Script error: %s", duk_safe_to_string(Duk(), -1));
		return false;
	}
	duk_pop(Duk());

	dbg_msg("duck", "'%s' loaded (%d)", pJsFilePath, FileSize);
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

bool CDuktape::LoadModFilesFromDisk(const SHA256_DIGEST* pModSha256)
{
	char aModRootPath[512];
	char aSha256Str[SHA256_MAXSTRSIZE];
	Storage()->GetCompletePath(IStorage::TYPE_SAVE, "mods", aModRootPath, sizeof(aModRootPath));
	sha256_str(*pModSha256, aSha256Str, sizeof(aSha256Str));
	str_append(aModRootPath, "/", sizeof(aModRootPath));
	str_append(aModRootPath, aSha256Str, sizeof(aModRootPath));

	// get files recursively on disk
	array<CPath> aFilePathList;
	CFileSearch FileSearch;
	str_copy(FileSearch.m_BaseBath.m_aBuff, aModRootPath, sizeof(FileSearch.m_BaseBath.m_aBuff));
	FileSearch.m_paFilePathList = &aFilePathList;

	fs_listdir(aModRootPath, ListDirCallback, 1, &FileSearch);

	// reset the duk context
	ResetDukContext();

	const int FileCount = aFilePathList.size();
	const CPath* pFilePaths = aFilePathList.base_ptr();
	for(int i = 0; i < FileCount; i++)
	{
		//dbg_msg("duck", "file='%s'", pFilePaths[i].m_aBuff);
		if(str_ends_with(pFilePaths[i].m_aBuff, ".js"))
		{
			const bool Loaded = LoadJsScriptFile(pFilePaths[i].m_aBuff);
			dbg_assert(Loaded, "error loading js script");
			// TODO: show error instead of breaking
		}
	}

	return true;
}

void CDuktape::ResetDukContext()
{
	if(m_pDukContext)
		duk_destroy_heap(m_pDukContext);

	// load ducktape, eval main.js
	m_pDukContext = duk_create_heap_default();

	// function binding
	// special functions
	duk_push_c_function(Duk(), NativePrint, 1);
	duk_put_global_string(Duk(), "print");

	duk_push_c_function(Duk(), NativeUnpackInteger<i32>, 1);
	duk_put_global_string(Duk(), "TwUnpackInt32");

	duk_push_c_function(Duk(), NativeUnpackInteger<u8>, 1);
	duk_put_global_string(Duk(), "TwUnpackUint8");

	duk_push_c_function(Duk(), NativeUnpackInteger<u16>, 1);
	duk_put_global_string(Duk(), "TwUnpackUint16");

	duk_push_c_function(Duk(), NativeUnpackInteger<u32>, 1);
	duk_put_global_string(Duk(), "TwUnpackUint32");

#define REGISTER_FUNC(fname, arg_count) \
	duk_push_c_function(Duk(), Native##fname, arg_count);\
	duk_put_global_string(Duk(), "Tw" #fname)

	REGISTER_FUNC(RenderQuad, 4);
	REGISTER_FUNC(RenderSetColorU32, 1);
	REGISTER_FUNC(RenderSetColorF4, 4);
	REGISTER_FUNC(RenderSetTexture, 1);
	REGISTER_FUNC(RenderSetQuadSubSet, 4);
	REGISTER_FUNC(RenderSetQuadRotation, 1);
	REGISTER_FUNC(RenderDrawTeeBodyAndFeet, 1);
	REGISTER_FUNC(SetDrawSpace, 1);
	REGISTER_FUNC(GetBaseTexture, 1);
	REGISTER_FUNC(GetSpriteSubSet, 1);
	REGISTER_FUNC(GetSpriteScale, 1);
	REGISTER_FUNC(GetWeaponSpec, 1);
	REGISTER_FUNC(MapSetTileCollisionFlags, 3);
	REGISTER_FUNC(UnpackFloat, 1);

#undef REGISTER_FUNC

	// Teeworlds global object
	duk_eval_string(Duk(),
		"var Teeworlds = {"
		"	DRAW_SPACE_GAME: 0,"
		"	aClients: [],"
		"};");
}

CDuktape::CDuktape()
{
	s_This = this;
	m_pDukContext = 0;
}

void CDuktape::OnInit()
{
	m_DukEntry.Init(this);
}

void CDuktape::OnShutdown()
{
	if(m_pDukContext)
	{
		duk_destroy_heap(m_pDukContext);
		m_pDukContext = 0;
	}
}

void CDuktape::OnRender()
{
	if(Client()->State() != IClient::STATE_ONLINE || !m_pDukContext)
		return;

	// Update Teeworlds global object
	char aEvalBuff[256];
	str_format(aEvalBuff, sizeof(aEvalBuff), "Teeworlds.intraTick = %g;", Client()->IntraGameTick());
	duk_eval_string(Duk(), aEvalBuff);
	str_format(aEvalBuff, sizeof(aEvalBuff), "Teeworlds.mapSize = { x: %d, y: %d };", Collision()->GetWidth(), Collision()->GetHeight());
	duk_eval_string(Duk(), aEvalBuff);

	// Call OnUpdate()
	duk_get_global_string(Duk(), "OnUpdate");
	/* push arguments here */
	duk_push_number(Duk(), Client()->LocalTime());
	int NumArgs = 1;
	if(duk_pcall(Duk(), NumArgs) != 0)
	{
		dbg_msg("duck", "OnUpdate(): Script error: %s", duk_safe_to_string(Duk(), -1));
		dbg_break();
	}
	duk_pop(Duk());
}

void CDuktape::OnMessage(int Msg, void* pRawMsg)
{
	if(Msg == NETMSG_DUCK_NETOBJ)
	{
		CUnpacker* pUnpacker = (CUnpacker*)pRawMsg;
		const int ObjID = pUnpacker->GetInt();
		const int ObjSize = pUnpacker->GetInt();
		const u8* pObjRawData = (u8*)pUnpacker->GetRaw(ObjSize);
		//dbg_msg("duck", "DUK packed netobj, id=0x%x size=%d", ObjID, ObjSize);

		duk_get_global_string(Duk(), "OnMessage");

		// make netObj
		PushObject();
		ObjectSetMemberInt("netID", ObjID);
		ObjectSetMemberInt("cursor", 0);
		ObjectSetMemberRawBuffer("raw", pObjRawData, ObjSize);

		// call OnMessage(netObj)
		int NumArgs = 1;
		if(duk_pcall(Duk(), NumArgs) != 0)
		{
			dbg_msg("duck", "OnMessage(): Script error: %s", duk_safe_to_string(Duk(), -1));
			dbg_break();
		}
		duk_pop(Duk());
	}
}

bool CDuktape::StartDuckModHttpDownload(const char* pModUrl, const SHA256_DIGEST* pModSha256)
{
	dbg_assert(!IsModAlreadyInstalled(pModSha256), "mod is already installed, check it before calling this");

	HttpBuffer Buff;
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

bool CDuktape::TryLoadInstalledDuckMod(const SHA256_DIGEST* pModSha256)
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

bool CDuktape::InstallAndLoadDuckModFromZipBuffer(const void* pBuffer, int BufferSize, const SHA256_DIGEST* pModSha256)
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
