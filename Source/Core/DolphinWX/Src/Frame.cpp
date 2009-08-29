// Copyright (C) 2003 Dolphin Project.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.0.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License 2.0 for more details.

// A copy of the GPL 2.0 should have been included with the program.
// If not, see http://www.gnu.org/licenses/

// Official SVN repository and contact information can be found at
// http://code.google.com/p/dolphin-emu/



//////////////////////////////////////////////////////////////////////////////////////////
// Windows
/* ŻŻŻŻŻŻŻŻŻŻŻŻŻŻ

CFrame is the main parent window. Inside CFrame there is an m_Panel that is the parent for
the rendering window (when we render to the main window). In Windows the rendering window is
created by giving CreateWindow() m_Panel->GetHandle() as parent window and creating a new
child window to m_Panel. The new child window handle that is returned by CreateWindow() can
be accessed from Core::GetWindowHandle().

///////////////////////////////////////////////*/


// ----------------------------------------------------------------------------
// includes
// ----------------------------------------------------------------------------

#include "Common.h" // Common
#include "FileUtil.h"
#include "Timer.h"
#include "Setup.h"

#include "Globals.h" // Local
#include "Frame.h"
#include "ConfigMain.h"
#include "PluginManager.h"
#include "MemcardManager.h"
#include "CheatsWindow.h"
#include "AboutDolphin.h"
#include "GameListCtrl.h"
#include "BootManager.h"

#include "ConfigManager.h" // Core
#include "Core.h"
#include "HW/DVDInterface.h"
#include "State.h"
#include "VolumeHandler.h"

#include <wx/datetime.h> // wxWidgets

// ----------------------------------------------------------------------------
// resources
// ----------------------------------------------------------------------------

extern "C" {
#include "../resources/Dolphin.c" // Dolphin icon
#include "../resources/toolbar_browse.c"
#include "../resources/toolbar_file_open.c"
#include "../resources/toolbar_fullscreen.c"
#include "../resources/toolbar_help.c"
#include "../resources/toolbar_pause.c"
#include "../resources/toolbar_play.c"
#include "../resources/toolbar_plugin_dsp.c"
#include "../resources/toolbar_plugin_gfx.c"
#include "../resources/toolbar_plugin_options.c"
#include "../resources/toolbar_plugin_pad.c"
#include "../resources/toolbar_refresh.c"
#include "../resources/toolbar_stop.c"
#include "../resources/Boomy.h" // Theme packages
#include "../resources/Vista.h"
#include "../resources/X-Plastik.h"
#include "../resources/KDE.h"
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Windows functions. Setting the cursor with wxSetCursor() did not work in this instance.
   Probably because it's somehow reset from the WndProc() in the child window */
// ŻŻŻŻŻŻŻŻŻŻŻŻŻŻ
#ifdef _WIN32
// Declare a blank icon and one that will be the normal cursor
HCURSOR hCursor = NULL, hCursorBlank = NULL;

// Create the default cursor
void CreateCursor()
{
	hCursor = LoadCursor( NULL, IDC_ARROW );
}

void MSWSetCursor(bool Show)
{
	if(Show)
		SetCursor(hCursor);
	else
	{
		SetCursor(hCursorBlank);
		//wxSetCursor(wxCursor(wxNullCursor));
	}
}

// I could not use FindItemByHWND() instead of this, it crashed on that occation I used it */
HWND MSWGetParent_(HWND Parent)
{
	return GetParent(Parent);
}
#endif
/////////////////////////////////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////////////////////////////////
/* The CPanel class to receive MSWWindowProc messages from the video plugin. */
// ŻŻŻŻŻŻŻŻŻŻŻŻŻŻ
extern CFrame* main_frame;

class CPanel : public wxPanel
{
	public:
		CPanel(
			wxWindow* parent,
			wxWindowID id = wxID_ANY
			);

	private:
		DECLARE_EVENT_TABLE();

		#ifdef _WIN32
			// Receive WndProc messages
			WXLRESULT MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam);
		#endif
};

BEGIN_EVENT_TABLE(CPanel, wxPanel)
END_EVENT_TABLE()

CPanel::CPanel(
			wxWindow *parent,
			wxWindowID id
			)
	: wxPanel(parent, id)
{
}
int abc = 0;
#ifdef _WIN32
	WXLRESULT CPanel::MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam)
	{
		switch (nMsg)
		{
		//case WM_LBUTTONDOWN:
		//case WM_LBUTTONUP:
		//case WM_MOUSEMOVE:
		//	break;		

		// This doesn't work, strange
		//case WM_LBUTTONDBLCLK:
			//PanicAlert("Double click");
			//break;

		case WM_USER:
			switch(wParam)
			{
			// Stop
			case OPENGL_WM_USER_STOP:
				main_frame->DoStop();
				return 0; // Don't bother letting wxWidgets process this at all
			
			case OPENGL_WM_USER_CREATE:
				// We don't have a local setting for bRenderToMain but we can detect it this way instead
				//PanicAlert("main call %i  %i  %i  %i", lParam, (HWND)Core::GetWindowHandle(), MSWGetParent_((HWND)Core::GetWindowHandle()), (HWND)this->GetHWND());
				if (lParam == NULL)
					main_frame->bRenderToMain = false;
				else
					main_frame->bRenderToMain = true;
				return 0;

			case NJOY_RELOAD:
				// DirectInput in nJoy has failed
				Core::ReconnectPad();
				return 0;

			case WIIMOTE_RECONNECT:
				// The Wiimote plugin has been shut down, now reconnect the Wiimote
				//INFO_LOG(CONSOLE, "WIIMOTE_RECONNECT\n");
				Core::ReconnectWiimote();
				return 0;

			// -----------------------------------------
			#ifdef RERECORDING
			// -----------------
				case INPUT_FRAME_COUNTER:
					// Wind back the frame counter after a save state has been loaded
					Core::WindBack((int)lParam);
					return 0;
			#endif
			// -----------------------------
			}
			break;

		//default:
		//	return wxPanel::MSWWindowProc(nMsg, wParam, lParam);
		}
		
		// By default let wxWidgets do what it normally does with this event
		return wxPanel::MSWWindowProc(nMsg, wParam, lParam);
	}
#endif
/////////////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// event tables
// ----------------------------

// Notice that wxID_HELP will be processed for the 'About' menu and the toolbar
// help button.

const wxEventType wxEVT_HOST_COMMAND = wxNewEventType();

BEGIN_EVENT_TABLE(CFrame, wxFrame)
EVT_CLOSE(CFrame::OnClose)
EVT_MENU(wxID_OPEN, CFrame::OnOpen)
EVT_MENU(wxID_EXIT, CFrame::OnQuit)
EVT_MENU(IDM_HELPWEBSITE, CFrame::OnHelp)
EVT_MENU(IDM_HELPGOOGLECODE, CFrame::OnHelp)
EVT_MENU(IDM_HELPABOUT, CFrame::OnHelp)
EVT_MENU(wxID_REFRESH, CFrame::OnRefresh)
EVT_MENU(IDM_PLAY, CFrame::OnPlay)
EVT_MENU(IDM_RECORD, CFrame::OnRecord)
EVT_MENU(IDM_PLAYRECORD, CFrame::OnPlayRecording)
EVT_MENU(IDM_STOP, CFrame::OnStop)
EVT_MENU(IDM_FRAMESTEP, CFrame::OnFrameStep)
EVT_MENU(IDM_SCREENSHOT, CFrame::OnScreenshot)
EVT_MENU(IDM_CONFIG_MAIN, CFrame::OnConfigMain)
EVT_MENU(IDM_CONFIG_GFX_PLUGIN, CFrame::OnPluginGFX)
EVT_MENU(IDM_CONFIG_DSP_PLUGIN, CFrame::OnPluginDSP)
EVT_MENU(IDM_CONFIG_PAD_PLUGIN, CFrame::OnPluginPAD)
EVT_MENU(IDM_CONFIG_WIIMOTE_PLUGIN, CFrame::OnPluginWiimote)

EVT_MENU(IDM_PERSPECTIVE_0, CFrame::OnToolBar)
EVT_MENU(IDM_PERSPECTIVE_1, CFrame::OnToolBar)
EVT_MENU(IDM_TAB_SPLIT, CFrame::OnToolBar)

#if defined(HAVE_SFML) && HAVE_SFML
EVT_MENU(IDM_NETPLAY, CFrame::OnNetPlay)
#endif

EVT_MENU(IDM_BROWSE, CFrame::OnBrowse)
EVT_MENU(IDM_MEMCARD, CFrame::OnMemcard)
EVT_MENU(IDM_CHEATS, CFrame::OnShow_CheatsWindow)
EVT_MENU(IDM_INFO, CFrame::OnShow_InfoWindow)
EVT_MENU(IDM_CHANGEDISC, CFrame::OnChangeDisc)
EVT_MENU(IDM_LOAD_WII_MENU, CFrame::OnLoadWiiMenu)
EVT_MENU(IDM_TOGGLE_FULLSCREEN, CFrame::OnToggleFullscreen)
EVT_MENU(IDM_TOGGLE_DUALCORE, CFrame::OnToggleDualCore)
EVT_MENU(IDM_TOGGLE_SKIPIDLE, CFrame::OnToggleSkipIdle)
EVT_MENU(IDM_TOGGLE_TOOLBAR, CFrame::OnToggleToolbar)
EVT_MENU(IDM_TOGGLE_STATUSBAR, CFrame::OnToggleStatusbar)
EVT_MENU(IDM_LOGWINDOW, CFrame::OnToggleLogWindow)
EVT_MENU(IDM_CONSOLEWINDOW, CFrame::OnToggleConsole)

EVT_MENU(IDM_LISTDRIVES, CFrame::GameListChanged)
EVT_MENU(IDM_LISTWII,	 CFrame::GameListChanged)
EVT_MENU(IDM_LISTGC,	 CFrame::GameListChanged)
EVT_MENU(IDM_LISTWAD,	 CFrame::GameListChanged)
EVT_MENU(IDM_LISTJAP,	 CFrame::GameListChanged)
EVT_MENU(IDM_LISTPAL,	 CFrame::GameListChanged)
EVT_MENU(IDM_LISTUSA,	 CFrame::GameListChanged)
EVT_MENU(IDM_PURGECACHE, CFrame::GameListChanged)

EVT_MENU(IDM_LOADLASTSTATE, CFrame::OnLoadLastState)
EVT_MENU(IDM_UNDOLOADSTATE,     CFrame::OnUndoLoadState)
EVT_MENU(IDM_UNDOSAVESTATE,     CFrame::OnUndoSaveState)
EVT_MENU(IDM_LOADSTATEFILE, CFrame::OnLoadStateFromFile)
EVT_MENU(IDM_SAVESTATEFILE, CFrame::OnSaveStateToFile)

EVT_MENU_RANGE(IDM_LOADSLOT1, IDM_LOADSLOT8, CFrame::OnLoadState)
EVT_MENU_RANGE(IDM_SAVESLOT1, IDM_SAVESLOT8, CFrame::OnSaveState)
EVT_MENU_RANGE(IDM_FRAMESKIP0, IDM_FRAMESKIP9, CFrame::OnFrameSkip)
EVT_MENU_RANGE(IDM_DRIVE1, IDM_DRIVE24, CFrame::OnBootDrive)

EVT_SIZE(CFrame::OnResize)
EVT_LIST_ITEM_ACTIVATED(LIST_CTRL, CFrame::OnGameListCtrl_ItemActivated)
EVT_HOST_COMMAND(wxID_ANY, CFrame::OnHostMessage)
#if wxUSE_TIMER
	EVT_TIMER(wxID_ANY, CFrame::OnTimer)
#endif

// Debugger Menu Entries
EVT_MENU(wxID_ANY, CFrame::PostEvent)
EVT_TEXT(wxID_ANY, CFrame::PostEvent)

//EVT_MENU_HIGHLIGHT_ALL(CFrame::PostMenuEvent)
//EVT_UPDATE_UI(wxID_ANY, CFrame::PostUpdateUIEvent)

EVT_AUINOTEBOOK_PAGE_CLOSE(wxID_ANY, CFrame::OnNotebookPageClose)
EVT_AUINOTEBOOK_ALLOW_DND(wxID_ANY, CFrame::OnAllowNotebookDnD)
EVT_AUINOTEBOOK_PAGE_CHANGED(wxID_ANY, CFrame::OnNotebookPageChanged)

END_EVENT_TABLE()
/////////////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Creation and close, quit functions
// ŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻ
CFrame::CFrame(bool showLogWindow,
		wxFrame* parent,
		wxWindowID id,
		const wxString& title,
		const wxPoint& pos,
		const wxSize& size,
		bool _UseDebugger,
		long style)
	: wxFrame(parent, id, title, pos, size, style)
	, UseDebugger(_UseDebugger), m_LogWindow(NULL)
	, m_pStatusBar(NULL), bRenderToMain(true), HaveLeds(false)
	, HaveSpeakers(false), m_Panel(NULL), m_ToolBar(NULL), m_ToolBarDebug(NULL)
	, m_bLogWindow(showLogWindow || SConfig::GetInstance().m_InterfaceLogWindow)
	, m_fLastClickTime(0), m_iLastMotionTime(0), LastMouseX(0), LastMouseY(0)
	#if wxUSE_TIMER
		, m_timer(this)
	#endif
          
{
	// Give it a console
	ConsoleListener *Console = LogManager::GetInstance()->getConsoleListener();
	if (SConfig::GetInstance().m_InterfaceConsole) Console->Open();

	// Default
	m_NB.resize(3); for (int i = 0; i < m_NB.size(); i++) m_NB[i] = NULL;

	// Start debugging mazimized
	if (UseDebugger) this->Maximize(true);
	// Debugger class
	if (UseDebugger)
		g_pCodeWindow = new CCodeWindow(SConfig::GetInstance().m_LocalCoreStartupParameter, this, this);

	// Create timer
	#if wxUSE_TIMER
		int TimesPerSecond = 10; // We don't need more than this
		m_timer.Start( floor((double)(1000 / TimesPerSecond)) );
	#endif

	// Create toolbar bitmaps	
	InitBitmaps();

	// Give it an icon
	wxIcon IconTemp;
	IconTemp.CopyFromBitmap(wxGetBitmapFromMemory(dolphin_ico32x32));
	SetIcon(IconTemp);

	// Give it a status bar
	m_pStatusBar = CreateStatusBar(1, wxST_SIZEGRIP, ID_STATUSBAR);
	if (!SConfig::GetInstance().m_InterfaceStatusbar)
		m_pStatusBar->Hide();

	// Give it a menu bar
	CreateMenu();

	// -------------------------------------------------------------------------
	// Panels
	// ŻŻŻŻŻŻŻŻŻŻŻŻŻ
	// This panel is the parent for rendering and it holds the gamelistctrl
	m_Panel = new CPanel(this, IDM_MPANEL);
	//wxPanel * m_Panel2 = new wxPanel(this, wxID_ANY);

	static int Style = wxAUI_NB_TOP | wxAUI_NB_SCROLL_BUTTONS | wxAUI_NB_TAB_EXTERNAL_MOVE | wxNO_BORDER;
	wxBitmap aNormalFile = wxArtProvider::GetBitmap(wxART_NORMAL_FILE, wxART_OTHER, wxSize(16,16));

	if (UseDebugger)
	{
		m_NB[0] = new wxAuiNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, Style);
		m_NB[1] = new wxAuiNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, Style);
		m_NB[2] = new wxAuiNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, Style);		
		m_NB[g_pCodeWindow->iCodeWindow]->AddPage(g_pCodeWindow, wxT("Code"), false, aNormalFile);
	}
	else
	{
		m_NB[0] = new wxAuiNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, Style);
	}
	// -------------------------------------------------------------------------

	m_GameListCtrl = new CGameListCtrl(m_Panel, LIST_CTRL,
			wxDefaultPosition, wxDefaultSize,
			wxLC_REPORT | wxSUNKEN_BORDER | wxLC_ALIGN_LEFT);

	sizerPanel = new wxBoxSizer(wxHORIZONTAL);
	sizerPanel->Add(m_GameListCtrl, 1, wxEXPAND | wxALL);
	m_Panel->SetSizer(sizerPanel);

	m_Mgr = new wxAuiManager();
	m_Mgr->SetManagedWindow(this);


	// Normal perspectives
	/*
	----------
	| Pane 0 |
	----------
	-------------------
	| Pane 0 | Pane 1 |
	-------------------
	*/
	// Debug perspectives
	/*
	-------------------
	| Pane 0 |        |
	|--------| Pane 2 |
	| Pane 1 |        |
	-------------------
	----------------------------
	| Pane 0 |        |        |
	|--------| Pane 2 | Pane 3 |
	| Pane 1 |        |        |
	----------------------------
	*/

	if (UseDebugger)
	{
		m_Mgr->AddPane(m_Panel, wxAuiPaneInfo().Name(wxT("Pane0")).Caption(wxT("Pane0")).Hide());
		m_Mgr->AddPane(m_NB[0], wxAuiPaneInfo().Name(wxT("Pane1")).Caption(wxT("Pane1")).Hide());
		m_Mgr->AddPane(m_NB[1], wxAuiPaneInfo().Name(wxT("Pane2")).Caption(wxT("Pane2")).Hide());
		m_Mgr->AddPane(m_NB[2], wxAuiPaneInfo().Name(wxT("Pane3")).Caption(wxT("Pane3")).Hide());
	}
	else
	{
		m_Mgr->AddPane(m_Panel, wxAuiPaneInfo().Name(wxT("Pane0")).Caption(wxT("Pane0")).Hide());
		m_Mgr->AddPane(m_NB[0], wxAuiPaneInfo().Name(wxT("Pane1")).Caption(wxT("Pane1")).Hide());
	}

	// Setup perspectives
	if (UseDebugger)
	{		
		m_Mgr->GetPane(wxT("Pane0")).CenterPane().PaneBorder(false);
		AuiFullscreen = m_Mgr->SavePerspective();
	}
	else
	{
		m_Mgr->GetPane(wxT("Pane0")).Show().PaneBorder(false).CaptionVisible(false).Layer(0).Center();
		AuiFullscreen = m_Mgr->SavePerspective();
	}

	// Create toolbar
	RecreateToolbar();
	if (!SConfig::GetInstance().m_InterfaceToolbar) DoToggleToolbar(false);

	// Setup perspectives
	if (UseDebugger)
	{
		m_Mgr->GetPane(wxT("Pane0")).Show().PaneBorder(true).CaptionVisible(false).Layer(0).Center().Position(0);
		m_Mgr->GetPane(wxT("Pane1")).Show().PaneBorder(true).CaptionVisible(false).Layer(0).Center().Position(1);
		m_Mgr->GetPane(wxT("Pane2")).Show().PaneBorder(true).CaptionVisible(false).Layer(0).Right();
		AuiPerspective.Add(m_Mgr->SavePerspective());
	
		m_Mgr->GetPane(wxT("Pane0")).Left();
		m_Mgr->GetPane(wxT("Pane1")).Left();
		m_Mgr->GetPane(wxT("Pane2")).Center();
		m_Mgr->GetPane(wxT("Pane3")).Show().PaneBorder(true).CaptionVisible(false).Right();
		AuiPerspective.Add(m_Mgr->SavePerspective());		
		
		// Load perspective
		int iPerspective;		
		IniFile ini;
		ini.Load(DEBUGGER_CONFIG_FILE);
		ini.Get("Perspectives", "Perspective", &iPerspective, 0);
		ini.Get("Perspective 0", "LeftWidth", &iLeftWidth[0], 50);
		ini.Get("Perspective 0", "AutomaticStart", &iLeftWidth[1], 33);
		ini.Get("Perspective 1", "BootToPause", &iMidWidth[1], 33);
		DoLoadPerspective(iPerspective);
	}
	else
	{
		m_Mgr->GetPane(wxT("Pane1")).Hide().PaneBorder(false).CaptionVisible(false).Layer(0).Right();
	}

	// Show titles to position the panes
	/*
	m_Mgr->GetPane(wxT("Pane0")).CaptionVisible(true);
	m_Mgr->GetPane(wxT("Pane1")).CaptionVisible(true);
	m_Mgr->GetPane(wxT("Pane2")).CaptionVisible(true);
	m_Mgr->GetPane(wxT("Pane3")).CaptionVisible(true);
	*/

	// Show window
	Show();

	// Create list of available plugins for the configuration window
	CPluginManager::GetInstance().ScanForPlugins();

	// Open notebook pages
	if (UseDebugger) g_pCodeWindow->OpenPages();
	if (m_bLogWindow) ToggleLogWindow(true, UseDebugger ? 1 : 0);
	if (SConfig::GetInstance().m_InterfaceConsole) ToggleConsole(true, UseDebugger ? 1 : 0);
	if (!UseDebugger) SetSimplePaneSize();

	//if we are ever going back to optional iso caching:
	//m_GameListCtrl->Update(SConfig::GetInstance().m_LocalCoreStartupParameter.bEnableIsoCache);
	m_GameListCtrl->Update();
	//sizerPanel->SetSizeHints(m_Panel);

	// Commit 
	m_Mgr->Update();

	// Create cursors
	#ifdef _WIN32
		CreateCursor();
	#endif

	// -------------------------
	// Connect event handlers
	// ----------
	wxTheApp->Connect(wxID_ANY, wxEVT_KEY_DOWN, // Keyboard
		wxKeyEventHandler(CFrame::OnKeyDown),
		(wxObject*)0, this);
	wxTheApp->Connect(wxID_ANY, wxEVT_KEY_UP,
		wxKeyEventHandler(CFrame::OnKeyUp),
		(wxObject*)0, this);

	m_Mgr->Connect(wxID_ANY, wxEVT_AUI_RENDER, // Resize
		wxAuiManagerEventHandler(CFrame::OnManagerResize),
		(wxObject*)0, this);	

	#ifdef _WIN32 // The functions are only tested in Windows so far
		wxTheApp->Connect(wxID_ANY, wxEVT_LEFT_DOWN,
			wxMouseEventHandler(CFrame::OnDoubleClick),
			(wxObject*)0, this);
		wxTheApp->Connect(wxID_ANY, wxEVT_MOTION,
			wxMouseEventHandler(CFrame::OnMotion),
			(wxObject*)0, this);
	#endif
	// ----------

	// Update controls
	UpdateGUI();

	// If we are rerecording create the status bar now instead of later when a game starts
	#ifdef RERECORDING
		ModifyStatusBar();
		// It's to early for the OnHostMessage(), we will update the status when Ctrl or Space is pressed
		//Core::WriteStatus();
	#endif
}
// Destructor
CFrame::~CFrame()
{
	cdio_free_device_list(drives);
	/* The statbar sample has this so I add this to, but I guess timer will be deleted after
	   this anyway */
	#if wxUSE_TIMER
		if (m_timer.IsRunning()) m_timer.Stop();
	#endif
}

void CFrame::OnQuit(wxCommandEvent& WXUNUSED (event))
{
	Close(true);
}

void CFrame::OnClose(wxCloseEvent& event)
{
	// Don't forget the skip or the window won't be destroyed
	event.Skip();
	// Save GUI settings
	if (UseDebugger) g_pCodeWindow->Save();
	if (UseDebugger) Save();

	if (Core::GetState() != Core::CORE_UNINITIALIZED)
	{
		Core::Stop();
		UpdateGUI();
	}
}

void CFrame::DoFullscreen(bool _F)
{
	ShowFullScreen(_F);
	if (_F)
	{
		// Save the current mode before going to fullscreen
		AuiCurrent = m_Mgr->SavePerspective();
		m_Mgr->LoadPerspective(AuiFullscreen, true);
	}
	else
	{
		// Restore saved perspective
		m_Mgr->LoadPerspective(AuiCurrent, true);
	}
}
void CFrame::SetSimplePaneSize()
{
	wxArrayInt i, j;
	i.Add(0); j.Add(50);
	i.Add(1); j.Add(50);
	SetPaneSize(i, j);
}
void CFrame::SetPaneSize(wxArrayInt Pane, wxArrayInt Size)
{
	int iClientSize = this->GetSize().GetX();

	for (int i = 0; i < Pane.size(); i++)
	{
		// Check limits
		if (Size[i] > 95) Size[i] = 95; if (Size[i] < 5) Size[i] = 5;
		// Produce pixel width from percentage width
		Size[i] = iClientSize * (float)(Size[i]/100.0);
		// Update size
		m_Mgr->GetPane(wxString::Format(wxT("Pane%i"), Pane[i])).BestSize(Size[i], -1).MinSize(Size[i], -1).MaxSize(Size[i], -1);
	}
	m_Mgr->Update();
	for (int i = 0; i < Pane.size(); i++)
	{
		// Remove the size limits
		m_Mgr->GetPane(wxString::Format(wxT("Pane%i"), Pane[i])).MinSize(-1, -1).MaxSize(-1, -1);
	}
}
void CFrame::DoLoadPerspective(int Perspective)
{
	Save();

	m_Mgr->LoadPerspective(AuiPerspective[Perspective], true);

	int _iRightWidth, iClientSize = this->GetSize().GetX();
	wxArrayInt i, j;

	// Set the size
	if (Perspective == 0)
	{
		_iRightWidth = 100 - iLeftWidth[0];	
		i.Add(0); j.Add(iLeftWidth[0]);
		i.Add(2); j.Add(_iRightWidth);
		//m_Mgr->GetPane(wxT("Pane1")).BestSize(_iLeftWidth, -1).MinSize(_iLeftWidth, -1).MaxSize(_iLeftWidth, -1);	
		//m_Mgr->GetPane(wxT("Pane2")).BestSize(_iRightWidth, -1).MinSize(_iRightWidth, -1).MaxSize(_iRightWidth, -1);
	}
	else
	{
		_iRightWidth = 100 - iLeftWidth[1] - iMidWidth[1];	
		i.Add(0); j.Add(iLeftWidth[1]);
		i.Add(2); j.Add(iMidWidth[1]);
		i.Add(3); j.Add(_iRightWidth);

		//m_Mgr->GetPane(wxT("Pane0")).BestSize(_iLeftWidth, -1).MinSize(_iLeftWidth, -1).MaxSize(_iLeftWidth, -1);
		//m_Mgr->GetPane(wxT("Pane1")).BestSize(_iLeftWidth, -1).MinSize(_iLeftWidth, -1).MaxSize(_iLeftWidth, -1);	
		//m_Mgr->GetPane(wxT("Pane2")).BestSize(_iMidWidth, -1).MinSize(_iMidWidth, -1).MaxSize(_iMidWidth, -1);
		//m_Mgr->GetPane(wxT("Pane3")).BestSize(_iRightWidth, -1).MinSize(_iRightWidth, -1).MaxSize(_iRightWidth, -1);
	}
	SetPaneSize(i, j);
}
void CFrame::Save()
{
	if (!m_Mgr->GetPane(wxT("Pane0")).IsOk() || !m_Mgr->GetPane(wxT("Pane2")).IsOk() ) return;

	// Get client size
	int _iLeftWidth, _iMidWidth, iClientSize = this->GetSize().GetX();
	_iLeftWidth = (int)(((float)m_Mgr->GetPane(wxT("Pane0")).window->GetClientSize().GetX() / (float)iClientSize) * 100.0);
	_iMidWidth = (int)(((float)m_Mgr->GetPane(wxT("Pane2")).window->GetClientSize().GetX() / (float)iClientSize) * 100.0);

	IniFile ini;
	ini.Load(DEBUGGER_CONFIG_FILE);
	ini.Set("Perspectives", "Perspective", m_Mgr->GetPane(wxT("Pane3")).IsShown() ? 1 : 0);
	if (!m_Mgr->GetPane(wxT("Pane3")).IsShown())
	{
		ini.Set("Perspective 0", "LeftWidth", _iLeftWidth);
	}
	else
	{
		ini.Set("Perspective 1", "LeftWidth", _iLeftWidth);
		ini.Set("Perspective 1", "MidWidth", _iMidWidth);
	}
	ini.Save(DEBUGGER_CONFIG_FILE);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Host messages
// ŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻ
#ifdef _WIN32
WXLRESULT CFrame::MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam)
{
	switch (nMsg)
	{
	case WM_SYSCOMMAND:
		switch (wParam & 0xFFF0)
		{
		case SC_SCREENSAVE:
		case SC_MONITORPOWER:
			return 0;
		}
	default:
		// Let wxWidgets process it as normal
		return wxFrame::MSWWindowProc(nMsg, wParam, lParam);
	}
}
#endif

void CFrame::OnHostMessage(wxCommandEvent& event)
{
	switch (event.GetId())
	{
	case IDM_UPDATEGUI:
		UpdateGUI();
		break;

	case IDM_UPDATESTATUSBAR:
		if (m_pStatusBar != NULL)
		{
			m_pStatusBar->SetStatusText(event.GetString(), event.GetInt());
		}
		break;
	}
}

// Post events
// Warning: This may cause an endless loop if the event is propagated back to its parent
void CFrame::PostEvent(wxCommandEvent& event)
{
	event.Skip();
	event.StopPropagation();

	if (g_pCodeWindow
		&& event.GetId() >= IDM_INTERPRETER && event.GetId() <= IDM_ADDRBOX
		&& event.GetId() != IDM_JITUNLIMITED
		)
		wxPostEvent(g_pCodeWindow, event);
}
void CFrame::PostMenuEvent(wxMenuEvent& event)
{
	if (g_pCodeWindow) wxPostEvent(g_pCodeWindow, event);
}
void CFrame::PostUpdateUIEvent(wxUpdateUIEvent& event)
{
	if (g_pCodeWindow) wxPostEvent(g_pCodeWindow, event);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Input
// ŻŻŻŻŻŻŻŻŻŻŻŻŻŻ
void CFrame::OnGameListCtrl_ItemActivated(wxListEvent& WXUNUSED (event))
{
	// Show all platforms and regions if...
	// 1. All platforms are set to hide
	// 2. All Regions are set to hide
	// Otherwise call BootGame to either...
	// 1. Boot the selected iso
	// 2. Boot the default or last loaded iso.
	// 3. Call BrowseForDirectory if the gamelist is empty
	if (!m_GameListCtrl->GetGameNames().size() &&
		!((SConfig::GetInstance().m_ListGC ||
		SConfig::GetInstance().m_ListWii ||
		SConfig::GetInstance().m_ListWad) &&
		(SConfig::GetInstance().m_ListJap ||
		SConfig::GetInstance().m_ListUsa  ||
		SConfig::GetInstance().m_ListPal)))
	{
		SConfig::GetInstance().m_ListGC  =	SConfig::GetInstance().m_ListWii =
		SConfig::GetInstance().m_ListWad =	SConfig::GetInstance().m_ListJap =
		SConfig::GetInstance().m_ListUsa =	SConfig::GetInstance().m_ListPal = true;

		GetMenuBar()->FindItem(IDM_LISTGC)->Check(true);
		GetMenuBar()->FindItem(IDM_LISTWII)->Check(true);
		GetMenuBar()->FindItem(IDM_LISTWAD)->Check(true);
		GetMenuBar()->FindItem(IDM_LISTJAP)->Check(true);
		GetMenuBar()->FindItem(IDM_LISTUSA)->Check(true);
		GetMenuBar()->FindItem(IDM_LISTPAL)->Check(true);

		m_GameListCtrl->Update();
	}			
	else BootGame();
}

void CFrame::OnKeyDown(wxKeyEvent& event)
{
	// Escape key turn off fullscreen then Stop emulation in windowed mode
	if (event.GetKeyCode() == WXK_ESCAPE)
	{
		// Temporary solution to double esc keydown. When the OpenGL plugin is running all esc keydowns are duplicated
		// I'm guessing it's coming from the OpenGL plugin but I couldn't find the source of it so I added this until
		// the source of the problem surfaces.
		static double Time = 0;
		if (Common::Timer::GetDoubleTime()-1 < Time) return;
		Time = Common::Timer::GetDoubleTime();

		DoFullscreen(!IsFullScreen());
		if (IsFullScreen())
		{
			#ifdef _WIN32
			MSWSetCursor(true);
			#endif
		}	
		//UpdateGUI();
	}
	if (event.GetKeyCode() == WXK_RETURN && event.GetModifiers() == wxMOD_ALT)
	{
		// For some reasons, wxWidget doesn't proccess the Alt+Enter event there on windows.
		// But still, pressing Alt+Enter make it Fullscreen, So this is for other OS... :P
		DoFullscreen(!IsFullScreen());
	}
#ifdef _WIN32
	if(event.GetKeyCode() == 'M', '3', '4', '5', '6') // Send this to the video plugin WndProc
	{
		PostMessage((HWND)Core::GetWindowHandle(), WM_KEYDOWN, event.GetKeyCode(), 0);
	}
#endif

#ifdef RERECORDING
	// Turn on or off frame advance
	if (event.GetKeyCode() == WXK_CONTROL) Core::FrameStepOnOff();

	// Step forward
	if (event.GetKeyCode() == WXK_SPACE) Core::FrameAdvance();
#endif

	// Send the keyboard status to the Input plugin
	if(Core::GetState() != Core::CORE_UNINITIALIZED)
	    CPluginManager::GetInstance().GetPad(0)->PAD_Input(event.GetKeyCode(), 1); // 1 = Down

	// Don't block other events
	event.Skip();
}

void CFrame::OnKeyUp(wxKeyEvent& event)
{
	if(Core::GetState() != Core::CORE_UNINITIALIZED)
		CPluginManager::GetInstance().GetPad(0)->PAD_Input(event.GetKeyCode(), 0); // 0 = Up
	event.Skip();
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Detect double click. Kind of, for some reason we have to manually create the double click for now.
// ŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻ
void CFrame::OnDoubleClick(wxMouseEvent& event)
{
	 // Don't block the mouse click
	event.Skip();

	// Don't use this in Wii mode since we use the mouse as input to the game there
	if (SConfig::GetInstance().m_LocalCoreStartupParameter.bWii) return;

	// Only detect double clicks in the rendering window, and only use this when a game is running
	if(Core::GetState() == Core::CORE_UNINITIALIZED || event.GetId() != IDM_MPANEL) return;

	// For first click just save the time
	if(m_fLastClickTime == 0) { m_fLastClickTime = Common::Timer::GetDoubleTime(); return; }

	// -------------------------------------------
	/* Manually detect double clicks since both wxEVT_LEFT_DCLICK and WM_LBUTTONDBLCLK stops
	   working after the child window is created by the plugin */
	// ----------------------
	double TmpTime = Common::Timer::GetDoubleTime();
	int Elapsed = (TmpTime - m_fLastClickTime) * 1000;

	// Get the double click time, if avaliable
	int DoubleClickTime;
	#ifdef _WIN32
		DoubleClickTime = GetDoubleClickTime();
	#else
		DoubleClickTime = 500; // The default in Windows
	#endif

	m_fLastClickTime = TmpTime; // Save the click time

	if (Elapsed < DoubleClickTime)
	{
		DoFullscreen(!IsFullScreen());
		#ifdef _WIN32
			MSWSetCursor(true); // Show the cursor again, in case it was hidden
		#endif
		m_fLastClickTime -= 10; // Don't treat repeated clicks as double clicks
	}

	UpdateGUI();
}


// Check for mouse motion. Here we process the bHideCursor setting.
// ŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻ
void CFrame::OnMotion(wxMouseEvent& event)
{
	event.Skip();

	// The following is only interesting when a game is running
	if(Core::GetState() == Core::CORE_UNINITIALIZED) return;

	/* For some reason WM_MOUSEMOVE events are sent from the plugin even when there is no movement
	   so we have to check that the cursor position has actually changed */
	if(bRenderToMain) //
	{
		bool PositionIdentical = false;
		if (event.GetX() == LastMouseX && event.GetY() == LastMouseY) PositionIdentical = true;
		LastMouseX = event.GetX(); LastMouseY = event.GetY();
		if(PositionIdentical) return;
	}

	// Now we know that we have an actual mouse movement event

	// Update motion for the auto hide option and return
	if(IsFullScreen() && SConfig::GetInstance().m_LocalCoreStartupParameter.bAutoHideCursor)
	{
		m_iLastMotionTime = Common::Timer::GetDoubleTime();
		#ifdef _WIN32
				MSWSetCursor(true);
		#endif
		return;
	}

	if(SConfig::GetInstance().m_LocalCoreStartupParameter.bHideCursor && event.GetId() == IDM_MPANEL)
	{
		#ifdef _WIN32
			if(bRenderToMain) MSWSetCursor(false);

			/* We only need to use this if we are rendering to a separate window. It does work
			   for rendering to the main window to, but in that case our MSWSetCursor() works to
			   so we can use that instead. If we one day determine that the separate window
			   rendering is superfluous we could do without this */
			else PostMessage((HWND)Core::GetWindowHandle(), WM_USER, 10, 0);
		#endif
	}

	// For some reason we need this to, otherwise the cursor can get stuck with the resizing arrows
	else
	{
		#ifdef _WIN32
			if(bRenderToMain) MSWSetCursor(true);
			else PostMessage((HWND)Core::GetWindowHandle(), WM_USER, 10, 1);
		#endif
	}

}

// Check for mouse status a couple of times per second for the auto hide option
// ŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻ
#if wxUSE_TIMER
void CFrame::Update()
{
	// Check if auto hide is on, or if we are already hiding the cursor all the time
	if(!SConfig::GetInstance().m_LocalCoreStartupParameter.bAutoHideCursor
		|| SConfig::GetInstance().m_LocalCoreStartupParameter.bHideCursor) return;

	if(IsFullScreen())
	{
		int HideDelay = 1; // Wait 1 second to hide the cursor, just like Windows Media Player
		double TmpSeconds = Common::Timer::GetDoubleTime(); // Get timestamp
		double CompareTime = TmpSeconds - HideDelay; // Compare it

		if(m_iLastMotionTime < CompareTime) // Update cursor
		#ifdef _WIN32
			MSWSetCursor(false);
		#else
			{}
		#endif
	}
}
#endif
/////////////////////////////////////////////////////////////////////////////////////////////////////////