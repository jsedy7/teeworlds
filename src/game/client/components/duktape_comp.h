#pragma once
#include <game/client/component.h>
#include <engine/client/duktape.h>

class CDuktape : public CComponent
{
	duk_context* m_pDukContext;
	inline duk_context* Duk() { return m_pDukContext; }

public:
	CDuktape();

	virtual void OnInit();
	virtual void OnShutdown();
	virtual void OnRender();
};
