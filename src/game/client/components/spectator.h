/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_SPECTATOR_H
#define GAME_CLIENT_COMPONENTS_SPECTATOR_H
#include <base/vmath.h>

#include <game/client/component.h>

class CSpectator : public CComponent
{
	enum
	{
		NO_SELECTION=-1,
		OVERVIEW=0,
		FREE_VIEW,
		FOLLOW
	};

	bool m_Active;
	bool m_WasActive;

	int m_SelectedSpectatorID;
	int m_SelectedSpecMode;
	vec2 m_MouseScreenPos;
	int64 m_MouseMoveTimer;

	static void ConKeySpectator(IConsole::IResult *pResult, void *pUserData);
	static void ConSpectate(IConsole::IResult *pResult, void *pUserData);
	static void ConSpectateNext(IConsole::IResult *pResult, void *pUserData);
	static void ConSpectatePrevious(IConsole::IResult *pResult, void *pUserData);
	void UpdatePositions();
	bool DoButtonSelect(void* pID, const char* pLabel, CUIRect Rect, bool Selected);

	void CameraOverview();
	void CameraFreeview();
	void CameraFollow();

public:
	CSpectator();

	virtual void OnConsoleInit();
	virtual bool OnMouseMove(float x, float y);
	virtual bool OnInput(IInput::CEvent InputEvent);
	virtual void OnRender();
	virtual void OnRelease();
	virtual void OnReset();

	void Spectate(int SpecMode, int SpectatorID);
};

#endif
