#pragma once
#include <base/tl/array.h>
#include <engine/client/duktape.h>
#include <engine/input.h>
#include <generated/protocol.h>
#include <game/gamecore.h>

struct CGrowBuffer;
struct CDuckBridge;

bool HttpRequestPage(const char* pUrl, CGrowBuffer* pHttpBuffer);

class CDuckJs
{
	CDuckBridge* m_pBridge;
	duk_context* m_pDukContext;
	int m_CurrentPushedObjID;
	char aLastCalledFunction[256];

	inline duk_context* Ctx() { return m_pDukContext; }
	inline CDuckBridge* Bridge() { return m_pBridge; }

	static duk_ret_t NativeRenderQuad(duk_context *ctx);
	static duk_ret_t NativeRenderQuadCentered(duk_context *ctx);
	static duk_ret_t NativeRenderSetColorU32(duk_context *ctx);
	static duk_ret_t NativeRenderSetColorF4(duk_context *ctx);
	static duk_ret_t NativeRenderSetTexture(duk_context *ctx);
	static duk_ret_t NativeRenderSetQuadSubSet(duk_context *ctx);
	static duk_ret_t NativeRenderSetQuadRotation(duk_context *ctx);
	static duk_ret_t NativeRenderSetTeeSkin(duk_context *ctx);
	static duk_ret_t NativeRenderSetFreeform(duk_context *ctx);
	static duk_ret_t NativeRenderSetDrawSpace(duk_context *ctx);
	static duk_ret_t NativeRenderDrawTeeBodyAndFeet(duk_context *ctx);
	static duk_ret_t NativeRenderDrawTeeHand(duk_context *ctx);
	static duk_ret_t NativeRenderDrawFreeform(duk_context *ctx);
	static duk_ret_t NativeRenderDrawText(duk_context *ctx);
	static duk_ret_t NativeGetBaseTexture(duk_context *ctx);
	static duk_ret_t NativeGetSpriteSubSet(duk_context *ctx);
	static duk_ret_t NativeGetSpriteScale(duk_context *ctx);
	static duk_ret_t NativeGetWeaponSpec(duk_context *ctx);
	static duk_ret_t NativeGetModTexture(duk_context *ctx);
	static duk_ret_t NativeGetClientSkinInfo(duk_context *ctx);
	static duk_ret_t NativeGetClientCharacterCores(duk_context *ctx);
	static duk_ret_t NativeGetStandardSkinInfo(duk_context *ctx);
	static duk_ret_t NativeGetSkinPartTexture(duk_context *ctx);
	static duk_ret_t NativeGetCursorPosition(duk_context *ctx);
	static duk_ret_t NativeMapSetTileCollisionFlags(duk_context *ctx);
	static duk_ret_t NativeDirectionFromAngle(duk_context *ctx);
	static duk_ret_t NativeCollisionSetStaticBlock(duk_context *ctx);
	static duk_ret_t NativeCollisionClearStaticBlock(duk_context *ctx);
	static duk_ret_t NativeCollisionSetDynamicDisk(duk_context *ctx);
	static duk_ret_t NativeCollisionClearDynamicDisk(duk_context *ctx);
	static duk_ret_t NativeCollisionGetPredictedDynamicDisks(duk_context *ctx);
	static duk_ret_t NativeSetHudPartsShown(duk_context *ctx);
	static duk_ret_t NativeNetSendPacket(duk_context *ctx);
	static duk_ret_t NativeNetPacketUnpack(duk_context *ctx);
	static duk_ret_t NativeAddWeapon(duk_context *ctx);
	static duk_ret_t NativePlaySoundAt(duk_context *ctx);
	static duk_ret_t NativePlaySoundGlobal(duk_context *ctx);
	static duk_ret_t NativePlayMusic(duk_context *ctx);
	static duk_ret_t NativeRandomInt(duk_context *ctx);

	// TODO: not great if we want to do nested objects
	void PushObject();
	void ObjectSetMemberInt(const char* MemberName, int Value);
	void ObjectSetMemberUint(const char* MemberName, uint32_t Value);
	void ObjectSetMemberFloat(const char* MemberName, float Value);
	void ObjectSetMemberRawBuffer(const char* MemberName, const void* pRawBuffer, int RawBufferSize);
	void ObjectSetMemberString(const char* MemberName, const char* pStr);
	void ObjectSetMemberBool(const char *MemberName, bool Val);
	void ObjectSetMember(const char* MemberName);

	bool LoadJsScriptFile(const char* pJsFilePath, const char* pJsRelFilePath);

	void ResetDukContext();

	bool GetJsFunction(const char* Name);
	void CallJsFunction(int NumArgs);
	bool HasJsFunctionReturned();

	bool MakeVanillaJsNetObj(int MsgID, void* pRawMsg);
	const char* GetContentEnumsAsJs();

public:
	CDuckJs();
	void Shutdown();
	void OnMessage(int Msg, void* pRawMsg);
	void OnInput(IInput::CEvent e);
	void OnModLoaded();

	friend class CDuckBridge;
};

inline void DukGetIntProp(duk_context* pCtx, duk_idx_t ObjIdx, const char* pPropName, int* pOutInt)
{
	if(duk_get_prop_string(pCtx, ObjIdx, pPropName))
	{
		*pOutInt = duk_to_int(pCtx, -1);
		duk_pop(pCtx);
	}
}

inline void DukGetFloatProp(duk_context* pCtx, duk_idx_t ObjIdx, const char* pPropName, float* pOutFloat)
{
	if(duk_get_prop_string(pCtx, ObjIdx, pPropName))
	{
		*pOutFloat = duk_to_number(pCtx, -1);
		duk_pop(pCtx);
	}
}

inline void DukGetStringProp(duk_context* pCtx, duk_idx_t ObjIdx, const char* pPropName, char* pOutBuff, int OutSize)
{
	if(duk_get_prop_string(pCtx, ObjIdx, pPropName))
	{
		const char* pStr = duk_to_string(pCtx, -1);
		str_copy(pOutBuff, pStr, OutSize);
		duk_pop(pCtx);
	}
}

inline void DukGetStringPropNoCopy(duk_context* pCtx, duk_idx_t ObjIdx, const char* pPropName, const char** pOutStr)
{
	if(duk_get_prop_string(pCtx, ObjIdx, pPropName))
	{
		*pOutStr = duk_to_string(pCtx, -1);
		duk_pop(pCtx);
		return;
	}
	*pOutStr = 0;
}

inline bool DukIsPropNull(duk_context* pCtx, duk_idx_t ObjIdx, const char* pPropName)
{
	bool IsNull = true;
	if(duk_get_prop_string(pCtx, ObjIdx, pPropName))
	{
		IsNull &= duk_is_null_or_undefined(pCtx, -1);
		duk_pop(pCtx);
	}
	return IsNull;
}

inline void DukSetIntProp(duk_context* pCtx, duk_idx_t ObjIdx, const char* pPropName, int Val)
{
	duk_push_int(pCtx, Val);
	duk_put_prop_string(pCtx, ObjIdx, pPropName);
}

inline void DukSetFloatProp(duk_context* pCtx, duk_idx_t ObjIdx, const char* pPropName, float Val)
{
	duk_push_number(pCtx, Val);
	duk_put_prop_string(pCtx, ObjIdx, pPropName);
}

duk_idx_t DuktapePushCharacterCore(duk_context* pCtx, const CCharacterCore* pCharCore);
duk_idx_t DuktapePushNetObjPlayerInput(duk_context* pCtx, const CNetObj_PlayerInput* pInput);
void DuktapeReadCharacterCore(duk_context* pCtx, duk_idx_t ObjIdx, CCharacterCore* pOutCharCore);
void DuktapeReadNetObjPlayerInput(duk_context* pCtx, duk_idx_t ObjIdx, CNetObj_PlayerInput* pOutInput);
