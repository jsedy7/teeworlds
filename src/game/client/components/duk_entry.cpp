#include "duk_entry.h"
#include "duktape_comp.h"
#include <game/client/animstate.h>
#include <game/client/render.h>
#include <engine/external/pnglite/pnglite.h>
#include <engine/storage.h>
#include <base/hash.h>

void CDukEntry::DrawTeeBodyAndFeet(const CTeeDrawBodyAndFeetInfo& TeeDrawInfo, const CTeeSkinInfo& SkinInfo)
{
	CAnimState State;
	State.Set(&g_pData->m_aAnimations[ANIM_BASE], 0);

	CTeeRenderInfo RenderInfo;
	mem_move(RenderInfo.m_aTextures, SkinInfo.m_aTextures, sizeof(RenderInfo.m_aTextures));
	mem_move(RenderInfo.m_aColors, SkinInfo.m_aColors, sizeof(RenderInfo.m_aColors));
	RenderInfo.m_Size = TeeDrawInfo.m_Size;
	RenderInfo.m_GotAirJump = TeeDrawInfo.m_GotAirJump;

	vec2 Direction = direction(TeeDrawInfo.m_Angle);
	vec2 Pos = vec2(TeeDrawInfo.m_Pos[0], TeeDrawInfo.m_Pos[1]);
	int Emote = TeeDrawInfo.m_Emote;

	const float WalkTimeMagic = 100.0f;
	float WalkTime =
		((Pos.x >= 0)
			? fmod(Pos.x, WalkTimeMagic)
			: WalkTimeMagic - fmod(-Pos.x, WalkTimeMagic))
		/ WalkTimeMagic;

	if(TeeDrawInfo.m_IsWalking)
		State.Add(&g_pData->m_aAnimations[ANIM_WALK], WalkTime, 1.0f);
	else
	{
		if(TeeDrawInfo.m_IsGrounded)
			State.Add(&g_pData->m_aAnimations[ANIM_IDLE], 0, 1.0f); // TODO: some sort of time here
		else
			State.Add(&g_pData->m_aAnimations[ANIM_INAIR], 0, 1.0f); // TODO: some sort of time here
	}

	RenderTools()->RenderTee(&State, &RenderInfo, Emote, Direction, Pos);
}

void CDukEntry::DrawTeeHand(const CDukEntry::CTeeDrawHand& Hand, const CTeeSkinInfo& SkinInfo)
{
	CTeeRenderInfo RenderInfo;
	mem_move(RenderInfo.m_aTextures, SkinInfo.m_aTextures, sizeof(RenderInfo.m_aTextures));
	mem_move(RenderInfo.m_aColors, SkinInfo.m_aColors, sizeof(RenderInfo.m_aColors));
	RenderInfo.m_Size = Hand.m_Size;
	vec2 Pos(Hand.m_Pos[0], Hand.m_Pos[1]);
	vec2 Offset(Hand.m_Offset[0], Hand.m_Offset[1]);
	vec2 Dir = direction(Hand.m_AngleDir);

	RenderTools()->RenderTeeHand(&RenderInfo, Pos, Dir, Hand.m_AngleOff, Offset);
}

void CDukEntry::Init(CDuktape* pDuktape)
{
	m_pDuktape = pDuktape;
	m_pGraphics = pDuktape->Graphics();
	m_pRenderTools = pDuktape->RenderTools();
	m_pGameClient = pDuktape->m_pClient;
	m_CurrentDrawSpace = 0;
}

void CDukEntry::Reset()
{
	for(int i = 0 ; i < m_aTextures.size(); i++)
	{
		Graphics()->UnloadTexture(&m_aTextures[i].m_Handle);
	}

	m_aTextures.clear();
}

void CDukEntry::QueueSetColor(const float* pColor)
{
	CRenderCmd Cmd;
	Cmd.m_Type = CRenderCmd::SET_COLOR;
	mem_move(Cmd.m_Color, pColor, sizeof(Cmd.m_Color));
	m_aRenderCmdList[m_CurrentDrawSpace].add(Cmd);
}

void CDukEntry::QueueSetTexture(int TextureID)
{
	CRenderCmd Cmd;
	Cmd.m_Type = CRenderCmd::SET_TEXTURE;
	Cmd.m_TextureID = TextureID;
	m_aRenderCmdList[m_CurrentDrawSpace].add(Cmd);
}

void CDukEntry::QueueSetQuadSubSet(const float* pSubSet)
{
	CRenderCmd Cmd;
	Cmd.m_Type = CRenderCmd::SET_QUAD_SUBSET;
	mem_move(Cmd.m_QuadSubSet, pSubSet, sizeof(Cmd.m_QuadSubSet));
	m_aRenderCmdList[m_CurrentDrawSpace].add(Cmd);
}

void CDukEntry::QueueSetQuadRotation(float Angle)
{
	CRenderCmd Cmd;
	Cmd.m_Type = CRenderCmd::SET_QUAD_ROTATION;
	Cmd.m_QuadRotation = Angle;
	m_aRenderCmdList[m_CurrentDrawSpace].add(Cmd);
}

void CDukEntry::QueueSetTeeSkin(const CTeeSkinInfo& SkinInfo)
{
	CRenderCmd Cmd;
	Cmd.m_Type = CRenderCmd::SET_TEE_SKIN;
	Cmd.m_TeeSkinInfo = SkinInfo;
	m_aRenderCmdList[m_CurrentDrawSpace].add(Cmd);
}

void CDukEntry::QueueDrawQuad(IGraphics::CQuadItem Quad)
{
	CRenderCmd Cmd;
	Cmd.m_Type = CDukEntry::CRenderCmd::DRAW_QUAD;
	mem_move(Cmd.m_Quad, &Quad, sizeof(Cmd.m_Quad)); // yep, this is because we don't have c++11
	m_aRenderCmdList[m_CurrentDrawSpace].add(Cmd);
}

void CDukEntry::QueueDrawQuadCentered(IGraphics::CQuadItem Quad)
{
	CRenderCmd Cmd;
	Cmd.m_Type = CDukEntry::CRenderCmd::DRAW_QUAD_CENTERED;
	mem_move(Cmd.m_Quad, &Quad, sizeof(Cmd.m_Quad));
	m_aRenderCmdList[m_CurrentDrawSpace].add(Cmd);
}

void CDukEntry::QueueDrawTeeBodyAndFeet(const CTeeDrawBodyAndFeetInfo& TeeDrawInfo)
{
	CRenderCmd Cmd;
	Cmd.m_Type = CDukEntry::CRenderCmd::DRAW_TEE_BODYANDFEET;
	Cmd.m_TeeBodyAndFeet = TeeDrawInfo;
	m_aRenderCmdList[m_CurrentDrawSpace].add(Cmd);
}

void CDukEntry::QueueDrawTeeHand(const CDukEntry::CTeeDrawHand& Hand)
{
	CRenderCmd Cmd;
	Cmd.m_Type = CDukEntry::CRenderCmd::DRAW_TEE_HAND;
	Cmd.m_TeeHand = Hand;
	m_aRenderCmdList[m_CurrentDrawSpace].add(Cmd);
}

bool CDukEntry::LoadTexture(const char *pTexturePath, const char* pTextureName)
{
	IGraphics::CTextureHandle Handle = Graphics()->LoadTexture(pTexturePath, IStorage::TYPE_SAVE, CImageInfo::FORMAT_AUTO, 0);
	uint32_t Hash = hash_fnv1a(pTextureName, str_length(pTextureName));
	CTextureHashPair Pair = { Hash, Handle };
	m_aTextures.add(Pair);
	return Handle.IsValid();
}

IGraphics::CTextureHandle CDukEntry::GetTexture(const char *pTextureName)
{
	const uint32_t SearchHash = hash_fnv1a(pTextureName, str_length(pTextureName));

	const CTextureHashPair* Pairs = m_aTextures.base_ptr();
	const int PairCount = m_aTextures.size();

	for(int i = 0; i < PairCount; i++)
	{
		if(Pairs[i].m_Hash == SearchHash)
			return Pairs[i].m_Handle;
	}

	return IGraphics::CTextureHandle();
}

void CDukEntry::RenderDrawSpace(DrawSpace::Enum Space)
{
	const int CmdCount = m_aRenderCmdList[Space].size();
	const CRenderCmd* aCmds = m_aRenderCmdList[Space].base_ptr();

	// TODO: merge CRenderSpace and DrawSpace
	CRenderSpace& RenderSpace = m_aRenderSpace[Space];
	float* pWantColor = RenderSpace.m_aWantColor;
	float* pCurrentColor = RenderSpace.m_aCurrentColor;
	float* pWantQuadSubSet = RenderSpace.m_aWantQuadSubSet;
	float* pCurrentQuadSubSet = RenderSpace.m_aCurrentQuadSubSet;

	for(int i = 0; i < CmdCount; i++)
	{
		const CRenderCmd& Cmd = aCmds[i];

		switch(Cmd.m_Type)
		{
			case CRenderCmd::SET_COLOR: {
				mem_move(pWantColor, Cmd.m_Color, sizeof(Cmd.m_Color));
			} break;

			case CRenderCmd::SET_TEXTURE: {
				RenderSpace.m_WantTextureID = Cmd.m_TextureID;
			} break;

			case CRenderCmd::SET_QUAD_SUBSET: {
				mem_move(pWantQuadSubSet, Cmd.m_QuadSubSet, sizeof(Cmd.m_QuadSubSet));
			} break;

			case CRenderCmd::SET_QUAD_ROTATION: {
				RenderSpace.m_WantQuadRotation = Cmd.m_QuadRotation;
			} break
				;
			case CRenderCmd::SET_TEE_SKIN: {
				RenderSpace.m_CurrentTeeSkin = Cmd.m_TeeSkinInfo;
			} break;

			case CRenderCmd::DRAW_QUAD_CENTERED:
			case CRenderCmd::DRAW_QUAD: {
				if(RenderSpace.m_WantTextureID != RenderSpace.m_CurrentTextureID)
				{
					if(RenderSpace.m_WantTextureID < 0)
						Graphics()->TextureClear();
					else
						Graphics()->TextureSet(*(IGraphics::CTextureHandle*)&RenderSpace.m_WantTextureID);
					RenderSpace.m_CurrentTextureID = RenderSpace.m_WantTextureID;
				}

				Graphics()->QuadsBegin();

				if(pWantColor[0] != pCurrentColor[0] ||
				   pWantColor[1] != pCurrentColor[1] ||
				   pWantColor[2] != pCurrentColor[2] ||
				   pWantColor[3] != pCurrentColor[3])
				{
					Graphics()->SetColor(pWantColor[0] * pWantColor[3], pWantColor[1] * pWantColor[3], pWantColor[2] * pWantColor[3], pWantColor[3]);
					mem_move(pCurrentColor, pWantColor, sizeof(float)*4);
				}

				if(pWantQuadSubSet[0] != pCurrentQuadSubSet[0] ||
				   pWantQuadSubSet[1] != pCurrentQuadSubSet[1] ||
				   pWantQuadSubSet[2] != pCurrentQuadSubSet[2] ||
				   pWantQuadSubSet[3] != pCurrentQuadSubSet[3])
				{
					Graphics()->QuadsSetSubset(pWantQuadSubSet[0], pWantQuadSubSet[1], pWantQuadSubSet[2], pWantQuadSubSet[3]);
					mem_move(pCurrentQuadSubSet, pWantQuadSubSet, sizeof(float)*4);
				}

				if(RenderSpace.m_WantQuadRotation != RenderSpace.m_CurrentQuadRotation)
				{
					Graphics()->QuadsSetRotation(RenderSpace.m_WantQuadRotation);
					RenderSpace.m_CurrentQuadRotation = RenderSpace.m_WantQuadRotation;
				}

				if(Cmd.m_Type == CRenderCmd::DRAW_QUAD_CENTERED)
					Graphics()->QuadsDraw((IGraphics::CQuadItem*)&Cmd.m_Quad, 1);
				else
					Graphics()->QuadsDrawTL((IGraphics::CQuadItem*)&Cmd.m_Quad, 1);
				Graphics()->QuadsEnd();
			} break;

			case CRenderCmd::DRAW_TEE_BODYANDFEET:
				DrawTeeBodyAndFeet(Cmd.m_TeeBodyAndFeet, RenderSpace.m_CurrentTeeSkin);
				break;

			case CRenderCmd::DRAW_TEE_HAND:
				DrawTeeHand(Cmd.m_TeeHand, RenderSpace.m_CurrentTeeSkin);
				break;

			default:
				dbg_assert(0, "Render command type not handled");
		}
	}

	m_aRenderCmdList[Space].clear();
	RenderSpace = CRenderSpace();
}

// TODO: move this?
static duk_idx_t DuktapePushCharacterCore(duk_context* pCtx, const CCharacterCore* pCharCore)
{
	duk_idx_t CharCoreObjIdx = duk_push_object(pCtx);
	duk_push_number(pCtx, pCharCore->m_Pos.x);
	duk_put_prop_string(pCtx, CharCoreObjIdx, "pos_x");
	duk_push_number(pCtx, pCharCore->m_Pos.y);
	duk_put_prop_string(pCtx, CharCoreObjIdx, "pos_y");
	duk_push_number(pCtx, pCharCore->m_Vel.x);
	duk_put_prop_string(pCtx, CharCoreObjIdx, "vel_x");
	duk_push_number(pCtx, pCharCore->m_Vel.y);
	duk_put_prop_string(pCtx, CharCoreObjIdx, "vel_y");
	duk_push_number(pCtx, pCharCore->m_HookPos.x);
	duk_put_prop_string(pCtx, CharCoreObjIdx, "hook_pos_x");
	duk_push_number(pCtx, pCharCore->m_HookPos.y);
	duk_put_prop_string(pCtx, CharCoreObjIdx, "hook_pos_y");
	duk_push_number(pCtx, pCharCore->m_HookDir.x);
	duk_put_prop_string(pCtx, CharCoreObjIdx, "hook_dir_x");
	duk_push_number(pCtx, pCharCore->m_HookDir.y);
	duk_put_prop_string(pCtx, CharCoreObjIdx, "hook_dir_y");

	duk_push_int(pCtx, pCharCore->m_HookTick);
	duk_put_prop_string(pCtx, CharCoreObjIdx, "hook_tick");
	duk_push_int(pCtx, pCharCore->m_HookState);
	duk_put_prop_string(pCtx, CharCoreObjIdx, "hook_state");
	duk_push_int(pCtx, pCharCore->m_HookedPlayer);
	duk_put_prop_string(pCtx, CharCoreObjIdx, "hooked_player");
	duk_push_int(pCtx, pCharCore->m_Jumped);
	duk_put_prop_string(pCtx, CharCoreObjIdx, "jumped");
	duk_push_int(pCtx, pCharCore->m_Direction);
	duk_put_prop_string(pCtx, CharCoreObjIdx, "direction");
	duk_push_int(pCtx, pCharCore->m_Angle);
	duk_put_prop_string(pCtx, CharCoreObjIdx, "angle");
	duk_push_int(pCtx, pCharCore->m_TriggeredEvents);
	duk_put_prop_string(pCtx, CharCoreObjIdx, "triggered_events");
	return CharCoreObjIdx;
}

static duk_idx_t DuktapePushNetObjPlayerInput(duk_context* pCtx, const CNetObj_PlayerInput* pInput)
{
	duk_idx_t InputObjIdx = duk_push_object(pCtx);
	duk_push_int(pCtx, pInput->m_Direction);
	duk_put_prop_string(pCtx, InputObjIdx, "direction");
	duk_push_int(pCtx, pInput->m_TargetX);
	duk_put_prop_string(pCtx, InputObjIdx, "target_x");
	duk_push_int(pCtx, pInput->m_TargetY);
	duk_put_prop_string(pCtx, InputObjIdx, "target_y");
	duk_push_int(pCtx, pInput->m_Jump);
	duk_put_prop_string(pCtx, InputObjIdx, "jump");
	duk_push_int(pCtx, pInput->m_Fire);
	duk_put_prop_string(pCtx, InputObjIdx, "fire");
	duk_push_int(pCtx, pInput->m_Hook);
	duk_put_prop_string(pCtx, InputObjIdx, "hook");
	duk_push_int(pCtx, pInput->m_PlayerFlags);
	duk_put_prop_string(pCtx, InputObjIdx, "player_flags");
	duk_push_int(pCtx, pInput->m_WantedWeapon);
	duk_put_prop_string(pCtx, InputObjIdx, "wanted_weapon");
	duk_push_int(pCtx, pInput->m_NextWeapon);
	duk_put_prop_string(pCtx, InputObjIdx, "next_weapon");
	duk_push_int(pCtx, pInput->m_PrevWeapon);
	duk_put_prop_string(pCtx, InputObjIdx, "prev_weapon");
	return InputObjIdx;
}

inline void GetIntProp(duk_context* pCtx, duk_idx_t ObjIdx, const char* pPropName, int* pOutInt)
{
	if(duk_get_prop_string(pCtx, ObjIdx, pPropName))
	{
		*pOutInt = duk_to_int(pCtx, -1);
		duk_pop(pCtx);
	}
}

inline void GetFloatProp(duk_context* pCtx, duk_idx_t ObjIdx, const char* pPropName, float* pOutFloat)
{
	if(duk_get_prop_string(pCtx, ObjIdx, pPropName))
	{
		*pOutFloat = duk_to_number(pCtx, -1);
		duk_pop(pCtx);
	}
}

static void DuktapeReadCharacterCore(duk_context* pCtx, duk_idx_t ObjIdx, CCharacterCore* pOutCharCore)
{
	GetFloatProp(pCtx, ObjIdx, "pos_x", &pOutCharCore->m_Pos.x);
	GetFloatProp(pCtx, ObjIdx, "pos_y", &pOutCharCore->m_Pos.y);
	GetFloatProp(pCtx, ObjIdx, "vel_x", &pOutCharCore->m_Vel.x);
	GetFloatProp(pCtx, ObjIdx, "vel_y", &pOutCharCore->m_Vel.y);
	GetFloatProp(pCtx, ObjIdx, "hook_pos_x", &pOutCharCore->m_HookPos.x);
	GetFloatProp(pCtx, ObjIdx, "hook_pos_y", &pOutCharCore->m_HookPos.y);
	GetFloatProp(pCtx, ObjIdx, "hook_dir_x", &pOutCharCore->m_HookDir.x);
	GetFloatProp(pCtx, ObjIdx, "hook_dir_y", &pOutCharCore->m_HookDir.y);

	GetIntProp(pCtx, ObjIdx, "hook_tick", &pOutCharCore->m_HookTick);
	GetIntProp(pCtx, ObjIdx, "hook_state", &pOutCharCore->m_HookState);
	GetIntProp(pCtx, ObjIdx, "hooked_player", &pOutCharCore->m_HookedPlayer);
	GetIntProp(pCtx, ObjIdx, "jumped", &pOutCharCore->m_Jumped);
	GetIntProp(pCtx, ObjIdx, "direction", &pOutCharCore->m_Direction);
	GetIntProp(pCtx, ObjIdx, "angle", &pOutCharCore->m_Angle);
	GetIntProp(pCtx, ObjIdx, "triggered_events", &pOutCharCore->m_TriggeredEvents);
}

static void DuktapeReadNetObjPlayerInput(duk_context* pCtx, duk_idx_t ObjIdx, CNetObj_PlayerInput* pOutInput)
{
	GetIntProp(pCtx, ObjIdx, "direction", &pOutInput->m_Direction);
	GetIntProp(pCtx, ObjIdx, "target_x", &pOutInput->m_TargetX);
	GetIntProp(pCtx, ObjIdx, "target_y", &pOutInput->m_TargetY);
	GetIntProp(pCtx, ObjIdx, "jump", &pOutInput->m_Jump);
	GetIntProp(pCtx, ObjIdx, "fire", &pOutInput->m_Fire);
	GetIntProp(pCtx, ObjIdx, "hook", &pOutInput->m_Hook);
	GetIntProp(pCtx, ObjIdx, "player_flags", &pOutInput->m_PlayerFlags);
	GetIntProp(pCtx, ObjIdx, "wanted_weapon", &pOutInput->m_WantedWeapon);
	GetIntProp(pCtx, ObjIdx, "next_weapon", &pOutInput->m_NextWeapon);
	GetIntProp(pCtx, ObjIdx, "prev_weapon", &pOutInput->m_PrevWeapon);
}

void CDukEntry::CharacterCorePreTick(CCharacterCore** apCharCores)
{
	if(!Duktape()->IsLoaded())
		return;

	duk_context* pCtx = Duktape()->Ctx();

	if(!duk_get_global_string(pCtx, "OnCharacterCorePreTick"))
	{
		duk_pop(pCtx);
		return;
	}

	// arguments (array[CCharacterCore object], array[CNetObj_PlayerInput object])
	duk_idx_t ArrayCharCoresIdx = duk_push_array(pCtx);
	for(int c = 0; c < MAX_CLIENTS; c++)
	{
		if(apCharCores[c])
			DuktapePushCharacterCore(pCtx, apCharCores[c]);
		else
			duk_push_null(pCtx);
		duk_put_prop_index(pCtx, ArrayCharCoresIdx, c);
	}

	duk_idx_t ArrayInputIdx = duk_push_array(pCtx);
	for(int c = 0; c < MAX_CLIENTS; c++)
	{
		if(apCharCores[c])
			DuktapePushNetObjPlayerInput(pCtx, &apCharCores[c]->m_Input);
		else
			duk_push_null(pCtx);
		duk_put_prop_index(pCtx, ArrayInputIdx, c);
	}

	// TODO: make a function out of this? CallJsFunction(funcname, numargs)?
	int NumArgs = 2;
	if(duk_pcall(pCtx, NumArgs) != DUK_EXEC_SUCCESS)
	{
		if(duk_is_error(pCtx, -1))
		{
			duk_get_prop_string(pCtx, -1, "stack");
			const char* pStack = duk_safe_to_string(pCtx, -1);
			duk_pop(pCtx);

			dbg_msg("duck", "[JS ERROR] OnCharacterCorePreTick(): %s", pStack);
		}
		else
		{
			dbg_msg("duck", "[JS ERROR] OnCharacterCorePreTick(): %s", duk_safe_to_string(pCtx, -1));
		}
		dbg_break();
	}

	if(duk_is_undefined(pCtx, -1))
	{
		dbg_msg("duck", "[JS WARNING] OnCharacterCorePreTick() must return a value");
		duk_pop(pCtx);
		return;
	}

	// EXPECTS RETURN: [array[CCharacterCore object], array[CNetObj_PlayerInput object]]
	if(duk_get_prop_index(pCtx, -1, 0))
	{
		for(int c = 0; c < MAX_CLIENTS; c++)
		{
			if(duk_get_prop_index(pCtx, -1, c))
			{
				if(!duk_is_null(pCtx, -1))
					DuktapeReadCharacterCore(pCtx, -1, apCharCores[c]);
				duk_pop(pCtx);
			}
		}
		duk_pop(pCtx);
	}
	if(duk_get_prop_index(pCtx, -1, 1))
	{
		for(int c = 0; c < MAX_CLIENTS; c++)
		{
			if(duk_get_prop_index(pCtx, -1, c))
			{
				if(!duk_is_null(pCtx, -1))
					DuktapeReadNetObjPlayerInput(pCtx, -1, &apCharCores[c]->m_Input);
				duk_pop(pCtx);
			}
		}
		duk_pop(pCtx);
	}

	duk_pop(pCtx);
}

void CDukEntry::CharacterCorePostTick(CCharacterCore** apCharCores)
{
	if(!Duktape()->IsLoaded())
		return;

	duk_context* pCtx = Duktape()->Ctx();

	if(!duk_get_global_string(pCtx, "OnCharacterCorePostTick"))
	{
		duk_pop(pCtx);
		return;
	}

	// arguments (array[CCharacterCore object], array[CNetObj_PlayerInput object])
	duk_idx_t ArrayCharCoresIdx = duk_push_array(pCtx);
	for(int c = 0; c < MAX_CLIENTS; c++)
	{
		if(apCharCores[c])
			DuktapePushCharacterCore(pCtx, apCharCores[c]);
		else
			duk_push_null(pCtx);
		duk_put_prop_index(pCtx, ArrayCharCoresIdx, c);
	}

	duk_idx_t ArrayInputIdx = duk_push_array(pCtx);
	for(int c = 0; c < MAX_CLIENTS; c++)
	{
		if(apCharCores[c])
			DuktapePushNetObjPlayerInput(pCtx, &apCharCores[c]->m_Input);
		else
			duk_push_null(pCtx);
		duk_put_prop_index(pCtx, ArrayInputIdx, c);
	}

	// TODO: make a function out of this? CallJsFunction(funcname, numargs)?
	int NumArgs = 2;
	if(duk_pcall(pCtx, NumArgs) != DUK_EXEC_SUCCESS)
	{
		if(duk_is_error(pCtx, -1))
		{
			duk_get_prop_string(pCtx, -1, "stack");
			const char* pStack = duk_safe_to_string(pCtx, -1);
			duk_pop(pCtx);

			dbg_msg("duck", "[JS ERROR] OnCharacterCorePostTick(): %s", pStack);
		}
		else
		{
			dbg_msg("duck", "[JS ERROR] OnCharacterCorePostTick(): %s", duk_safe_to_string(pCtx, -1));
		}
		dbg_break();
	}

	if(duk_is_undefined(pCtx, -1))
	{
		dbg_msg("duck", "[JS WARNING] OnCharacterCorePostTick() must return a value");
		duk_pop(pCtx);
		return;
	}

	// EXPECTS RETURN: [array[CCharacterCore object], array[CNetObj_PlayerInput object]]
	if(duk_get_prop_index(pCtx, -1, 0))
	{
		for(int c = 0; c < MAX_CLIENTS; c++)
		{
			if(duk_get_prop_index(pCtx, -1, c))
			{
				if(!duk_is_null(pCtx, -1))
					DuktapeReadCharacterCore(pCtx, -1, apCharCores[c]);
				duk_pop(pCtx);
			}
		}
		duk_pop(pCtx);
	}
	if(duk_get_prop_index(pCtx, -1, 1))
	{
		for(int c = 0; c < MAX_CLIENTS; c++)
		{
			if(duk_get_prop_index(pCtx, -1, c))
			{
				if(!duk_is_null(pCtx, -1))
					DuktapeReadNetObjPlayerInput(pCtx, -1, &apCharCores[c]->m_Input);
				duk_pop(pCtx);
			}
		}
		duk_pop(pCtx);
	}

	duk_pop(pCtx);
}
