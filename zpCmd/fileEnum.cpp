#include "Stdafx.h"
#include "fileEnum.h"
#include <string>
#include <iostream>
#include <cassert>
#include "zpack.h"
#include "zpExplorer.h"
#include "windows.h"

#ifdef UNICODE
	#define Strcmp wcscmp
#else
	#define Strcmp strcmp
#endif

bool enumFile(const zp::String& searchPath, EnumCallback callback, void* param)
{
	WIN32_FIND_DATA fd;
	HANDLE findFile = ::FindFirstFile((searchPath + _T("*")).c_str(), &fd);

	if (findFile == INVALID_HANDLE_VALUE)
	{
		::FindClose(findFile);
		return true;
	}

	while (true)
	{
		if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
		{
			//file
			if (fd.nFileSizeHigh == 0)	//4g top
			{
				if (!callback(searchPath + fd.cFileName, fd.nFileSizeLow, param))
				{
					return false;
				}
			}
		}
		else if (Strcmp(fd.cFileName, _T(".")) != 0 && Strcmp(fd.cFileName, _T("..")) != 0)
		{
			//folder
			if (!enumFile(searchPath + fd.cFileName + DIR_STR, callback, param))
			{
				return false;
			}
		}
		if (!FindNextFile(findFile, &fd))
		{
			::FindClose(findFile);
			return true;
		}
	}
	::FindClose(findFile);
	return true;
}

bool addPackFile(const zp::String& filename, zp::u32 fileSize, void* param)
{
	ZpExplorer* explorer = reinterpret_cast<ZpExplorer*>(param);
	zp::String relativePath = filename.substr(explorer->m_basePath.length(), filename.length() - explorer->m_basePath.length());
	return explorer->addFile(filename, relativePath, fileSize);
}

bool countFile(const zp::String& filename, zp::u32 fileSize, void* param)
{
	ZpExplorer* explorer = reinterpret_cast<ZpExplorer*>(param);
	//++explorer->m_fileCount;
	explorer->m_totalFileSize += fileSize;
	return true;
}
