#pragma once

#include <windows.h>

class MouseScaler
{
public:
    MouseScaler();
    ~MouseScaler();

    MouseScaler(const MouseScaler&) = delete;
    MouseScaler& operator=(const MouseScaler&) = delete;

    bool Install();
    void Uninstall();
    bool IsInstalled() const;
    bool IsScalingRequired() const;
    void UpdateHookState();
    void SetBaseResolution(double width, double height);

    void RecalculateScales();

private:
    static LRESULT CALLBACK LowLevelMouseProc(
        int nCode,
        WPARAM wParam,
        LPARAM lParam);

    LRESULT HandleMouseMessage(
        int nCode,
        WPARAM wParam,
        LPARAM lParam);

    bool IsCursorVisible() const;

    bool InstallHook();
    void RemoveHook();

    static MouseScaler* instance_;

    HHOOK mouseHook_;
    POINT lastPoint_;
    bool isModifying_;
    double scaleX_;
    double scaleY_;
    double baseX_;
    double baseY_;
    bool needsScaling_;
    bool enabled_;
    mutable ULONGLONG cursorVisibilityCheckedAt_;
    mutable bool cursorVisible_;
};