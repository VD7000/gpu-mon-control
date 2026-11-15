#pragma once

#include <windows.h>

#include <string>

class TrayController
{
public:
    static const UINT WM_TRAYICON;

    TrayController();
    ~TrayController();

    bool Initialize(HWND window, const std::wstring& profile, bool scalerActive);
    void Update(const std::wstring& profile, bool scalerActive);
    void Shutdown();

private:
    HWND window_;
    bool initialized_;
};
