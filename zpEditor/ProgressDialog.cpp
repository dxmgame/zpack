// ProgressDialog.cpp : implementation file
//

#include "stdafx.h"
#include "zpEditor.h"
#include "ProgressDialog.h"
#include "afxdialogex.h"
#include "zpExplorer.h"

// ProgressDialog dialog

const int PROGRESS_STEP = 1000;

IMPLEMENT_DYNAMIC(ProgressDialog, CDialogEx)

ProgressDialog::ProgressDialog(CWnd* pParent /*=NULL*/)
	: CDialogEx(ProgressDialog::IDD, pParent)
	, m_filename(_T("Counting files..."))
	, m_running(false)
	, m_progress(0)
	, m_totalFileSize(0)
	, m_doneFileSize(0)
	, m_thread(NULL)
{
	::InitializeCriticalSection(&m_lock);
}

ProgressDialog::~ProgressDialog()
{
	::DeleteCriticalSection(&m_lock);
}

BOOL ProgressDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_progressCtrl.SetRange(0, PROGRESS_STEP);
	SetTimer(0, 50, NULL);
	switch (m_operation)
	{
	case OP_ADD:
		SetWindowText(_T("Adding"));
		break;
	case OP_EXTRACT:
		SetWindowText(_T("Extracting"));
		break;
	case OP_DEFRAG:
		SetWindowText(_T("Moving"));
		break;
	}
	
	DWORD dwThreadID = 0;
	m_thread = ::CreateThread( NULL,
								0,
								(LPTHREAD_START_ROUTINE)(threadFunc),
								this,
								0,
								&dwThreadID );
	return TRUE;
}

void ProgressDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Text(pDX, IDC_STATIC_FILENAME, m_filename);
	DDX_Control(pDX, IDC_PROGRESS, m_progressCtrl);
}


BEGIN_MESSAGE_MAP(ProgressDialog, CDialogEx)
	ON_BN_CLICKED(IDCANCEL, &ProgressDialog::OnBnClickedCancel)
	ON_WM_TIMER()
END_MESSAGE_MAP()

void ProgressDialog::setProgress(const zp::String& currentFilename, float progress)
{
	::EnterCriticalSection(&m_lock);
	m_currentFilename = currentFilename;
	m_progress = int(progress * PROGRESS_STEP);
	::LeaveCriticalSection(&m_lock);
}

// ProgressDialog message handlers

void ProgressDialog::OnBnClickedCancel()
{
	m_running = false;
	m_filename = _T("Canceling...");
	UpdateData(FALSE);
	
	::WaitForSingleObject(m_thread, INFINITE);

	CDialogEx::OnCancel();
}

void ProgressDialog::OnTimer(UINT_PTR nIDEvent)
{
	CDialogEx::OnTimer(nIDEvent);

	::EnterCriticalSection(&m_lock);
	if (!m_currentFilename.empty())
	{
		m_filename = m_currentFilename.c_str();
	}
	m_progressCtrl.SetPos(m_progress);
	::LeaveCriticalSection(&m_lock);
	UpdateData(FALSE);	//refresh
	if (!m_running)
	{
		CDialogEx::OnCancel();
	}
}

bool ZpCallback(const zp::Char* path, zp::u32 fileSize, void* param)
{
	ProgressDialog* dlg = (ProgressDialog*)param;
	if (dlg == NULL)
	{
		return true;
	}
	if (dlg->m_running)
	{
		if (dlg->m_totalFileSize != 0)
		{
			dlg->setProgress(path, (float)dlg->m_doneFileSize / dlg->m_totalFileSize);
		}
		else
		{
			dlg->setProgress(path, 1.0f);
		}
	}
	dlg->m_doneFileSize += fileSize;
	return dlg->m_running;
}

DWORD WINAPI ProgressDialog::threadFunc(LPVOID pointer)
{
	ProgressDialog* dlg = (ProgressDialog*)pointer;
	dlg->m_explorer->setCallback(ZpCallback, dlg);
	if (dlg->m_operation == ProgressDialog::OP_DEFRAG)
	{
		dlg->m_explorer->defrag();
		dlg->m_running = false;
		return 0;
	}
	for (size_t i = 0; i < dlg->m_params->size(); ++i)
	{
		const std::pair<zp::String, zp::String>& p = dlg->m_params->at(i);
		switch (dlg->m_operation)
		{
		case ProgressDialog::OP_ADD:
			dlg->m_explorer->add(p.first, p.second, false);
			break;
		case ProgressDialog::OP_EXTRACT:
			dlg->m_explorer->extract(p.first, p.second);
			break;
		}
	}
	//单个add已经不再flush，避免一次添加产生碎片
	dlg->m_explorer->flush();
	dlg->m_running = false;
	return 0;
}
