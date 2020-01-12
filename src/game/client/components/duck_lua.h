#pragma once
#include <base/tl/array.h>
#include <base/vmath.h>
#include <engine/input.h>
#include <stdint.h>

struct CGrowBuffer;
struct CDuckBridge;
struct lua_State;
struct CNetObj_Character;
struct CNetObj_PlayerInfo;
struct CAnimState;
struct CTeeRenderInfo;
struct CWeaponSpriteInfo;

class CDuckLua
{
	CDuckBridge* m_pBridge;
	lua_State* m_pLuaState;
	char aLastCalledFunction[256];

	struct CScriptFile
	{
		uint32_t m_NameHash;
		char m_aName[128];
		const char* m_pFileData;
		int m_FileSize;
	};

	enum { SCRIPTFILE_MAXCOUNT = 128 };
	CScriptFile m_aScriptFiles[SCRIPTFILE_MAXCOUNT];
	bool m_aIsScriptLuaLoaded[SCRIPTFILE_MAXCOUNT];
	int m_ScriptFileCount;

	int m_FuncRefOnLoad;
	int m_FuncRefOnMessage;
	int m_FuncRefOnSnap;
	int m_FuncRefOnInput;
	int m_FuncRefOnRender;
	int m_FuncRefOnRenderPlayer;
	int m_FuncRefOnUpdate;
	int m_FuncRefOnBind;

	inline lua_State* L() { return m_pLuaState; }
	inline CDuckBridge* Bridge() { return m_pBridge; }

	bool _CheckArgumentCountImp(lua_State* L, int NumArgs, const char* pFuncName);
	void PrintToConsole(const char* pStr, int Len);

	static int NativePrint(lua_State *L);
	static int NativeRequire(lua_State *L);
	static int NativeCos(lua_State *L);
	static int NativeSin(lua_State *L);
	static int NativeSqrt(lua_State *L);
	static int NativeAtan2(lua_State *L);
	static int NativeFloor(lua_State *L);
	static int NativeFmod(lua_State *L);
	static int NativeAbs(lua_State *L);

	static int NativeAnd(lua_State *L);
	static int NativeOr(lua_State *L);
	static int NativeLShift(lua_State *L);
	static int NativeRShift(lua_State *L);

	static int NativeRenderQuad(lua_State *L);
	static int NativeRenderQuadCentered(lua_State *L);
	static int NativeRenderSetColorU32(lua_State *L);
	static int NativeRenderSetColorF4(lua_State *L);
	static int NativeRenderSetTexture(lua_State* L);
	static int NativeRenderSetQuadSubSet(lua_State* L);
	static int NativeRenderSetQuadRotation(lua_State* L);
	static int NativeRenderSetTeeSkin(lua_State* L);
	static int NativeRenderSetFreeform(lua_State* L);
	static int NativeRenderSetDrawSpace(lua_State* L);
	static int NativeRenderDrawTeeBodyAndFeet(lua_State* L);
	static int NativeRenderDrawTeeHand(lua_State* L);
	static int NativeRenderDrawFreeform(lua_State* L);
	static int NativeRenderDrawText(lua_State* L);
	static int NativeRenderDrawCircle(lua_State* L);
	static int NativeRenderDrawLine(lua_State* L);
	static int NativeGetBaseTexture(lua_State* L);
	static int NativeGetSpriteSubSet(lua_State* L);
	static int NativeGetSpriteScale(lua_State* L);
	static int NativeGetWeaponSpec(lua_State* L);
	static int NativeGetModTexture(lua_State* L);
	static int NativeGetClientSkinInfo(lua_State* L);
	static int NativeGetClientCharacterCores(lua_State* L);
	static int NativeGetStandardSkinInfo(lua_State* L);
	static int NativeGetSkinPartTexture(lua_State* L);
	static int NativeGetCursorPosition(lua_State* L);
	static int NativeGetUiScreenRect(lua_State* L);
	static int NativeGetScreenSize(lua_State* L);
	static int NativeGetCamera(lua_State* L);
	static int NativeGetUiMousePos(lua_State* L);
	static int NativeGetPixelScale(lua_State* L);
	static int NativePhysGetCores(lua_State* L);
	static int NativePhysGetJoints(lua_State* L);
	static int NativePhysSetTileCollisionFlags(lua_State* L);
	static int NativeDirectionFromAngle(lua_State* L);
	static int NativeCollisionSetStaticBlock(lua_State* L);
	static int NativeCollisionClearStaticBlock(lua_State* L);
	static int NativeSetHudPartsShown(lua_State* L);
	static int NativeNetSendPacket(lua_State* L);
	static int NativeNetPacketUnpack(lua_State* L);
	static int NativeAddWeapon(lua_State* L);
	static int NativePlaySoundAt(lua_State* L);
	static int NativePlaySoundGlobal(lua_State* L);
	static int NativePlayMusic(lua_State* L);
	static int NativeRandomInt(lua_State* L);
	static int NativeCalculateTextSize(lua_State* L);
	static int NativeSetMenuModeActive(lua_State* L);

	bool LoadScriptFile(const char *pFileName, const char* pFileData, int FileSize);
	void AddScriptFileItem(const char* pScriptFilename, const char* pFileData, int FileSize);
	int FindScriptFileFromName(const char* pScriptFilename);
	bool LuaLoadScriptFileData(int ScriptFileID);

	void ResetLuaState();
	void Reset();

	bool GetFunction(const char* Name);
	bool GetFunctionFromRef(int Ref, const char *Name);
	void CallFunction(int ArgCount, int ReturnCount);

	bool MakeVanillaLuaNetMessage(int MsgID, const void* pRawMsg);
	bool MakeVanillaLuaNetObj(int MsgID, const void* pRawMsg);
	void GetContentEnumsAsLua();

public:
	CDuckLua();
	void Shutdown();
	void OnMessage(int Msg, void *pRawMsg);
	void OnSnapItem(int Msg, int SnapID, void* pRawMsg);
	void OnDuckSnapItem(int Msg, int SnapID, void* pRawMsg, int Size);
	void OnInput(IInput::CEvent e);
	void OnModLoaded();
	bool OnRenderPlayer(CAnimState *pState, CTeeRenderInfo* pTeeInfo, vec2 Pos, vec2 Dir, int Emote, const CWeaponSpriteInfo *pWeaponSprite, const CNetObj_Character& PrevChara, const CNetObj_Character& CurChara, int ClientID);
	bool OnBind(int Stroke, const char* pCmd);

	inline bool IsLoaded() const { return m_pLuaState != NULL; }

	void OnRender(float LocalTime, float IntraGameTick);
	void OnUpdate(float LocalTime, float IntraGameTick);
	bool IsStackLeaking();

	friend class CDuckBridge;
};
