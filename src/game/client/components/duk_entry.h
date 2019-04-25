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
		float m_Angle;
		float m_Pos[2]; // vec2
		bool m_IsWalking;
		bool m_IsGrounded;
		bool m_GotAirJump;
		int m_Emote;
	};

	struct CRenderCmd
	{
		enum TypeEnum
		{
			SET_COLOR=0,
			SET_TEXTURE,
			SET_QUAD_SUBSET,
			SET_QUAD_ROTATION,
			DRAW_QUAD,
			DRAW_TEE_BODYANDFEET
		};

		int m_Type;

		union
		{
			float m_Color[4];
			float m_Quad[4]; // POD IGraphics::CQuadItem
			int m_TextureID;
			float m_QuadSubSet[4];
			float m_QuadRotation;

			// TODO: this is kinda big...
			CTeeDrawInfo m_TeeBodyAndFeet;
		};
	};

	struct CRenderSpace
	{
		float m_aWantColor[4];
		float m_aCurrentColor[4];
		float m_aWantQuadSubSet[4];
		float m_aCurrentQuadSubSet[4];
		int m_WantTextureID;
		int m_CurrentTextureID;
		float m_WantQuadRotation;
		float m_CurrentQuadRotation;

		CRenderSpace()
		{
			mem_zero(m_aWantColor, sizeof(m_aWantColor));
			mem_zero(m_aCurrentColor, sizeof(m_aCurrentColor));
			mem_zero(m_aWantQuadSubSet, sizeof(m_aWantQuadSubSet));
			mem_zero(m_aCurrentQuadSubSet, sizeof(m_aCurrentQuadSubSet));
			m_WantTextureID = -1; // clear by default
			m_CurrentTextureID = 0;
			m_WantQuadRotation = 0; // clear by default
			m_CurrentQuadRotation = -1;
		}
	};

	int m_CurrentDrawSpace;
	array<CRenderCmd> m_aRenderCmdList[DrawSpace::_COUNT];
	CRenderSpace m_aRenderSpace[DrawSpace::_COUNT];

	void DrawTeeBodyAndFeet(const CTeeDrawInfo& TeeDrawInfo);

	void Init(CDuktape* pDuktape);

	void QueueSetColor(const float* pColor);
	void QueueSetTexture(int TextureID);
	void QueueSetQuadSubSet(const float* pSubSet);
	void QueueSetQuadRotation(float Angle);
	void QueueDrawQuad(IGraphics::CQuadItem Quad);
	void QueueDrawTeeBodyAndFeet(const CTeeDrawInfo& TeeDrawInfo);

	void RenderDrawSpace(DrawSpace::Enum Space);
};
