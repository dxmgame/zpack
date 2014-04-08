
// LeftView.cpp : implementation of the CLeftView class
//

#include "stdafx.h"
#include "zpEditor.h"

#include "zpEditorDoc.h"
#include "LeftView.h"
#include "ProgressDialog.h"
#include "FolderDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CLeftView

IMPLEMENT_DYNCREATE(CLeftView, CTreeView)

BEGIN_MESSAGE_MAP(CLeftView, CTreeView)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelectChanged)
	ON_WM_KEYDOWN()
	ON_COMMAND(ID_EDIT_ADD, &CLeftView::OnEditAdd)
	ON_COMMAND(ID_EDIT_ADD_FOLDER, &CLeftView::OnEditAddFolder)
	ON_COMMAND(ID_EDIT_DELETE, &CLeftView::OnEditDelete)
	ON_COMMAND(ID_EDIT_EXTRACT, &CLeftView::OnEditExtract)
END_MESSAGE_MAP()


// CLeftView construction/destruction

CLeftView::CLeftView()
{
	// TODO: add construction code here
}

CLeftView::~CLeftView()
{
}

BOOL CLeftView::PreCreateWindow(CREATESTRUCT& cs)
{
	CBitmap bm;
	bm.LoadBitmap(IDB_BITMAP_FOLDER);
	m_imageList.Create(16, 16, ILC_COLOR24 | ILC_MASK, 0, 0);
	m_imageList.Add(&bm, RGB(255, 255, 255));
	return CTreeView::PreCreateWindow(cs);
}

void CLeftView::OnInitialUpdate()
{
	CTreeView::OnInitialUpdate();

	CTreeCtrl& treeCtrl = GetTreeCtrl();
	treeCtrl.ModifyStyle(0, TVS_HASBUTTONS | TVS_HASLINES | TVS_SHOWSELALWAYS);
	treeCtrl.SetImageList(&m_imageList, TVSIL_NORMAL);
}

void CLeftView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	CTreeCtrl& treeCtrl = GetTreeCtrl();
	BOOL reset = (BOOL)lHint;
	if (reset)
	{
		treeCtrl.DeleteAllItems();
	}

	const ZpExplorer& explorer = GetDocument()->GetZpExplorer();
	const ZpNode* node = explorer.currentNode();
	HTREEITEM currentItem = (HTREEITEM)node->userData;
	if (currentItem != NULL)
	{
		updateNode(currentItem);
		treeCtrl.SelectItem(currentItem);
		return;
	}

	const ZpNode* rootNode = explorer.rootNode();
	if (!explorer.isOpen())
	{
		return;
	}
	zp::String packName;
	const zp::String& packFilename = explorer.packageFilename();
	size_t slashPos = packFilename.find_last_of(_T('\\'));
	if (slashPos == zp::String::npos)
	{
		packName = packFilename;
	}
	else
	{
		packName = packFilename.substr(slashPos + 1, packFilename.length() - slashPos - 1);
	}
	if (explorer.getPack()->readonly())
	{
		packName += _T(" (read only)");
	}
	HTREEITEM rootItem = treeCtrl.InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE | TVIF_PARAM,
										packName.c_str(),
										0,
										1,
										TVIS_EXPANDED | INDEXTOSTATEIMAGEMASK(1),
										TVIS_EXPANDED | TVIS_STATEIMAGEMASK,
										(LPARAM)rootNode,
										TVI_ROOT,
										TVI_LAST);
	rootNode->userData = rootItem;

	updateNode(rootItem);

	treeCtrl.Select(rootItem, TVGN_CARET);
}

void CLeftView::updateNode(HTREEITEM ti)
{
	CTreeCtrl& treeCtrl = GetTreeCtrl();
	//delete old children
	if (treeCtrl.ItemHasChildren(ti))
	{
		HTREEITEM childItem = treeCtrl.GetChildItem(ti);
		while (childItem != NULL)
		{
			HTREEITEM nextItem = treeCtrl.GetNextItem(childItem, TVGN_NEXT);
			treeCtrl.DeleteItem(childItem);
			childItem = nextItem;
		}
	}
	//add new children
	const ZpNode* node = (const ZpNode*)treeCtrl.GetItemData(ti);
	for (std::list<ZpNode>::const_iterator iter = node->children.cbegin();
		iter != node->children.cend();
		++iter)
	{
		const ZpNode& child = *iter;
		if (!child.isDirectory)
		{
			continue;
		}
		HTREEITEM childItem = treeCtrl.InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM,
												child.name.c_str(),
												0,
												1,
												TVIS_EXPANDED | INDEXTOSTATEIMAGEMASK(1),
												TVIS_EXPANDED | TVIS_STATEIMAGEMASK,
												(LPARAM)&child,
												ti,
												TVI_LAST);
		child.userData = childItem;
		updateNode(childItem);
	}
}

void CLeftView::OnSelectChanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	CTreeCtrl& treeCtrl = GetTreeCtrl();
	HTREEITEM selectedItem = treeCtrl.GetSelectedItem();
	if (selectedItem != NULL)
	{
		const ZpNode* selectedNode = (const ZpNode*)treeCtrl.GetItemData(selectedItem);
		ZpExplorer& explorer = GetDocument()->GetZpExplorer();
		explorer.setCurrentNode(selectedNode);
		m_pDocument->UpdateAllViews(this);
	}
}

// CLeftView diagnostics

#ifdef _DEBUG
void CLeftView::AssertValid() const
{
	CTreeView::AssertValid();
}

void CLeftView::Dump(CDumpContext& dc) const
{
	CTreeView::Dump(dc);
}

CzpEditorDoc* CLeftView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CzpEditorDoc)));
	return (CzpEditorDoc*)m_pDocument;
}
#endif //_DEBUG


// CLeftView message handlers

void CLeftView::OnEditAdd()
{
	CFileDialog dlg(TRUE, NULL, NULL, OFN_ALLOWMULTISELECT);
	if (dlg.DoModal() != IDOK)
	{
		return;
	}
	zp::u64 totalFileSize = 0;
	std::vector<std::pair<zp::String, zp::String>> params;
	ZpExplorer& explorer = GetDocument()->GetZpExplorer();
	POSITION pos = dlg.GetStartPosition();
	while (pos)
	{
		CString filename = dlg.GetNextPathName(pos);
		totalFileSize += explorer.countDiskFileSize(filename.GetString());
		params.push_back(std::make_pair(filename.GetString(), _T("")));
	}
	startOperation(ProgressDialog::OP_ADD, totalFileSize, &params);
	m_pDocument->UpdateAllViews(NULL);
}

void CLeftView::OnEditAddFolder()
{
	CFolderDialog folderDlg(NULL, _T("Select folder to add to package."));
	if (folderDlg.DoModal() != IDOK)
	{
		return;
	}
	CString path = folderDlg.GetPathName();
	ZpExplorer& explorer = GetDocument()->GetZpExplorer();
	zp::u64 totalFileSize = explorer.countDiskFileSize(path.GetString());

	std::vector<std::pair<zp::String, zp::String>> params;
	params.push_back(std::make_pair(path.GetString(), _T("")));
	startOperation(ProgressDialog::OP_ADD, totalFileSize, &params);
	m_pDocument->UpdateAllViews(NULL);
}

void CLeftView::OnEditDelete()
{
	ZpExplorer& explorer = GetDocument()->GetZpExplorer();
	explorer.setCallback(NULL, NULL);
	if (explorer.currentNode() == explorer.rootNode())
	{
		::MessageBox(NULL, _T("This folder can not be deleted."), _T("Information"), MB_OK | MB_ICONINFORMATION);
		return;
	}
	zp::String warning = _T("Do you want to delete ");
	warning += _T("\"");
	warning += explorer.currentNode()->name;
	warning += _T("\"?");
	if (::MessageBox(NULL, warning.c_str(), _T("Question"), MB_YESNO | MB_ICONQUESTION) != IDYES)
	{
		return;
	}
	explorer.remove(_T("."));
	m_pDocument->UpdateAllViews(NULL);
}

void CLeftView::OnEditExtract()
{
	CFolderDialog dlg(NULL, _T("Select dest folder to extract."));
	if (dlg.DoModal() != IDOK)
	{
		return;
	}
	zp::String destPath = dlg.GetPathName().GetString();

	ZpExplorer& explorer = GetDocument()->GetZpExplorer();

	zp::u64 totalFileSize = explorer.countNodeFileSize(explorer.currentNode());
	std::vector<std::pair<zp::String, zp::String>> params;
	params.push_back(std::make_pair(_T("."), destPath));
	startOperation(ProgressDialog::OP_EXTRACT, totalFileSize, &params);
}

void CLeftView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == VK_DELETE)
	{
		OnEditDelete();
	}
	CTreeView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CLeftView::startOperation(ProgressDialog::Operation op, zp::u64 totalFileSize,
							const std::vector<std::pair<zp::String, zp::String>>* params)
{
	ProgressDialog progressDlg;
	progressDlg.m_explorer = &(GetDocument()->GetZpExplorer());
	progressDlg.m_running = true;
	progressDlg.m_params = params;
	progressDlg.m_operation = op;
	progressDlg.m_totalFileSize = totalFileSize;
	progressDlg.DoModal();
}

