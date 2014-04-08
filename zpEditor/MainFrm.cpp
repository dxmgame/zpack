
// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "zpEditor.h"

#include "MainFrm.h"
#include "LeftView.h"
#include "zpEditorView.h"
#include "zpEditorDoc.h"
#include "ProgressDialog.h"
#include <sstream>
#include <algorithm>
#include <vector>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWndEx)

const int  iMaxUserToolbars = 10;
const UINT uiFirstUserToolBarId = AFX_IDW_CONTROLBAR_FIRST + 40;
const UINT uiLastUserToolBarId = uiFirstUserToolBarId + iMaxUserToolbars - 1;

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWndEx)
	ON_WM_CREATE()
	ON_REGISTERED_MESSAGE(AFX_WM_CREATETOOLBAR, &CMainFrame::OnToolbarCreateNew)
	ON_COMMAND(ID_FILE_NEW, &CMainFrame::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CMainFrame::OnFileOpen)
	ON_COMMAND(ID_FILE_DEFRAG, &CMainFrame::OnFileDefrag)
	ON_UPDATE_COMMAND_UI(ID_EDIT_ADD, &CMainFrame::OnUpdateMenu)
	ON_UPDATE_COMMAND_UI(ID_EDIT_ADD_FOLDER, &CMainFrame::OnUpdateMenu)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DELETE, &CMainFrame::OnUpdateMenu)
	ON_UPDATE_COMMAND_UI(ID_EDIT_EXTRACT, &CMainFrame::OnUpdateMenu)
	ON_UPDATE_COMMAND_UI(ID_EDIT_EXTRACT_CUR, &CMainFrame::OnUpdateMenu)
	ON_UPDATE_COMMAND_UI(ID_FILE_DEFRAG, &CMainFrame::OnUpdateMenu)
	ON_WM_DROPFILES()
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	// TODO: add member initialization code here
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWndEx::OnCreate(lpCreateStruct) == -1)
		return -1;

	BOOL bNameValid;

	// set the visual manager used to draw all user interface elements
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	//if (!m_wndMenuBar.Create(this))
	//{
	//	TRACE0("Failed to create menubar\n");
	//	return -1;      // fail to create
	//}

	//m_wndMenuBar.SetPaneStyle(m_wndMenuBar.GetPaneStyle() | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY);

	// prevent the menu bar from taking the focus on activation
	CMFCPopupMenu::SetForceMenuFocus(FALSE);

	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(theApp.m_bHiColorIcons ? IDR_MAINFRAME_256 : IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	CString strToolBarName;
	bNameValid = strToolBarName.LoadString(IDS_TOOLBAR_STANDARD);
	ASSERT(bNameValid);
	m_wndToolBar.SetWindowText(strToolBarName);

	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);
	//m_wndToolBar.EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);

	// Allow user-defined toolbars operations:
	InitUserToolbars(NULL, uiFirstUserToolBarId, uiLastUserToolBarId);

	if (!m_wndStatusBar.Create(this))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT));

	// TODO: Delete these five lines if you don't want the toolbar and menubar to be dockable
	//m_wndMenuBar.EnableDocking(CBRS_ALIGN_ANY);
	//m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	//EnableDocking(CBRS_ALIGN_ANY);
	//DockPane(&m_wndMenuBar);
	DockPane(&m_wndToolBar);


	// enable Visual Studio 2005 style docking window behavior
	CDockingManager::SetDockingMode(DT_SMART);
	// enable Visual Studio 2005 style docking window auto-hide behavior
	EnableAutoHidePanes(CBRS_ALIGN_ANY);

	// Enable toolbar and docking window menu replacement
	EnablePaneMenu(TRUE, ID_VIEW_CUSTOMIZE, strCustomize, ID_VIEW_TOOLBAR);

	// enable quick (Alt+drag) toolbar customization
	CMFCToolBar::EnableQuickCustomization();

	if (CMFCToolBar::GetUserImages() == NULL)
	{
		// load user-defined toolbar images
		if (m_UserImages.Load(_T(".\\UserImages.bmp")))
		{
			CMFCToolBar::SetUserImages(&m_UserImages);
		}
	}

	//CMenu* menu = m_wndToolBar.GetMenu();
	//menu->EnableMenuItem(ID_EDIT_ADD, MF_BYCOMMAND|MF_DISABLED);
	return 0;
}

BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT /*lpcs*/,
	CCreateContext* pContext)
{
	// create splitter window
	if (!m_wndSplitter.CreateStatic(this, 1, 2))
		return FALSE;

	if (!m_wndSplitter.CreateView(0, 0, RUNTIME_CLASS(CLeftView), CSize(200, 100), pContext) ||
		!m_wndSplitter.CreateView(0, 1, RUNTIME_CLASS(CzpEditorView), CSize(100, 100), pContext))
	{
		m_wndSplitter.DestroyWindow();
		return FALSE;
	}

	return TRUE;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWndEx::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return TRUE;
}

// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWndEx::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWndEx::Dump(dc);
}
#endif //_DEBUG


// CMainFrame message handlers

CzpEditorView* CMainFrame::GetRightPane()
{
	CWnd* pWnd = m_wndSplitter.GetPane(0, 1);
	CzpEditorView* pView = DYNAMIC_DOWNCAST(CzpEditorView, pWnd);
	return pView;
}

LRESULT CMainFrame::OnToolbarCreateNew(WPARAM wp,LPARAM lp)
{
	LRESULT lres = CFrameWndEx::OnToolbarCreateNew(wp,lp);
	if (lres == 0)
	{
		return 0;
	}

	CMFCToolBar* pUserToolbar = (CMFCToolBar*)lres;
	ASSERT_VALID(pUserToolbar);

	BOOL bNameValid;
	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);

	pUserToolbar->EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);
	return lres;
}

BOOL CMainFrame::LoadFrame(UINT nIDResource, DWORD dwDefaultStyle, CWnd* pParentWnd, CCreateContext* pContext) 
{
	// base class does the real work

	if (!CFrameWndEx::LoadFrame(nIDResource, dwDefaultStyle, pParentWnd, pContext))
	{
		return FALSE;
	}


	// enable customization button for all user toolbars
	BOOL bNameValid;
	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);

	for (int i = 0; i < iMaxUserToolbars; i ++)
	{
		CMFCToolBar* pUserToolbar = GetUserToolBarByIndex(i);
		if (pUserToolbar != NULL)
		{
			pUserToolbar->EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);
		}
	}
	return TRUE;
}

void CMainFrame::OnFileNew()
{
	CFileDialog dlg(FALSE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("zpack archives (*.zpk)|*.zpk|All Files (*.*)|*.*||"));
	if (dlg.DoModal() != IDOK)
	{
		return;
	}
	CString filename = dlg.GetPathName();
	if (dlg.GetFileExt().IsEmpty())
	{
		filename += ".zpk";
	}
	CzpEditorDoc* document = (CzpEditorDoc*)GetActiveDocument();
	ZpExplorer& explorer = document->GetZpExplorer();
	if (!explorer.create(filename.GetString(), _T("")))
	{
		::MessageBox(NULL, _T("Create package failed."), _T("Error"), MB_OK | MB_ICONERROR);
	}
	document->UpdateAllViews(NULL, TRUE);
	CString title = dlg.GetFileName() + _T(" - zpEditor");
	this->SetWindowText(title.GetString());
}

void CMainFrame::OnFileOpen()
{
	CFileDialog dlg(TRUE, NULL, NULL, 0, _T("zpack files (*.zpk)|*.zpk|All Files (*.*)|*.*||"));
	if (dlg.DoModal() != IDOK)
	{
		return;
	}
	CzpEditorDoc* document = (CzpEditorDoc*)GetActiveDocument();
	document->OnOpenDocument(dlg.GetPathName());
	CString title = dlg.GetFileName() + _T(" - zpEditor");
	this->SetWindowText(title.GetString());
}

void CMainFrame::OnFileDefrag()
{
	CzpEditorDoc* document = (CzpEditorDoc*)GetActiveDocument();
	ZpExplorer& explorer = document->GetZpExplorer();
	if (!explorer.isOpen())
	{
		return;
	}
	//zp::u64 fragSize = explorer.getPack()->countFragmentSize();
	//if (fragSize == 0)
	//{
	//	::MessageBox(NULL, _T("No fragment found in package."), _T("Note"), MB_OK | MB_ICONINFORMATION);
	//	return;
	//}
	//StringStream tip;
	//tip << _T("You can save ") << fragSize << _T(" bytes, continue?");
	if (::MessageBox(NULL, _T("It will take minutes for large package, continue?"), _T("Note"), MB_YESNO | MB_ICONQUESTION) != IDYES)
	{
		return;
	}

	ProgressDialog progressDlg;
	progressDlg.m_explorer = &(document->GetZpExplorer());
	progressDlg.m_running = true;
	progressDlg.m_params = NULL;
	progressDlg.m_operation = ProgressDialog::OP_DEFRAG;
	progressDlg.m_totalFileSize = explorer.countNodeFileSize(explorer.rootNode());
	progressDlg.DoModal();
}

void CMainFrame::OnUpdateMenu(CCmdUI* pCmdUI)
{
	CzpEditorDoc* document = (CzpEditorDoc*)GetActiveDocument();
	if (document == NULL)
	{
		pCmdUI->Enable(FALSE);
		return;
	}
	ZpExplorer& explorer = document->GetZpExplorer();
	if (!explorer.isOpen())
	{
		pCmdUI->Enable(FALSE);
		return;
	}
	if (explorer.getPack()->readonly()
		&& (pCmdUI->m_nID == ID_FILE_DEFRAG
			|| pCmdUI->m_nID == ID_EDIT_ADD
			|| pCmdUI->m_nID == ID_EDIT_ADD_FOLDER
			|| pCmdUI->m_nID == ID_EDIT_DELETE))
	{
		pCmdUI->Enable(FALSE);
		return;
	}
	pCmdUI->Enable(TRUE);
}


void CMainFrame::OnDropFiles(HDROP hDropInfo)
{
	SetActiveWindow();      // activate us first !

	UINT nFiles = ::DragQueryFile(hDropInfo, (UINT)-1, NULL, 0);
	if (nFiles == 1)
	{
		//if only 1 file with .zpk extension, open it as a package
		TCHAR szFileName[_MAX_PATH];
		::DragQueryFile(hDropInfo, 0, szFileName, _MAX_PATH);
		zp::String filename = szFileName;
		size_t length = filename.length();
		zp::String lowerExt;
		if (length >= 4)
		{
			zp::String temp = filename.substr(length - 4, 4);
			stringToLower(lowerExt, temp);
		}
		if (lowerExt == _T(".zpk"))
		{
			CWinApp* pApp = AfxGetApp();
			pApp->OpenDocumentFile(szFileName);
			return;
		}
	}

	//more than 1 file, try to add to package
	std::vector<zp::String> filenames(nFiles);
	for (UINT i = 0; i < nFiles; i++)
	{
		TCHAR szFileName[_MAX_PATH];
		::DragQueryFile(hDropInfo, i, szFileName, _MAX_PATH);
		filenames[i] = szFileName;
	}
	CzpEditorDoc* doc = (CzpEditorDoc*)GetActiveDocument();
	if (doc != NULL)
	{
		doc->addFilesToPackage(filenames);
	}
	::DragFinish(hDropInfo);

	//CFrameWndEx::OnDropFiles(hDropInfo);
}
