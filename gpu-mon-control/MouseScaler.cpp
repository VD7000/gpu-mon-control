#include "MouseScaler.h"

#include "MouseScaler.h"

#include <cmath>
#include <iostream>

bool MouseScaler::IsInstalled() const
{
    return mouseHook_ != nullptr;
}

bool MouseScaler::IsScalingRequired() const
{
    return needsScaling_;
}

MouseScaler* MouseScaler::instance_ = nullptr;

MouseScaler::MouseScaler()
    : mouseHook_(nullptr)
    , lastPoint_{ 0, 0 }
    , isModifying_(false)
    , scaleX_(1.0)
    , scaleY_(1.0)
    , baseX_(1920.0)
    , baseY_(1080.0)
    , needsScaling_(false)
    , enabled_(true)
    , cursorVisibilityCheckedAt_(0)
    , cursorVisible_(true)
{
}

void MouseScaler::SetBaseResolution(double width, double height)
{
    if (width > 0.0 && height > 0.0)
    {
        baseX_ = width;
        baseY_ = height;
        RecalculateScales();
    }
}

MouseScaler::~MouseScaler()
{
    Uninstall();
}

bool MouseScaler::Install()
{
    enabled_ = true;

    if (!needsScaling_ || !IsCursorVisible())
    {
        RemoveHook();
        return true;
    }

    return InstallHook();
}

bool MouseScaler::InstallHook()
{
    if (!needsScaling_ || mouseHook_ != nullptr)
    {
        return true;
    }

    if (instance_ != nullptr)
    {
        return false;
    }

    if (!GetCursorPos(&lastPoint_))
    {
        std::cerr << "[ERROR] Не удалось получить позицию курсора."
                  << std::endl;
        return false;
    }

    instance_ = this;

    mouseHook_ = SetWindowsHookExW(
        WH_MOUSE_LL,
        LowLevelMouseProc,
        GetModuleHandleW(nullptr),
        0);

    if (mouseHook_ == nullptr)
    {
        instance_ = nullptr;

        std::cerr << "[ERROR] Не удалось запустить хук."
                  << std::endl;
        return false;
    }

    return true;
}

void MouseScaler::Uninstall()
{
    enabled_ = false;
    RemoveHook();
}

void MouseScaler::RemoveHook()
{
    if (mouseHook_ != nullptr)
    {
        UnhookWindowsHookEx(mouseHook_);
        mouseHook_ = nullptr;
    }

    if (instance_ == this)
    {
        instance_ = nullptr;
    }
}

void MouseScaler::RecalculateScales()
{
    const double currentX =
        static_cast<double>(GetSystemMetrics(SM_CXSCREEN));

    const double currentY =
        static_cast<double>(GetSystemMetrics(SM_CYSCREEN));

    if (currentX <= 0.0 || currentY <= 0.0)
    {
        return;
    }

    const double currentAspect = currentX / currentY;
    const double baseAspect = baseX_ / baseY_;
    const double aspectDifference = std::fabs(currentAspect - baseAspect);
    const double aspectTolerance = 0.000001;

    needsScaling_ = aspectDifference > aspectTolerance;

    const double rawX =
        (currentX / baseX_) * (baseY_ / currentY);

    const double rawY = 1.0;
    const double speedCompensation = rawY / rawX;

    scaleX_ = rawX * speedCompensation;
    scaleY_ = rawY * speedCompensation;

    UpdateHookState();

    std::cout << "[SYSTEM] RESOLUTION: "
              << currentX << "x" << currentY
              << " | X: " << scaleX_
              << " | Y: " << scaleY_
              << std::endl;
}

void MouseScaler::UpdateHookState()
{
    if (!needsScaling_ || !enabled_)
    {
        RemoveHook();
        return;
    }

    if (IsCursorVisible())
    {
        InstallHook();
    }
    else
    {
        RemoveHook();
    }
}

bool MouseScaler::IsCursorVisible() const
{
    const ULONGLONG now = GetTickCount64();
    const ULONGLONG visibilityCheckInterval = 50;

    if (now - cursorVisibilityCheckedAt_ < visibilityCheckInterval)
    {
        return cursorVisible_;
    }

    cursorVisibilityCheckedAt_ = now;

    CURSORINFO cursorInfo = {};
    cursorInfo.cbSize = sizeof(cursorInfo);

    if (!GetCursorInfo(&cursorInfo))
    {
        cursorVisible_ = false;
        return false;
    }

    cursorVisible_ = (cursorInfo.flags & CURSOR_SHOWING) != 0;
    return cursorVisible_;
}

LRESULT CALLBACK MouseScaler::LowLevelMouseProc(
    int nCode,
    WPARAM wParam,
    LPARAM lParam)
{
    if (instance_ == nullptr)
    {
        return CallNextHookEx(nullptr, nCode, wParam, lParam);
    }

    return instance_->HandleMouseMessage(nCode, wParam, lParam);
}

LRESULT MouseScaler::HandleMouseMessage(
    int nCode,
    WPARAM wParam,
    LPARAM lParam)
{
    if (nCode < 0 || wParam != WM_MOUSEMOVE)
    {
        return CallNextHookEx(mouseHook_, nCode, wParam, lParam);
    }

    if (scaleX_ == 1.0 && scaleY_ == 1.0)
    {
        return CallNextHookEx(mouseHook_, nCode, wParam, lParam);
    }

    const MSLLHOOKSTRUCT* mouseStruct =
        reinterpret_cast<const MSLLHOOKSTRUCT*>(lParam);

    if (isModifying_ || !IsCursorVisible())
    {
        lastPoint_ = mouseStruct->pt;
        return CallNextHookEx(mouseHook_, nCode, wParam, lParam);
    }

    const int deltaX = mouseStruct->pt.x - lastPoint_.x;
    const int deltaY = mouseStruct->pt.y - lastPoint_.y;

    if (deltaX == 0 && deltaY == 0)
    {
        lastPoint_ = mouseStruct->pt;
        return CallNextHookEx(mouseHook_, nCode, wParam, lParam);
    }

    isModifying_ = true;

    const int newX =
        lastPoint_.x + static_cast<int>(std::round(deltaX * scaleX_));

    const int newY =
        lastPoint_.y + static_cast<int>(std::round(deltaY * scaleY_));

    SetCursorPos(newX, newY);

    lastPoint_.x = newX;
    lastPoint_.y = newY;

    isModifying_ = false;

    return 1;
}