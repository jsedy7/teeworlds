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

	inline lua_State* State() { return m_pLuaState; }
	inline CDuckBridge* Bridge() { return m_pBridge; }

	bool LoadScriptFile(const char* pFilePath, const char* pRelFilePath);
	void ResetLuaState();
	void Reset();

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
	bool DetectStackLeak();

	friend class CDuckBridge;
};
