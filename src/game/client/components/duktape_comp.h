#pragma once
#include <base/tl/array.h>
#include <engine/client/duktape.h>
#include <game/client/component.h>
#include <engine/graphics.h>

struct DrawSpace
{
	enum Enum {
		GAME=0,
		_COUNT
	};
};

class CDuktape : public CComponent
{
	duk_context* m_pDukContext;
	inline duk_context* Duk() { return m_pDukContext; }

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
			IGraphics::CQuadItem m_Quad;
		};

		CRenderCmd() {}
	};

	int m_CurrentDrawSpace;
	array<CRenderCmd> m_aRenderCmdList[DrawSpace::_COUNT];

	static duk_ret_t NativeRenderQuad(duk_context *ctx);
	static duk_ret_t NativeRenderSetColorU32(duk_context *ctx);
	static duk_ret_t NativeRenderSetColorF4(duk_context *ctx);
	static duk_ret_t NativeSetDrawSpace(duk_context *ctx);
	static duk_ret_t NativeMapSetTileCollisionFlags(duk_context *ctx);

	template<typename IntT>
	static duk_ret_t NativeUnpackInteger(duk_context *ctx);
	static duk_ret_t NativeUnpackFloat(duk_context *ctx);

	int m_CurrentPushedObjID;

	// TODO: not great if we want to do nested objects
	void PushObject();
	void ObjectSetMemberInt(const char* MemberName, int Value);
	void ObjectSetMemberFloat(const char* MemberName, float Value);
	void ObjectSetMemberRawBuffer(const char* MemberName, const void* pRawBuffer, int RawBufferSize);

	bool LoadJsScriptFile(const char* pJsFilePath);
	bool LoadModFilesFromDisk(const char* pModRootPath);

public:
	CDuktape();

	virtual void OnInit();
	virtual void OnShutdown();
	virtual void OnRender();
	virtual void OnMessage(int Msg, void *pRawMsg);

	void RenderDrawSpaceGame();

	void LoadDuckMod(const char* pModUrl, const SHA256_DIGEST* pModSha256);
};