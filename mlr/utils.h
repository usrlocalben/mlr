
#ifndef __UTILS_H
#define __UTILS_H

#include "stdafx.h"

std::wstring s2ws(const std::string& str);
std::string ws2s(const std::wstring& wstr);
std::vector<std::string> fileglob(const std::string& pathpat);
std::vector<std::string> explode(const std::string& str, char ch);
long long getmtime(const std::string& fn);

#endif //__UTILS_H
