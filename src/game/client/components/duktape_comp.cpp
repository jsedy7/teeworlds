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
	CRenderCmd Cmd;
	Cmd.m_Type = CRenderCmd::QUAD;
	Cmd.m_Quad = Quad;
	This()->m_aRenderCmdList[This()->m_CurrentDrawSpace].add(Cmd);

	return 0;
}

duk_ret_t CDuktape::NativeRenderSetColorU32(duk_context *ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 1, "Wrong argument count");
	int x = duk_to_int(ctx, 0);

	float Color[4];
	Color[0] = (x & 0xFF) / 255.f;
	Color[1] = ((x >> 8) & 0xFF) / 255.f;
	Color[2] = ((x >> 16) & 0xFF) / 255.f;
	Color[3] = ((x >> 24) & 0xFF) / 255.f;

	CRenderCmd Cmd;
	Cmd.m_Type = CRenderCmd::COLOR;
	memmove(Cmd.m_Color, Color, sizeof(Color));
	This()->m_aRenderCmdList[This()->m_CurrentDrawSpace].add(Cmd);

	return 0;
}

duk_ret_t CDuktape::NativeRenderSetColorF4(duk_context *ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 4, "Wrong argument count");

	float Color[4];
	Color[0] = duk_to_number(ctx, 0);
	Color[1] = duk_to_number(ctx, 1);
	Color[2] = duk_to_number(ctx, 2);
	Color[3] = duk_to_number(ctx, 3);

	CRenderCmd Cmd;
	Cmd.m_Type = CRenderCmd::COLOR;
	memmove(Cmd.m_Color, Color, sizeof(Color));
	This()->m_aRenderCmdList[This()->m_CurrentDrawSpace].add(Cmd);

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

duk_ret_t CDuktape::NativeMapSetTileCollisionFlags(duk_context *ctx)
{
	int n = duk_get_top(ctx);  /* #args */
	dbg_assert(n == 3, "Wrong argument count");

	// TODO: bound check
	int Tx = duk_to_int(ctx, 0);
	int Ty = duk_to_int(ctx, 1);
	int Flags = duk_to_int(ctx, 2);

	This()->Collision()->SetTileCollisionFlags(Tx, Ty, Flags);

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

	duk_push_c_function(Duk(), NativeRenderSetColorU32, 1);
	duk_put_global_string(Duk(), "TwRenderSetColorU32");

	duk_push_c_function(Duk(), NativeRenderSetColorF4, 4);
	duk_put_global_string(Duk(), "TwRenderSetColorF4");

	duk_push_c_function(Duk(), NativeSetDrawSpace, 1);
	duk_put_global_string(Duk(), "TwSetDrawSpace");

	duk_push_c_function(Duk(), NativeMapSetTileCollisionFlags, 3);
	duk_put_global_string(Duk(), "TwMapSetTileCollisionFlags");

	// Teeworlds global object
	duk_eval_string(Duk(),
		"var Teeworlds = {"
		"	DRAW_SPACE_GAME: 0,"
		"	aClients: [],"
		"};");

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
	// Update Teeworlds global object
	char aEvalBuff[256];
	str_format(aEvalBuff, sizeof(aEvalBuff), "Teeworlds.intraTick = %g;", Client()->IntraGameTick());
	duk_eval_string(Duk(), aEvalBuff);
	str_format(aEvalBuff, sizeof(aEvalBuff), "Teeworlds.mapSize = { x: %d, y: %d };", Collision()->GetWidth(), Collision()->GetHeight());
	duk_eval_string(Duk(), aEvalBuff);

	// Call OnUpdate()
	duk_get_global_string(Duk(), "OnUpdate");
	/* push arguments here */
	duk_push_number(Duk(), Client()->LocalTime());
	int NumArgs = 1;
	if(duk_pcall(Duk(), NumArgs) != 0)
	{
		dbg_msg("duk", "OnUpdate(): Script error: %s", duk_safe_to_string(Duk(), -1));
		dbg_break();
	}
	duk_pop(Duk());
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

					case DukNetObjID::DEBUG_RECT: {
						CNetObj_DebugRect Rect;
						UnpackStrAsNetObj(ObjStr, ObjStrLen, &Rect, sizeof(Rect));
						/*dbg_msg("duk", "CNetObj_Rect = { x=%g, y=%g, w=%g, h=%g }", Rect.x,
							Rect.y, Rect.w, Rect.h);*/

						duk_get_global_string(Duk(), "OnMessage");

						PushObject();
						ObjectSetMemberInt("netID", DukNetObjID::DEBUG_RECT);
						ObjectSetMemberFloat("x", Rect.x);
						ObjectSetMemberFloat("y", Rect.y);
						ObjectSetMemberFloat("w", Rect.w);
						ObjectSetMemberFloat("h", Rect.h);
						ObjectSetMemberInt("color", Rect.color);

						int NumArgs = 1;
						if(duk_pcall(Duk(), NumArgs) != 0)
						{
							dbg_msg("duk", "OnMessage(): Script error: %s", duk_safe_to_string(Duk(), -1));
							dbg_break();
						}
						duk_pop(Duk());
					} break;

					case DukNetObjID::MAP_RECT_SET_SOLID: {
						CNetObj_MapRectSetSolid Flip;
						UnpackStrAsNetObj(ObjStr, ObjStrLen, &Flip, sizeof(Flip));
						/*dbg_msg("duk", "CNetObj_Rect = { x=%g, y=%g, w=%g, h=%g }", Rect.x,
							Rect.y, Rect.w, Rect.h);*/

						duk_get_global_string(Duk(), "OnMessage");

						PushObject();
						ObjectSetMemberInt("netID", DukNetObjID::MAP_RECT_SET_SOLID);
						ObjectSetMemberInt("solid", Flip.solid); // TODO: boolean
						ObjectSetMemberInt("x", Flip.x);
						ObjectSetMemberInt("y", Flip.y);
						ObjectSetMemberInt("w", Flip.w);
						ObjectSetMemberInt("h", Flip.h);

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

	const int CmdCount = m_aRenderCmdList[DrawSpace::GAME].size();
	const CRenderCmd* aCmds = m_aRenderCmdList[DrawSpace::GAME].base_ptr();

	for(int i = 0; i < CmdCount; i++)
	{
		switch(aCmds[i].m_Type)
		{
			case CRenderCmd::COLOR: {
				const float* pColor = aCmds[i].m_Color;
				Graphics()->SetColor(pColor[0], pColor[1], pColor[2], pColor[3]);
			} break;
			case CRenderCmd::QUAD:
				Graphics()->QuadsDrawTL(&aCmds[i].m_Quad, 1);
				break;
			default:
				dbg_assert(0, "Render command type not handled");
		}
	}

	Graphics()->QuadsEnd();

	m_aRenderCmdList[DrawSpace::GAME].clear();
}
