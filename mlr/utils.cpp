
//#include "utils.h"
#include "stdafx.h"

#include <vector>
#include <codecvt>

#include <Windows.h>

using namespace std;

wstring s2ws(const string& str)
{
	typedef codecvt_utf8<wchar_t> convert_typeX;
	wstring_convert<convert_typeX, wchar_t> converterX;
	return converterX.from_bytes(str);
}

string ws2s(const wstring& wstr)
{
	typedef codecvt_utf8<wchar_t> convert_typeX;
	wstring_convert<convert_typeX, wchar_t> converterX;
	return converterX.to_bytes(wstr);
}

vector<string> fileglob(const string& pathpat) {
	vector<string> lst;
	WIN32_FIND_DATA ffd;

	HANDLE hFind = FindFirstFile(s2ws(pathpat).c_str(), &ffd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
			lst.push_back(ws2s(wstring(ffd.cFileName)));
		} while (FindNextFile(hFind, &ffd) != 0);
	}
	return lst;
}

vector<string> explode(const string& str, char ch) {
	vector<string> items;
	string src(str);
	int nextmatch = src.find(ch);
	while (1) {
		string item = src.substr(0, nextmatch);
		items.push_back(item);
		if (nextmatch == string::npos) break;
		src = src.substr(nextmatch + 1);
		nextmatch = src.find(ch);
	}
	return items;
}


/*
* example from
* http://nickperrysays.wordpress.com/2011/05/24/monitoring-a-file-last-modified-date-with-visual-c/
*/
long long getmtime(const string& fn) {
	long long mtime = -1;
	HANDLE hFile = CreateFileA(fn.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		FILETIME ftCreate, ftAccess, ftWrite;
		if (!GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite)) {
			mtime = 0;
		} else {
			mtime = (long long)ftWrite.dwHighDateTime << 32 | ftWrite.dwLowDateTime;
		}
		CloseHandle(hFile);
	}
	return mtime;
}

