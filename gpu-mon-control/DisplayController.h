#pragma once

#pragma once

#include <string>

enum class TopologyMode
{
    Internal,
    External,
    Extend
};

class DisplayController
{
public:
    bool ApplyTopology(TopologyMode mode, std::wstring& error) const;
    static bool ParseTopology(
        const std::wstring& value,
        TopologyMode& mode);
};
