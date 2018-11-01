/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/demo.h>
#include <engine/keys.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>

#include <generated/client_data.h>
#include <generated/protocol.h>

#include <game/client/animstate.h>
#include <game/client/render.h>

#include "menus.h"
#include "controls.h"
#include "spectator.h"

static float s_TextScale = 0.8f;

void CSpectator::ConKeySpectator(IConsole::IResult *pResult, void *pUserData)
{
	CSpectator *pSelf = (CSpectator *)pUserData;
	if(pSelf->m_pClient->m_Snap.m_SpecInfo.m_Active &&
		(pSelf->Client()->State() != IClient::STATE_DEMOPLAYBACK || pSelf->DemoPlayer()->GetDemoType() == IDemoPlayer::DEMOTYPE_SERVER))
		pSelf->m_Active = pResult->GetInteger(0) != 0;
}

void CSpectator::ConSpectate(IConsole::IResult *pResult, void *pUserData)
{
	((CSpectator *)pUserData)->Spectate(pResult->GetInteger(0), pResult->GetInteger(1));
}

void CSpectator::ConSpectateNext(IConsole::IResult *pResult, void *pUserData)
{
	CSpectator *pSelf = (CSpectator *)pUserData;
	int NewSpecMode = pSelf->m_pClient->m_Snap.m_SpecInfo.m_SpecMode;
	int NewSpectatorID = -1;
	bool GotNewSpectatorID = false;

	if(NewSpecMode != SPEC_PLAYER)
		NewSpecMode = (NewSpecMode + 1) % NUM_SPECMODES;
	else
		NewSpectatorID = pSelf->m_pClient->m_Snap.m_SpecInfo.m_SpectatorID;

	switch(NewSpecMode)
	{	// drop through
	case SPEC_PLAYER:
		for(int i = NewSpectatorID + 1; i < MAX_CLIENTS; i++)
		{
			if(!pSelf->m_pClient->m_aClients[i].m_Active || pSelf->m_pClient->m_aClients[i].m_Team == TEAM_SPECTATORS)
				continue;

			NewSpectatorID = i;
			GotNewSpectatorID = true;
			break;
		}
		if(GotNewSpectatorID)
			break;
		NewSpecMode = SPEC_FLAGRED;
		NewSpectatorID = -1;
	case SPEC_FLAGRED:
	case SPEC_FLAGBLUE:
		if(pSelf->m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_FLAGS)
		{
			GotNewSpectatorID = true;
			break;
		}
		NewSpecMode = SPEC_FREEVIEW;
	case SPEC_FREEVIEW:
		GotNewSpectatorID = true;
	}

	if(GotNewSpectatorID)
		pSelf->Spectate(NewSpecMode, NewSpectatorID);
}

void CSpectator::ConSpectatePrevious(IConsole::IResult *pResult, void *pUserData)
{
	CSpectator *pSelf = (CSpectator *)pUserData;
	int NewSpecMode = pSelf->m_pClient->m_Snap.m_SpecInfo.m_SpecMode;
	int NewSpectatorID = MAX_CLIENTS;
	bool GotNewSpectatorID = false;

	if(NewSpecMode != SPEC_PLAYER)
		NewSpecMode = (NewSpecMode - 1 + NUM_SPECMODES) % NUM_SPECMODES;
	else
		NewSpectatorID = pSelf->m_pClient->m_Snap.m_SpecInfo.m_SpectatorID;

	switch(NewSpecMode)
	{	// drop through
	case SPEC_FLAGBLUE:
	case SPEC_FLAGRED:
		if(pSelf->m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_FLAGS)
		{
			NewSpectatorID = -1;
			GotNewSpectatorID = true;
			break;
		}
		NewSpecMode = SPEC_PLAYER;
		NewSpectatorID = MAX_CLIENTS;
	case SPEC_PLAYER:
		for(int i = NewSpectatorID - 1; i >= 0; i--)
		{
			if(!pSelf->m_pClient->m_aClients[i].m_Active || pSelf->m_pClient->m_aClients[i].m_Team == TEAM_SPECTATORS)
				continue;

			NewSpectatorID = i;
			GotNewSpectatorID = true;
			break;
		}
		if(GotNewSpectatorID)
			break;
		NewSpecMode = SPEC_FREEVIEW;
	case SPEC_FREEVIEW:
		NewSpectatorID = -1;
		GotNewSpectatorID = true;
	}

	if(GotNewSpectatorID)
		pSelf->Spectate(NewSpecMode, NewSpectatorID);
}

CSpectator::CSpectator()
{
	OnReset();
}

void CSpectator::OnConsoleInit()
{
	Console()->Register("+spectate", "", CFGFLAG_CLIENT, ConKeySpectator, this, "Open spectator mode selector");
	Console()->Register("spectate", "ii", CFGFLAG_CLIENT, ConSpectate, this, "Switch spectator mode");
	Console()->Register("spectate_next", "", CFGFLAG_CLIENT, ConSpectateNext, this, "Spectate the next player");
	Console()->Register("spectate_previous", "", CFGFLAG_CLIENT, ConSpectatePrevious, this, "Spectate the previous player");
}

bool CSpectator::OnMouseMove(float x, float y)
{
	// we snatch mouse movement if spectating is active

	if(!m_pClient->m_Snap.m_SpecInfo.m_Active)
		return false;

	const bool ShowMouse = m_SelectedSpecMode != FREE_VIEW;

	if(ShowMouse)
	{
		UI()->ConvertMouseMove(&x, &y);
		m_MouseScreenPos += vec2(x,y);

		// clamp mouse position to screen
		m_MouseScreenPos.x = clamp(m_MouseScreenPos.x, 0.f, (float)Graphics()->ScreenWidth() - 1.0f);
		m_MouseScreenPos.y = clamp(m_MouseScreenPos.y, 0.f, (float)Graphics()->ScreenHeight() - 1.0f);

		if(x != 0 || y != 0)
			m_MouseMoveTimer = time_get();
	}
	else
	{
		// free view
		m_pClient->m_pControls->m_MousePos += vec2(x, y);
		// -> this goes to CCamera::OnRender()
	}

	return true;
}

bool CSpectator::OnInput(IInput::CEvent InputEvent)
{
	if(m_SelectedSpecMode == FREE_VIEW && InputEvent.m_Key == KEY_SPACE &&
	   InputEvent.m_Flags&IInput::FLAG_PRESS)
	{
		CameraOverview();

		if(m_MouseScreenPos.x == 0)
		{
			m_MouseScreenPos.x = Graphics()->ScreenWidth() * 0.5f;
			m_MouseScreenPos.y = Graphics()->ScreenHeight() * 0.5f;
		}

		return true;
	}

	return false;
}

void CSpectator::OnRelease()
{
	float mx = UI()->MouseX();
	float my = UI()->MouseY();
	CUIRect Screen = *UI()->Screen();

	m_MouseScreenPos.x = (mx / Screen.w) * Graphics()->ScreenWidth();
	m_MouseScreenPos.y = (my / Screen.h) * Graphics()->ScreenHeight();
}

void CSpectator::UpdatePositions()
{
	CGameClient::CSnapState& Snap = m_pClient->m_Snap;
	CGameClient::CSnapState::CSpectateInfo& SpecInfo = Snap.m_SpecInfo;

	// spectator position
	if(SpecInfo.m_Active)
	{
		if(Client()->State() == IClient::STATE_DEMOPLAYBACK &&
		   DemoPlayer()->GetDemoType() == IDemoPlayer::DEMOTYPE_SERVER &&
			SpecInfo.m_SpectatorID != -1)
		{
			SpecInfo.m_Position = mix(
				vec2(Snap.m_aCharacters[SpecInfo.m_SpectatorID].m_Prev.m_X,
					Snap.m_aCharacters[SpecInfo.m_SpectatorID].m_Prev.m_Y),
				vec2(Snap.m_aCharacters[SpecInfo.m_SpectatorID].m_Cur.m_X,
					Snap.m_aCharacters[SpecInfo.m_SpectatorID].m_Cur.m_Y),
				Client()->IntraGameTick());
			SpecInfo.m_UsePosition = true;
		}
		else if(Snap.m_pSpectatorInfo &&
				(Client()->State() == IClient::STATE_DEMOPLAYBACK || SpecInfo.m_SpecMode != SPEC_FREEVIEW))
		{
			if(Snap.m_pPrevSpectatorInfo)
				SpecInfo.m_Position = mix(vec2(Snap.m_pPrevSpectatorInfo->m_X, Snap.m_pPrevSpectatorInfo->m_Y),
											vec2(Snap.m_pSpectatorInfo->m_X, Snap.m_pSpectatorInfo->m_Y),
											Client()->IntraGameTick());
			else
				SpecInfo.m_Position = vec2(Snap.m_pSpectatorInfo->m_X, Snap.m_pSpectatorInfo->m_Y);
			SpecInfo.m_UsePosition = true;
		}
	}

	static int64 LastUpdateTime = time_get();
	int64 Now = time_get();
	const double Delta = (Now - LastUpdateTime) / (double)time_freq();
	LastUpdateTime = Now;

	// edge scrolling
	if(m_SelectedSpecMode == OVERVIEW || m_SelectedSpecMode == FOLLOW)
	{
		vec2& CtrlMp = m_pClient->m_pControls->m_MousePos;
		const float Speed = 700.0f;
		const float Edge = 30.0f;

		if(m_MouseScreenPos.x < Edge) CtrlMp.x -= Speed * Delta;
		if(m_MouseScreenPos.y < Edge) CtrlMp.y -= Speed * Delta;
		if(m_MouseScreenPos.x > Graphics()->ScreenWidth()-Edge)  CtrlMp.x += Speed * Delta;
		if(m_MouseScreenPos.y > Graphics()->ScreenHeight()-Edge) CtrlMp.y += Speed * Delta;
	}
}

bool CSpectator::DoButtonSelect(void* pID, const char* pLabel, CUIRect Rect, bool Selected)
{
	const bool Hovered = UI()->HotItem() == pID;
	vec4 ButtonColor = vec4(1.0f, 1.0f, 1.0f, 0.25f);
	if(Selected)
		ButtonColor = vec4(0.5f, 0.75f, 1.0f, 0.5f);
	else if(Hovered)
		ButtonColor = vec4(1.0f, 1.0f, 1.0f, 0.5f);

	RenderTools()->DrawRoundRect(&Rect, ButtonColor, 3.0f);

	UI()->DoLabel(&Rect, pLabel, Rect.h*s_TextScale*0.8f, CUI::ALIGN_CENTER);

	bool Modified = false;
	if(UI()->DoButtonLogic(pID, 0, 0, &Rect))
	{
		Modified = true;
	}

	return Modified;
}

void CSpectator::CameraOverview()
{
	m_SelectedSpecMode = OVERVIEW;
}

void CSpectator::CameraFreeview()
{
	m_SelectedSpecMode = FREE_VIEW;
}

void CSpectator::CameraFollow()
{
	m_SelectedSpecMode = FOLLOW;
}

void CSpectator::OnRender()
{
	if(!m_pClient->m_Snap.m_SpecInfo.m_Active)
		return;

	if(m_pClient->m_pMenus->IsActive())
		return;

	UpdatePositions();

	const bool ShowMouse = m_SelectedSpecMode != FREE_VIEW;

	int64 Now = time_get();
	double MouseLastMovedTime = (Now - m_MouseMoveTimer) / (double)time_freq();

	// update the ui
	CUIRect *pScreen = UI()->Screen();
	float mx = (m_MouseScreenPos.x/(float)Graphics()->ScreenWidth())*pScreen->w;
	float my = (m_MouseScreenPos.y/(float)Graphics()->ScreenHeight())*pScreen->h;

	if(ShowMouse)
	{
		int MouseButtons = 0;
		if(Input()->KeyIsPressed(KEY_MOUSE_1)) MouseButtons |= 1;
		if(Input()->KeyIsPressed(KEY_MOUSE_2)) MouseButtons |= 2;
		if(Input()->KeyIsPressed(KEY_MOUSE_3)) MouseButtons |= 4;

		UI()->Update(mx, my, mx*3.0f, my*3.0f, MouseButtons);
	}


	CUIRect Screen = *UI()->Screen();
	Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);

	// spectator right window
	CUIRect MainView;
	Screen.VSplitMid(0, &MainView);
	MainView.VSplitMid(0, &MainView);
	MainView.h *= 0.5f;
	MainView.y += MainView.h * 0.5f;

	RenderTools()->DrawRoundRect(&MainView, vec4(0.0f, 0.0f, 0.0f, 0.5f), 5.0f);

	const float Spacing = 3.f;
	CUIRect LineRect, Button, Label;

	MainView.HSplitTop(Spacing, 0, &MainView);
	MainView.HSplitTop(20.f, &Label, &MainView);
	Label.x += 8.f;

	UI()->DoLabel(&Label, Localize("Camera"), Label.h*s_TextScale*0.8f, CUI::ALIGN_LEFT);

	MainView.HSplitTop(Spacing, 0, &MainView);
	MainView.HSplitTop(20.f, &LineRect, &MainView);
	LineRect.VMargin(Spacing, &LineRect);

	const float ButWidth = (LineRect.w - Spacing * 2.f) / 3.f;
	LineRect.VSplitLeft(ButWidth, &Button, &LineRect);

	static int s_ButtonFreeViewID;
	if(DoButtonSelect(&s_ButtonFreeViewID, "Free-view", Button, m_SelectedSpecMode == FREE_VIEW))
	{
		CameraFreeview();
	}

	LineRect.VSplitLeft(Spacing, 0, &LineRect);
	LineRect.VSplitLeft(ButWidth, &Button, &LineRect);

	static int s_ButtonOverViewID;
	if(DoButtonSelect(&s_ButtonOverViewID, "Overview", Button, m_SelectedSpecMode == OVERVIEW))
	{
		CameraOverview();
	}

	LineRect.VSplitLeft(Spacing, 0, &LineRect);
	LineRect.VSplitLeft(ButWidth, &Button, &LineRect);

	static int s_ButtonFollowID;
	if(DoButtonSelect(&s_ButtonFollowID, "Follow", Button, m_SelectedSpecMode == FOLLOW))
	{
		CameraFollow();
	}


	// draw cursor
	if(MouseLastMovedTime < 3.0 && ShowMouse)
	{
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_CURSOR].m_Id);
		Graphics()->QuadsBegin();
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
		IGraphics::CQuadItem QuadItem(mx, my, 24.0f, 24.0f);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();
	}
}

#if 0
void CSpectator::OnRender()
{
	if(!m_Active)
	{
		if(m_WasActive)
		{
			if(m_SelectedSpecMode != NO_SELECTION)
				Spectate(m_SelectedSpecMode, m_SelectedSpectatorID);
			m_WasActive = false;
		}
		return;
	}

	if(!m_pClient->m_Snap.m_SpecInfo.m_Active)
	{
		m_Active = false;
		m_WasActive = false;
		return;
	}

	m_WasActive = true;
	m_SelectedSpecMode = NO_SELECTION;
	m_SelectedSpectatorID = -1;

	// draw background
	float Width = 400*3.0f*Graphics()->ScreenAspect();
	float Height = 400*3.0f;

	Graphics()->MapScreen(0, 0, Width, Height);

	CUIRect Rect = {Width/2.0f-300.0f, Height/2.0f-300.0f, 600.0f, 600.0f};
	Graphics()->BlendNormal();
	RenderTools()->DrawRoundRect(&Rect, vec4(0.0f, 0.0f, 0.0f, 0.3f), 20.0f);

	// clamp mouse position to selector area
	m_SelectorMouse.x = clamp(m_SelectorMouse.x, -280.0f, 280.0f);
	m_SelectorMouse.y = clamp(m_SelectorMouse.y, -280.0f, 280.0f);

	// draw selections
	float FontSize = 20.0f;
	float StartY = -190.0f;
	float LineHeight = 60.0f;
	bool Selected = false;

	if(m_pClient->m_aClients[m_pClient->m_LocalClientID].m_Team == TEAM_SPECTATORS)
	{
		if(m_pClient->m_Snap.m_SpecInfo.m_SpecMode == SPEC_FREEVIEW)
		{
			Rect.x = Width/2.0f-280.0f;
			Rect.y = Height/2.0f-280.0f;
			Rect.w = 270.0f;
			Rect.h = 60.0f;
			RenderTools()->DrawRoundRect(&Rect, vec4(1.0f, 1.0f, 1.0f, 0.25f), 20.0f);
		}

		if(m_SelectorMouse.x >= -280.0f && m_SelectorMouse.x <= -10.0f &&
			m_SelectorMouse.y >= -280.0f && m_SelectorMouse.y <= -220.0f)
		{
			m_SelectedSpecMode = SPEC_FREEVIEW;
			Selected = true;
		}
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, Selected?1.0f:0.5f);
		TextRender()->Text(0, Width/2.0f-240.0f, Height/2.0f-265.0f, FontSize, Localize("Free-View"), -1);
	}

	//
	float x = 20.0f, y = -270;
	if (m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_FLAGS)
	{
		for(int Flag = SPEC_FLAGRED; Flag <= SPEC_FLAGBLUE; ++Flag)
		{
			if(m_pClient->m_Snap.m_SpecInfo.m_SpecMode == Flag)
			{
				Rect.x = Width/2.0f+x-10.0f;
				Rect.y = Height/2.0f+y-10.0f;
				Rect.w = 120.0f;
				Rect.h = 60.0f;
				RenderTools()->DrawRoundRect(&Rect, vec4(1.0f, 1.0f, 1.0f, 0.25f), 20.0f);
			}

			Selected = false;
			if(m_SelectorMouse.x >= x-10.0f && m_SelectorMouse.x <= x+110.0f &&
				m_SelectorMouse.y >= y-10.0f && m_SelectorMouse.y <= y+50.0f)
			{
				m_SelectedSpecMode = Flag;
				Selected = true;
			}

			Graphics()->BlendNormal();
			Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
			Graphics()->QuadsBegin();

			RenderTools()->SelectSprite(Flag == SPEC_FLAGRED ? SPRITE_FLAG_RED : SPRITE_FLAG_BLUE);

			float Size = LineHeight/1.5f + (Selected ? 12.0f : 8.0f);
			float FlagWidth = Width/2.0f + x + 40.0f + (Selected ? -3.0f : -2.0f);
			float FlagHeight = Height/2.0f + y + (Selected ? -6.0f : -4.0f);

			IGraphics::CQuadItem QuadItem(FlagWidth, FlagHeight, Size/2.0f, Size);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();

			x+=140.0f;
		}
	}

	x = -270.0f, y = StartY;
	for(int i = 0, Count = 0; i < MAX_CLIENTS; ++i)
	{
		if(!m_pClient->m_Snap.m_paPlayerInfos[i] || m_pClient->m_aClients[i].m_Team == TEAM_SPECTATORS ||
			(m_pClient->m_aClients[m_pClient->m_LocalClientID].m_Team != TEAM_SPECTATORS && (m_pClient->m_Snap.m_paPlayerInfos[i]->m_PlayerFlags&PLAYERFLAG_DEAD ||
			m_pClient->m_aClients[m_pClient->m_LocalClientID].m_Team != m_pClient->m_aClients[i].m_Team || i == m_pClient->m_LocalClientID)))
			continue;

		if(++Count%9 == 0)
		{
			x += 290.0f;
			y = StartY;
		}

		if(m_pClient->m_Snap.m_SpecInfo.m_SpecMode == SPEC_PLAYER && m_pClient->m_Snap.m_SpecInfo.m_SpectatorID == i)
		{
			Rect.x = Width/2.0f+x-10.0f;
			Rect.y = Height/2.0f+y-10.0f;
			Rect.w = 270.0f;
			Rect.h = 60.0f;
			RenderTools()->DrawRoundRect(&Rect, vec4(1.0f, 1.0f, 1.0f, 0.25f), 20.0f);
		}

		Selected = false;
		if(m_SelectorMouse.x >= x-10.0f && m_SelectorMouse.x <= x+260.0f &&
			m_SelectorMouse.y >= y-10.0f && m_SelectorMouse.y <= y+50.0f)
		{
			m_SelectedSpecMode = SPEC_PLAYER;
			m_SelectedSpectatorID = i;
			Selected = true;
		}
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, Selected?1.0f:0.5f);
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%2d: %s", i, g_Config.m_ClShowsocial ? m_pClient->m_aClients[i].m_aName : "");
		TextRender()->Text(0, Width/2.0f+x+50.0f, Height/2.0f+y+5.0f, FontSize, aBuf, 220.0f);

		// flag
		if(m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_FLAGS &&
			m_pClient->m_Snap.m_pGameDataFlag && (m_pClient->m_Snap.m_pGameDataFlag->m_FlagCarrierRed == i ||
			m_pClient->m_Snap.m_pGameDataFlag->m_FlagCarrierBlue == i))
		{
			Graphics()->BlendNormal();
			Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
			Graphics()->QuadsBegin();

			RenderTools()->SelectSprite(m_pClient->m_aClients[i].m_Team==TEAM_RED ? SPRITE_FLAG_BLUE : SPRITE_FLAG_RED, SPRITE_FLAG_FLIP_X);

			float Size = LineHeight;
			IGraphics::CQuadItem QuadItem(Width/2.0f+x-LineHeight/5.0f, Height/2.0f+y-LineHeight/3.0f, Size/2.0f, Size);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();
		}

		CTeeRenderInfo TeeInfo = m_pClient->m_aClients[i].m_RenderInfo;
		RenderTools()->RenderTee(CAnimState::GetIdle(), &TeeInfo, EMOTE_NORMAL, vec2(1.0f, 0.0f), vec2(Width/2.0f+x+20.0f, Height/2.0f+y+20.0f));

		y += LineHeight;
	}
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

	// draw cursor
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_CURSOR].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
	IGraphics::CQuadItem QuadItem(m_SelectorMouse.x+Width/2.0f, m_SelectorMouse.y+Height/2.0f, 48.0f, 48.0f);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();
}
#endif

void CSpectator::OnReset()
{
	m_WasActive = false;
	m_Active = false;
	CameraFreeview();
	m_SelectedSpectatorID = -1;
	m_MouseScreenPos = vec2(0, 0);
	m_MouseMoveTimer = 0;
}

void CSpectator::Spectate(int SpecMode, int SpectatorID)
{
	if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		m_pClient->m_DemoSpecMode = clamp(SpecMode, 0, NUM_SPECMODES-1);
		m_pClient->m_DemoSpecID = clamp(SpectatorID, -1, MAX_CLIENTS-1);
		return;
	}

	if(m_pClient->m_Snap.m_SpecInfo.m_SpecMode == SpecMode &&
	   m_pClient->m_Snap.m_SpecInfo.m_SpectatorID == SpectatorID)
		return;

	CNetMsg_Cl_SetSpectatorMode Msg;
	Msg.m_SpecMode = SpecMode;
	Msg.m_SpectatorID = SpectatorID;
	Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);
}
