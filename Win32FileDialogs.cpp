#include "Win32FileDialogs.h"
#include <windows.h>
#include <commdlg.h>


std::string OpenFileDialog(const char* filter)
{
    char filename[MAX_PATH] = "";

    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.Flags =
        OFN_EXPLORER |
        OFN_PATHMUSTEXIST |
        OFN_FILEMUSTEXIST |
        OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn))
        return std::string(filename);

    return {};
}

std::string SaveFileDialog(const char* filter)
{
    char filename[MAX_PATH] = "";

    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.Flags =
        OFN_EXPLORER |
        OFN_PATHMUSTEXIST |
        OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn))
        return std::string(filename);

    return {};
}
bool fileExists(const std::string& filename) {
    std::ifstream file(filename);
    return file.is_open();
}