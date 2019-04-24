#pragma once
#include <base/tl/array.h>
#include <engine/graphics.h>

struct CDukEntry
{
	IGraphics* m_pGraphics;
	inline IGraphics* Graphics() { return m_pGraphics; }

	struct DrawSpace
	{
		enum Enum
		{
			GAME=0,
			_COUNT
		};
	};

	struct CRenderCmd
	{
		enum TypeEnum
		{
			COLOR=0,
			QUAD
		};

		int m_Type;

		union
		{
			float m_Color[4];
			float m_Quad[4]; // POD IGraphics::CQuadItem
		};

		CRenderCmd() {}
	};

	int m_CurrentDrawSpace;
	array<CRenderCmd> m_aRenderCmdList[DrawSpace::_COUNT];

	void Init(IGraphics* pGraphics);

	void QueueSetColor(const float* pColor);
	void QueueDrawQuad(IGraphics::CQuadItem Quad);

	void RenderDrawSpace(DrawSpace::Enum Space);
};
