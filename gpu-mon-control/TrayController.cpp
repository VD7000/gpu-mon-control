#include "TrayController.h"

#include <shellapi.h>

#include <cwchar>

const UINT TrayController::WM_TRAYICON = WM_APP + 1;

TrayController::TrayController()
    : window_(nullptr)
    , initialized_(false)
{
}

TrayController::~TrayController()
{
    Shutdown();
}

bool TrayController::Initialize(
    HWND window,
    const std::wstring& profile,
    bool scalerActive)
{
    window_ = window;

    NOTIFYICONDATAW data = {};
    data.cbSize = sizeof(data);
    data.hWnd = window_;
    data.uID = 1;
    data.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    data.uCallbackMessage = WM_TRAYICON;
    data.hIcon = LoadIconW(nullptr, IDI_APPLICATION);

    const std::wstring tooltip =
        L"gpu-mon-control | GPU: " +
        (profile.empty() ? L"unknown" : profile) +
        L" | Scale: " +
        (scalerActive ? L"Active" : L"Inactive");

    wcsncpy_s(data.szTip, tooltip.c_str(), _TRUNCATE);

    if (!Shell_NotifyIconW(NIM_ADD, &data))
    {
        window_ = nullptr;
        return false;
    }

    initialized_ = true;
    return true;
}

void TrayController::Update(
    const std::wstring& profile,
    bool scalerActive)
{
    if (window_ == nullptr)
    {
        return;
    }

    NOTIFYICONDATAW data = {};
    data.cbSize = sizeof(data);
    data.hWnd = window_;
    data.uID = 1;
    data.uFlags = NIF_TIP;

    const std::wstring tooltip =
        L"gpu-mon-control | GPU: " +
        (profile.empty() ? L"unknown" : profile) +
        L" | Scale: " +
        (scalerActive ? L"Active" : L"Inactive");

    wcsncpy_s(data.szTip, tooltip.c_str(), _TRUNCATE);
    Shell_NotifyIconW(initialized_ ? NIM_MODIFY : NIM_ADD, &data);
}

void TrayController::Shutdown()
{
    if (window_ != nullptr && initialized_)
    {
        NOTIFYICONDATAW data = {};
        data.cbSize = sizeof(data);
        data.hWnd = window_;
        data.uID = 1;
        Shell_NotifyIconW(NIM_DELETE, &data);
    }

    initialized_ = false;
    window_ = nullptr;
}
