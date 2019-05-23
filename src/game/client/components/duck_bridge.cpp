#include "duck_bridge.h"
#include "duck_js.h"
#include <game/client/animstate.h>
#include <game/client/render.h>
#include <engine/external/pnglite/pnglite.h>
#include <engine/storage.h>
#include <base/hash.h>

CMultiStackAllocator::CMultiStackAllocator()
{
	CStackBuffer StackBuffer;
	StackBuffer.m_pBuffer = (char*)mem_alloc(STACK_BUFFER_CAPACITY, 1);
	StackBuffer.m_Cursor = 0;

	m_aStacks.hint_size(8);
	m_aStacks.add(StackBuffer);
	m_CurrentStack = 0;
}

CMultiStackAllocator::~CMultiStackAllocator()
{
	const int StackBufferCount = m_aStacks.size();
	for(int s = 0; s < StackBufferCount; s++)
	{
		mem_free(m_aStacks[s].m_pBuffer);
	}
}

void* CMultiStackAllocator::Alloc(int Size)
{
	dbg_assert(Size <= STACK_BUFFER_CAPACITY, "Trying to alloc a large buffer");

	// if current stack is not full, alloc from it
	CStackBuffer& CurrentSb = m_aStacks[m_CurrentStack];
	if(CurrentSb.m_Cursor + Size <= STACK_BUFFER_CAPACITY)
	{
		int MemBlockStart = CurrentSb.m_Cursor;
		CurrentSb.m_Cursor += Size;
		return CurrentSb.m_pBuffer + MemBlockStart;
	}

	// else add a new stack if needed
	if(m_CurrentStack+1 >= m_aStacks.size())
	{
		CStackBuffer StackBuffer;
		StackBuffer.m_pBuffer = (char*)mem_alloc(STACK_BUFFER_CAPACITY, 1);
		StackBuffer.m_Cursor = 0;
		m_aStacks.add(StackBuffer);
	}

	// and try again
	m_CurrentStack++;
	return Alloc(Size);
}

void CMultiStackAllocator::Clear()
{
	const int StackBufferCount = m_aStacks.size();
	for(int s = 0; s < StackBufferCount; s++)
	{
		m_aStacks[s].m_Cursor = 0;
	}
	m_CurrentStack = 0;
}


void CDuckBridge::DrawTeeBodyAndFeet(const CTeeDrawBodyAndFeetInfo& TeeDrawInfo, const CTeeSkinInfo& SkinInfo)
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

void CDuckBridge::DrawTeeHand(const CDuckBridge::CTeeDrawHand& Hand, const CTeeSkinInfo& SkinInfo)
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

void CDuckBridge::Init(CDuckJs* pDuckJs)
{
	m_pDuckJs = pDuckJs;
	m_pGraphics = pDuckJs->Graphics();
	m_pRenderTools = pDuckJs->RenderTools();
	m_pGameClient = pDuckJs->m_pClient;
	m_CurrentDrawSpace = 0;
}

void CDuckBridge::Reset()
{
	for(int i = 0 ; i < m_aTextures.size(); i++)
	{
		Graphics()->UnloadTexture(&m_aTextures[i].m_Handle);
	}

	m_aTextures.clear();
}

void CDuckBridge::QueueSetColor(const float* pColor)
{
	CRenderCmd Cmd;
	Cmd.m_Type = CRenderCmd::SET_COLOR;
	mem_move(Cmd.m_Color, pColor, sizeof(Cmd.m_Color));
	m_aRenderCmdList[m_CurrentDrawSpace].add(Cmd);
}

void CDuckBridge::QueueSetTexture(int TextureID)
{
	CRenderCmd Cmd;
	Cmd.m_Type = CRenderCmd::SET_TEXTURE;
	Cmd.m_TextureID = TextureID;
	m_aRenderCmdList[m_CurrentDrawSpace].add(Cmd);
}

void CDuckBridge::QueueSetQuadSubSet(const float* pSubSet)
{
	CRenderCmd Cmd;
	Cmd.m_Type = CRenderCmd::SET_QUAD_SUBSET;
	mem_move(Cmd.m_QuadSubSet, pSubSet, sizeof(Cmd.m_QuadSubSet));
	m_aRenderCmdList[m_CurrentDrawSpace].add(Cmd);
}

void CDuckBridge::QueueSetQuadRotation(float Angle)
{
	CRenderCmd Cmd;
	Cmd.m_Type = CRenderCmd::SET_QUAD_ROTATION;
	Cmd.m_QuadRotation = Angle;
	m_aRenderCmdList[m_CurrentDrawSpace].add(Cmd);
}

void CDuckBridge::QueueSetTeeSkin(const CTeeSkinInfo& SkinInfo)
{
	CRenderCmd Cmd;
	Cmd.m_Type = CRenderCmd::SET_TEE_SKIN;
	Cmd.m_TeeSkinInfo = SkinInfo;
	m_aRenderCmdList[m_CurrentDrawSpace].add(Cmd);
}

void CDuckBridge::QueueDrawQuad(IGraphics::CQuadItem Quad)
{
	CRenderCmd Cmd;
	Cmd.m_Type = CDuckBridge::CRenderCmd::DRAW_QUAD;
	mem_move(Cmd.m_Quad, &Quad, sizeof(Cmd.m_Quad)); // yep, this is because we don't have c++11
	m_aRenderCmdList[m_CurrentDrawSpace].add(Cmd);
}

void CDuckBridge::QueueDrawQuadCentered(IGraphics::CQuadItem Quad)
{
	CRenderCmd Cmd;
	Cmd.m_Type = CDuckBridge::CRenderCmd::DRAW_QUAD_CENTERED;
	mem_move(Cmd.m_Quad, &Quad, sizeof(Cmd.m_Quad));
	m_aRenderCmdList[m_CurrentDrawSpace].add(Cmd);
}

void CDuckBridge::QueueDrawTeeBodyAndFeet(const CTeeDrawBodyAndFeetInfo& TeeDrawInfo)
{
	CRenderCmd Cmd;
	Cmd.m_Type = CDuckBridge::CRenderCmd::DRAW_TEE_BODYANDFEET;
	Cmd.m_TeeBodyAndFeet = TeeDrawInfo;
	m_aRenderCmdList[m_CurrentDrawSpace].add(Cmd);
}

void CDuckBridge::QueueDrawTeeHand(const CDuckBridge::CTeeDrawHand& Hand)
{
	CRenderCmd Cmd;
	Cmd.m_Type = CDuckBridge::CRenderCmd::DRAW_TEE_HAND;
	Cmd.m_TeeHand = Hand;
	m_aRenderCmdList[m_CurrentDrawSpace].add(Cmd);
}

bool CDuckBridge::LoadTexture(const char *pTexturePath, const char* pTextureName)
{
	IGraphics::CTextureHandle Handle = Graphics()->LoadTexture(pTexturePath, IStorage::TYPE_SAVE, CImageInfo::FORMAT_AUTO, 0);
	uint32_t Hash = hash_fnv1a(pTextureName, str_length(pTextureName));
	CTextureHashPair Pair = { Hash, Handle };
	m_aTextures.add(Pair);
	return Handle.IsValid();
}

IGraphics::CTextureHandle CDuckBridge::GetTexture(const char *pTextureName)
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

void CDuckBridge::RenderDrawSpace(DrawSpace::Enum Space)
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
			case CRenderCmd::DRAW_QUAD:
			case CRenderCmd::DRAW_FREEFORM: {
				if(RenderSpace.m_WantTextureID != RenderSpace.m_CurrentTextureID)
				{
					if(RenderSpace.m_WantTextureID < 0)
						Graphics()->TextureClear();
					else
						Graphics()->TextureSet(*(IGraphics::CTextureHandle*)&RenderSpace.m_WantTextureID);
					RenderSpace.m_CurrentTextureID = RenderSpace.m_WantTextureID;
				}
				Graphics()->TextureSet(*(IGraphics::CTextureHandle*)&RenderSpace.m_WantTextureID);

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
				else if(Cmd.m_Type == CRenderCmd::DRAW_FREEFORM)
					Graphics()->QuadsDrawFreeform((IGraphics::CFreeformItem*)&Cmd.m_pFreeFormQuads, Cmd.m_FreeFormQuadCount);
				else
					Graphics()->QuadsDrawTL((IGraphics::CQuadItem*)&Cmd.m_Quad, 1);

				Graphics()->QuadsEnd();
			} break;

			case CRenderCmd::DRAW_TEE_BODYANDFEET:
				DrawTeeBodyAndFeet(Cmd.m_TeeBodyAndFeet, RenderSpace.m_CurrentTeeSkin);

				// TODO: do this better
				mem_zero(RenderSpace.m_aWantColor, sizeof(RenderSpace.m_aWantColor));
				mem_zero(RenderSpace.m_aCurrentColor, sizeof(RenderSpace.m_aCurrentColor));
				mem_zero(RenderSpace.m_aWantQuadSubSet, sizeof(RenderSpace.m_aWantQuadSubSet));
				mem_zero(RenderSpace.m_aCurrentQuadSubSet, sizeof(RenderSpace.m_aCurrentQuadSubSet));
				RenderSpace.m_WantTextureID = -1; // clear by default
				RenderSpace.m_CurrentTextureID = 0;
				RenderSpace.m_WantQuadRotation = 0; // clear by default
				RenderSpace.m_CurrentQuadRotation = -1;
				break;

			case CRenderCmd::DRAW_TEE_HAND:
				DrawTeeHand(Cmd.m_TeeHand, RenderSpace.m_CurrentTeeSkin);

				// TODO: do this better
				mem_zero(RenderSpace.m_aWantColor, sizeof(RenderSpace.m_aWantColor));
				mem_zero(RenderSpace.m_aCurrentColor, sizeof(RenderSpace.m_aCurrentColor));
				mem_zero(RenderSpace.m_aWantQuadSubSet, sizeof(RenderSpace.m_aWantQuadSubSet));
				mem_zero(RenderSpace.m_aCurrentQuadSubSet, sizeof(RenderSpace.m_aCurrentQuadSubSet));
				RenderSpace.m_WantTextureID = -1; // clear by default
				RenderSpace.m_CurrentTextureID = 0;
				RenderSpace.m_WantQuadRotation = 0; // clear by default
				RenderSpace.m_CurrentQuadRotation = -1;
				break;

			default:
				dbg_assert(0, "Render command type not handled");
		}
	}

	m_aRenderCmdList[Space].set_size(0);
	RenderSpace = CRenderSpace();
}

void CDuckBridge::CharacterCorePreTick(CCharacterCore** apCharCores)
{
	if(!DuckJs()->IsLoaded())
		return;

	duk_context* pCtx = DuckJs()->Ctx();

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

void CDuckBridge::CharacterCorePostTick(CCharacterCore** apCharCores)
{
	if(!DuckJs()->IsLoaded())
		return;

	duk_context* pCtx = DuckJs()->Ctx();

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

void CDuckBridge::Predict(CWorldCore* pWorld)
{
	const int DiskCount = m_Collision.m_aDynamicDisks.size();
	for(int i = 0; i < DiskCount; i++)
	{
		m_Collision.m_aDynamicDisks[i].Tick(&m_Collision, pWorld);
	}
	for(int i = 0; i < DiskCount; i++)
	{
		m_Collision.m_aDynamicDisks[i].Move(&m_Collision, pWorld);
	}
}
