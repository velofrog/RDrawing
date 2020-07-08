#pragma once

#include <string>

bool AnsiToWideChar(const std::string& src, std::wstring& dest);
bool UTF8ToWideChar(const std::string &src, std::wstring &dest);
bool WideCharToUTF8(const std::wstring&src, std::string &dest);
