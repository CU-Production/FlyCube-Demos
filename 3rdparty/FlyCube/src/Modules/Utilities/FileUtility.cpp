#include "FileUtility.h"

std::string GetExecutablePath()
{
#ifdef _WIN32
    char buf[MAX_PATH + 1];
    GetModuleFileNameA(nullptr, buf, sizeof(buf));
    return buf;
#else
    char buf[BUFSIZ];
    readlink("/proc/self/exe", buf, sizeof(buf));
    return buf;
#endif
}

std::wstring utf8_to_wstring(const std::string& str)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
    return myconv.from_bytes(str);
}

std::string wstring_to_utf8(const std::wstring& str)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
    return myconv.to_bytes(str);
}

std::string GetExecutableDir()
{
    auto path = GetExecutablePath();
    return path.substr(0, path.find_last_of("\\/"));
}

std::string GetEnv(const std::string& var)
{
    const char* res = getenv("VULKAN_SDK");
    return res ? res : "";
}
