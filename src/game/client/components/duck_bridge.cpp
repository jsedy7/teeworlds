#include "duck_bridge.h"
#include "duck_js.h"
#include <game/client/animstate.h>
#include <game/client/render.h>
#include <game/client/components/skins.h>
#include <engine/external/pnglite/pnglite.h>
#include <engine/storage.h>
#include <engine/shared/network.h>
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
	m_CurrentPacketFlags = -1;
}

void CDuckBridge::Reset()
{
	for(int i = 0 ; i < m_aTextures.size(); i++)
	{
		Graphics()->UnloadTexture(&m_aTextures[i].m_Handle);
	}

	m_aTextures.clear();
	m_HudPartsShown = CHudPartsShown();
	m_CurrentPacketFlags = -1;

	// unload skin parts
	CSkins* pSkins = DuckJs()->m_pClient->m_pSkins;
	for(int i = 0; i < m_aSkinPartsToUnload.size(); i++)
	{
		const CSkinPartName& spn = m_aSkinPartsToUnload[i];
		int Index = pSkins->FindSkinPart(spn.m_Type, spn.m_aName, false);
		if(Index != -1)
			pSkins->RemoveSkinPart(spn.m_Type, Index);
	}
	m_aSkinPartsToUnload.clear();
}

void CDuckBridge::OnRender()
{
	m_FrameAllocator.Clear(); // clear frame allocator

	duk_context* pCtx = DuckJs()->Ctx();

	const float LocalTime = DuckJs()->Client()->LocalTime();
	const float IntraGameTick = DuckJs()->Client()->IntraGameTick();

	// Call OnRender(LocalTime, IntraGameTick)
	if(DuckJs()->GetJsFunction("OnRender"))
	{
		duk_push_number(pCtx, LocalTime);
		duk_push_number(pCtx, IntraGameTick);

		DuckJs()->CallJsFunction(2);

		duk_pop(pCtx);
	}

	static float LastTime = LocalTime;
	static float Accumulator = 0.0f;
	const float UPDATE_RATE = 1.0/60.0;

	Accumulator += LocalTime - LastTime;
	LastTime = LocalTime;

	int UpdateCount = 0;
	while(Accumulator > UPDATE_RATE)
	{
		// Call OnUpdate(LocalTime, IntraGameTick)
		if(DuckJs()->GetJsFunction("OnUpdate"))
		{
			duk_push_number(pCtx, LocalTime);
			duk_push_number(pCtx, IntraGameTick);

			DuckJs()->CallJsFunction(2);

			duk_pop(pCtx);
		}

		Accumulator -= UPDATE_RATE;
		UpdateCount++;

		if(UpdateCount > 2) {
			Accumulator = 0.0;
			break;
		}
	}
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

void CDuckBridge::QueueSetFreeform(const IGraphics::CFreeformItem *pFreeform, int FreeformCount)
{
	CRenderCmd Cmd;
	Cmd.m_Type = CRenderCmd::SET_FREEFORM_VERTICES;
	Cmd.m_pFreeformQuads = (float*)pFreeform;
	Cmd.m_FreeformQuadCount = FreeformCount;
	m_aRenderCmdList[m_CurrentDrawSpace].add(Cmd);
}

void CDuckBridge::QueueDrawFreeform(vec2 Pos)
{
	CRenderCmd Cmd;
	Cmd.m_Type = CRenderCmd::DRAW_FREEFORM;
	mem_move(Cmd.m_FreeformPos, &Pos, sizeof(float)*2);
	m_aRenderCmdList[m_CurrentDrawSpace].add(Cmd);
}

void CDuckBridge::SetHudPartsShown(CHudPartsShown hps)
{
	m_HudPartsShown = hps;
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

void CDuckBridge::PacketCreate(int NetID, int Flags)
{
	NetID = max(NetID, 0);
	// manual contructor here needed
	m_CurrentPacket.Reset();
	m_CurrentPacket.AddInt((NETMSG_DUCK_NETOBJ) << 1 | 1);
	m_CurrentPacket.AddInt(NetID);

	m_CurrentPacketFlags = Flags;
	dbg_assert(m_CurrentPacket.Size() < 16 && m_CurrentPacket.Size() > 0, "Hmm");
}

void CDuckBridge::PacketPackFloat(float f)
{
	if(m_CurrentPacketFlags == -1) {
		dbg_msg("duck", "ERROR: can't add float to undefined packet");
		return;
	}

	m_CurrentPacket.AddInt(f * 256);
}

void CDuckBridge::PacketPackInt(int i)
{
	if(m_CurrentPacketFlags == -1) {
		dbg_msg("duck", "ERROR: can't add int to undefined packet");
		return;
	}

	m_CurrentPacket.AddInt(i);
}

void CDuckBridge::PacketPackString(const char *pStr, int SizeLimit)
{
	if(m_CurrentPacketFlags == -1) {
		dbg_msg("duck", "ERROR: can't add string to undefined packet");
		return;
	}

	SizeLimit = clamp(SizeLimit, 0, 1024); // reasonable limits
	m_CurrentPacket.AddString(pStr, SizeLimit);
}

void CDuckBridge::SendPacket()
{
	if(m_CurrentPacketFlags == -1) {
		dbg_msg("duck", "ERROR: can't send undefined packet");
		return;
	}

	DuckJs()->Client()->SendMsg(&m_CurrentPacket, m_CurrentPacketFlags);
	m_CurrentPacket.Reset();
	m_CurrentPacketFlags = -1;
}

void CDuckBridge::AddSkinPart(const char *pPart, const char *pName, IGraphics::CTextureHandle Handle)
{
	int Type = -1;
	if(str_comp(pPart, "body") == 0) {
		Type = SKINPART_BODY;
	}
	else if(str_comp(pPart, "marking") == 0) {
		Type = SKINPART_MARKING;
	}
	else if(str_comp(pPart, "decoration") == 0) {
		Type = SKINPART_MARKING;
	}
	else if(str_comp(pPart, "hands") == 0) {
		Type = SKINPART_HANDS;
	}
	else if(str_comp(pPart, "feet") == 0) {
		Type = SKINPART_FEET;
	}
	else if(str_comp(pPart, "eyes") == 0) {
		Type = SKINPART_EYES;
	}

	if(Type == -1) {
		return; // part type not found
	}

	CSkins::CSkinPart SkinPart;
	SkinPart.m_OrgTexture = Handle;
	SkinPart.m_ColorTexture = Handle;
	SkinPart.m_BloodColor = vec3(1.0f, 0.0f, 0.0f);
	SkinPart.m_Flags = 0;

	char aPartName[256];
	str_truncate(aPartName, sizeof(aPartName), pName, str_length(pName) - 4);
	str_copy(SkinPart.m_aName, aPartName, sizeof(SkinPart.m_aName));

	DuckJs()->m_pClient->m_pSkins->AddSkinPart(Type, SkinPart);

	CSkinPartName SkinPartName;
	str_copy(SkinPartName.m_aName, SkinPart.m_aName, sizeof(SkinPartName.m_aName));
	SkinPartName.m_Type = Type;
	m_aSkinPartsToUnload.add(SkinPartName);
	// FIXME: breaks loaded "skins" (invalidates indexes)
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
			} break;

			case CRenderCmd::SET_TEE_SKIN: {
				RenderSpace.m_CurrentTeeSkin = Cmd.m_TeeSkinInfo;
			} break;

			case CRenderCmd::SET_FREEFORM_VERTICES: {
				RenderSpace.m_FreeformQuadCount = min(Cmd.m_FreeformQuadCount, CRenderSpace::FREEFORM_MAX_COUNT-1);
				mem_move(RenderSpace.m_aFreeformQuads, Cmd.m_pFreeformQuads, RenderSpace.m_FreeformQuadCount * sizeof(RenderSpace.m_aFreeformQuads[0]));
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
				{
					// TODO: is the position even useful here?
					IGraphics::CFreeformItem aTransFreeform[CRenderSpace::FREEFORM_MAX_COUNT];
					const vec2 FfPos(Cmd.m_FreeformPos[0], Cmd.m_FreeformPos[1]);

					// transform freeform object based on position
					for(int f = 0; f < RenderSpace.m_FreeformQuadCount; f++)
					{
						IGraphics::CFreeformItem& ff = aTransFreeform[f];
						IGraphics::CFreeformItem& rff = RenderSpace.m_aFreeformQuads[f];
						ff.m_X0 = rff.m_X0 + FfPos.x;
						ff.m_X1 = rff.m_X1 + FfPos.x;
						ff.m_X2 = rff.m_X2 + FfPos.x;
						ff.m_X3 = rff.m_X3 + FfPos.x;
						ff.m_Y0 = rff.m_Y0 + FfPos.y;
						ff.m_Y1 = rff.m_Y1 + FfPos.y;
						ff.m_Y2 = rff.m_Y2 + FfPos.y;
						ff.m_Y3 = rff.m_Y3 + FfPos.y;
					}
					Graphics()->QuadsDrawFreeform(aTransFreeform, RenderSpace.m_FreeformQuadCount);
				}
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

	if(!DuckJs()->GetJsFunction("OnCharacterCorePreTick")) {
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

	DuckJs()->CallJsFunction(2);

	if(!DuckJs()->HasJsFunctionReturned()) {
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

	if(!DuckJs()->GetJsFunction("OnCharacterCorePostTick")) {
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

	DuckJs()->CallJsFunction(2);

	if(!DuckJs()->HasJsFunctionReturned()) {
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
	// TODO: fix this
	/*const int DiskCount = m_Collision.m_aDynamicDisks.size();
	for(int i = 0; i < DiskCount; i++)
	{
		m_Collision.m_aDynamicDisks[i].Tick(&m_Collision, pWorld);
	}
	for(int i = 0; i < DiskCount; i++)
	{
		m_Collision.m_aDynamicDisks[i].Move(&m_Collision, pWorld);
	}*/
}
