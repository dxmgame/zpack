#pragma once
#include "afxcmn.h"
#include <string>
#include <vector>

class ZpExplorer;
// ProgressDialog dialog

class ProgressDialog : public CDialogEx
{
	friend class CzpEditorView;

	DECLARE_DYNAMIC(ProgressDialog)

public:
	ProgressDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~ProgressDialog();

	virtual BOOL OnInitDialog();

// Dialog Data
	enum { IDD = IDD_DLG_PROGRESS };

	void setProgress(const zp::String& currentFilename, float progress);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	static DWORD WINAPI threadFunc(LPVOID pointer);

	DECLARE_MESSAGE_MAP()

public:
	ZpExplorer*		m_explorer;
	bool			m_running;

	CRITICAL_SECTION	m_lock;
	zp::String			m_currentFilename;
	zp::u64				m_totalFileSize;
	zp::u64				m_doneFileSize;
	int					m_progress;

	CString m_filename;
	CProgressCtrl m_progressCtrl;

	enum Operation
	{
		OP_ADD = 0,
		OP_EXTRACT,
		OP_DEFRAG
	};
	Operation	m_operation;
	const std::vector<std::pair<zp::String, zp::String>>*	m_params;

	HANDLE	m_thread;

	afx_msg void OnBnClickedCancel();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};
