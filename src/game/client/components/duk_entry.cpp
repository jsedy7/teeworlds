#include "duk_entry.h"

void CDukEntry::Init(IGraphics* pGraphics)
{
	m_pGraphics = pGraphics;
	m_CurrentDrawSpace = 0;
}

void CDukEntry::QueueSetColor(const float* pColor)
{
	CRenderCmd Cmd;
	Cmd.m_Type = CRenderCmd::COLOR;
	mem_move(Cmd.m_Color, pColor, sizeof(Cmd.m_Color));
	m_aRenderCmdList[m_CurrentDrawSpace].add(Cmd);
}

void CDukEntry::QueueDrawQuad(IGraphics::CQuadItem Quad)
{
	CRenderCmd Cmd;
	Cmd.m_Type = CDukEntry::CRenderCmd::QUAD;
	mem_move(Cmd.m_Quad, &Quad, sizeof(Cmd.m_Quad)); // yep, this is because we don't have c++11
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
			case CRenderCmd::COLOR: {
				const float* pColor = aCmds[i].m_Color;
				Graphics()->SetColor(pColor[0] * pColor[3], pColor[1] * pColor[3], pColor[2] * pColor[3], pColor[3]);
			} break;
			case CRenderCmd::QUAD:
				Graphics()->QuadsDrawTL((IGraphics::CQuadItem*)&aCmds[i].m_Quad, 1);
				break;
			default:
				dbg_assert(0, "Render command type not handled");
		}
	}

	Graphics()->QuadsEnd();

	m_aRenderCmdList[Space].clear();
}
