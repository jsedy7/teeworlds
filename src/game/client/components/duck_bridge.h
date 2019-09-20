#pragma once
#include <stdint.h>
#include <base/tl/array.h>
#include <engine/graphics.h>
#include <game/duck_collision.h>
#include <generated/protocol.h>

// Bridge between teeworlds and duktape

class CDuckJs;
class CRenderTools;
class CGameClient;
class CCharacterCore;

struct CMultiStackAllocator
{
	struct CStackBuffer
	{
		char* m_pBuffer;
		int m_Cursor;
	};

	enum {
		STACK_BUFFER_CAPACITY=1024*1024*10 // 10 KB
	};

	array<CStackBuffer> m_aStacks;
	int m_CurrentStack;

	CMultiStackAllocator();
	~CMultiStackAllocator();

	void* Alloc(int Size);
	void Clear();
};

struct CDuckBridge
{
	CDuckJs* m_pDuckJs;
	IGraphics* m_pGraphics;
	CRenderTools* m_pRenderTools;
	CGameClient* m_pGameClient;
	inline CDuckJs* DuckJs() { return m_pDuckJs; }
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

	struct CTeeDrawBodyAndFeetInfo
	{
		float m_Size;
		float m_Angle;
		float m_Pos[2]; // vec2
		bool m_IsWalking;
		bool m_IsGrounded;
		bool m_GotAirJump;
		int m_Emote;
	};

	struct CTeeDrawHand
	{
		float m_Size;
		float m_AngleDir;
		float m_AngleOff;
		float m_Pos[2]; // vec2
		float m_Offset[2]; // vec2
	};

	struct CTeeSkinInfo
	{
		int m_aTextures[NUM_SKINPARTS];
		float m_aColors[NUM_SKINPARTS][4];
	};

	struct CRenderCmd
	{
		enum TypeEnum
		{
			SET_COLOR=0,
			SET_TEXTURE,
			SET_QUAD_SUBSET,
			SET_QUAD_ROTATION,
			SET_TEE_SKIN,
			SET_FREEFORM_VERTICES,
			DRAW_QUAD,
			DRAW_QUAD_CENTERED,
			DRAW_TEE_BODYANDFEET,
			DRAW_TEE_HAND,
			DRAW_FREEFORM,
		};

		int m_Type;

		union
		{
			float m_Color[4];
			float m_Quad[4]; // POD IGraphics::CQuadItem
			int m_TextureID;
			float m_QuadSubSet[4];
			float m_QuadRotation;
			float m_FreeformPos[2];

			struct {
				const float* m_pFreeformQuads;
				int m_FreeformQuadCount;
			};

			// TODO: this is kinda big...
			CTeeDrawBodyAndFeetInfo m_TeeBodyAndFeet;
			CTeeDrawHand m_TeeHand;
			CTeeSkinInfo m_TeeSkinInfo;
		};
	};

	struct CRenderSpace
	{
		enum {
			FREEFORM_MAX_COUNT=256
		};

		float m_aWantColor[4];
		float m_aCurrentColor[4];
		float m_aWantQuadSubSet[4];
		float m_aCurrentQuadSubSet[4];
		int m_WantTextureID;
		int m_CurrentTextureID;
		float m_WantQuadRotation;
		float m_CurrentQuadRotation;
		CTeeSkinInfo m_CurrentTeeSkin;
		IGraphics::CFreeformItem m_aFreeformQuads[FREEFORM_MAX_COUNT];
		int m_FreeformQuadCount;

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
			m_FreeformQuadCount = 0;

			for(int i = 0; i < NUM_SKINPARTS; i++)
			{
				m_CurrentTeeSkin.m_aTextures[i] = -1;
				m_CurrentTeeSkin.m_aColors[i][0] = 1.0f;
				m_CurrentTeeSkin.m_aColors[i][1] = 1.0f;
				m_CurrentTeeSkin.m_aColors[i][2] = 1.0f;
				m_CurrentTeeSkin.m_aColors[i][3] = 1.0f;
			}
		}
	};

	CMultiStackAllocator m_FrameAllocator;

	int m_CurrentDrawSpace;
	array<CRenderCmd> m_aRenderCmdList[DrawSpace::_COUNT];
	CRenderSpace m_aRenderSpace[DrawSpace::_COUNT];

	struct CTextureHashPair
	{
		uint32_t m_Hash;
		IGraphics::CTextureHandle m_Handle;
	};

	array<CTextureHashPair> m_aTextures;

	CDuckCollision m_Collision;

	void DrawTeeBodyAndFeet(const CTeeDrawBodyAndFeetInfo& TeeDrawInfo, const CTeeSkinInfo& SkinInfo);
	void DrawTeeHand(const CTeeDrawHand& Hand, const CTeeSkinInfo& SkinInfo);

	void Init(CDuckJs* pDuckJs);
	void Reset();

	void OnRender();

	void QueueSetColor(const float* pColor);
	void QueueSetTexture(int TextureID);
	void QueueSetQuadSubSet(const float* pSubSet);
	void QueueSetQuadRotation(float Angle);
	void QueueSetTeeSkin(const CTeeSkinInfo& SkinInfo);
	void QueueSetFreeform(const IGraphics::CFreeformItem* pFreeform, int FreeformCount);
	void QueueDrawQuad(IGraphics::CQuadItem Quad);
	void QueueDrawQuadCentered(IGraphics::CQuadItem Quad);
	void QueueDrawTeeBodyAndFeet(const CTeeDrawBodyAndFeetInfo& TeeDrawInfo);
	void QueueDrawTeeHand(const CTeeDrawHand& Hand);
	void QueueDrawFreeform(vec2 Pos);

	bool LoadTexture(const char* pTexturePath, const char *pTextureName);
	IGraphics::CTextureHandle GetTexture(const char* pTextureName);

	// "entries"
	void RenderDrawSpace(DrawSpace::Enum Space);
	void CharacterCorePreTick(CCharacterCore** apCharCores);
	void CharacterCorePostTick(CCharacterCore** apCharCores);
	void Predict(CWorldCore *pWorld);
};
