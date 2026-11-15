#include "DisplayController.h"

#include <windows.h>

namespace
{
    UINT TopologyFlag(TopologyMode mode)
    {
        switch (mode)
        {
        case TopologyMode::Internal:
            return SDC_TOPOLOGY_INTERNAL;
        case TopologyMode::External:
            return SDC_TOPOLOGY_EXTERNAL;
        case TopologyMode::Extend:
            return SDC_TOPOLOGY_EXTEND;
        default:
            return 0;
        }
    }
}

bool DisplayController::ApplyTopology(
    TopologyMode mode,
    std::wstring& error) const
{
    const UINT flag = TopologyFlag(mode);

    if (flag == 0)
    {
        error = L"Invalid display topology.";
        return false;
    }

    const LONG result = SetDisplayConfig(
        0,
        nullptr,
        0,
        nullptr,
        SDC_APPLY | flag);

    if (result != ERROR_SUCCESS)
    {
        error = L"SetDisplayConfig failed with error code " +
            std::to_wstring(result) + L".";
        return false;
    }

    return true;
}

bool DisplayController::ParseTopology(
    const std::wstring& value,
    TopologyMode& mode)
{
    if (value == L"internal")
    {
        mode = TopologyMode::Internal;
        return true;
    }

    if (value == L"external")
    {
        mode = TopologyMode::External;
        return true;
    }

    if (value == L"extend")
    {
        mode = TopologyMode::Extend;
        return true;
    }

    return false;
}
