#pragma once

#pragma once

#include <string>

#include "ConfigManager.h"

class GpuSwitcher
{
public:
    bool GetCurrentAdapterId(std::wstring& adapterId, std::wstring& error) const;
    bool SetAdapterId(const std::wstring& adapterId, std::wstring& error) const;
    bool ToggleProfile(
        const Config& config,
        std::wstring& selectedProfile,
        std::wstring& error) const;
};
