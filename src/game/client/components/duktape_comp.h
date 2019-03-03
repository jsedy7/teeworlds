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

	int m_CurrentDrawSpace;
	array<IGraphics::CQuadItem> m_aQuadList[DrawSpace::_COUNT];

	static duk_ret_t NativeRenderQuad(duk_context *ctx);
	static duk_ret_t NativeSetDrawSpace(duk_context *ctx);

	int m_CurrentPushedObjID;

	// TODO: not great if we want to do nested objects
	void PushObject();
	void ObjectSetMemberInt(const char* MemberName, int Value);
	void ObjectSetMemberFloat(const char* MemberName, float Value);

public:
	CDuktape();

	virtual void OnInit();
	virtual void OnShutdown();
	virtual void OnRender();
	virtual void OnMessage(int Msg, void *pRawMsg);

	void RenderDrawSpaceGame();
};
