#include "Stdafx.h"
#include "zpExplorer.h"
#include "zpack.h"
#include <cassert>
#include <fstream>
#include <algorithm>
#include "fileEnum.h"
#include "windows.h"
#include "shlobj.h"
//#include "PerfUtil.h"

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////
ZpExplorer::ZpExplorer()
	: m_pack(NULL)
	, m_callback(NULL)
	//, m_fileCount(0)
	, m_totalFileSize(0)
	, m_callbackParam(NULL)
{
	m_root.isDirectory = true;
	m_root.parent = NULL;
	m_currentNode = &m_root;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ZpExplorer::~ZpExplorer()
{
	if (m_pack != NULL)
	{
		zp::close(m_pack);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void ZpExplorer::setCallback(zp::Callback callback, void* param)
{
	m_callback = callback;
	m_callbackParam = param;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool ZpExplorer::open(const zp::String& path, bool readonly)
{
	//BEGIN_PERF("open")
	clear();
	m_pack = zp::open(path.c_str(), readonly ? zp::OPEN_READONLY : 0);
	if (m_pack == NULL)
	{
		return false;
	}
	build();
	//END_PERF
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool ZpExplorer::create(const zp::String& path, const zp::String& inputPath)
{
	clear();
	if (path.empty())
	{
		return false;
	}
	m_pack = zp::create(path.c_str());
	if (m_pack == NULL)
	{
		return false;
	}
	if (inputPath.empty())
	{
		return true;
	}
	m_basePath = inputPath;
	if (m_basePath.c_str()[m_basePath.length() - 1] != DIR_CHAR)
	{
		m_basePath += DIR_STR;
	}
	enumFile(m_basePath, addPackFile, this);
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void ZpExplorer::close()
{
	clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool ZpExplorer::isOpen() const
{
	return (m_pack != NULL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool ZpExplorer::defrag()
{
	if (m_pack == NULL)
	{
		return false;
	}
	return m_pack->defrag(m_callback, m_callbackParam);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
zp::IPackage* ZpExplorer::getPack() const
{
	return m_pack;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const zp::Char* ZpExplorer::packageFilename() const
{
	if (m_pack != NULL)
	{
		return m_pack->packageFilename();
	}
	return _T("");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const zp::String& ZpExplorer::currentPath() const
{
	return m_currentPath;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool ZpExplorer::enterDir(const zp::String& path)
{
	assert(m_currentNode != NULL);
#if !(ZP_CASE_SENSITIVE)
	zp::String lowerPath = path;
	stringToLower(lowerPath, path);
	ZpNode* child = findChildRecursively(m_currentNode, lowerPath, FIND_DIR);
#else
	ZpNode* child = findChildRecursively(m_currentNode, path, FIND_DIR);
#endif
	if (child == NULL)
	{
		return false;
	}
	m_currentNode = child;
	getNodePath(m_currentNode, m_currentPath);
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool ZpExplorer::add(const zp::String& srcPath, const zp::String& dstPath, bool flush)
{
	if (m_pack == NULL || m_pack->readonly() || srcPath.empty())
	{
		return false;
	}
	if (dstPath.empty())
	{
		m_workingPath = m_currentPath;
	}
	else if (dstPath[0] == DIR_CHAR)
	{
		m_workingPath = dstPath.substr(1, dstPath.length() - 1);
	}
	else
	{
		m_workingPath = m_currentPath + dstPath;
	}
	if (!m_workingPath.empty() && m_workingPath[m_workingPath.length() - 1] != DIR_CHAR)
	{
		m_workingPath += DIR_STR;
	}
	m_basePath.clear();
	size_t pos = srcPath.rfind(DIR_CHAR);

	WIN32_FIND_DATA fd;
	HANDLE findFile = ::FindFirstFile(srcPath.c_str(), &fd);
	if (findFile != INVALID_HANDLE_VALUE && (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
	{
		//it's a file
		if (fd.nFileSizeHigh != 0)
		{
			return false;
		}
		zp::String nudeFilename = srcPath.substr(pos + 1, srcPath.length() - pos - 1);
		bool ret = addFile(srcPath, nudeFilename, fd.nFileSizeLow);
		if (ret && flush)
		{
			m_pack->flush();
		}
		::FindClose(findFile);
		return ret;
	}
	//it's a directory
	if (pos != zp::String::npos)
	{
		//dir
		m_basePath = srcPath.substr(0, pos + 1);
	}
	zp::String searchDirectory = srcPath;
	if (srcPath.c_str()[srcPath.length() - 1] != DIR_CHAR)
	{
		searchDirectory += DIR_STR;
	}
	bool ret = enumFile(searchDirectory, addPackFile, this);
	
	::FindClose(findFile);

	if (ret && flush)
	{
		m_pack->flush();
	}
	return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool ZpExplorer::remove(const zp::String& path)
{
	if (m_pack == NULL || m_pack->readonly() || path.empty())
	{
		return false;
	}
	list<ZpNode>::iterator found;
#if !(ZP_CASE_SENSITIVE)
	zp::String lowerPath = path;
	stringToLower(lowerPath, path);
	ZpNode* child = findChildRecursively(m_currentNode, lowerPath, FIND_ANY);
#else
	ZpNode* child = findChildRecursively(m_currentNode, path, FIND_ANY);
#endif
	if (child == NULL)
	{
		return false;
	}
	zp::String internalPath;
	getNodePath(child, internalPath);
	//remove '\'
	if (!internalPath.empty())
	{
		internalPath.resize(internalPath.size() - 1);
	}
	bool ret = false;
	if (removeChildRecursively(child, internalPath))
	{
		ret = true;
		if (child->parent != NULL)
		{
			if (child == m_currentNode)
			{
				m_currentNode = child->parent;
			}
			removeChild(child->parent, child);
		}
	}
	getNodePath(m_currentNode, m_currentPath);
	m_pack->flush();
	return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void ZpExplorer::flush()
{
	if (m_pack != NULL)
	{
		m_pack->flush();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool ZpExplorer::extract(const zp::String& srcPath, const zp::String& dstPath)
{
	if (m_pack == NULL)
	{
		return false;
	}
	zp::String externalPath = dstPath;
	if (externalPath.empty())
	{
		externalPath = _T(".")DIR_STR;
	}
	else if (externalPath.c_str()[externalPath.length() - 1] != DIR_CHAR)
	{
		externalPath += DIR_STR;
	}
#if !(ZP_CASE_SENSITIVE)
	zp::String lowerPath = srcPath;
	stringToLower(lowerPath, srcPath);
	ZpNode* child = findChildRecursively(m_currentNode, lowerPath, FIND_ANY);
#else
	ZpNode* child = findChildRecursively(m_currentNode, srcPath, FIND_ANY);
#endif
	if (child == NULL)
	{
		return false;
	}
	zp::String internalPath;
	getNodePath(child, internalPath);
	//remove '\'
	if (!internalPath.empty())
	{
		internalPath.resize(internalPath.size() - 1);
	}
	return extractRecursively(child, externalPath, internalPath);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void ZpExplorer::setCurrentNode(const ZpNode* node)
{
	assert(node != NULL);
	m_currentNode = const_cast<ZpNode*>(node);
	getNodePath(m_currentNode, m_currentPath);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const ZpNode* ZpExplorer::currentNode() const
{
	return m_currentNode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const ZpNode* ZpExplorer::rootNode() const
{
	return &m_root;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void ZpExplorer::clear()
{
	m_root.children.clear();
	m_root.userData = NULL;
	m_currentNode = &m_root;
	getNodePath(m_currentNode, m_currentPath);
	m_workingPath = m_currentPath;
	if (m_pack != NULL)
	{
		zp::close(m_pack);
		m_pack = NULL;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void ZpExplorer::build()
{
	zp::u32 count = m_pack->getFileCount();
	for (zp::u32 i = 0; i < count; ++i)
	{
		zp::Char buffer[256];
		zp::u32 fileSize, compressSize, flag;
		m_pack->getFileInfo(i, buffer, sizeof(buffer)/sizeof(zp::Char), &fileSize, &compressSize, &flag);
		zp::String filename = buffer;
		for (zp::u32 i = 0; i < filename.length(); ++i)
		{
			if (filename[i] == _T('/'))
			{
				filename[i] = DIR_CHAR;
			}
		}
		insertFileToTree(filename, fileSize, compressSize, flag, false);
	}
}

//double g_readTime = 0;

////////////////////////////////////////////////////////////////////////////////////////////////////
bool ZpExplorer::addFile(const zp::String& filename, const zp::String& relativePath, zp::u32 fileSize)
{
	zp::String internalName = m_workingPath + relativePath;
	zp::u32 compressSize = 0;
	zp::u32 flag = 0;
	if (!m_pack->addFile(internalName.c_str(), filename.c_str(), fileSize, zp::FILE_COMPRESS, &compressSize, &flag))
	{
		return false;
	}
	insertFileToTree(internalName, fileSize, compressSize, flag, true);
	
	if (m_callback != NULL && !m_callback(relativePath.c_str(), fileSize, m_callbackParam))
	{
		return false;
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool ZpExplorer::extractFile(const zp::String& externalPath, const zp::String& internalPath)
{
	assert(m_pack != NULL);
	zp::IReadFile* file = m_pack->openFile(internalPath.c_str());
	if (file == NULL)
	{
		return false;
	}
	fstream stream;
	locale loc = locale::global(locale(""));
	stream.open(externalPath.c_str(), ios_base::out | ios_base::trunc | ios_base::binary);
	locale::global(loc);
	if (!stream.is_open())
	{
		return false;
	}
	char* buffer = new char[file->size()];
	zp::u32 readSize = file->read((zp::u8*)buffer, file->size());
	stream.write(buffer, readSize);
	stream.close();
	m_pack->closeFile(file);
	delete [] buffer;
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void ZpExplorer::countChildRecursively(const ZpNode* node)
{
	if (!node->isDirectory)
	{
		m_totalFileSize += node->fileSize;
	}
	for (list<ZpNode>::const_iterator iter = node->children.begin();
		iter != node->children.end();
		++iter)
	{
		countChildRecursively(&(*iter));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool ZpExplorer::removeChild(ZpNode* node, ZpNode* child)
{
	assert(node != NULL && child != NULL);
	for (list<ZpNode>::iterator iter = node->children.begin();
		iter != node->children.end();
		++iter)
	{
		if (child == &(*iter))
		{
			minusAncesterSize(child);
			node->children.erase(iter);
			return true;
		}
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool ZpExplorer::removeChildRecursively(ZpNode* node, zp::String path)
{
	assert(node != NULL && m_pack != NULL);
	if (!node->isDirectory)
	{
		if (m_callback != NULL && !m_callback(node->name.c_str(), (zp::u32)node->fileSize, m_callbackParam))
		{
			return false;
		}
		if (!m_pack->removeFile(path.c_str()))
		{
			return false;
		}
		return true;
	}
	if (!path.empty())
	{
		path += DIR_STR;
	}
	//recurse
	for (list<ZpNode>::iterator iter = node->children.begin();
		iter != node->children.end();)
	{
		ZpNode* child = &(*iter);
		if (!removeChildRecursively(child, path + child->name))
		{
			return false;
		}
		if (child == m_currentNode)
		{
			m_currentNode = m_currentNode->parent;
			assert(m_currentNode != NULL);
		}
		minusAncesterSize(child);
		iter = node->children.erase(iter);
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool ZpExplorer::extractRecursively(ZpNode* node, zp::String externalPath, zp::String internalPath)
{
	assert(node != NULL && m_pack != NULL);
	externalPath += node->name;
	if (!node->isDirectory)
	{
		if (m_callback != NULL && !m_callback(internalPath.c_str(), (zp::u32)node->fileSize, m_callbackParam))
		{
			return false;
		}
		if (!extractFile(externalPath, internalPath))
		{
			return false;
		}
		return true;
	}
	if (!internalPath.empty())	//in case extract the root directory
	{
		externalPath += DIR_STR;
		internalPath += DIR_STR;
		//create directory if necessary
		WIN32_FIND_DATA fd;
		HANDLE findFile = ::FindFirstFile(externalPath.c_str(), &fd);
		if (findFile == INVALID_HANDLE_VALUE)
		{
			if (::SHCreateDirectoryEx(NULL, externalPath.c_str(), NULL) != ERROR_SUCCESS)
			{
				return false;
			}
		}
		else if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
		{
			return false;
		}
	}
	//recurse
	for (list<ZpNode>::iterator iter = node->children.begin();
		iter != node->children.end();
		++iter)
	{
		if (!extractRecursively(&(*iter), externalPath, internalPath + iter->name))
		{
			return false;
		}
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void ZpExplorer::insertFileToTree(const zp::String& filename, zp::u32 fileSize, zp::u32 compressSize,
									zp::u32 flag, bool checkFileExist)
{
	ZpNode* node = &m_root;
	zp::String filenameLeft = filename;
	while (filenameLeft.length() > 0)
	{
		size_t pos = filenameLeft.find_first_of(DIR_STR);
		if (pos == zp::String::npos)
		{
			//it's a file
		#if !(ZP_CASE_SENSITIVE)
			zp::String lowerName = filenameLeft;
			stringToLower(lowerName, filenameLeft);
			ZpNode* child = checkFileExist ? findChild(node, lowerName, FIND_FILE) : NULL;
		#else
			ZpNode* child = checkFileExist ? findChild(node, filenameLeft, FIND_FILE) : NULL;
		#endif
			if (child == NULL)
			{
				ZpNode newNode;
				newNode.parent = node;
				newNode.isDirectory = false;
				newNode.name = filenameLeft;
			#if !(ZP_CASE_SENSITIVE)
				newNode.lowerName = lowerName;
			#endif
				newNode.fileSize = fileSize;
				newNode.compressSize = compressSize;
				newNode.flag = flag;
				node->children.push_back(newNode);
			}
			else
			{
				minusAncesterSize(child);
				child->fileSize = fileSize;
				child->compressSize = compressSize;
				child->flag = flag;
			}
			return;
		}
		zp::String dirName = filenameLeft.substr(0, pos);
		filenameLeft = filenameLeft.substr(pos + 1, filenameLeft.length() - pos - 1);
	#if !(ZP_CASE_SENSITIVE)
		zp::String lowerName = dirName;
		stringToLower(lowerName, dirName);
		ZpNode* child = findChild(node, lowerName, FIND_DIR);
	#else
		ZpNode* child = findChild(node, dirName, FIND_DIR);
	#endif
		if (child != NULL)
		{
			node = child;
			node->fileSize += fileSize;
			node->compressSize += compressSize;
		}
		else
		{
			ZpNode newNode;
			newNode.isDirectory = true;
			newNode.parent = node;
			newNode.name = dirName;
		#if !(ZP_CASE_SENSITIVE)
			newNode.lowerName = lowerName;
		#endif
			newNode.fileSize = fileSize;
			newNode.compressSize = compressSize;
			newNode.flag = 0;
			//insert after all directories
			list<ZpNode>::iterator insertPoint;
			for (insertPoint = node->children.begin(); insertPoint != node->children.end();	++insertPoint)
			{
				if (!insertPoint->isDirectory)
				{
					break;
				}
			}
			list<ZpNode>::iterator inserted;
			inserted = node->children.insert(insertPoint, newNode);
			node = &(*inserted);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ZpNode* ZpExplorer::findChild(ZpNode* node, const zp::String& name, FindType type)
{
	if (name.empty())
	{
		return  type == FIND_FILE ? NULL : &m_root;
	}
	assert(node != NULL);
	if (name == _T("."))
	{
		return type == FIND_FILE ? NULL : node;
	}
	else if (name == _T(".."))
	{
		return type == FIND_FILE ? NULL : node->parent;
	}
	for (list<ZpNode>::iterator iter = node->children.begin();
		iter != node->children.end();
		++iter)
	{
		if ((type == FIND_DIR && !iter->isDirectory) || (type == FIND_FILE && iter->isDirectory))
		{
			continue;
		}
	#if !(ZP_CASE_SENSITIVE)
		if (name == iter->lowerName)
		{
			return &(*iter);
		}
	#else
		if (name == iter->name)
		{
			return &(*iter);
		}
	#endif
	}
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ZpNode* ZpExplorer::findChildRecursively(ZpNode* node, const zp::String& name, FindType type)
{
	size_t pos = name.find_first_of(DIR_STR);
	if (pos == zp::String::npos)
	{
		//doesn't have any directory name
		return name.empty()? node : findChild(node, name, type);
	}
	ZpNode* nextNode = NULL;
	zp::String dirName = name.substr(0, pos);
	nextNode = findChild(node, dirName, FIND_DIR);
	if (nextNode == NULL)
	{
		return NULL;
	}
	zp::String nameLeft = name.substr(pos + 1, name.length() - pos - 1);
	return findChildRecursively(nextNode, nameLeft, type);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void ZpExplorer::getNodePath(const ZpNode* node, zp::String& path) const
{
	path.clear();
	while (node != NULL && node != &m_root)
	{
		path = node->name + DIR_STR + path;
		node = node->parent;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
zp::u64 ZpExplorer::countDiskFileSize(const zp::String& path)
{
	m_basePath = path;
	if (m_basePath.c_str()[m_basePath.length() - 1] != DIR_CHAR)
	{
		m_basePath += DIR_STR;
	}
	m_totalFileSize = 0;
	enumFile(m_basePath, countFile, this);
	//if (m_totalFileSize == 0)
	//{
	//	return 1;	//single file
	//}
	return m_totalFileSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
zp::u64 ZpExplorer::countNodeFileSize(const ZpNode* node)
{
	m_totalFileSize = 0;
	countChildRecursively(node);
	return m_totalFileSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void ZpExplorer::minusAncesterSize(ZpNode* node)
{
	if (node->isDirectory)
	{
		return;
	}
	ZpNode* ancester = node->parent;
	while (ancester != NULL)
	{
		ancester->fileSize -= node->fileSize;
		ancester->compressSize -= node->compressSize;
		ancester = ancester->parent;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void stringToLower(zp::String& dst, const zp::String& src)
{
	dst = src;
#if defined ZP_USE_WCHAR
	transform(src.begin(), src.end(), dst.begin(), ::towlower);
#else
	transform(src.begin(), src.end(), dst.begin(), ::tolower);
#endif
}
