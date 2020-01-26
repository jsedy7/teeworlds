#pragma once
#include <stdint.h>
#include <base/tl/array.h>
#include <base/tl/sorted_array.h>
#include <engine/graphics.h>
#include <engine/sound.h>
#include <engine/shared/snapshot.h>
#include <engine/shared/duck_modfile.h>
#include <game/duck_collision.h>
#include <game/duck_gamecore.h>
#include <game/client/component.h>
#include <game/client/animstate.h>
#include <generated/protocol.h>

#ifdef DUCK_LUA_BACKEND
#include "duck_lua.h"
#endif

#define DUCK_VERSION 0x1

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t i32;

// Bridge between teeworlds and duktape

class CRenderTools;
class CGameClient;
class CCharacterCore;
class CTeeRenderInfo;
struct CGrowBuffer;

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

struct JsErrorLvl
{
	enum Enum {
		WARNING=0,
		ERROR,
		CRITICAL
	};
};

struct CWeaponSpriteInfo
{
	int m_ID;
	float m_Recoil;
};

struct CDuckBridge : public CComponent
{
#ifdef DUCK_JS_BACKEND
	CDuckJs m_Backend;
#endif

#ifdef DUCK_LUA_BACKEND
	CDuckLua m_Backend;
#endif

	struct DrawSpace
	{
		enum Enum
		{
			GAME=0,
			GAME_FOREGROUND,
			HUD,
			PLAYER,
			PLAYER_END = PLAYER + MAX_CLIENTS-1,
			_COUNT
		};
	};

	struct CTextInfo
	{
		const char* m_pStr;
		float m_aColors[4];
		float m_FontSize;
		float m_LineWidth;
		float m_aPos[2];
		float m_aClip[4];
	};

	struct CRenderCmd
	{
		enum TypeEnum
		{
			SET_COLOR=0,
			SET_TEXTURE,
			SET_QUAD_SUBSET,
			SET_QUAD_ROTATION,
			SET_FREEFORM_VERTICES,
			SET_TEXTURE_WRAP,
			DRAW_QUAD,
			DRAW_QUAD_CENTERED,
			DRAW_FREEFORM,
			DRAW_TEXT,
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
			int m_TextureWrap; // 0 = clamp, 1 = repeat

			struct {
				const float* m_pFreeformQuads;
				int m_FreeformQuadCount;
			};

			CTextInfo m_Text;
		};
	};

	struct CRenderSpace
	{
		enum {
			FREEFORM_MAX_COUNT=256,
			WRAP_CLAMP = 0,
			WRAP_REPEAT = 1,
		};

		float m_aWantColor[4];
		float m_aWantQuadSubSet[4];
		int m_WantTextureID;
		float m_WantQuadRotation;
		IGraphics::CFreeformItem m_aFreeformQuads[FREEFORM_MAX_COUNT];
		int m_FreeformQuadCount;
		int m_TextureWrap;

		CRenderSpace()
		{
			mem_zero(m_aWantColor, sizeof(m_aWantColor));
			m_WantTextureID = -1; // clear by default
			m_WantQuadRotation = 0; // clear by default
			m_FreeformQuadCount = 0;
			m_aWantQuadSubSet[0] = 0;
			m_aWantQuadSubSet[1] = 0;
			m_aWantQuadSubSet[2] = 1;
			m_aWantQuadSubSet[3] = 1;
			m_TextureWrap = WRAP_CLAMP;
		}
	};

	CDuckModFileExtracted m_ModFiles;
	bool m_IsModLoaded;
	bool m_DoUnloadModBecauseError;
	CMultiStackAllocator m_FrameAllocator; // holds data for a frame

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

	struct CHudPartsShown
	{
		// TODO: make those bools?
		bool m_Health;
		bool m_Armor;
		bool m_Ammo;
		bool m_Time;
		bool m_KillFeed;
		bool m_Score;
		bool m_Chat;
		bool m_Scoreboard;
		bool m_WeaponCursor;

		CHudPartsShown() {
			m_Health = true;
			m_Armor = true;
			m_Ammo = true;
			m_Time = true;
			m_KillFeed = true;
			m_Score = true;
			m_Chat = true;
			m_Scoreboard = true;
			m_WeaponCursor = true;
		}
	};

	CHudPartsShown m_HudPartsShown;
	CMsgPacker m_CurrentPacket;
	int m_CurrentPacketFlags;

	struct CSkinPartName
	{
		char m_aName[24];
		int m_Type;
	};

	array<CSkinPartName> m_aSkinPartsToUnload;

	struct CWeaponCustomJs
	{
		int WeaponID;
		char aTexWeapon[64];
		char aTexCursor[64];
		float WeaponX;
		float WeaponY;
		float WeaponSizeX;
		float WeaponSizeY;
		float HandX;
		float HandY;
		float HandAngle;
		float Recoil;
	};

	struct CWeaponCustom
	{
		int WeaponID;
		IGraphics::CTextureHandle TexWeaponHandle;
		IGraphics::CTextureHandle TexCursorHandle;
		vec2 WeaponPos;
		vec2 WeaponSize;
		vec2 HandPos;
		float HandAngle;
		float Recoil;

		bool operator < (const CWeaponCustom& other) {
			return WeaponID < other.WeaponID;
		}

		bool operator == (const CWeaponCustom& other) {
			return WeaponID == other.WeaponID;
		}
	};

	sorted_array<CWeaponCustom> m_aWeapons;

	struct CSoundHashPair
	{
		uint32_t m_Hash;
		ISound::CSampleHandle m_Handle;
	};

	array<CSoundHashPair> m_aSounds;

	class CRenderGroup : public CComponent
	{
	protected:
		CDuckBridge* m_pBridge;
		DrawSpace::Enum m_DrawSpace;

	public:
		void Init(CDuckBridge* pBridge, DrawSpace::Enum DrawSpace)
		{
			m_pBridge = pBridge;
			m_DrawSpace = DrawSpace;
		}
		virtual void OnRender()
		{
			if(m_pBridge->IsLoaded()) {
				m_pBridge->RenderDrawSpace(m_DrawSpace);
			}
		}
	};

	class CRenderGroupHud : public CRenderGroup
	{
	public:
		virtual void OnRender()
		{
			if(m_pBridge->IsLoaded()) {
				CUIRect Rect = *UI()->Screen();
				Graphics()->MapScreen(0.0f, 0.0f, Rect.w, Rect.h);
				m_pBridge->RenderDrawSpace(m_DrawSpace);
			}
		}
	};

	class CRenderGroupJsErrors : public CRenderGroup
	{
	public:
		virtual void OnRender();
	};

	CRenderGroup m_RgGame;
	CRenderGroup m_RgGameForeGround;
	CRenderGroupHud m_RgHud;
	CRenderGroupJsErrors m_RgJsErrors;

	vec2 m_MousePos;
	bool m_IsMenuModeActive;

	struct CJsErrorStr
	{
		float m_Time;
		int m_Level;
		char m_aText[1024];
	};

	array<CJsErrorStr> m_aJsErrors;

	CDuckWorldCore m_WorldCorePredicted;
	CDuckWorldCore m_WorldCorePredictedPrev;

	struct CSnapState
	{
		array<CNetObj_DuckCustomCore> m_aCustomCores;
		array<CNetObj_DuckPhysJoint> m_aJoints;
		array<CNetObj_DuckPhysicsLawsGroup> m_aPhysicsLawsGroups;
		CNetObj_DuckCharCoreExtra m_aCharCoreExtra[MAX_CLIENTS];

		void Clear()
		{
			m_aCustomCores.set_size(0);
			m_aJoints.set_size(0);
			m_aPhysicsLawsGroups.set_size(0);
		}
	};

	CSnapState m_SnapPrev;
	CSnapState m_Snap;

	CDuckBridge();

	void Reset();

	void QueueSetColor(const float* pColor);
	void QueueSetTexture(int TextureID);
	void QueueSetQuadSubSet(const float* pSubSet);
	void QueueSetQuadRotation(float Angle);
	void QueueSetFreeform(const IGraphics::CFreeformItem* pFreeform, int FreeformCount);
	void QueueDrawQuad(IGraphics::CQuadItem Quad);
	void QueueDrawQuadCentered(IGraphics::CQuadItem Quad);
	void QueueDrawFreeform(vec2 Pos);
	void QueueDrawText(const char* pStr, float FontSize, float LineWidth, float *pPos, float *pClip, float *pColors);
	void QueueDrawCircle(vec2 Pos, float Radius);
	void QueueDrawLine(vec2 Pos1, vec2 Pos2, float Thickness);

	void SetHudPartsShown(CHudPartsShown hps);

	bool LoadTexture(const char* pTexturePath, const char *pTextureName);
	bool LoadTextureRaw(const char* pTextureName, const char* pFileData, int FileSize);
	IGraphics::CTextureHandle GetTextureFromName(const char* pTextureName);

	void PacketCreate(int NetID, int Flags);
	void PacketPackFloat(float f);
	void PacketPackInt(int i);
	void PacketPackString(const char* pStr, int SizeLimit);
	void SendPacket();

	void AddSkinPart(const char* pPart, const char* pName, IGraphics::CTextureHandle Handle);
	void AddWeapon(const CWeaponCustomJs& Wc);
	CWeaponCustom* FindWeapon(int WeaponID);

	void PlaySoundAt(const char* pSoundName, float x, float y);
	void PlaySoundGlobal(const char* pSoundName);
	void PlayMusic(const char* pSoundName);

	CUIRect GetUiScreenRect() const;
	vec2 GetScreenSize() const;
	vec2 GetPixelScale() const;
	vec2 GetCameraPos() const;
	float GetCameraZoom() const;
	vec2 GetUiMousePos() const;
	int GetBaseTextureHandle(int ImgID) const;
	void GetBaseSpriteSubset(int SpriteID, float *pSubSet) const;
	void GetBaseSpriteScale(int SpriteID, float* pOutScale) const;
	bool GetSkinPart(int PartID, const char *pPartName, IGraphics::CTextureHandle *pOrgText, IGraphics::CTextureHandle *pColorText) const;
	vec2 GetLocalCursorPos() const;

	void SetMenuModeActive(bool Active);

	vec2 CalculateTextSize(const char* pStr, float FontSize, float LineWidth);

	void ScriptError(int ErrorLevel, const char* format, ...);

	bool RenderSetDrawSpace(int Space);
	void RenderDrawSpace(int Space);
	void CharacterCorePreTick(CCharacterCore** apCharCores);
	void CharacterCorePostTick(CCharacterCore** apCharCores);
	void Predict(CWorldCore *pWorld);
	void RenderPlayerWeapon(int WeaponID, vec2 Pos, float AttachAngle, float Angle, CTeeRenderInfo *pRenderInfo, float RecoilAlpha);
	void RenderWeaponCursor(int WeaponID, vec2 Pos);
	void RenderWeaponAmmo(int WeaponID, vec2 Pos);

	void OnNewSnapshot();
	bool OnRenderPlayer(const CNetObj_Character *pPrevChar, const CNetObj_Character *pPlayerChar, const CNetObj_PlayerInfo *pPrevInfo, const CNetObj_PlayerInfo *pPlayerInfo, int ClientID);
	void OnUpdatePlayer(const CNetObj_Character *pPrevChar, const CNetObj_Character *pPlayerChar, const CNetObj_PlayerInfo *pPrevInfo, const CNetObj_PlayerInfo *pPlayerInfo, int ClientID);
	bool OnBind(int Stroke, const char* pCmd);

	// mod installation
	bool IsModAlreadyInstalled(const SHA256_DIGEST* pModSha256);
	bool ExtractAndInstallModZipBuffer(const CGrowBuffer* pHttpZipData, const SHA256_DIGEST* pModSha256);
	bool ExtractAndInstallModCompressedBuffer(const void* pCompBuff, int CompBuffSize, const SHA256_DIGEST* pModSha256);
	bool LoadModFilesFromDisk(const SHA256_DIGEST* pModSha256);
	bool TryLoadInstalledDuckMod(const SHA256_DIGEST* pModSha256);
	bool InstallAndLoadDuckModFromModFile(const void* pBuffer, int BufferSize, const SHA256_DIGEST* pModSha256);

	virtual void OnInit();
	virtual void OnReset();
	virtual void OnShutdown();
	virtual void OnRender();
	virtual void OnMessage(int Msg, void *pRawMsg);
	virtual bool OnMouseMove(float x, float y);
	virtual void OnStateChange(int NewState, int OldState);
	virtual bool OnInput(IInput::CEvent e);

	void ModInit();
	void Unload();
	inline bool IsLoaded() const { return m_IsModLoaded; }

	inline int DuckVersion() const { return DUCK_VERSION; }

	friend class CDuckJs;
	friend class CDuckLua;
};

inline uint32_t hash_fnv1a(const void* pData, int DataSize)
{
	uint32_t Hash = 0x811c9dc5;
	for(int i = 0; i < DataSize; i++)
	{
		Hash ^= ((uint8_t*)pData)[i];
		Hash *= 16777619;
	}
	return Hash;
}
