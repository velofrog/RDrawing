#include "win_string.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

bool WideCharToUTF8(const std::wstring& src, std::string& dest) {
  std::size_t req_size = WideCharToMultiByte(CP_UTF8, 0, src.c_str(), src.length(), nullptr, 0, nullptr, nullptr);

  if (req_size == 0) return false;
  dest.resize(req_size);
  return (WideCharToMultiByte(CP_UTF8, 0, src.c_str(), src.length(), (LPSTR)dest.data(), req_size, nullptr, nullptr) != 0);
}

bool UTF8ToWideChar(const std::string& src, std::wstring& dest) {
  std::size_t req_size = MultiByteToWideChar(CP_UTF8, 0, src.c_str(), src.length(), nullptr, 0);

  if (req_size == 0) return false;
  dest.resize(req_size);
  return (MultiByteToWideChar(CP_UTF8, 0, src.c_str(), src.length(), (LPWSTR)dest.data(), req_size) != 0);
}

bool AnsiToWideChar(const std::string& src, std::wstring& dest) {
  std::size_t req_size = MultiByteToWideChar(CP_ACP, 0, src.c_str(), src.length(), nullptr, 0);

  if (req_size == 0) return false;
  dest.resize(req_size);
  return (MultiByteToWideChar(CP_ACP, 0, src.c_str(), src.length(), (LPWSTR)dest.data(), req_size) != 0);
}
