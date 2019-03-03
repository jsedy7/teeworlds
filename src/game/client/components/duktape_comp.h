#pragma once
#include <base/tl/array.h>
#include <engine/client/duktape.h>
#include <game/client/component.h>
#include <engine/graphics.h>

class CDuktape : public CComponent
{
	duk_context* m_pDukContext;
	inline duk_context* Duk() { return m_pDukContext; }

	array<IGraphics::CQuadItem> m_aQuadList;

	static duk_ret_t NativeRenderQuad(duk_context *ctx);

public:
	CDuktape();

	virtual void OnInit();
	virtual void OnShutdown();
	virtual void OnRender();
	virtual void OnMessage(int Msg, void *pRawMsg);
};
