#include "duk_entry.h"
#include "duktape_comp.h"
#include <game/client/animstate.h>
#include <game/client/render.h>

void CDukEntry::DrawTeeBodyAndFeet(const CTeeDrawInfo& TeeDrawInfo)
{
	CAnimState State;
	State.Set(&g_pData->m_aAnimations[ANIM_BASE], 0);
	CTeeRenderInfo RenderInfo = GameClient()->m_aClients[GameClient()->m_LocalClientID].m_RenderInfo;
	RenderInfo.m_Size = TeeDrawInfo.m_Size;

	vec2 Direction = vec2(1, 0);
	vec2 Pos = vec2(TeeDrawInfo.m_Pos[0], TeeDrawInfo.m_Pos[1]);
	int Emote = -1;

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

void CDukEntry::Init(CDuktape* pDuktape)
{
	m_pGraphics = pDuktape->Graphics();
	m_pRenderTools = pDuktape->RenderTools();
	m_pGameClient = pDuktape->m_pClient;
	m_CurrentDrawSpace = 0;
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

void CDukEntry::QueueSetTextureUV(const float* pUV)
{
	CRenderCmd Cmd;
	Cmd.m_Type = CRenderCmd::SET_TEXTURE_UV;
	mem_move(Cmd.m_TextureUV, pUV, sizeof(Cmd.m_TextureUV));
	m_aRenderCmdList[m_CurrentDrawSpace].add(Cmd);
}

void CDukEntry::QueueDrawQuad(IGraphics::CQuadItem Quad)
{
	CRenderCmd Cmd;
	Cmd.m_Type = CDukEntry::CRenderCmd::DRAW_QUAD;
	mem_move(Cmd.m_Quad, &Quad, sizeof(Cmd.m_Quad)); // yep, this is because we don't have c++11
	m_aRenderCmdList[m_CurrentDrawSpace].add(Cmd);
}

void CDukEntry::QueueDrawTeeBodyAndFeet(const CTeeDrawInfo& TeeDrawInfo)
{
	CRenderCmd Cmd;
	Cmd.m_Type = CDukEntry::CRenderCmd::DRAW_TEE_BODYANDFEET;
	Cmd.m_TeeBodyAndFeet = TeeDrawInfo;
	m_aRenderCmdList[m_CurrentDrawSpace].add(Cmd);
}

void CDukEntry::RenderDrawSpace(DrawSpace::Enum Space)
{
	const int CmdCount = m_aRenderCmdList[Space].size();
	const CRenderCmd* aCmds = m_aRenderCmdList[Space].base_ptr();

	// TODO: merge CRenderSpace and DrawSpace
	CRenderSpace& RenderSpace = m_aRenderSpace[Space];
	float* pWantColor = RenderSpace.m_aWantColor;
	float* pCurrentColor = RenderSpace.m_aCurrentColor;
	int& rWantTextureID = RenderSpace.m_WantTextureID;
	int& rCurrentTextureID = RenderSpace.m_CurrentTextureID;

	for(int i = 0; i < CmdCount; i++)
	{
		const CRenderCmd& Cmd = aCmds[i];

		switch(Cmd.m_Type)
		{
			case CRenderCmd::SET_COLOR: {
				mem_move(pWantColor, Cmd.m_Color, sizeof(Cmd.m_Color));
			} break;

			case CRenderCmd::SET_TEXTURE: {
				rWantTextureID = Cmd.m_TextureID;
			} break;

			case CRenderCmd::DRAW_QUAD: {
				if(rWantTextureID != rCurrentTextureID)
				{
					if(rWantTextureID < 0)
						Graphics()->TextureClear();
					else
						Graphics()->TextureSet(*(IGraphics::CTextureHandle*)&rWantTextureID);
					rCurrentTextureID = rWantTextureID;
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

				Graphics()->QuadsDrawTL((IGraphics::CQuadItem*)&Cmd.m_Quad, 1);
				Graphics()->QuadsEnd();
			} break;

			case CRenderCmd::DRAW_TEE_BODYANDFEET:
				DrawTeeBodyAndFeet(Cmd.m_TeeBodyAndFeet);
				break;

			default:
				dbg_assert(0, "Render command type not handled");
		}
	}

	m_aRenderCmdList[Space].clear();
	RenderSpace = CRenderSpace();
}
