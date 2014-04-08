// ptest.cpp : Defines the entry point for the console application.
//
#include <stdio.h>
#include <tchar.h>
#include "zpack.h"
#include <cassert>
#include <string>
#include <sstream>
#include <iostream>
#include <map>
#include "zpExplorer.h"

using namespace std;

#ifdef UNICODE
	#define COUT wcout
	#define CIN wcin
	typedef std::wistringstream IStringStream;
#else
	#define COUT cout
	#define CIN cin
	typedef std::istringstream IStringStream;
#endif

bool zpcallback(const zp::Char* path, zp::u32 fileSize, void* param)
{
	COUT << path << endl;
	return true;
}

typedef bool (*CommandProc)(const zp::String& param0, const zp::String& param1);

map<zp::String, CommandProc> g_commandHandlers;

#define CMD_PROC(cmd) bool cmd##_proc(const zp::String& param0, const zp::String& param1)
#define REGISTER_CMD(cmd) g_commandHandlers[_T(#cmd)] = &cmd##_proc;

ZpExplorer g_explorer;

CMD_PROC(exit)
{
	exit(0);
	return true;
}

CMD_PROC(open)
{
	return g_explorer.open(param0, false) || g_explorer.open(param0, true);
}

CMD_PROC(create)
{
	return g_explorer.create(param0, param1);
}

CMD_PROC(close)
{
	g_explorer.close();
	return true;
}

CMD_PROC(dir)
{
	const ZpNode* node = g_explorer.currentNode();
	for (list<ZpNode>::const_iterator iter = node->children.begin();
		iter != node->children.end();
		++iter)
	{
		if (iter->isDirectory)
		{
			COUT << _T("  <") << iter->name << _T(">") << endl; 
		}
		else
		{
			COUT << _T("  ") << iter->name << endl; 
		}
	}
	return true;
}

CMD_PROC(cd)
{
	return g_explorer.enterDir(param0);
}

CMD_PROC(add)
{
	return g_explorer.add(param0, param1);
}

CMD_PROC(del)
{
	return g_explorer.remove(param0);
}

CMD_PROC(extract)
{
	return g_explorer.extract(param0, param1);
}

//CMD_PROC(fragment)
//{
//	zp::IPackage* pack = g_explorer.getPack();
//	if (pack == NULL)
//	{
//		return false;
//	}
//	unsigned __int64 fragSize = pack->countFragmentSize();
//	COUT << "fragment:" << fragSize << " bytes" << endl;
//	return true;
//}

CMD_PROC(defrag)
{
	zp::IPackage* pack = g_explorer.getPack();
	if (pack == NULL)
	{
		return false;
	}
	return (pack != NULL && pack->defrag(NULL, NULL));
}

CMD_PROC(help)
{
#define HELP_ITEM(cmd, explain) COUT << cmd << endl << "    "explain << endl;
	HELP_ITEM("create [package path] [initial dir path]", "create a package from scratch");
	HELP_ITEM("open [package path]", "open an existing package");
	HELP_ITEM("close", "close current package");
	HELP_ITEM("cd [internal path]", "enter specified directory of package");
	HELP_ITEM("dir", "show files and sub-directories of current directory of package");
	HELP_ITEM("add [soure path] [dest path]", "add a disk file or directory to package");
	HELP_ITEM("del [internal path]", "delete a file or directory from package");
	HELP_ITEM("extract [source path] [dest path]", "extrace file or directories to disk");
	//HELP_ITEM("fragment", "calculate fragment bytes and how many bytes to move to defrag");
	HELP_ITEM("defrag", "compact file, remove all fragments");
	HELP_ITEM("exit", "exit program");
	return true;
}

bool processCmdLine(int argc, _TCHAR* argv[])
{
	if (argc < 4)
	{
		return false;
	}
	zp::String cmd = argv[1];
	zp::String packPath = argv[2];
	zp::String filePath = argv[3];

	if (cmd == _T("add"))
	{
		if (!g_explorer.open(packPath))
		{
			g_explorer.create(packPath, _T(""));
		}
		zp::String relativePath;
		if (argc > 4)
		{
			relativePath = argv[4];
		}
		g_explorer.add(filePath, relativePath);
	}
	else if (cmd == _T("extract"))
	{
		if (g_explorer.open(packPath))
		{
			g_explorer.extract(_T("."), filePath);
		}
	}
	g_explorer.close();
	return true;
}

int _tmain(int argc, _TCHAR* argv[])
{
	g_explorer.setCallback(zpcallback, NULL);

	if (processCmdLine(argc, argv))
	{
		return 0;
	}
	REGISTER_CMD(exit);
	REGISTER_CMD(create);
	REGISTER_CMD(open);
	REGISTER_CMD(add);
	REGISTER_CMD(del);
	REGISTER_CMD(extract);
	REGISTER_CMD(close);
	REGISTER_CMD(dir);
	REGISTER_CMD(cd);
	//REGISTER_CMD(fragment);
	REGISTER_CMD(defrag);
	REGISTER_CMD(help);

	while (true)
	{
		const zp::String packName = g_explorer.packageFilename();
		if (!packName.empty())
		{
			COUT << packName;
			if (g_explorer.isOpen())
			{
				COUT << DIR_STR << g_explorer.currentPath() << _T("\b");	//delete extra '\'
			}
			if (g_explorer.getPack()->readonly())
			{
				COUT << _T("-readonly");
			}
			COUT << _T(">");
		}
		else
		{
			COUT << _T("zp>");
		}
		zp::String input, command, param0, param1;
		getline(CIN, input);

		size_t pos = 0;

		IStringStream iss(input, IStringStream::in);
		iss >> command;
		iss >> param0;
		iss >> param1;
		map<zp::String, CommandProc>::iterator found = g_commandHandlers.find(command);
		if (found == g_commandHandlers.end())
		{
			COUT << _T("<") << command << _T("> command not found. try 'help' command.") << endl;
		}
		else if (!found->second(param0, param1))
		{
			COUT << _T("<") << command << _T("> execute failed.") << endl;
		}
	}
	return 0;
}
