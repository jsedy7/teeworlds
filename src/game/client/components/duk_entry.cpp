#include "duk_entry.h"
#include "duktape_comp.h"
#include <game/client/animstate.h>
#include <game/client/render.h>

void CDukEntry::DrawTeeBodyAndFeet(const CDukEntry::CTeeDrawInfo& TeeDrawInfo)
{
	CAnimState State;
	State.Set(&g_pData->m_aAnimations[ANIM_BASE], 0);
	CTeeRenderInfo RenderInfo = GameClient()->m_aClients[GameClient()->m_LocalClientID].m_RenderInfo;
	RenderInfo.m_Size = TeeDrawInfo.m_Size;

	vec2 Direction = vec2(1, 0);
	int Emote = -1;
	RenderTools()->RenderTee(&State, &RenderInfo, Emote, Direction, *(vec2*)TeeDrawInfo.m_Pos);
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

void CDukEntry::QueueDrawQuad(IGraphics::CQuadItem Quad)
{
	CRenderCmd Cmd;
	Cmd.m_Type = CDukEntry::CRenderCmd::DRAW_QUAD;
	mem_move(Cmd.m_Quad, &Quad, sizeof(Cmd.m_Quad)); // yep, this is because we don't have c++11
	m_aRenderCmdList[m_CurrentDrawSpace].add(Cmd);
}

void CDukEntry::QueueDrawTeeBodyAndFeet(const CDukEntry::CTeeDrawInfo& TeeDrawInfo)
{
	CRenderCmd Cmd;
	Cmd.m_Type = CDukEntry::CRenderCmd::DRAW_TEE;
	Cmd.m_Tee = TeeDrawInfo;
	m_aRenderCmdList[m_CurrentDrawSpace].add(Cmd);
}

void CDukEntry::RenderDrawSpace(DrawSpace::Enum Space)
{
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();

	const int CmdCount = m_aRenderCmdList[Space].size();
	const CRenderCmd* aCmds = m_aRenderCmdList[Space].base_ptr();

	for(int i = 0; i < CmdCount; i++)
	{
		switch(aCmds[i].m_Type)
		{
			case CRenderCmd::SET_COLOR: {
				const float* pColor = aCmds[i].m_Color;
				Graphics()->SetColor(pColor[0] * pColor[3], pColor[1] * pColor[3], pColor[2] * pColor[3], pColor[3]);
			} break;
			case CRenderCmd::DRAW_QUAD:
				Graphics()->QuadsDrawTL((IGraphics::CQuadItem*)&aCmds[i].m_Quad, 1);
				break;
			case CRenderCmd::DRAW_TEE:

				break;
			default:
				dbg_assert(0, "Render command type not handled");
		}
	}

	Graphics()->QuadsEnd();

	m_aRenderCmdList[Space].clear();
}
