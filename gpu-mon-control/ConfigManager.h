#pragma once

#pragma once

#include <map>
#include <string>

struct GpuProfile
{
    std::wstring adapterId;
    std::wstring topology;
};

using Config = std::map<std::wstring, GpuProfile>;

class ConfigManager
{
public:
    ConfigManager();

    bool Load(Config& config, std::wstring& error) const;
    bool WriteDefaultTemplate(std::wstring& error) const;

    const std::wstring& GetPath() const;
    const std::wstring& GetToggleHotkey() const;
    double GetBaseResolutionX() const;
    double GetBaseResolutionY() const;

private:
    bool ReadFile(std::wstring& content, std::wstring& error) const;
    bool WriteFile(const std::wstring& content, std::wstring& error) const;
    bool Parse(const std::wstring& content, Config& config, std::wstring& error) const;

    std::wstring path_;
    mutable std::wstring toggleHotkey_;
    mutable double baseResolutionX_;
    mutable double baseResolutionY_;
};
