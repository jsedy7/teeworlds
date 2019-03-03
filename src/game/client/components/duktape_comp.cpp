#include "duktape_comp.h"
#include <engine/storage.h>
#include <generated/protocol.h>
#include <game/my_protocol.h>
#include <stdint.h>

inline u8 HexCharToValue(char c)
{
	if(c >= '0' && c <= '9')
		return c - '0';
	if(c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return 0;
}

static void UnpackStrAsNetObj(const char* Str, int StrSize, void* pOutNetObj, int NetObjSize)
{
	dbg_assert(StrSize == NetObjSize*2, "String size does not correspond to NetObj size");

	for(int i = 0; i < NetObjSize; i++)
	{
		const u8 v1 = HexCharToValue(Str[i*2]);
		const u8 v2 = HexCharToValue(Str[i*2+1]);
		const u8 Byte = v1 * 0x10 + v2;
		((u8*)pOutNetObj)[i] = Byte;
	}
}


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
	dbg_assert(n == 4, "Wrong argument count");
	double x = duk_to_number(ctx, 0);
	double y = duk_to_number(ctx, 1);
	double Width = duk_to_number(ctx, 2);
	double Height = duk_to_number(ctx, 3);

	IGraphics::CQuadItem Quad(x, y, Width, Height);
	This()->m_aQuadList[This()->m_CurrentDrawSpace].add(Quad);

	return 0;
}

duk_ret_t CDuktape::NativeSetDrawSpace(duk_context *ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 1, "Wrong argument count");

	int ds = duk_to_int(ctx, 0);
	dbg_assert(ds >= 0 && ds < DrawSpace::_COUNT, "Draw space undefined");
	This()->m_CurrentDrawSpace = ds;

	return 0;
}

void CDuktape::PushObject()
{
	m_CurrentPushedObjID = duk_push_object(Duk());
}

void CDuktape::ObjectSetMemberInt(const char* MemberName, int Value)
{
	duk_push_int(Duk(), Value);
	duk_put_prop_string(Duk(), m_CurrentPushedObjID, MemberName);
}

void CDuktape::ObjectSetMemberFloat(const char* MemberName, float Value)
{
	duk_push_number(Duk(), Value);
	duk_put_prop_string(Duk(), m_CurrentPushedObjID, MemberName);
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
	duk_put_global_string(Duk(), "TwRenderQuad");

	duk_push_c_function(Duk(), NativeSetDrawSpace, 1);
	duk_put_global_string(Duk(), "TwSetDrawSpace");

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

	m_CurrentDrawSpace = DrawSpace::GAME;
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
}

void CDuktape::OnMessage(int Msg, void* pRawMsg)
{
	if(Msg == NETMSGTYPE_SV_BROADCAST)
	{
		const char* pMsg = ((CNetMsg_Sv_Broadcast *)pRawMsg)->m_pMessage;
		//dbg_msg("duk", "broadcast msg: {%s}", pMsg);

		const int MsgLen = str_length(pMsg);
		if(MsgLen > 4)
		{
			u32 Head = *(u32*)pMsg;
			//dbg_msg("duk", "Head=%x", Head);

			if((Head & 0xFFFFFF) == 0x4B5544) // DUK header
			{
				const int ObjStrLen = MsgLen - 4;
				const char* ObjStr = pMsg + 4;
				const int ObjID = ((Head & 0xFF000000) >> 24) - 0x21;
				const int ObjSize = (ObjStrLen)/2;
				//dbg_msg("duk", "DUK packed netobj, id=0x%x size=%d", ObjID, ObjSize);

				switch(ObjID)
				{
					case DukNetObjID::TEST: {
						CNetObj_Test Test;
						UnpackStrAsNetObj(ObjStr, ObjStrLen, &Test, sizeof(Test));
						/*dbg_msg("duk", "CNetObj_Test = { ClientID=%d, Value1=%g }", Test.ClientID,
							Test.Value1, ObjSize);*/
					} break;

					case DukNetObjID::RECT: {
						CNetObj_Rect Rect;
						UnpackStrAsNetObj(ObjStr, ObjStrLen, &Rect, sizeof(Rect));
						/*dbg_msg("duk", "CNetObj_Rect = { x=%g, y=%g, w=%g, h=%g }", Rect.x,
							Rect.y, Rect.w, Rect.h);*/

						duk_get_global_string(Duk(), "OnMessage");

						PushObject();
						ObjectSetMemberInt("netID", DukNetObjID::RECT);
						ObjectSetMemberFloat("x", Rect.x);
						ObjectSetMemberFloat("y", Rect.y);
						ObjectSetMemberFloat("w", Rect.w);
						ObjectSetMemberFloat("h", Rect.h);

						int NumArgs = 1;
						if(duk_pcall(Duk(), NumArgs) != 0)
						{
							dbg_msg("duk", "OnMessage(): Script error: %s", duk_safe_to_string(Duk(), -1));
							dbg_break();
						}
						duk_pop(Duk());
					} break;
				}

				static const char* s_NullStr = "";
				((CNetMsg_Sv_Broadcast *)pRawMsg)->m_pMessage = s_NullStr;
			}
		}
	}
}

void CDuktape::RenderDrawSpaceGame()
{
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();

	Graphics()->SetColor(1, 0, 0, 1);
	Graphics()->QuadsDrawTL(m_aQuadList[DrawSpace::GAME].base_ptr(), m_aQuadList[DrawSpace::GAME].size());

	Graphics()->QuadsEnd();

	m_aQuadList[DrawSpace::GAME].clear();
}
