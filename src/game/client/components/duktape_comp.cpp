#include "duktape_comp.h"
#include <engine/storage.h>

// TODO: rename?
static CDuktape* s_This = 0;
inline CDuktape* This() { return s_This; }

static duk_ret_t NativePrint(duk_context *ctx)
{
	dbg_msg("duk", "%s", duk_to_string(ctx, 0));
	return 0;  /* no return value (= undefined) */
}

duk_ret_t CDuktape::NativeRenderQuad(duk_context *ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	double x = duk_to_number(ctx, 0);
	double y = duk_to_number(ctx, 1);
	double Width = duk_to_number(ctx, 2);
	double Height = duk_to_number(ctx, 3);

	IGraphics::CQuadItem Quad(x, y, Width, Height);
	This()->m_aQuadList.add(Quad);

	return 0;
}

CDuktape::CDuktape()
{
	s_This = this;
}

void CDuktape::OnInit()
{
	// load ducktape, eval main.js
	m_pDukContext = duk_create_heap_default();

	char aModPath[256];
	str_format(aModPath, sizeof(aModPath), "mods/duck/main.js");
	IOHANDLE ModFile = Storage()->OpenFile(aModPath, IOFLAG_READ, IStorage::TYPE_ALL);
	dbg_assert(ModFile != 0, "Could not load duck/main.js");

	int FileSize = (int)io_length(ModFile);
	char *pFileData = (char *)mem_alloc(FileSize + 1, 1);
	io_read(ModFile, pFileData, FileSize);
	pFileData[FileSize] = 0;
	io_close(ModFile);

	// function binding
	duk_push_c_function(Duk(), NativePrint, 1 /*nargs*/);
	duk_put_global_string(Duk(), "print");

	duk_push_c_function(Duk(), NativeRenderQuad, 4 /*nargs*/);
	duk_put_global_string(Duk(), "RenderQuad");

	// eval script
	duk_push_string(Duk(), pFileData);
	if(duk_peval(Duk()) != 0)
	{
		/* Use duk_safe_to_string() to convert error into string.  This API
		 * call is guaranteed not to throw an error during the coercion.
		 */
		dbg_msg("duk", "Script error: %s", duk_safe_to_string(Duk(), -1));
		dbg_break();
	}
	duk_pop(Duk());

	dbg_msg("duk", "main.js loaded (%d)", FileSize);
	mem_free(pFileData);
}

void CDuktape::OnShutdown()
{
	duk_destroy_heap(m_pDukContext);
}

void CDuktape::OnRender()
{
	// TODO: remove
	duk_get_global_string(Duk(), "OnUpdate");
	/* push arguments here */
	duk_push_number(Duk(), Client()->LocalTime());
	int NumArgs = 1;
	duk_call(Duk(), NumArgs);
	duk_pop(Duk());

	/*
	if(duk_pcall(ctx, 1) != 0) {
		printf("Error: %s\n", duk_safe_to_string(Duk(), -1));
	} else {
		printf("%s\n", duk_safe_to_string(Duk(), -1));
	}
	*/

	Graphics()->TextureClear();
	Graphics()->QuadsBegin();

	Graphics()->SetColor(1, 0, 0, 1);
	Graphics()->QuadsDrawTL(m_aQuadList.base_ptr(), m_aQuadList.size());

	Graphics()->QuadsEnd();

	m_aQuadList.clear();
}
