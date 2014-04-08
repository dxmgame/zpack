
// LeftView.h : interface of the CLeftView class
//

#pragma once
#include "ProgressDialog.h"

struct ZpNode;
class CzpEditorDoc;

class CLeftView : public CTreeView
{
protected: // create from serialization only
	CLeftView();
	DECLARE_DYNCREATE(CLeftView)

// Attributes
public:
	CzpEditorDoc* GetDocument();

// Operations
public:

// Overrides
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual void OnInitialUpdate(); // called first time after construct

	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
// Implementation
public:
	virtual ~CLeftView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	void updateNode(HTREEITEM ti);
	void OnSelectChanged(NMHDR *pNMHDR, LRESULT *pResult);

	ZpNode* getSelectedNode();
	void startOperation(ProgressDialog::Operation op, zp::u64 totalFileSize,
					const std::vector<std::pair<zp::String, zp::String>>* operations);

	void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);

private:
	CImageList	m_imageList;

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnEditAdd();
	afx_msg void OnEditAddFolder();
	afx_msg void OnEditDelete();
	afx_msg void OnEditExtract();
};

#ifndef _DEBUG  // debug version in LeftView.cpp
inline CzpEditorDoc* CLeftView::GetDocument()
   { return reinterpret_cast<CzpEditorDoc*>(m_pDocument); }
#endif

