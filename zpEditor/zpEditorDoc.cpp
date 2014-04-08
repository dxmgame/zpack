
// zpEditorDoc.cpp : implementation of the CzpEditorDoc class
//

#include "stdafx.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "zpEditor.h"
#endif

#include "zpEditorDoc.h"
#include "ProgressDialog.h"

#include <propkey.h>
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CzpEditorDoc

IMPLEMENT_DYNCREATE(CzpEditorDoc, CDocument)

BEGIN_MESSAGE_MAP(CzpEditorDoc, CDocument)
END_MESSAGE_MAP()


// CzpEditorDoc construction/destruction

CzpEditorDoc::CzpEditorDoc()
{
	// TODO: add one-time construction code here

}

CzpEditorDoc::~CzpEditorDoc()
{
}

BOOL CzpEditorDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}

BOOL CzpEditorDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	zp::String filename = lpszPathName;

	if (!m_explorer.open(lpszPathName, false)
		&& !m_explorer.open(lpszPathName, true))
	{
		::MessageBox(NULL, _T("Invalid zpack file or version mismatch."), _T("Error"), MB_OK | MB_ICONERROR);
		return FALSE;
	}
	size_t pos = filename.find_last_of(_T('\\'));
	if (pos != std::string::npos)
	{
		::SetCurrentDirectory(filename.substr(0, pos).c_str());
	}

	UpdateAllViews(NULL, TRUE);
	return TRUE;
}

//double g_addTime = 0;

void CzpEditorDoc::addFilesToPackage(std::vector<zp::String>& filenames)
{
	if (!m_explorer.isOpen())
	{
		return;
	}

	//__int64 perfBefore, perfAfter, perfFreq;
	//::QueryPerformanceFrequency((LARGE_INTEGER*)&perfFreq);
	//::QueryPerformanceCounter((LARGE_INTEGER*)&perfBefore);

	std::vector<std::pair<zp::String, zp::String>> params;
	zp::u64 totalFileSize = 0;
	for (zp::u32 i = 0; i < filenames.size(); ++i)
	{
		totalFileSize += m_explorer.countDiskFileSize(filenames[i]);
		params.push_back(std::make_pair(filenames[i], _T("")));
	}
	startOperation(ProgressDialog::OP_ADD, totalFileSize, &params);

	//::QueryPerformanceCounter((LARGE_INTEGER*)&perfAfter);
	//double perfTime = 1000.0 * (perfAfter - perfBefore) / perfFreq;
	//g_addTime += perfTime;

	//temp
	//extern double g_compressTime;
	//extern double g_addFileTime;
	//extern double g_readTime;
	//char out[128];
	//sprintf(out, "total:%fms, readTime:%fms, addFile:%fms, compress:%fms", g_addTime, g_readTime, g_addFileTime, g_compressTime);
	//::MessageBoxA(NULL, out, "compress", MB_OK);
	//g_compressTime = 0;
	//g_addFileTime = 0;
	//g_addTime = 0;
	//g_readTime = 0;

	UpdateAllViews(NULL);
}

// CzpEditorDoc serialization

void CzpEditorDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

#ifdef SHARED_HANDLERS

// Support for thumbnails
void CzpEditorDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
{
	// Modify this code to draw the document's data
	dc.FillSolidRect(lprcBounds, RGB(255, 255, 255));

	CString strText = _T("TODO: implement thumbnail drawing here");
	LOGFONT lf;

	CFont* pDefaultGUIFont = CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT));
	pDefaultGUIFont->GetLogFont(&lf);
	lf.lfHeight = 36;

	CFont fontDraw;
	fontDraw.CreateFontIndirect(&lf);

	CFont* pOldFont = dc.SelectObject(&fontDraw);
	dc.DrawText(strText, lprcBounds, DT_CENTER | DT_WORDBREAK);
	dc.SelectObject(pOldFont);
}

// Support for Search Handlers
void CzpEditorDoc::InitializeSearchContent()
{
	CString strSearchContent;
	// Set search contents from document's data. 
	// The content parts should be separated by ";"

	// For example:  strSearchContent = _T("point;rectangle;circle;ole object;");
	SetSearchContent(strSearchContent);
}

void CzpEditorDoc::SetSearchContent(const CString& value)
{
	if (value.IsEmpty())
	{
		RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
	}
	else
	{
		CMFCFilterChunkValueImpl *pChunk = NULL;
		ATLTRY(pChunk = new CMFCFilterChunkValueImpl);
		if (pChunk != NULL)
		{
			pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
			SetChunkValue(pChunk);
		}
	}
}

#endif // SHARED_HANDLERS

// CzpEditorDoc diagnostics

#ifdef _DEBUG
void CzpEditorDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CzpEditorDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CzpEditorDoc commands

ZpExplorer& CzpEditorDoc::GetZpExplorer()
{
	return m_explorer;
}

void CzpEditorDoc::startOperation(ProgressDialog::Operation op, zp::u64 totalFileSize,
							const std::vector<std::pair<zp::String, zp::String>>* params)
{
	ProgressDialog progressDlg;
	progressDlg.m_explorer = &m_explorer;
	progressDlg.m_running = true;
	progressDlg.m_params = params;
	progressDlg.m_operation = op;
	progressDlg.m_totalFileSize = totalFileSize;
	progressDlg.DoModal();
}