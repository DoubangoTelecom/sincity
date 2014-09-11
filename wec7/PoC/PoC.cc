// PoC.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "PoC.h"
#include "sincity/sc_api.h"

#include <assert.h>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE			g_hInst;			// current instance
HWND				g_hWndCommandBar;	// command bar handle
HWND				g_hWndDlg;

Json::Value			g_jsonConfig;

// Forward declarations of functions included in this code module:
ATOM			MyRegisterClass(HINSTANCE, LPTSTR);
BOOL			InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	AboutProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK DlgMainProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

static BOOL loadConfig();
static BOOL saveConfig();

static BOOL setText(HWND hDlg, int nIDDlgItem, __in_opt const char* lpString);

static char config_file_path[MAX_PATH] = { "./config.json" };

SCObjWrapper<SCSessionCall*>callSession;
SCObjWrapper<SCSignaling*>signalSession;
SCObjWrapper<SCSignalingCallEvent*>pendingOffer;

class SCSignalingCallbackDummy : public SCSignalingCallback
{
protected:
    SCSignalingCallbackDummy() {

    }
public:
    virtual ~SCSignalingCallbackDummy() {

    }

    virtual bool onEventNet(SCObjWrapper<SCSignalingEvent*>& e) {
        //!\Deadlock issue: You must not call any function from 'SCSignaling' class unless you fork a new thread.
        switch (e->getType()) {
			case SCSignalingEventType_NetReady:
				{
					EnableWindow(GetDlgItem(g_hWndDlg, IDC_BUTTON_CONNECT), FALSE);
					EnableWindow(GetDlgItem(g_hWndDlg, IDC_BUTTON_CALL), TRUE);
					EnableWindow(GetDlgItem(g_hWndDlg, IDC_BUTTON_DISCONNECT), TRUE);

					setText(g_hWndDlg, IDC_STATIC_INFO, "Connected!");
					break;
				}
			case SCSignalingEventType_NetDisconnected:
			case SCSignalingEventType_NetError:
				{
					EnableWindow(GetDlgItem(g_hWndDlg, IDC_BUTTON_CONNECT), TRUE);
					EnableWindow(GetDlgItem(g_hWndDlg, IDC_BUTTON_CALL), FALSE);
					EnableWindow(GetDlgItem(g_hWndDlg, IDC_BUTTON_DISCONNECT), FALSE);

					setText(g_hWndDlg, IDC_STATIC_INFO, "Disconnected!");
					break;
				}
        }

        return true;
    }
    virtual SC_INLINE const char* getObjectId() {
        return "SCSignalingCallbackDummy";
    }
    virtual bool onEventCall(SCObjWrapper<SCSignalingCallEvent*>& e) {
        //!\Deadlock issue: You must not call any function from 'SCSignaling' class unless you fork a new thread.
        if (callSession) {
            if (callSession->getCallId() != e->getCallId()) {
                SC_DEBUG_ERROR("Call id mismatch: '%s'<>'%s'", callSession->getCallId().c_str(), e->getCallId().c_str());
                return e->reject();
            }
            bool ret = callSession->handEvent(e);
			if (e->getType() == "hangup") {
				callSession = NULL;
				setText(g_hWndDlg, IDC_STATIC_INFO, "Terminated!");
				setText(g_hWndDlg, IDC_BUTTON_CALL, "call");
			}
			return ret;
        }
        else {
            if (e->getType() == "offer") {
				if (callSession || pendingOffer) { // already in call?
					return e->reject();
				}
				pendingOffer = e;
				setText(g_hWndDlg, IDC_STATIC_INFO, "Incoming call!");
				setText(g_hWndDlg, IDC_BUTTON_CALL, "accept");
            }
            // Silently ignore any other event type
        }

        return true;
    }

    static SCObjWrapper<SCSignalingCallback*> newObj() {
        return new SCSignalingCallbackDummy();
    }
};

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPTSTR    lpCmdLine,
                   int       nCmdShow)
{
	MSG msg;

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow)) 
	{
		return FALSE;
	}

	HACCEL hAccelTable;
	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_POC));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
ATOM MyRegisterClass(HINSTANCE hInstance, LPTSTR szWindowClass)
{
	WNDCLASS wc;

	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = WndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_POC));
	wc.hCursor       = 0;
	wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName  = 0;
	wc.lpszClassName = szWindowClass;

	return RegisterClass(&wc);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    HWND hWnd;
    TCHAR szTitle[MAX_LOADSTRING];		// title bar text
    TCHAR szWindowClass[MAX_LOADSTRING];	// main window class name

    g_hInst = hInstance; // Store instance handle in our global variable


    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING); 
    LoadString(hInstance, IDC_POC, szWindowClass, MAX_LOADSTRING);


    if (!MyRegisterClass(hInstance, szWindowClass))
    {
    	return FALSE;
    }

    hWnd = CreateWindow(szWindowClass, szTitle, WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);

    if (!hWnd)
    {
        return FALSE;
    }


    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    if (g_hWndCommandBar)
    {
        CommandBar_Show(g_hWndCommandBar, TRUE);
    }

    return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;

	
    switch (message) 
    {
        case WM_COMMAND:
            wmId    = LOWORD(wParam); 
            wmEvent = HIWORD(wParam); 
            // Parse the menu selections:
            switch (wmId)
            {
                case IDM_HELP_ABOUT:
                    DialogBox(g_hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, AboutProc);
                    break;
                case IDM_FILE_EXIT:
                    DestroyWindow(hWnd);
                    break;
                default:
                    return DefWindowProc(hWnd, message, wParam, lParam);
            }
            break;
        case WM_CREATE:
            g_hWndCommandBar = CommandBar_Create(g_hInst, hWnd, 1);
            CommandBar_InsertMenubar(g_hWndCommandBar, g_hInst, IDR_MENU, 0);
            CommandBar_AddAdornments(g_hWndCommandBar, 0, 0);

			DialogBox(g_hInst, (LPCTSTR)IDD_POCKETPC_LANDSCAPE, hWnd, DlgMainProc);
            break;
        case WM_PAINT:
            hdc = BeginPaint(hWnd, &ps);
            
            // TODO: Add any drawing code here...
            
            EndPaint(hWnd, &ps);
            break;
        case WM_DESTROY:
            CommandBar_Destroy(g_hWndCommandBar);
            PostQuitMessage(0);
            break;


        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

INT_PTR CALLBACK DlgMainProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
    {
		case WM_INITDIALOG:
			{
				g_hWndDlg = hDlg;

				SC_ASSERT(loadConfig());

				SC_ASSERT(SCEngine::init(g_jsonConfig["local_id"].asCString()));
				SC_ASSERT(SCEngine::setDebugLevel(g_jsonConfig["debug_level"].isNumeric() ? (SCDebugLevel_t)g_jsonConfig["debug_level"].asInt() : SCDebugLevel_Info));
				
				static const char* __entries[] = { 
					"debug_level",
					"ssl_file_pub", "ssl_file_priv", "ssl_file_ca", "connection_url", "local_id", "remote_id",
					"video_pref_size", "video_fps", "video_bandwidth_up_max", "video_bandwidth_down_max", "video_motion_rank", "video_congestion_ctrl_enabled",
					"natt_stun_server_host", "natt_stun_server_port", "natt_stun_username", "natt_stun_password", "natt_ice_stun_enabled", "natt_ice_turn_enabled" 
				};
				for (size_t i = 0; i < sizeof(__entries)/sizeof(__entries[0]); ++i)
				{
					if (g_jsonConfig[__entries[i]].isString())
					{
						SC_DEBUG_INFO("%s: %s", __entries[i], g_jsonConfig[__entries[i]].asCString());
					}
					else if (g_jsonConfig[__entries[i]].isNumeric())
					{
						SC_DEBUG_INFO("%s: %d", __entries[i], g_jsonConfig[__entries[i]].asInt());
					}
				}
				
				SC_ASSERT(SCEngine::setSSLCertificates(
					g_jsonConfig["ssl_file_pub"].isString() ? g_jsonConfig["ssl_file_pub"].asCString() : "SSL_Pub.pem", 
					g_jsonConfig["ssl_file_priv"].isString() ? g_jsonConfig["ssl_file_priv"].asCString() : "SSL_Priv.pem", 
					g_jsonConfig["ssl_file_ca"].isString() ? g_jsonConfig["ssl_file_ca"].asCString() : "SSL_CA.pem"
					));
				if (g_jsonConfig["video_pref_size"].isString())
				{
					SC_ASSERT(SCEngine::setVideoPrefSize(g_jsonConfig["video_pref_size"].asCString()));
				}
				if (g_jsonConfig["video_fps"].isNumeric())
				{
					SC_ASSERT(SCEngine::setVideoFps(g_jsonConfig["video_fps"].asInt()));
				}
				if (g_jsonConfig["video_bandwidth_up_max"].isNumeric())
				{
					SC_ASSERT(SCEngine::setVideoBandwidthUpMax(g_jsonConfig["video_bandwidth_up_max"].asInt()));
				}
				if (g_jsonConfig["video_bandwidth_down_max"].isNumeric())
				{
					SC_ASSERT(SCEngine::setVideoBandwidthDownMax(g_jsonConfig["video_bandwidth_down_max"].asInt()));
				}
				if (g_jsonConfig["video_motion_rank"].isNumeric())
				{
					SC_ASSERT(SCEngine::setVideoMotionRank(g_jsonConfig["video_motion_rank"].asInt()));
				}
				if (g_jsonConfig["video_congestion_ctrl_enabled"].isBool())
				{
					SC_ASSERT(SCEngine::setVideoCongestionCtrlEnabled(g_jsonConfig["video_congestion_ctrl_enabled"].asBool()));
				}
				if (g_jsonConfig["natt_stun_server_host"].isString() && g_jsonConfig["natt_stun_server_port"].isNumeric())
				{
					SC_ASSERT(SCEngine::setNattStunServer(g_jsonConfig["natt_stun_server_host"].asCString(), g_jsonConfig["natt_stun_server_port"].asInt()));
				}
				SC_ASSERT(SCEngine::setNattStunCredentials(
					g_jsonConfig["natt_stun_username"].isString() ? g_jsonConfig["natt_stun_username"].asCString() : NULL,
					g_jsonConfig["natt_stun_password"].isString() ? g_jsonConfig["natt_stun_password"].asCString() : NULL
					));
				if (g_jsonConfig["natt_ice_stun_enabled"].isBool())
				{
					SC_ASSERT(SCEngine::setNattIceStunEnabled(g_jsonConfig["natt_ice_stun_enabled"].asBool()));
				}
				if (g_jsonConfig["natt_ice_turn_enabled"].isBool())
				{
					SC_ASSERT(SCEngine::setNattIceTurnEnabled(g_jsonConfig["natt_ice_turn_enabled"].asBool()));
				}

				setText(hDlg, IDC_EDIT_CONNECTION_URL, g_jsonConfig["connection_url"].asCString());
				setText(hDlg, IDC_EDIT_LOCAL_ID, g_jsonConfig["local_id"].asCString());
				setText(hDlg, IDC_EDIT_REMOTE_ID, g_jsonConfig["remote_id"].asCString());

				return (INT_PTR)TRUE;
			}
		
		case WM_COMMAND:
            if ((LOWORD(wParam) == IDC_BUTTON_CONNECT))
            {
				SC_ASSERT(saveConfig());

				signalSession = SCSignaling::newObj(g_jsonConfig["connection_url"].asCString());
				SC_ASSERT(signalSession);

				SC_ASSERT(signalSession->setCallback(SCSignalingCallbackDummy::newObj()));
				SC_ASSERT(signalSession->connect());

                return (INT_PTR)TRUE;
            }
			else if ((LOWORD(wParam) == IDC_BUTTON_DISCONNECT))
            {
				if (callSession)
				{
					callSession->hangup();
					callSession = NULL;
				}
				SC_ASSERT(signalSession->disConnect());
			}
			else if ((LOWORD(wParam) == IDC_BUTTON_CALL))
            {
				setText(hDlg, IDC_STATIC_INFO, "---");
				if (pendingOffer)
				{
					setText(hDlg, IDC_BUTTON_CALL, "hangup");
					SC_ASSERT(callSession = SCSessionCall::newObj(signalSession, pendingOffer));
					SC_ASSERT(callSession->handEvent(pendingOffer));
					pendingOffer = NULL;
				}
				else if (callSession)
				{
					setText(hDlg, IDC_BUTTON_CALL, "call");
					SC_ASSERT(callSession->hangup());
					callSession = NULL;
				}
				else
				{
					setText(hDlg, IDC_BUTTON_CALL, "hangup");
					SC_ASSERT(callSession = SCSessionCall::newObj(signalSession));
					SC_ASSERT(callSession->call(SCMediaType_ScreenCast, g_jsonConfig["remote_id"].asString()));
				}
			}
            break;

		case WM_CLOSE:
            EndDialog(hDlg, message);
			PostQuitMessage(0);
            return (INT_PTR)TRUE;
	}
	return (INT_PTR)FALSE;
}

// Message handler for about box.
INT_PTR CALLBACK AboutProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
            RECT rectChild, rectParent;
            int DlgWidth, DlgHeight;	// dialog width and height in pixel units
            int NewPosX, NewPosY;

            // trying to center the About dialog
            if (GetWindowRect(hDlg, &rectChild)) 
            {
                GetClientRect(GetParent(hDlg), &rectParent);
                DlgWidth	= rectChild.right - rectChild.left;
                DlgHeight	= rectChild.bottom - rectChild.top ;
                NewPosX		= (rectParent.right - rectParent.left - DlgWidth) / 2;
                NewPosY		= (rectParent.bottom - rectParent.top - DlgHeight) / 2;
				
                // if the About box is larger than the physical screen 
                if (NewPosX < 0) NewPosX = 0;
                if (NewPosY < 0) NewPosY = 0;
                SetWindowPos(hDlg, 0, NewPosX, NewPosY,
                    0, 0, SWP_NOZORDER | SWP_NOSIZE);
            }
            return (INT_PTR)TRUE;

        case WM_COMMAND:
            if ((LOWORD(wParam) == IDOK) || (LOWORD(wParam) == IDCANCEL))
            {
                EndDialog(hDlg, LOWORD(wParam));
                return TRUE;
            }
            break;

        case WM_CLOSE:
            EndDialog(hDlg, message);
            return TRUE;

    }
    return (INT_PTR)FALSE;
}


static BOOL loadConfig()
{
	if (!SCUtils::fileExists(config_file_path)) {
		_snprintf(config_file_path, sizeof(config_file_path), "%s/%s", SCUtils::currentDirectoryPath(), "config.json");
	}

	FILE* p_file = fopen(config_file_path, "rb");
	if (!p_file) {
		SC_DEBUG_ERROR("Failed to open file at %s", config_file_path);
		return FALSE;
	}
	fseek(p_file, 0, SEEK_END);
	long fsize = ftell(p_file);
	fseek(p_file, 0, SEEK_SET);

	char *p_buffer = new char[fsize + 1];
	SC_ASSERT(p_buffer);
	p_buffer[fsize] = 0;
	SC_ASSERT(fread(p_buffer, 1, fsize, p_file) == fsize);
	std::string jsonText(p_buffer);
	fclose(p_file);
	delete[] p_buffer;

	Json::Reader reader;
	SC_ASSERT(reader.parse(jsonText, g_jsonConfig, false));
	
	return true;
}

static BOOL saveConfig()
{
	if (!SCUtils::fileExists(config_file_path)) {
		_snprintf(config_file_path, sizeof(config_file_path), "%s/%s", SCUtils::currentDirectoryPath(), "config.json");
	}

	FILE* p_file = fopen(config_file_path, "wb+");
	if (!p_file) {
		SC_DEBUG_ERROR("Failed to open file at %s", config_file_path);
		return FALSE;
	}
	Json::StyledWriter writer;
	std::string output = writer.write(g_jsonConfig);
	SC_ASSERT(!output.empty());
	SC_ASSERT(fwrite(output.c_str(), 1, output.length(), p_file) == output.length());

	return TRUE;
}

static BOOL setText(HWND hDlg, int nIDDlgItem, __in_opt const char* lpString)
{
	size_t cSize = strlen(lpString);
	wchar_t* _lpString = new wchar_t[cSize + 1];
	SC_ASSERT(_lpString);
	SC_ASSERT(mbstowcs(_lpString, lpString, cSize) == cSize);
	_lpString[cSize] = 0;
	::SetDlgItemText(hDlg, nIDDlgItem, _lpString);
	delete[]_lpString;
	return TRUE;
}