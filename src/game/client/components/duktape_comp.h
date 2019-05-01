#pragma once
#include <base/tl/array.h>
#include <engine/client/duktape.h>
#include <game/client/component.h>
#include <engine/graphics.h>

#include "duk_entry.h"

struct HttpBuffer;
struct CGrowBuffer;

class CDuktape : public CComponent
{
	duk_context* m_pDukContext;
	int m_CurrentPushedObjID;
	bool m_IsModLoaded;
	inline duk_context* Ctx() { return m_pDukContext; }

	static duk_ret_t NativeRenderQuad(duk_context *ctx);
	static duk_ret_t NativeRenderQuadCentered(duk_context *ctx);
	static duk_ret_t NativeRenderSetColorU32(duk_context *ctx);
	static duk_ret_t NativeRenderSetColorF4(duk_context *ctx);
	static duk_ret_t NativeRenderSetTexture(duk_context *ctx);
	static duk_ret_t NativeRenderSetQuadSubSet(duk_context *ctx);
	static duk_ret_t NativeRenderSetQuadRotation(duk_context *ctx);
	static duk_ret_t NativeRenderSetTeeSkin(duk_context *ctx);
	static duk_ret_t NativeSetDrawSpace(duk_context *ctx);
	static duk_ret_t NativeRenderDrawTeeBodyAndFeet(duk_context *ctx);
	static duk_ret_t NativeRenderDrawTeeHand(duk_context *ctx);
	static duk_ret_t NativeGetBaseTexture(duk_context *ctx);
	static duk_ret_t NativeGetSpriteSubSet(duk_context *ctx);
	static duk_ret_t NativeGetSpriteScale(duk_context *ctx);
	static duk_ret_t NativeGetWeaponSpec(duk_context *ctx);
	static duk_ret_t NativeGetModTexture(duk_context *ctx);
	static duk_ret_t NativeGetClientSkinInfo(duk_context *ctx);
	static duk_ret_t NativeGetClientCharacterCores(duk_context *ctx);
	static duk_ret_t NativeGetStandardSkinInfo(duk_context *ctx);
	static duk_ret_t NativeGetSkinPartTexture(duk_context *ctx);
	static duk_ret_t NativeMapSetTileCollisionFlags(duk_context *ctx);
	static duk_ret_t NativeDirectionFromAngle(duk_context *ctx);
	static duk_ret_t NativeCollisionSetSolidBlock(duk_context *ctx);
	static duk_ret_t NativeCollisionClearSolidBlock(duk_context *ctx);

	template<typename IntT>
	static duk_ret_t NativeUnpackInteger(duk_context *ctx);
	static duk_ret_t NativeUnpackFloat(duk_context *ctx);

	// TODO: not great if we want to do nested objects
	void PushObject();
	void ObjectSetMemberInt(const char* MemberName, int Value);
	void ObjectSetMemberFloat(const char* MemberName, float Value);
	void ObjectSetMemberRawBuffer(const char* MemberName, const void* pRawBuffer, int RawBufferSize);
	void ObjectSetMember(const char* MemberName);

	bool IsModAlreadyInstalled(const SHA256_DIGEST* pModSha256);
	bool ExtractAndInstallModZipBuffer(const HttpBuffer* pHttpZipData, const SHA256_DIGEST* pModSha256);
	bool ExtractAndInstallModCompressedBuffer(const void* pCompBuff, int CompBuffSize, const SHA256_DIGEST* pModSha256);
	bool LoadJsScriptFile(const char* pJsFilePath, const char* pJsRelFilePath);
	bool LoadModFilesFromDisk(const SHA256_DIGEST* pModSha256);

	void ResetDukContext();

public:
	CDukEntry m_DukEntry;

	CDuktape();

	virtual void OnInit();
	virtual void OnShutdown();
	virtual void OnRender();
	virtual void OnMessage(int Msg, void *pRawMsg);
	virtual void OnStateChange(int NewState, int OldState);
	void OnModReset();
	void OnModUnload();

	bool StartDuckModHttpDownload(const char* pModUrl, const SHA256_DIGEST* pModSha256);
	bool TryLoadInstalledDuckMod(const SHA256_DIGEST* pModSha256);
	bool InstallAndLoadDuckModFromZipBuffer(const void* pBuffer, int BufferSize, const SHA256_DIGEST* pModSha256);

	inline bool IsLoaded() const { return m_pDukContext != 0 && m_IsModLoaded; }

	friend class CDukEntry;
};

inline void GetIntProp(duk_context* pCtx, duk_idx_t ObjIdx, const char* pPropName, int* pOutInt)
{
	if(duk_get_prop_string(pCtx, ObjIdx, pPropName))
	{
		*pOutInt = duk_to_int(pCtx, -1);
		duk_pop(pCtx);
	}
}

inline void GetFloatProp(duk_context* pCtx, duk_idx_t ObjIdx, const char* pPropName, float* pOutFloat)
{
	if(duk_get_prop_string(pCtx, ObjIdx, pPropName))
	{
		*pOutFloat = duk_to_number(pCtx, -1);
		duk_pop(pCtx);
	}
}

inline void SetIntProp(duk_context* pCtx, duk_idx_t ObjIdx, const char* pPropName, int Val)
{
	duk_push_int(pCtx, Val);
	duk_put_prop_string(pCtx, ObjIdx, pPropName);
}

inline void SetFloatProp(duk_context* pCtx, duk_idx_t ObjIdx, const char* pPropName, float Val)
{
	duk_push_number(pCtx, Val);
	duk_put_prop_string(pCtx, ObjIdx, pPropName);
}

duk_idx_t DuktapePushCharacterCore(duk_context* pCtx, const CCharacterCore* pCharCore);
duk_idx_t DuktapePushNetObjPlayerInput(duk_context* pCtx, const CNetObj_PlayerInput* pInput);
void DuktapeReadCharacterCore(duk_context* pCtx, duk_idx_t ObjIdx, CCharacterCore* pOutCharCore);
void DuktapeReadNetObjPlayerInput(duk_context* pCtx, duk_idx_t ObjIdx, CNetObj_PlayerInput* pOutInput);
