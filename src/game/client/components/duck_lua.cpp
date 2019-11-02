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

struct CFileChunk
{
	const char* m_pData;
	int m_Cursor;
	int m_Size;
};

static const char* LuaReader(lua_State* pState, void* pData, size_t* pSize)
{
	CFileChunk& Chunk = *(CFileChunk*)pData;

	if(Chunk.m_Cursor < Chunk.m_Size) {
		Chunk.m_Cursor += Chunk.m_Size;
		*pSize = Chunk.m_Size;
		return Chunk.m_pData;
	}

	return NULL;
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
	CFileChunk Chunk;
	Chunk.m_pData = pFileData;
	Chunk.m_Cursor = 0;
	Chunk.m_Size = FileSize;

	int r = lua_load(State(), LuaReader, &Chunk, pRelFilePath);
	dbg_assert(r != LUA_ERRMEM, "Lua memory error");

	if(r == LUA_ERRSYNTAX)
	{
		LUA_ERR("'%s': \n   %s", pRelFilePath, lua_tostring(State(), -1));
		mem_free(pFileData);
		return false;
	}

	dbg_msg("duck", "'%s' loaded (%d)", pRelFilePath, FileSize);
	mem_free(pFileData);
	return true;
}

void CDuckLua::ResetLuaState()
{
	if(m_pLuaState)
		lua_close(m_pLuaState);

	m_pLuaState = luaL_newstate();
}

void CDuckLua::Reset()
{
	ResetLuaState();
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

}

void CDuckLua::OnRender(float LocalTime, float IntraGameTick)
{

}

void CDuckLua::OnUpdate(float LocalTime, float IntraGameTick)
{

}

bool CDuckLua::DetectStackLeak()
{
	return false;
}
