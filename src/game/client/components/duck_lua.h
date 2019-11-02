#pragma once
#include <base/tl/array.h>
#include <engine/input.h>

struct CGrowBuffer;
struct CDuckBridge;
struct lua_State;

class CDuckLua
{
	CDuckBridge* m_pBridge;
	lua_State* m_pLuaState;
	char aLastCalledFunction[256];

	int m_FuncRefOnLoad;
	int m_FuncRefOnMessage;
	int m_FuncRefOnSnapItem;
	int m_FuncRefOnDuckSnapItem;
	int m_FuncRefOnInput;
	int m_FuncRefOnRender;
	int m_FuncRefOnUpdate;

	inline lua_State* L() { return m_pLuaState; }
	inline CDuckBridge* Bridge() { return m_pBridge; }

	bool _CheckArgumentCountImp(lua_State* L, int NumArgs, const char* pFuncName);

	static int NativePrint(lua_State *L);

	bool LoadScriptFile(const char* pFilePath, const char* pRelFilePath);
	void ResetLuaState();
	void Reset();

	bool GetFunction(const char* Name);
	bool GetFunctionFromRef(int Ref, const char *Name);
	void CallFunction(int ArgCount, int ReturnCount);

public:
	CDuckLua();
	void Shutdown();
	void OnMessage(int Msg, void *pRawMsg);
	void OnSnapItem(int Msg, int SnapID, void* pRawMsg);
	void OnDuckSnapItem(int Msg, int SnapID, void* pRawMsg, int Size);
	void OnInput(IInput::CEvent e);
	void OnModLoaded();

	inline bool IsLoaded() const { return m_pLuaState != NULL; }

	void OnRender(float LocalTime, float IntraGameTick);
	void OnUpdate(float LocalTime, float IntraGameTick);
	bool IsStackLeaking();

	friend class CDuckBridge;
};
