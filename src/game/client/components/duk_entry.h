#pragma once
#include <base/tl/array.h>
#include <engine/graphics.h>
#include <generated/protocol.h>

class CDuktape;
class CRenderTools;
class CGameClient;

struct CDukEntry
{
	IGraphics* m_pGraphics;
	CRenderTools* m_pRenderTools;
	CGameClient* m_pGameClient;
	inline IGraphics* Graphics() { return m_pGraphics; }
	inline CRenderTools* RenderTools() { return m_pRenderTools; }
	inline CGameClient* GameClient() { return m_pGameClient; }

	struct DrawSpace
	{
		enum Enum
		{
			GAME=0,
			_COUNT
		};
	};

	struct CTeeDrawInfo
	{
		float m_Size;
		float m_Pos[2]; // vec2
	};

	struct CRenderCmd
	{
		enum TypeEnum
		{
			SET_COLOR=0,
			DRAW_QUAD,
			DRAW_TEE
		};

		int m_Type;

		union
		{
			float m_Color[4];
			float m_Quad[4]; // POD IGraphics::CQuadItem

			// TODO: this is kinda big...
			CTeeDrawInfo m_Tee;
		};

		CRenderCmd() {}
	};

	int m_CurrentDrawSpace;
	array<CRenderCmd> m_aRenderCmdList[DrawSpace::_COUNT];

	void DrawTeeBodyAndFeet(const CTeeDrawInfo& TeeDrawInfo);

	void Init(CDuktape* pDuktape);

	void QueueSetColor(const float* pColor);
	void QueueDrawQuad(IGraphics::CQuadItem Quad);
	void QueueDrawTeeBodyAndFeet(const CTeeDrawInfo& TeeDrawInfo);

	void RenderDrawSpace(DrawSpace::Enum Space);
};
