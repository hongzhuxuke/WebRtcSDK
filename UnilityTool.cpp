#include "UnilityTool.h"
#include <windows.h>

std::string UnilityTool::WString2String(const std::wstring& ws)
{
   std::string strLocale = setlocale(LC_ALL, "");
   const wchar_t* wchSrc = ws.c_str();
   size_t nDestSize = wcstombs(NULL, wchSrc, 0) + 1;
   char *chDest = new char[nDestSize];
   memset(chDest, 0, nDestSize);
   wcstombs(chDest, wchSrc, nDestSize);
   std::string strResult = chDest;
   delete[]chDest;
   setlocale(LC_ALL, strLocale.c_str());
   return strResult;
}

// string => wstring
std::wstring UnilityTool::String2WString(const std::string& str)
{
   int num = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
   wchar_t *wide = new wchar_t[num];
   MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, wide, num);
   std::wstring w_str(wide);
   delete[] wide;
   return w_str;
}
