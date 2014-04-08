
// zpEditorView.h : interface of the CzpEditorView class
//

#pragma once

#include "ProgressDialog.h"

struct ZpNode;

class CzpEditorView : public CListView
{
protected: // create from serialization only
	CzpEditorView();
	DECLARE_DYNCREATE(CzpEditorView)

// Attributes
public:
	CzpEditorDoc* GetDocument() const;

// Operations
public:

// Overrides
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual void OnInitialUpdate(); // called first time after construct

	void OnDbClick(NMHDR *pNMHDR, LRESULT *pResult);

// Implementation
public:
	virtual ~CzpEditorView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);

	ZpNode* getSelectedNode();

	void startOperation(ProgressDialog::Operation op, zp::u64 totalFileSize,
					const std::vector<std::pair<zp::String, zp::String>>* operations);

	void openNode(ZpNode* node);

protected:
	afx_msg void OnStyleChanged(int nStyleType, LPSTYLESTRUCT lpStyleStruct);
	afx_msg void OnFilePrintPreview();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnEditDelete();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnEditAddFolder();
	afx_msg void OnEditAdd();
	afx_msg void OnEditExtract();
	afx_msg void OnUpdateMenu(CCmdUI* pCmdUI);
	afx_msg void OnEditOpen();
	afx_msg void OnEditExtractCur();

private:
	bool m_initiallized;
};

#ifndef _DEBUG  // debug version in zpEditorView.cpp
inline CzpEditorDoc* CzpEditorView::GetDocument() const
   { return reinterpret_cast<CzpEditorDoc*>(m_pDocument); }
#endif

