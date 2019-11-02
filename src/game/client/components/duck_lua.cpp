#include "duck_lua.h"
#include "duck_bridge.h"

extern "C" {
	#include <lua.h>
	#include <lualib.h>
	#include <lauxlib.h>
}

static CDuckLua* s_DuckLua = 0;
inline CDuckLua* This() { return s_DuckLua; }

#define LUA_WARN(fmt, ...) This()->Bridge()->JsError(JsErrorLvl::WARNING, fmt, __VA_ARGS__)
#define LUA_ERR(fmt, ...) This()->Bridge()->JsError(JsErrorLvl::ERROR, fmt, __VA_ARGS__)
#define LUA_CRIT(fmt, ...) This()->Bridge()->JsError(JsErrorLvl::CRITICAL, fmt, __VA_ARGS__)

#define GetFunctionRef(F) GetFunctionFromRef(m_FuncRef##F, #F)

#ifdef CONF_DEBUG
	#define DBG_DETECTSTACKLEAK() if(IsStackLeaking()) {\
		volatile int StackTop = lua_gettop(L());\
		dbg_assert(0, "stack leak"); }
#else
	#define DBG_DETECTSTACKLEAK()
#endif

bool CDuckLua::_CheckArgumentCountImp(lua_State* L, int NumArgs, const char* pFuncName)
{
	int n = lua_gettop(L);
	if(n != NumArgs)
	{
		LUA_ERR("Tw%s() wrong argument count (%d), expected %d", pFuncName, n, NumArgs);
		return false;
	}

	for(int i = 0; i < n; i++)
	{
		if(lua_isnil(L, i+1))
		{
			LUA_ERR("Tw%s() wrong argument count (%d), expected %d (%d argument is nil)", pFuncName, n, NumArgs, i);
			return false;
		}
	}

	return true;
}

#define CheckArgumentCount(ctx, num) if(!This()->_CheckArgumentCountImp(ctx, num, __FUNCTION__ + 16)) { return 0; }

int CDuckLua::NativePrint(lua_State *L)
{
	CheckArgumentCount(L, 1);

	const char* pStr = lua_tostring(L, 1);
	const int Len = str_length(pStr);
	const int MaxLen = 128;
	char aBuff[MaxLen+1];

	for(int i = 0; i < Len; i += MaxLen)
	{
		str_copy(aBuff, pStr + i, min(Len-i+1, MaxLen+1));
		This()->Bridge()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "mod", aBuff);
	}
	return 0;  /* no return value (= undefined) */
}

bool CDuckLua::LoadScriptFile(const char *pFilePath, const char *pRelFilePath)
{
	IOHANDLE ScriptFile = io_open(pFilePath, IOFLAG_READ);
	if(!ScriptFile)
	{
		dbg_msg("duck", "could not open '%s'", pFilePath);
		return false;
	}

	int FileSize = (int)io_length(ScriptFile);
	char *pFileData = (char *)mem_alloc(FileSize + 1, 1);
	io_read(ScriptFile, pFileData, FileSize);
	pFileData[FileSize] = 0;
	io_close(ScriptFile);

	// load script
	int r = luaL_loadbuffer(L(), pFileData, FileSize, pRelFilePath);
	dbg_assert(r != LUA_ERRMEM, "Lua memory error");

	if(r != 0)
	{
		LUA_ERR("'%s': \n   %s", pRelFilePath, lua_tostring(L(), -1));
		lua_pop(L(), 1);
		mem_free(pFileData);
		return false;
	}

	if(lua_pcall(L(), 0, 0, 0) != 0)
	{
		LUA_ERR("'%s': \n   %s", pRelFilePath, lua_tostring(L(), -1));
		lua_pop(L(), 1);
		mem_free(pFileData);
		return false;
	}

	dbg_msg("duck", "'%s' loaded (%d)", pRelFilePath, FileSize);
	mem_free(pFileData);

	DBG_DETECTSTACKLEAK();
	return true;
}

void CDuckLua::ResetLuaState()
{
	if(m_pLuaState)
		lua_close(m_pLuaState);

	m_pLuaState = luaL_newstate();

	lua_pushcfunction(L(), NativePrint);
	lua_setglobal(L(), "print");

#define REGISTER_FUNC(fname, arg_count) \
	lua_pushcfunction(L(), Native##fname);\
	lua_setglobal(L(), "Tw" #fname)

	//REGISTER_FUNC(Print, 1);

#undef REGISTER_FUNC
}

void CDuckLua::Reset()
{
	ResetLuaState();
}

bool CDuckLua::GetFunction(const char *Name)
{
	lua_getglobal(L(), Name);
	if(lua_isfunction(L(), -1) == 0)
	{
		lua_pop(L(), 1);
		aLastCalledFunction[0] = 0;
		return false;
	}

	str_copy(aLastCalledFunction, Name, sizeof(aLastCalledFunction));
	return true;
}

bool CDuckLua::GetFunctionFromRef(int Ref, const char* Name)
{
	lua_rawgeti(L(), LUA_REGISTRYINDEX, Ref);
	if(lua_isfunction(L(), -1) == 0)
	{
		lua_pop(L(), 1);
		aLastCalledFunction[0] = 0;
		return false;
	}

	str_copy(aLastCalledFunction, Name, sizeof(aLastCalledFunction));
	return true;
}

void CDuckLua::CallFunction(int ArgCount, int ReturnCount)
{
	dbg_assert(lua_gettop(L()) == ArgCount + 1, "Wrong argument count pushed on stack");

	int ret = lua_pcall(L(), ArgCount, ReturnCount, 0);
	if(ret != 0)
	{
		LUA_ERR("%s(): \n    %s", aLastCalledFunction, lua_tostring(L(), -1));
		lua_pop(L(), 1);
	}

	DBG_DETECTSTACKLEAK();
}

CDuckLua::CDuckLua()
{
	s_DuckLua = this;
	m_pLuaState = NULL;
}

void CDuckLua::Shutdown()
{
	if(m_pLuaState)
		lua_close(m_pLuaState);
}

void CDuckLua::OnMessage(int Msg, void *pRawMsg)
{

}

void CDuckLua::OnSnapItem(int Msg, int SnapID, void *pRawMsg)
{

}

void CDuckLua::OnDuckSnapItem(int Msg, int SnapID, void *pRawMsg, int Size)
{

}

void CDuckLua::OnInput(IInput::CEvent e)
{

}

void CDuckLua::OnModLoaded()
{
#define SEARCH_FUNCTION(F)\
	lua_getglobal(L(), #F);\
	m_FuncRef##F = luaL_ref(L(), LUA_REGISTRYINDEX)

	SEARCH_FUNCTION(OnLoad);
	SEARCH_FUNCTION(OnMessage);
	SEARCH_FUNCTION(OnSnapItem);
	SEARCH_FUNCTION(OnDuckSnapItem);
	SEARCH_FUNCTION(OnInput);
	SEARCH_FUNCTION(OnRender);
	SEARCH_FUNCTION(OnUpdate);

#undef SEARCH_FUNCTION

	if(GetFunctionRef(OnLoad))
	{
		CallFunction(0, 0);
	}
}

void CDuckLua::OnRender(float LocalTime, float IntraGameTick)
{
	if(GetFunctionRef(OnRender))
	{
		lua_pushnumber(L(), LocalTime);
		lua_pushnumber(L(), IntraGameTick);

		CallFunction(2, 0);
	}
}

void CDuckLua::OnUpdate(float LocalTime, float IntraGameTick)
{

}

bool CDuckLua::IsStackLeaking()
{
	return lua_gettop(L()) != 0;
}
