
#ifndef __UTILS_H
#define __UTILS_H

#include "stdafx.h"

#include <string>
#include <vector>

std::wstring s2ws(const std::string& str);
std::string ws2s(const std::wstring& wstr);
std::vector<std::string> fileglob(const std::string& pathpat);
std::vector<std::string> explode(const std::string& str, char ch);
long long getmtime(const std::string& fn);

std::vector<char> file_get_contents(const std::string& fn);
void file_get_contents(const std::string& fn, std::vector<char>& buf);

unsigned get_cpu_count();
void sse_speedup();
void bind_to_cpu(const int cpu);

#endif //__UTILS_H
