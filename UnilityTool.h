#pragma once
#include <string>
class UnilityTool
{
public:
   static std::string WString2String(const std::wstring& ws);
   // string => wstring
   static std::wstring String2WString(const std::string& str);
};

